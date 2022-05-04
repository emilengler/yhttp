/*
 * Copyright (c) 2022 Emil Engler <engler+yhttp@unveil2.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>

#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "abnf.h"
#include "buf.h"
#include "hash.h"
#include "parser.h"
#include "yhttp.h"
#include "yhttp-internal.h"

static unsigned char	*parser_find_eol(unsigned char *, size_t);

static int		 parser_query(struct parser *, struct hash *[],
				      const char *, size_t);
static int		 parser_keyvalue(struct parser *, struct hash *[],
					 const char *, size_t);

static int		 parser_rline_method(struct parser *, const char *,
					     size_t);
static int		 parser_rline_path(struct parser *, const char *,
					   size_t);
static int		 parser_rline_query(struct parser *, const char *,
					    size_t);
static int		 parser_rline_target(struct parser *, const char *,
					     size_t);
static int		 parser_rline(struct parser *);

static int		 parser_header_field(struct parser *, const char *,
					     size_t);
static int		 parser_headers(struct parser *);

static int		 parser_cl(struct parser *);
static int		 parser_body(struct parser *);

/* String associations for methods with their enum yhttp_method. */
static const char	*methods[] = {
	"GET",		/* GET */
	"HEAD",		/* HEAD */
	"POST",		/* POST */
	"PUT",		/* PUT */
	"DELETE",	/* DELETE */
	"PATCH",	/* PATCH */
	NULL
};

/*
 * Find the end of a line by either looking for CRLF or just LF.
 */
static unsigned char *
parser_find_eol(unsigned char *data, size_t ndata)
{
	size_t	i;

	for (i = 0; i < ndata; ++i) {
		if (data[i] == '\r') {
			/*
			 * Exit the loop if and only if the next character to
			 * a CR is an LF.
			 */
			if (i + 1 != ndata && data[i + 1] == '\n')
				break;
		} else if (data[i] == '\n')
			break;
	}

	/* Return NULL if no EOL has been found. */
	return (i == ndata ? NULL : data + i);
}

static int
parser_query(struct parser *parser, struct hash *ht[], const char *s,
	     size_t ns)
{
	const char	*start, *end;
	size_t		 i, remaining, len;
	int		 rc;

	/* Validate the query string. */
	for (i = 0; i < ns; ++i) {
		remaining = ns - i;
		if (!(abnf_is_unreserved(s[i]) ||
		      abnf_is_pct_encoded(s + i, remaining) ||
		      abnf_is_sub_delims(s[i]) ||
		      s[i] == ':' || s[i] == '@' || s[i] == '/' ||
		      s[i] == '?'))
			goto malformatted;
	}

	/* Parse all key/value pairs. */
	start = s;
	do {
		/* Find the end of the key/value pair. */
		remaining = ns - (start - s);
		end = memchr(start, '&', remaining);
		if (end == NULL) {
			/* The last key/value pair. */
			end = s + ns;
		}
		if (end == start) {
			/* The key/value pair is empty. */
			++start;
			continue;
		}

		len = end - start;
		rc = parser_keyvalue(parser, ht, start, len);
		if (rc != YHTTP_OK || parser->err)
			return (rc);

		/* Go to the next key/value pair. */
		start = end + 1;
	} while(end != s + ns);

	return (YHTTP_OK);
malformatted:
	parser->err = 400;
	return (YHTTP_OK);
}

