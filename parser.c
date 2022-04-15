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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "buf.h"
#include "parser.h"
#include "yhttp.h"
#include "yhttp-internal.h"

static unsigned char	*parser_find_eol(unsigned char *, size_t);

static int		 parser_rline_method(struct parser *, const char *);

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
