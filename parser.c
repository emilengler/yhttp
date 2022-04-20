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

static int		 parser_abnf_is_pct_encoded(const char *);
static int		 parser_abnf_is_unreserved(int);
static int		 parser_abnf_is_sub_delims(int);

static unsigned char	*parser_find_eol(unsigned char *, size_t);

static int		 parser_query(struct parser *, struct hash *[],
				      const char *, size_t);
static int		 parser_keyvalue(struct parser *, struct hash *[],
					 const char *, size_t);

static int		 parser_rline(struct parser *);
static int		 parser_headers(struct parser *);
static int		 parser_body(struct parser *);

static int
parser_abnf_is_pct_encoded(const char *s)
{
	if (s[0] != '%')
		return (0);
	if (!isxdigit(s[1]) || !isxdigit(s[2]))
		return (0);
	if (s[1] == '0' || s[2] == '0')
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
parser_query(struct parser *parser, struct hash *ht[], const char *s,
	     size_t ns)
{
	const char	*start, *end;
	size_t		 i;
	int		 rc;

	/* Validate the query string. */
	for (i = 0; i < ns; ++i) {
		if (!(parser_abnf_is_unreserved(s[i]) ||
		      parser_abnf_is_pct_encoded(s + i) ||
		      parser_abnf_is_sub_delims(s[i]) ||
		      s[i] == ':' || s[i] == '@' || s[i] == '/' ||
		      s[i] == '?'))
			goto malformatted;
	}

	/* Parse all key/value pairs. */
	start = s;
	do {
		/* Find the end of the key/value pair. */
		end = memchr(start, '&', ns - (start - s));
		if (end == NULL) {
			/* The last key/value pair. */
			end = s + ns;
		}
		if (end == start) {
			/* The key/value pair is empty. */
			++start;
			continue;
		}

		rc = parser_keyvalue(parser, ht, start, end - start);
		if (rc != YHTTP_OK || parser->state == PARSER_ERR)
			return (rc);

		/* Go to the next key/value pair. */
		start = end + 1;
	} while(end != s + ns);

	return (YHTTP_OK);
malformatted:
	parser->state = PARSER_ERR;
	return (YHTTP_OK);
}

static int
parser_keyvalue(struct parser *parser, struct hash *ht[], const char *s,
		size_t ns)
{
	const char	*equal;
	char		*key, *value;
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
		key = strndup(s, equal - s);
		value = strndup(equal + 1, ((s + ns) - equal) - 1);
	} else {
		key = strndup(s, ns);
		value = strdup("");
	}
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
	parser->state = PARSER_ERR;
	return (YHTTP_OK);
}

static int
parser_rline(struct parser *parser)
{
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

	if (parser->state == PARSER_RLINE && parser->buf.used != 0) {
		if ((rc = parser_rline(parser)) != YHTTP_OK)
			return (rc);
	}
	if (parser->state == PARSER_HEADERS && parser->buf.used != 0) {
		if ((rc = parser_headers(parser)) != YHTTP_OK)
			return (rc);
	}
	if (parser->state == PARSER_BODY && parser->buf.used != 0) {
		if ((rc = parser_body(parser)) != YHTTP_OK)
			return (rc);
	}

	return (YHTTP_OK);
}
