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

#include <stdlib.h>
#include <string.h>

#include "buf.h"
#include "parser.h"
#include "yhttp.h"
#include "yhttp-internal.h"

static int	parser_rline(struct parser *, const unsigned char *, size_t);
static int	parser_headers(struct parser *, const unsigned char *, size_t);
static int	parser_body(struct parser *, const unsigned char *, size_t);

static int
parser_rline(struct parser *parser, const unsigned char *data, size_t ndata)
{
	return (YHTTP_OK);
}

static int
parser_headers(struct parser *parser, const unsigned char *data, size_t ndata)
{
	return (YHTTP_OK);
}

static int
parser_body(struct parser *parser, const unsigned char *data, size_t ndata)
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
	switch (parser->state) {
	case PARSER_RLINE:
		return (parser_rline(parser, data, ndata));
	case PARSER_HEADERS:
		return (parser_headers(parser, data, ndata));
	case PARSER_BODY:
		return (parser_body(parser, data, ndata));
	default:
		return (YHTTP_EINVAL);
	}

	return (YHTTP_OK);
}
