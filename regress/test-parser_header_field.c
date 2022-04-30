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
	"foo",
	"f:",
	"f: ",
	"f:  ",
	":",
	": ",
	":   ",
	": f",
	"foo: b[r",
	NULL
};

int
main(int argc, char *argv[])
{
	struct parser	*parser;
	const char	*s, *v;
	size_t		 i;
	int		 rc;

	/* Test with malformatted input. */
	for (i = 0; malformatted_tests[i] != NULL; ++i) {
		if ((parser = parser_init()) == NULL)
			errx(1, "parser_init");

		rc = parser_header_field(parser, malformatted_tests[i], strlen(malformatted_tests[i]));
		if (rc != YHTTP_OK)
			errx(1, "parser_header: have %d, want YHTTP_OK", rc);
		if (!parser->err)
			errx(1, "parser_header: want err");

		parser_free(parser);
	}

	/* Test with valid input. */
	if ((parser = parser_init()) == NULL)
		errx(1, "parser_init");

	s = "FOO: bar";
	rc = parser_header_field(parser, s, strlen(s));
	if (rc != YHTTP_OK)
		errx(1, "parser_header: have %d, want YHTTP_OK", rc);
	if (parser->err)
		errx(1, "parser_header: have err");
	if ((v = yhttp_header(parser->requ, "foo")) == NULL)
		errx(1, "parser_header: FOO is NULL");
	if (strcmp(v, "bar") != 0)
		errx(1, "parser_header: have value %s, want bar", v);

	/* Test with duplicate field name. */
	s = "fOo: bar";
	rc = parser_header_field(parser, s, strlen(s));
	if (rc != YHTTP_OK)
		errx(1, "parser_header: have %d, want YHTTP_OK", rc);
	if (!parser->err)
		errx(1, "parser_header: want err");

	parser_free(parser);

	return (0);
}