static int
parser_keyvalue(struct parser *parser, struct hash *ht[], const char *s,
		size_t ns)
{
	const char	*equal;
	char		*key, *value;
	size_t		 keylen, valuelen;
	int		 has_value;
	int		 rc;

	equal = memchr(s, '=', ns);
	if (equal == NULL)
		has_value = 0;
	else
		has_value = 1;

	/* Check if the key is empty. */
	/* TODO: Be more liberal. */
	if (equal == s)
		goto malformatted;

	/* Extract the key and the value. */
	if (has_value) {
		keylen = equal - s;
		/*
		 * The valuelen is being composed by calculating the length of
		 * the string from the equal sign up until the end of the
		 * string.  The -1 is necessary because the equal sign itself
		 * should not be a part of the value.
		 */
		valuelen = ((s + ns) - equal) - 1;
	} else {
		keylen = ns;
		valuelen = 0;
	}

	key = strndup(s, keylen);
	value = strndup(equal + 1, valuelen);
	if (key == NULL || value == NULL) {
		free(key);
		free(value);
		return (YHTTP_ERRNO);
	}

	/* Insert the key and the value into the hash table. */
	rc = hash_set(ht, key, value);
	free(key);
	free(value);
	if (rc != YHTTP_OK)
		return (rc);

	return (YHTTP_OK);
malformatted:
	parser->err = 400;
	return (YHTTP_OK);
}

static int
parser_rline_method(struct parser *parser, const char *s, size_t ns)
{
	size_t	len;
	int	i;

	for (i = 0; methods[i] != NULL; ++i) {
		len = strlen(methods[i]);
		assert(len != 0);
		if (ns == len && memcmp(s, methods[i], len) == 0)
			break;
	}

	if (methods[i] == NULL) {
		/* No supported method found. */
		parser->err = 501;
	} else
		parser->requ->method = i;

	return (YHTTP_OK);
}

static int
parser_rline_path(struct parser *parser, const char *s, size_t ns)
{
	size_t	i, remaining;

	/* Validate the path. */
	/* The first character must be a slash. */
	if (s == 0 || *s != '/')
		goto malformatted;
	for (i = 1; i < ns; ++i) {
		if (s[i] == '/') {
			/* Two slashes may not follow each other. */
			if (s[i - 1] == '/')
				goto malformatted;
		} else {
			remaining = ns - i;
			if (!(abnf_is_unreserved(s[i]) ||
			      abnf_is_pct_encoded(s + i, remaining) ||
			      abnf_is_sub_delims(s[i]) ||
			      s[i] == ':' || s[i] == '@'))
				goto malformatted;
		}
	}

	/* Extract the path. */
	if ((parser->requ->path = strndup(s, ns)) == NULL)
		return (YHTTP_ERRNO);

	return (YHTTP_OK);
malformatted:
	parser->err = 400;
	return (YHTTP_OK);
}

static int
parser_rline_query(struct parser *parser, const char *s, size_t ns)
{
	struct yhttp_requ_internal	*internal;

	internal = parser->requ->internal;

	return (parser_query(parser, internal->queries, s, ns));
}

static int
parser_rline_target(struct parser *parser, const char *s, size_t ns)
{
	const char	*query;
	size_t		 pathlen, querylen;
	int		 rc;

	query = memchr(s, '?', ns);

	if (query == NULL) {
		/* No '?' found. */
		pathlen = ns;
		querylen = 0;
	} else if (query + 1 == s + ns) {
		/* '?' found but it is the last character. */
		pathlen = ns - 1;
		querylen = 0;
	} else {
		/* '?' found and a query is present. */
		pathlen = query - s;
		/*
		 * The querylen is being composed by calculating the length of
		 * the string from the questionmark up until the end of the
		 * string.  The -1 is necessary because the questionmark itself
		 * should not be a part of the query value.
		 */
		querylen = ns - (query - s) - 1;
	}

	rc = parser_rline_path(parser, s, pathlen);
	if (rc != YHTTP_OK || parser->err)
		return (rc);

	if (querylen != 0)
		rc = parser_rline_query(parser, query + 1, querylen);

	return (rc);
}

