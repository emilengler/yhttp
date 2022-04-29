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

#include "../hash.c"
#include "../parser.c"

struct test {
	const char	*input;
	const char	*path;
	int		 malformatted;
	int		 has_query;
};

static const struct test	tests[] = {
	{ "/", "/", 0, 0 },
	{ "/?", "/", 0, 0 },
	{ "/foo", "/foo", 0, 0 },
	{ "/foo?", "/foo", 0, 0 },
	{ "/foo/bar?", "/foo/bar", 0, 0 },
	{ "/foo?foo", "/foo", 0, 1 },
	{ "/foo??", "/foo", 0, 1 },
	{ "/?foo", "/", 0, 1 },
	{ "", NULL, 1, 0 },
	{ "foo", NULL, 1, 0 },
	{ "?foo", NULL, 1, 0 },
	{ NULL, NULL, 0, 0 }
};

int
main(int argc, char *argv[])
{
	const struct test		*t;
	struct yhttp_requ_internal	*internal;
	struct parser			*parser;
	size_t				 i;
	int				 rc;

	for (t = tests; t->input != NULL; ++t) {
		if ((parser = parser_init()) == NULL)
			errx(1, "parser_rline_target: parser_init");

		rc = parser_rline_target(parser, t->input, strlen(t->input));
		if (rc != YHTTP_OK)
			errx(1, "parser_rline_target: have %d, want YHTTP_OK", rc);

		if (t->malformatted) {
			if (!parser->err)
				errx(1, "parser_rline_target: want err");
		} else {
			if (parser->err)
				errx(1, "parser_rline_target: have err");
			if (strcmp(t->path, parser->requ->path) != 0)
				errx(1, "parser_rline_target: path was not extracted");

			if (t->has_query) {
				/* Just check if the hash table contains some content. */
				internal = parser->requ->internal;
				for (i = 0; i < NHASH; ++i) {
					if (internal->queries[i] != NULL)
						break;
				}
				if (i == NHASH)
					errx(1, "parser_rline_target: hash table contains no entries");
			}
		}

		parser_free(parser);
	}

	return (0);
}
