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
	int		 malformatted;
	const char	*input;
	const char	*key;
	const char	*value;
};

static const struct test	tests[] = {
	{ 0, "foo=bar", "foo", "bar" },
	{ 0, "foo=", "foo", "" },
	{ 0, "foo", "foo", "" },
	{ 1, "=", NULL, NULL },
	{ 1, NULL, NULL, NULL }
};

int
main(int argc, char *argv[])
{
	struct hash		**ht, *node;
	const struct test	 *t;
	struct parser		 *parser;
	int			  rc;

	for (t = tests; t->input != NULL; ++t) {
		if ((ht = hash_init()) == NULL)
			errx(1, "parser_keyvalue: hash_init");
		if ((parser = parser_init()) == NULL)
			errx(1, "parser_keyvalue: parser_init");

		rc = parser_keyvalue(parser, ht, t->input, strlen(t->input));
		if (rc != YHTTP_OK)
			errx(1, "parser_keyvalue: have %d, want YHTTP_OK", rc);

		if (t->malformatted) {
			if (!parser->err)
				errx(1, "parser_keyvalue: want err");
		} else {
			if ((node = hash_get(ht, t->key)) == NULL)
				errx(1, "parser_keyvalue: node is NULL, want not NULL");
			if (strcmp(node->name, t->key) != 0)
				errx(1, "parser_keyvalue: have node->name %s, want %s", node->name, t->key);
			if (strcmp(node->value, t->value) != 0)
				errx(1, "parser_keyvalue: have node->value %s, want %s", node->value, t->value);
		}

		hash_free(ht);
		parser_free(parser);
	}

	return (0);
}
