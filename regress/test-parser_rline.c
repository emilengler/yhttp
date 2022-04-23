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

static const char	*malformatted_tests[] = {
	"\r\n",
	" \r\n",
	"  \r\n",
	"   \r\n",
	"UNKNOWN /foo HTTP/1.1\r\n",
	"GET foo HTTP/1.1\r\n",
	"GET  HTTP/1.1\r\n",
	NULL
};

int
main(int argc, char *argv[])
{
	struct parser	*parser;
	const char	*s;
	size_t		 i;
	int		 rc;

	/* Test with malformatted input. */
	for (i = 0; malformatted_tests[i] != NULL; ++i) {
		if ((parser = parser_init()) == NULL)
			errx(1, "parser_rline: parser_init");

		rc = buf_append(&parser->buf, (const unsigned char *)malformatted_tests[i], strlen(malformatted_tests[i]));
		if (rc != YHTTP_OK)
			errx(1, "parser_rline: have %d, want YHTTP_OK", rc);

		rc = parser_rline(parser);
		if (rc != YHTTP_OK)
			errx(1, "parser_rline: have %d, want YHTTP_OK", rc);
		if (parser->state != PARSER_ERR)
			errx(1, "parser_rline: have state %d, want PARSER_ERR", parser->state);

		parser_free(parser);
	}

	/* Test with valid input. */
	if ((parser = parser_init()) == NULL)
		errx(1, "parser_rline: parser_init");

	s = "PUT /foo?foz=baz&baz&fozz=fooo& HTTP/1.1\nfoo";
	rc = buf_append(&parser->buf, (const unsigned char *)s, strlen(s));
	if (rc != YHTTP_OK)
		errx(1, "parser_rline: have %d, want YHTTP_OK", rc);

	rc = parser_rline(parser);
	if (rc != YHTTP_OK)
		errx(1, "parser_rline: have %d, want YHTTP_OK", rc);

	/* Validate the state switch. */
	if (parser->state != PARSER_HEADERS)
		errx(1, "parser_rline: have state %d, want PARSER_HEADERS", parser->state);
	if (parser->buf.used != 3)
		errx(1, "parser_rline: have parser->buf.used %zu, want 3", parser->buf.used);
	if (memcmp(parser->buf.buf, "foo", 3) != 0)
		errx(1, "parser_rline: buf_pop() failed");

	/* Validate the parsing (only partially). */
	if (parser->requ->method != YHTTP_PUT)
		errx(1, "parser_rline: have method %d, want YHTTP_PUT", parser->requ->method);

	if ((s = yhttp_query(parser->requ, "foz")) == NULL)
		errx(1, "parser_rline: foz is NULL");
	if (strcmp(s, "baz") != 0)
		errx(1, "parser_rline: have foz = %s, want baz", s);

	if ((s = yhttp_query(parser->requ, "baz")) == NULL)
		errx(1, "parser_rline: baz is NULL");
	if (strcmp(s, "") != 0)
		errx(1, "parser_rline: have baz = %s, want empty string", s);

	if ((s = yhttp_query(parser->requ, "fozz")) == NULL)
		errx(1, "parser_rline: fozz is NULL");
	if (strcmp(s, "fooo") != 0)
		errx(1, "parser_rline: have fozz = %s, want fooo", s);

	parser_free(parser);

	return (0);
}
