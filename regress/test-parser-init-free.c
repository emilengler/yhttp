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

#include <err.h>
#include <stdlib.h>
#include <string.h>

#include "../buf.h"
#include "../parser.h"
#include "../yhttp.h"
#include "../yhttp-internal.h"

int
main(int argc, char *argv[])
{
	struct parser		*parser;
	struct yhttp_requ	*default_requ;
	void			*tmp;
	struct buf		 default_buf;

	if ((parser = parser_init()) == NULL)
		err(1, "parser_init");

	/* Validate the initialized content. */
	if ((default_requ = yhttp_requ_init()) == NULL)
		err(1, "yhttp_requ_init");
	tmp = default_requ->internal;
	default_requ->internal = parser->requ->internal;
	if (memcmp(parser->requ, default_requ, sizeof(struct yhttp_requ)) != 0)
		errx(1, "parser_init: parser->requ is not yhttp_requ_init");
	default_requ->internal = tmp;
	yhttp_requ_free(default_requ);

	buf_init(&default_buf);
	if (memcmp(&parser->buf, &default_buf, sizeof(struct buf)) != 0)
		errx(1, "parser_init: parser->buf is not buf_init");
	buf_wipe(&default_buf);

	if (parser->state != PARSER_RLINE)
		errx(1, "parser_init: parser->state is not PARSER_RLINE");

	parser_free(parser);
	parser_free(NULL);
	return (0);
}
