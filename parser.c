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
#include <stdlib.h>
#include <string.h>

#include "buf.h"
#include "hash.h"
#include "parser.h"
#include "yhttp.h"
#include "yhttp-internal.h"

/* TODO: Reduce the amount of strdup(3) and strndup(3) calls. */

static int		 parser_abnf_is_pct_encoded(const char *);
static int		 parser_abnf_is_unreserved(int);
static int		 parser_abnf_is_sub_delims(int);

static unsigned char	*parser_find_eol(unsigned char *, size_t);

static int		 parser_query(struct parser *, struct hash *[],
				      const char *);

static int		 parser_rline_method(struct parser *, const char *);
static int		 parser_rline_path(struct parser *, const char *);
static int		 parser_rline_query(struct parser *, const char *);
static int		 parser_rline_target(struct parser *, const char *);

static int		 parser_rline(struct parser *);
static int		 parser_headers(struct parser *);
static int		 parser_body(struct parser *);

/* enum yhttp_method <=> string associations. */
static const char	*methods[] = {
	"GET",		/* YHTTP_GET */
	"HEAD",		/* YHTTP_HEAD */
	"POST",		/* YHTTP_POST */
	"PUT",		/* YHTTP_PUT */
	"DELETE",	/* YHTTP_DELETE */
	"PATCH",	/* YHTTP_PATCH */
	NULL
};

static int
parser_abnf_is_pct_encoded(const char *s)
{
	if (s[0] != '%')
		return (0);
	if (!isxdigit(s[1]) || !isxdigit(s[2]))
		return (0);
	if (s[1] == '0' && s[2] == '0')
		return (0);
	return (1);
}

static int
parser_abnf_is_unreserved(int c)
{
	return (isalnum(c) || c == '-' || c == '.' || c == '_' || c == '~');
}

static int
parser_abnf_is_sub_delims(int c)
{
	return (c == '!' || c == '$' || c == '&' || c == '\'' || c == '(' ||
		c == ')' || c == '*' || c == '+' || c == ',' || c == ';' ||
		c == '=');
}

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
parser_query(struct parser *parser, struct hash *ht[], const char *query)
{
	const char	*current, *next, *ampersand, *equal;
	char		*key, *value;
	size_t		 i, len;
	int		 quit, rc;

	len = strlen(query);

	/* Validate the query. */
	for (i = 0; i < strlen(query); ++i) {
		if (!(parser_abnf_is_pct_encoded(query + i) ||
		      parser_abnf_is_unreserved(query[i]) ||
		      parser_abnf_is_sub_delims(query[i]) ||
		      query[i] == '@' || query[i] == ':' ||
		      query[i] == '/' || query[i] == '?'))
			goto malformatted;
	}

	next = query;
	quit = 0;
	while (!quit) {
		/*
		 * This is the core of the actual query string parser, which
		 * works as follows:
		 * 1. Determine the end of the current key/value pair
		 *    - Look for '&' first and '\0' afterwards.
		 * 2. Determine if there is a value.
		 *    - Check for the presence of '=' before '&'.
		 *    - If not, set equal to ampersand in order to let
		 *      strdup(3) return the zero-length string.
		 * 3. Extract the key and the value accordingly.
		 * 4. Check if the key is already present within the hash
		 *    table.
		 * 5. Insert the key and the value into the hash table.
		 * 6. Continue with the character that follows to the ampersand
		 *    or stop if the end of the query string has been reached.
		 */

		current = next;

		/* Determine the end of this key/value pair. */
		if ((ampersand = strchr(current, '&')) == NULL) {
			/* Last key/value pair in the query string. */
			ampersand = strchr(current, '\0');
			quit = 1;
		}
		/* Determine if there is a value. */
		equal = strchr(current, '=');
		if (equal != NULL && equal > ampersand)
			equal = ampersand;

		/* Some boundary checks. */
		if (current == ampersand || current == equal)
			goto malformatted;

		/* Extract the key and the value. */
		if ((key = strndup(current, equal - current)) == NULL)
			return (YHTTP_ERRNO);
		value = strndup(equal + 1, ampersand - equal - 1);
		if (value == NULL) {
			free(key);
			return (YHTTP_ERRNO);
		}

		/* Check if the key is already present in the hash table. */
		if (hash_get(ht, key) != NULL) {
			free(key);
			free(value);
			goto malformatted;
		}

		/* Insert the key and the value into the hash table. */
		rc = hash_set(ht, key, value);
		free(key);
		free(value);
		if (rc != YHTTP_OK)
			return (rc);

		/* Extract the next key/value pair. */
		next = ampersand + 1;
	}

	return (YHTTP_OK);
malformatted:
	parser->state = PARSER_ERR;
	return (YHTTP_OK);
}