static int
parser_rline(struct parser *parser)
{
	unsigned char	*eol, *p, *spaces[2];
	size_t		 len, methodlen, targetlen;
	int		 i, rc;

	if (parser->buf.used == 0)
		return (YHTTP_OK);

	eol = parser_find_eol(parser->buf.buf, parser->buf.used);
	if (eol == NULL)
		return (YHTTP_OK);
	len = eol - parser->buf.buf;

	/* Check for ASCII '\0'. */
	if (memchr(parser->buf.buf, '\0', len) != NULL)
		goto malformatted;

	/* Get the two spaces. */
	i = 0;
	for (p = parser->buf.buf; p != eol && i < 2; ++p) {
		if (*p == ' ')
			spaces[i++] = p;
	}
	if (i != 2)
		goto malformatted;

	/*
	 * Check the position of the two spaces.  They may not occur as the
	 * first character in the line, nor as the last character.
	 */
	if (spaces[0] == parser->buf.buf || spaces[1] == eol - 1)
		goto malformatted;

	/* Extract the method. */
	/* From the start of the line to the first space. */
	methodlen = spaces[0] - parser->buf.buf;
	rc = parser_rline_method(parser, (char *)parser->buf.buf, methodlen);
	if (rc != YHTTP_OK || parser->err)
		goto malformatted;

	/* Extract the target. */
	/* From the character after the first space until the second space. */
	targetlen = spaces[1] - spaces[0] - 1;
	rc = parser_rline_target(parser, (char *)spaces[0] + 1, targetlen);
	if (rc != YHTTP_OK || parser->err)
		goto malformatted;

	/* The HTTP-version can be ignored (for now). */

	/* We are done with the rline. */
	parser->state = PARSER_HEADERS;
	if (*eol == '\r')
		return (buf_pop(&parser->buf, len + 2));
	else
		return (buf_pop(&parser->buf, len + 1));
malformatted:
	parser->err = 400;
	return (YHTTP_OK);
}

static int
parser_header_field(struct parser *parser, const char *s, size_t ns)
{
	struct yhttp_requ_internal	*internal;
	const char			*colon, *p;
	char				*name, *value;
	size_t				 namelen, valuelen;
	int				 rc;

	/* TODO: Trim the OWS. */

	if ((colon = memchr(s, ':', ns)) == NULL)
		goto malformatted;

	/*
	 * Validate the general structure, by checking if the colon is at the
	 * correct position (not the start), followed by a space and has a value
	 * after that space.
	 */
	if (colon == s || colon + 2 >= s + ns || colon[1] != ' ')
		goto malformatted;

	/* Validate the field-name. */
	for (p = s; p != colon; ++p) {
		if (!abnf_is_tchar(*p))
			goto malformatted;
	}
	/* Validate the field-value. */
	for (p = colon + 2; p != s + ns; ++p) {
		if (!isprint(*p))
			goto malformatted;
	}

	/* Extract the field-name and convert it to lower-case. */
	namelen = colon - s;
	if ((name = strndup(s, namelen)) == NULL)
		return (YHTTP_ERRNO);

	/* Extract the field-value. */
	/*
	 * The valuelen is being composed by calculating the length of the
	 * length of the string from two characters after the colon up until the
	 * end of the string.
	 */
	valuelen = ns - (colon - s) - 2;
	if ((value = strndup(colon + 2, valuelen)) == NULL) {
		free(name);
		return (YHTTP_ERRNO);
	}

	internal = parser->requ->internal;

	/* Check if a header field with name is already present. */
	if (hash_get(internal->headers, name) != NULL) {
		free(name);
		free(value);
		goto malformatted;
	}

	/* Insert the header field into the hash table. */
	rc = hash_set(internal->headers, name, value);
	free(name);
	free(value);
	if (rc != YHTTP_OK)
		return (rc);

	return (rc);
malformatted:
	parser->err = 400;
	return (YHTTP_OK);
}

