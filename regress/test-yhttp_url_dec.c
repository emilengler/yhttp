/*
 * Copyright (c) 2022 Emil Engler <engler+yhttp@unveil2.org>
 * Copyright (c) 2018, 2020 Kristaps Dzonsons <kristaps@bsd.lv>
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

#include "../yhttp.h"

struct	test {
	const char	*input;
	const char	*output;
};

static const struct test	tests[] = {
	{ "", "" },
	{ "foobar", "foobar" },
	{ "foo+bar", "foo bar" },
	{ "foo~bar", "foo~bar" },
	{ "foo-bar", "foo-bar" },
	{ "foo_bar", "foo_bar" },
	{ "foo.bar", "foo.bar" },
	{ "foo.bar.", "foo.bar." },
	{ "foo.bar.-", "foo.bar.-" },
	{ "-_foo.bar.-", "-_foo.bar.-" },
	{ "-_foo%2Bbar.-", "-_foo+bar.-" },
	{ "-_foo%2bbar.-", "-_foo+bar.-" },
	{ "-_foo%09bar.-", "-_foo\tbar.-" },
	{ "%09-_foo%09bar.-", "\t-_foo\tbar.-" },
	{ "%09-_foo%09bar.-%09", "\t-_foo\tbar.-\t" },
	{ "%09-_foo%25%09bar.-%09", "\t-_foo%\tbar.-\t" },
	{ "-_foo%2509%7dbar.-", "-_foo%09}bar.-" },
	{ "-_foo%2509%7Dbar.-", "-_foo%09}bar.-" },
	{ "%09%09%09%09", "\t\t\t\t" },
	{ "%F8", "\xf8" },
	{ "%-9", NULL },
	{ "% 9", NULL },
	{ "%9 ", NULL },
	{ "%9", NULL },
	{ "% ", NULL },
	{ "%", NULL },
	{ "foo%zubar", NULL },
	{ "foo%", NULL },
	{ "%%%%%%", NULL },
	{ "ABCDEFGHIJKLMNOPQRSTUVWXYZ", "ABCDEFGHIJKLMNOPQRSTUVWXYZ" },
	{ "abcdefghijklmnopqrstuvwxyz", "abcdefghijklmnopqrstuvwxyz" },
	{ "0123456789-_.~", "0123456789-_.~" },
	{ "%21%23%24%25%26%27%28%29%2A%2B%2C%2F%3A%3B%3D%3F%40%5B%5D", "!#$%&'()*+,/:;=?@[]" },
	{ NULL, NULL }
};

int
main(int argc, char *argv[])
{
	const struct test	*t;
	char			*dec;

	if (yhttp_url_dec(NULL) != NULL)
		errx(1, "yhttp_url_dec: passed NULL, have value");

	for (t = tests; t->input != NULL; ++t) {
		dec = yhttp_url_dec(t->input);
		if (dec == NULL && t->output != NULL)
			errx(1, "yhttp_url_dec: have NULL, want value");
		else if (dec != NULL && t->output == NULL)
			errx(1, "yhttp_url_dec: have value, want NULL");
		else {
			if (dec != t->output && strcmp(dec, t->output) != 0)
				errx(1, "yhttp_url_dec: have %s, want %s", dec, t->output);
		}
		free(dec);
	}

	return (0);
}
