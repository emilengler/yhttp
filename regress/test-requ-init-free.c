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

#include "../yhttp.h"
#include "../yhttp-internal.h"
#include "../hash.c"

int
main(int argc, char *argv[])
{
	struct yhttp_requ_internal	*internal;
	struct yhttp_requ		*requ;
	struct yhttp_resp		*default_resp;
	size_t				 i;

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

	internal = requ->internal;
	if (internal == NULL)
		errx(1, "yhttp_requ_init: requ->internal is NULL");
	if (internal->headers == NULL)
		errx(1, "yhttp_requ_init: internal->headers is NULL");
	if (internal->queries == NULL)
		errx(1, "yhttp_requ_init: internal->queries is NULL");
	for (i = 0; i < NHASH; ++i) {
		if (internal->headers[i] != NULL || internal->queries[i])
			errx(1, "yhttp_requ_init: hash_init");
	}

	if ((default_resp = yhttp_resp_init()) == NULL)
		errx(1, "yhttp_requ_init: yhttp_resp_init");
	hash_free(default_resp->headers);
	hash_free(internal->resp->headers);
	default_resp->headers = NULL;
	internal->resp->headers = NULL;
	if (memcmp(default_resp, internal->resp, sizeof(struct yhttp_resp)) != 0)
		errx(1, "yhttp_requ_init: internal->resp is not initialized");
	yhttp_resp_free(default_resp);

	yhttp_requ_free(requ);
	yhttp_requ_free(NULL);

	return (0);
}