static int
parser_headers(struct parser *parser)
{
	unsigned char	*sol, *eol;
	size_t		 remaining, linelen;
	int		 rc;

	if (parser->buf.used == 0)
		return (YHTTP_OK);

	/* Parse the header fields line by line. */
	sol = parser->buf.buf;
	do {
		/*
		 * The remaining value is being composed by subtracting the
		 * amount of already parsed data from the amount of the totally
		 * available data.
		 */
		remaining = parser->buf.used - (sol - parser->buf.buf);
		eol = parser_find_eol(sol, remaining);
		if (eol == NULL)
			return (YHTTP_OK);

		/* Check for ASCII '\0'. */
		if (memchr(sol, '\0', eol - sol) != NULL)
			goto malformatted;

		linelen = eol - sol;
		rc = parser_header_field(parser, (char *)sol, linelen);
		if (rc != YHTTP_OK || parser->err)
			return (rc);

		/* Go to the next line. */
		if (*eol == '\r')
			sol = eol + 2;
		else
			sol = eol + 1;
	} while (!(*sol == '\r' || *sol == '\n'));

	/*
	 * Check if a Transfer-Encoding has been supplied, which we do not
	 * support.
	 */
	if (yhttp_header(parser->requ, "Transfer-Encoding") != NULL)
		goto unsupported;

	/* Get the Content-Length. */
	rc = parser_cl(parser);
	if (rc != YHTTP_OK || parser->err)
		return (rc);

	/* We are done with the header. */
	parser->state = PARSER_BODY;
	if (*sol == '\r')
		return (buf_pop(&parser->buf, (sol - parser->buf.buf) + 2));
	else
		return (buf_pop(&parser->buf, (sol - parser->buf.buf) + 1));
malformatted:
	parser->err = 400;
	return (YHTTP_OK);
unsupported:
	parser->err = 501;
	return (YHTTP_OK);
}

static int
parser_cl(struct parser *parser)
{
	const char	*value;

	/* Check if a "Content-Length" header field has been supplied. */
	if ((value = yhttp_header(parser->requ, "Content-Length")) == NULL)
		return (YHTTP_OK);

	if (sscanf(value, "%zu", &parser->requ->nbody) == 1)
		return (YHTTP_OK);
	else {
		parser->err = 400;
		return (YHTTP_OK);
	}
}

static int
parser_body(struct parser *parser)
{
	if (parser->buf.used == parser->requ->nbody) {
		parser->requ->body = parser->buf.buf;
		parser->state = PARSER_DONE;
	}

	return (YHTTP_OK);
}

struct parser *
parser_init(void)
{
	struct parser	*parser;

	if ((parser = malloc(sizeof(struct parser))) == NULL)
		return (NULL);

	if ((parser->requ = yhttp_requ_init()) == NULL)
		goto err;

	buf_init(&parser->buf);
	parser->state = PARSER_RLINE;
	parser->err = 0;

	return (parser);
err:
	free(parser);
	return (NULL);
}

void
parser_free(struct parser *parser)
{
	if (parser == NULL)
		return;

	yhttp_requ_free(parser->requ);
	buf_wipe(&parser->buf);
	free(parser);
}

int
parser_parse(struct parser *parser, const unsigned char *data, size_t ndata)
{
	int	rc;

	/*
	 * Every new TCP message is being added to the buffer first.
	 * Afterwards, the appropriate state function (rline, headers, body)
	 * looks for its ending character inside the buffer.
	 * If it has been found, the buffer is wiped up until that ending
	 * character with this wiped content being parsed.  Otherwise, it just
	 * returns.
	 */
	if ((rc = buf_append(&parser->buf, data, ndata)) != YHTTP_OK)
		return (rc);

	if (parser->state == PARSER_RLINE) {
		if ((rc = parser_rline(parser)) != YHTTP_OK)
			return (rc);
	}
	if (parser->state == PARSER_HEADERS) {
		if ((rc = parser_headers(parser)) != YHTTP_OK)
			return (rc);
	}
	if (parser->state == PARSER_BODY) {
		if ((rc = parser_body(parser)) != YHTTP_OK)
			return (rc);
	}

	return (YHTTP_OK);
}
