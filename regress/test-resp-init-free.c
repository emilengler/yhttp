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
#include "../hash.c"

int
main(int argc, char *argv[])
{
	struct yhttp_resp	*resp;
	size_t			 i;

	if ((resp = yhttp_resp_init()) == NULL)
		errx(1, "yhttp_resp_init");

	if (resp->headers == NULL)
		errx(1, "yhttp_resp_init: resp->headers is NULL");
	for (i = 0; i < NHASH; ++i) {
		if (resp->headers[i] != NULL)
			errx(1, "yhttp_resp_init: hash_init");
	}

	if (resp->body != NULL)
		errx(1, "yhttp_resp_init: resp->body is not NULL");
	if (resp->nbody != 0)
		errx(1, "yhttp_resp_init: resp->nbody is not 0");
	if (resp->status != 200)
		errx(1, "yhttp_resp_init: resp->status is not 200");

	yhttp_resp_free(resp);
	yhttp_resp_free(NULL);

	return (0);
}
