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

#include "../yhttp.h"
#include "../yhttp-internal.h"

int
main(int argc, char *argv[])
{
	struct yhttp	*yh;
	uint16_t	 i;

	for (i = 0; i < 1024; ++i) {
		if (yhttp_init(i) != NULL)
			errx(1, "yhttp_init: can use port %d", i);
	}

	if ((yh = yhttp_init(8080)) == NULL)
		err(1, "yhttp_init");

	if (yh->is_dispatched != 0)
		errx(1, "yhttp_init: have is_dispatched %d, want 0", yh->is_dispatched);
	if (yh->quit != 0)
		errx(1, "yhttp_init: have quit %d, want 0", yh->quit);
	if (yh->port != 8080)
		errx(1, "yhttp_init: have port %d, want 8080", yh->port);

	yhttp_free(&yh);
	if (yh != NULL)
		errx(1, "yhttp_free: have value, want NULL");
	yhttp_free(&yh);
	yhttp_free(NULL);

	return (0);
}
