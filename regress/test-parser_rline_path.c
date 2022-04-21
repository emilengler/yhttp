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
	const char	*input;
	int		 malformatted;
};

static const struct test	tests[] = {
	{ "/foo:@", 0 },
	{ "/foo%20bar", 0 },
	{ "/foo/bar/foz", 0 },
	{ "/foo%", 1 },
	{ "/foo%KV", 1 },
	{ "/foo%A", 1 },
	{ "/foo%00", 1 },
	{ "", 1 },
	{ "foo", 1 },
	{ "/foo//bar", 1 },
	{ "//", 1 },
	{ "/foo//", 1 },
	{ NULL, 0 }
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

		rc = parser_rline_path(parser, t->input, strlen(t->input));
		if (rc != YHTTP_OK)
			errx(1, "parser_rline_path: have %d, want YHTTP_OK", rc);

		if (t->malformatted) {
			if (parser->state != PARSER_ERR)
				errx(1, "parser_rline_path: have state %d, want PARSER_ERR", parser->state);
		} else {
			if (parser->state == PARSER_ERR)
				errx(1, "parser_rline_path: have state PARSER_ERR");
			if (strcmp(t->input, parser->requ->path) != 0)
				errx(1, "parser_rline_path: path not copied");
		}

		parser_free(parser);
	}

	return (0);
}
