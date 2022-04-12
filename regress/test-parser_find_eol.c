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
	const char	*str;
	ssize_t		 offset;
};

static const struct test	tests[] = {
	{ "", -1 },
	{ "test", -1 },
	{ "test\r", -1 },
	{ "test\n", 4 },
	{ "test\r\n", 4 },
	{ "test\r\ntest", 4 },
	{ "test\ntest", 4 },
	{ "test\rfoo\n", 8 },
	{ "test\rfoo\r\n", 8 },
	{ NULL, -1 }
};

int
main(int argc, char *argv[])
{
	const struct test	*t;
	unsigned char		*r;
	ssize_t			 offset;

	for (t = tests; t->str != NULL; ++t) {
		r = parser_find_eol((unsigned char *)t->str, strlen(t->str));

		if (t->offset == -1 || r == NULL) {
			if (!(t->offset == -1 && r == NULL))
				errx(1, "parser_find_eol: have/want NULL on %s", t->str);
		} else {
			offset = r - (const unsigned char *)t->str;
			if (t->offset != offset)
				errx(1, "parser_find_eol: have offset %zd, want %zd on %s", offset, t->offset, t->str);
		}
	}

	return (0);
}
