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
#include <stdlib.h>

#include "../yhttp.h"
#include "../yhttp-internal.h"

int
main(int argc, char *argv[])
{
	struct yhttp	*yh;

	if ((yh = yhttp_init()) == NULL)
		err(1, "yhttp_init");

	if (yh->is_dispatched != 0)
		errx(1, "yhttp_init: have is_dispatched %d, want 0", yh->is_dispatched);
	if (yh->quit != 0)
		errx(1, "yhttp_init: have quit %d, want 0", yh->quit);
	return (0);
}
