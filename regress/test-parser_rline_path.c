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
	enum parser_state	 state;
};

static const struct test	tests[] = {
	{ "/", PARSER_RLINE },
	{ "/f1o", PARSER_RLINE },
	{ "/foo-.~+():@%FB", PARSER_RLINE },
	{ "foo", PARSER_ERR },
	{ "/foo//foo", PARSER_ERR },
	{ "/foo%00", PARSER_ERR },
	{ "/foo%GB", PARSER_ERR },
	{ "/foo%%%", PARSER_ERR },
	{ "/foo%", PARSER_ERR },
	{ NULL, PARSER_RLINE }
};

int
main(int argc, char *argv[])
{
	const struct test	*t;
	struct parser		*parser;
	int			 rc;

	for (t = tests; t->input != NULL; ++t) {
		if ((parser = parser_init()) == NULL)
			errx(1, "parser_rline_path: parser_init");

		if ((rc = parser_rline_path(parser, t->input)) != YHTTP_OK)
			errx(1, "parser_rline_path: have %d, want YHTTP_OK", rc);

		if (parser->state != t->state)
			errx(1, "parser_rline_path: have state %d, want %d", parser->state, t->state);
		if (parser->state != PARSER_ERR && strcmp(parser->requ->path, t->input) != 0)
			errx(1, "parser_rline_path: string was not copied");

		parser_free(parser);
	}

	return (0);
}
