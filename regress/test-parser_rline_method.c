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

struct test {
	const char		*input;
	enum yhttp_method	 output;
};

static const struct test	tests[] = {
	{ "GET", YHTTP_GET },
	{ "HEAD", YHTTP_HEAD },
	{ "POST", YHTTP_POST },
	{ "PUT", YHTTP_PUT },
	{ "DELETE", YHTTP_DELETE },
	{ "PATCH", YHTTP_PATCH },
	{ NULL, -1 }
};

int
main(int argc, char *argv[])
{
	const struct test	*t;
	struct parser		*parser;
	int			 rc;

	for (t = tests; t->input != NULL; ++t) {
		if ((parser = parser_init()) == NULL)
			errx(1, "parser_rline_method: parser_init");

		if ((rc = parser_rline_method(parser, t->input)) != YHTTP_OK)
			errx(1, "parser_rline_method: have %d, want YHTTP_OK", rc);

		if (parser->state != PARSER_RLINE)
			errx(1, "parser_rline_method: have %d, want PARSER_RLINE", parser->state);

		if (parser->requ->method != t->output)
			errx(1, "parser_rline_method: have %d, want %s", parser->requ->method, t->input);

		parser_free(parser);
	}

	/* Try out an invalid method. */
	if ((parser = parser_init()) == NULL)
		errx(1, "parser_rline_method: parser_init");

	if ((rc = parser_rline_method(parser, "FOO")) != YHTTP_OK)
		errx(1, "parser_rline_method: have %d, want YHTTP_OK", rc);

	if (parser->state != PARSER_ERR)
		errx(1, "parser_rline_method: have %d, want PARSER_ERR", parser->state);

	parser_free(parser);

	return (0);
}
