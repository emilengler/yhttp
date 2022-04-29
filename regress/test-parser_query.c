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
	"foo[]",
	"%",
	"%00",
	"=",
	"=&",
	NULL
};

int
main(int argc, char *argv[])
{
	struct hash	**ht, *node;
	struct parser	 *parser;
	const char	 *query;
	size_t		  i;
	int		  rc;

	/* Test the malformatted inputs. */
	for (i = 0; malformatted_tests[i] != NULL; ++i) {
		if ((ht = hash_init()) == NULL)
			errx(1, "parser_query: hash_init");
		if ((parser = parser_init()) == NULL)
			errx(1, "parser_query: parser_init");

		rc = parser_query(parser, ht, malformatted_tests[i], strlen(malformatted_tests[i]));
		if (rc != YHTTP_OK)
			errx(1, "parser_query: have %d, want YHTTP_OK", rc);
		if (!parser->err)
			errx(1, "parser_query: want err");

		parser_free(parser);
		hash_free(ht);
	}

	/* Test with normal input. */
	if ((ht = hash_init()) == NULL)
		errx(1, "parser_query: hash_init");
	if ((parser = parser_init()) == NULL)
		errx(1, "parser_query: parser_init");

	query = "foo=bar&foz=foz&&&bar&baz=&foo";
	rc = parser_query(parser, ht, query, strlen(query));
	if (rc != YHTTP_OK)
		errx(1, "parser_query: have %d, want YHTTP_OK", rc);
	if (parser->err)
		errx(1, "parser_query: have err");

	if ((node = hash_get(ht, "foo")) == NULL)
		errx(1, "parser_query: foo is NULL");
	if (strcmp("", node->value) != 0)
		errx(1, "parser_query: have value %s, want empty string", node->value);

	if ((node = hash_get(ht, "foz")) == NULL)
		errx(1, "parser_query: foz is NULL");
	if (strcmp("foz", node->value) != 0)
		errx(1, "parser_query: have value %s, want foz", node->value);

	if ((node = hash_get(ht, "bar")) == NULL)
		errx(1, "parser_query: bar is NULL");
	if (strcmp("", node->value) != 0)
		errx(1, "parser_query: have value %s, want empty string", node->value);

	if ((node = hash_get(ht, "baz")) == NULL)
		errx(1, "parser_query: baz is NULL");
	if (strcmp("", node->value) != 0)
		errx(1, "parser_query: have value %s, want empty string", node->value);

	parser_free(parser);
	hash_free(ht);

	return (0);
}