static int
parser_rline_method(struct parser *parser, const char *method)
{
	size_t	i;

	for (i = 0; methods[i] != NULL; ++i) {
		if (strcmp(methods[i], method) == 0)
			break;
	}

	if (methods[i] == NULL) {
		/* No supported method found. */
		parser->state = PARSER_ERR;
	} else
		parser->requ->method = i;

	return (YHTTP_OK);
}

static int
parser_rline_path(struct parser *parser, const char *path)
{
	size_t	i, len;

	len = strlen(path);

	/* Validate the path. */
	if (path[0] != '/')
		goto malformatted;
	for (i = 1; i < len; ++i) {
		if (path[i] == '/') {
			/* Two slashes cannot follow each other. */
			if (path[i - 1] == '/')
				goto malformatted;
		} else {
			if (!(parser_abnf_is_unreserved(path[i]) ||
			      parser_abnf_is_pct_encoded(path + i) ||
			      parser_abnf_is_sub_delims(path[i]) ||
			      path[i] == ':' || path[i] == '@'))
				goto malformatted;
		}
	}

	/* Copy the path. */
	if ((parser->requ->path = strdup(path)) == NULL)
		return (YHTTP_ERRNO);

	return (YHTTP_OK);
malformatted:
	parser->state = PARSER_ERR;
	return (YHTTP_OK);
}

static int
parser_rline_query(struct parser *parser, const char *query)
{
	struct yhttp_requ_internal	*internal;

	internal = parser->requ->internal;

	return (parser_query(parser, internal->queries, query));
}

static int
parser_rline_target(struct parser *parser, const char *target)
{
	const char	*questionmark;
	char		*path, *query;
	int		 rc;

	/* Check if a query is present. */
	questionmark = strchr(target, '?');

	/* Extract the path depending on the presence of a query. */
	if (questionmark == NULL)
		path = strdup(target);
	else
		path = strndup(target, questionmark - target);
	if (path == NULL)
		return (YHTTP_ERRNO);

	/* Parse the path. */
	rc = parser_rline_path(parser, path);
	free(path);
	if (rc != YHTTP_OK || parser->state == PARSER_ERR)
		return (rc);

	/* Parse the query, if present. */
	if (questionmark != NULL && questionmark[1] != '\0') {
		if ((query = strdup(questionmark + 1)) == NULL)
			return (YHTTP_ERRNO);
		rc = parser_rline_query(parser, query);
		free(query);
		if (rc != YHTTP_OK || parser->state == PARSER_ERR)
			return (rc);
	}

	return (YHTTP_OK);
}

static int
parser_rline(struct parser *parser)
{
	unsigned char	*eol, *spaces[2];
	char		*method;
	size_t		 len;
	int		 rc;

	eol = parser_find_eol(parser->buf.buf, parser->buf.used);
	if (eol == NULL)
		return (YHTTP_OK);
	len = eol - parser->buf.buf;

	/* See if we can treat the binary string like a normal string. */
	if (memchr(parser->buf.buf, '\0', len) != NULL)
		goto malformatted;

	/* Find the two spaces in the rline. */
	spaces[0] = memchr(parser->buf.buf, ' ', len);
	if (spaces[0] == NULL)
		goto malformatted;
	spaces[1] = memchr(spaces[0] + 1, ' ', eol - spaces[0] + 1);
	if (spaces[1] == NULL)
		goto malformatted;

	/* Parse the method. */
	method = strndup((char *)parser->buf.buf, spaces[0] - parser->buf.buf);
	if (method == NULL)
		return (YHTTP_ERRNO);
	rc = parser_rline_method(parser, method);
	free(method);
	if (rc != YHTTP_OK || parser->state == PARSER_ERR)
		return (rc);

	return (YHTTP_OK);
malformatted:
	parser->state = PARSER_ERR;
	return (YHTTP_OK);
}

static int
parser_headers(struct parser *parser)
{
	return (YHTTP_OK);
}

static int
parser_body(struct parser *parser)
{
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

	switch (parser->state) {
	case PARSER_RLINE:
		return (parser_rline(parser));
	case PARSER_HEADERS:
		return (parser_headers(parser));
	case PARSER_BODY:
		return (parser_body(parser));
	default:
		assert(0);
	}

	return (YHTTP_OK);
}
