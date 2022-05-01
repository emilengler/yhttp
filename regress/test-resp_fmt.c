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
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "../resp.c"

static void	test_resp_fmt_rline(void);
static void	test_resp_fmt_header(void);

static void
test_resp_fmt_rline(void)
{
	char			*rline;

	if ((rline = resp_fmt_rline(200)) == NULL)
		errx(1, "resp_fmt_rline");
	if (strcmp(rline, "HTTP/1.1 200 OK\r\n") != 0)
		errx(1, "resp_fmt_rline: have %s, want HTTP/1.1 200 OK\r\n", rline);
	free(rline);

	if ((rline = resp_fmt_rline(1)) == NULL)
		errx(1, "resp_fmt_rline");
	if (strcmp(rline, "HTTP/1.1 1 NULL\r\n") != 0)
		errx(1, "resp_fmt_rline: have %s, want HTTP/1.1 1 NULL\r\n", rline);
	free(rline);
}

static void
test_resp_fmt_header(void)
{
	char		*header;
	struct hash	 node;

	node.name = (char *)"Foo";
	node.value = (char *)"Bar";

	if ((header = resp_fmt_header(&node)) == NULL)
		errx(1, "resp_fmt_header");
	if (strcmp(header, "Foo: Bar\r\n") != 0)
		errx(1, "resp_fmt_header: have %s, want Foo: Bar\r\n", header);
	free(header);
}

int
main(int argc, char *argv[])
{
	test_resp_fmt_rline();
	test_resp_fmt_header();
	return (0);
}
