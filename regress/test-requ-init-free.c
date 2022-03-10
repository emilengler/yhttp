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

#include "../yhttp.h"
#include "../yhttp-internal.h"

int
main(int argc, char *argv[])
{
	struct yhttp_requ	*requ;

	if ((requ = yhttp_requ_init()) == NULL)
		err(1, "yhttp_requ_init");

	if (requ->path != NULL)
		errx(1, "yhttp_requ_init: requ->path is not NULL");
	if (requ->client_ip != NULL)
		errx(1, "yhttp_requ_init: requ->client_ip is not NULL");
	if (requ->body != NULL)
		errx(1, "yhttp_requ_init: requ->body is not NULL");
	if (requ->nbody != 0)
		errx(1, "yhttp_requ_init: requ->nbody is not 0");
	if (requ->method != YHTTP_GET)
		errx(1, "yhttp_requ_init: requ->method is not YHTTP_GET");
	if (requ->internal != NULL)
		errx(1, "yhttp_requ_init: requ->internal is not NULL");

	yhttp_requ_free(requ);

	return (0);
}
