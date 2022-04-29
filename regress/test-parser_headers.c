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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../parser.c"

static const char	*test = "Foo: Bar\n"
				"Bar: Foo\r\n"
				"foz: baz\n"
				"\r\n"
				"foo";

int
main(int argc, char *argv[])
{
	struct parser	*parser;
	const char	*v;
	int		 rc;

	if ((parser = parser_init()) == NULL)
		errx(1, "parser_headers: parser_init");

	if (buf_append(&parser->buf, (unsigned char *)test, strlen(test)) != YHTTP_OK)
		errx(1, "parser_headers: buf_append");

	rc = parser_headers(parser);
	if (rc != YHTTP_OK)
		errx(1, "parser_headers: have %d, want YHTTP_OK", rc);
	if (parser->state != PARSER_BODY)
		errx(1, "parser_headers: have state %d, want PARSER_BODY", parser->state);
	if (parser->buf.used != 3)
		errx(1, "parser_headers: have parser->buf.used %zu, want 3", parser->buf.used);
	if (memcmp(parser->buf.buf, "foo", 3) != 0)
		errx(1, "parser_headers: buf_pop() failed");

	if ((v = yhttp_header(parser->requ, "Foo")) == NULL)
		errx(1, "parser_headers: Foo is NULL");
	if (strcmp(v, "Bar") != 0)
		errx(1, "parser_headers: have value %s, want Bar", v);
	if ((v = yhttp_header(parser->requ, "BAR")) == NULL)
		errx(1, "parser_headers: Bar is NULL");
	if (strcmp(v, "Foo") != 0)
		errx(1, "parser_headers: have value %s, want Foo", v);
	if ((v = yhttp_header(parser->requ, "fOz")) == NULL)
		errx(1, "parser_headers: foz is NULL");
	if (strcmp(v, "baz") != 0)
		errx(1, "parser_headers: have value %s, want foz", v);

	/* TODO: Add test for Transfer-Encoding. */
	/* TODO: Add test for Content-Length. */

	parser_free(parser);

	return (0);
}
