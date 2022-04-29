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

#include "../hash.h"
#include "../yhttp.h"
#include "../yhttp-internal.h"

static void	test_resp_status(void);
static void	test_resp_header(void);
static void	test_resp_body(void);

static void
test_resp_status(void)
{
	struct yhttp_requ_internal	*internal;
	struct yhttp_requ		*requ;

	if ((requ = yhttp_requ_init()) == NULL)
		errx(1, "yhttp_resp_status: yhttp_requ_init");
	internal = requ->internal;

	/* Test with invalid parameters. */
	if (yhttp_resp_status(NULL, 1000) != YHTTP_EINVAL)
		errx(1, "yhttp_resp_status: want YHTTP_EINVAL");
	if (yhttp_resp_status(requ, 0) != YHTTP_EINVAL)
		errx(1, "yhttp_resp_status: want YHTTP_EINVAL");
	if (yhttp_resp_status(requ, 1000) != YHTTP_EINVAL)
		errx(1, "yhttp_resp_status: want YHTTP_EINVAL");

	if (yhttp_resp_status(requ, 400) != YHTTP_OK)
		errx(1, "yhttp_resp_status: want YHTTP_OK");
	if (internal->resp->status != 400)
		errx(1, "yhttp_resp_status: have status %d, want 400", internal->resp->status);

	yhttp_requ_free(requ);
}

static void
test_resp_header(void)
{
	struct yhttp_requ_internal	*internal;
	struct yhttp_requ		*requ;
	struct hash			*node;

	if ((requ = yhttp_requ_init()) == NULL)
		errx(1, "yhttp_resp_header: yhttp_requ_init");
	internal = requ->internal;

	/* Test with invalid parameters. */
	if (yhttp_resp_header(NULL, NULL, NULL) != YHTTP_EINVAL)
		errx(1, "yhttp_resp_header: want YHTTP_EINVAL");
	if (yhttp_resp_header(requ, NULL, NULL) != YHTTP_EINVAL)
		errx(1, "yhttp_resp_header: want YHTTP_EINVAL");
	if (yhttp_resp_header(requ, "TRANSFER-encoding", "foo") != YHTTP_EINVAL)
		errx(1, "yhttp_resp_header: want YHTTP_EINVAL");
	if (yhttp_resp_header(requ, "CONTENT-lENgth", "foo") != YHTTP_EINVAL)
		errx(1, "yhttp_resp_header: want YHTTP_EINVAL");

	/* Set a header field. */
	if (yhttp_resp_header(requ, "lOcAtiOn", "index.html") != YHTTP_OK)
		errx(1, "yhttp_resp_header: want YHTTP_OK");
	node = hash_get(internal->resp->headers, "Location");
	if (node == NULL)
		errx(1, "yhttp_resp_header: lOcAtiOn is unset");
	if (strcmp(node->name, "lOcAtiOn") != 0)
		errx(1, "yhttp_resp_header: have name %s, want lOcAtiOn", node->name);
	if (strcmp(node->value, "index.html") != 0)
		errx(1, "yhttp_resp_header: have value %s, want index.html", node->value);

	/* Modify this header field. */
	if (yhttp_resp_header(requ, "LOCATION", "foo.html") != YHTTP_OK)
		errx(1, "yhttp_resp_header: want YHTTP_OK");
	node = hash_get(internal->resp->headers, "location");
	if (node == NULL)
		errx(1, "yhttp_resp_header: LOCATION IS unset");
	if (strcmp(node->name, "LOCATION") != 0)
		errx(1, "yhttp_resp_header: have name %s, want LOCATION", node->name);
	if (strcmp(node->value, "foo.html") != 0)
		errx(1, "yhttp_resp_header: have value %s, want foo.html", node->value);

	/* Unset the header field. */
	if (yhttp_resp_header(requ, "location", NULL) != YHTTP_OK)
		errx(1, "yhttp_resp_header: want YHTTP_OK");
	node = hash_get(internal->resp->headers, "LOCATiON");
	if (node != NULL)
		errx(1, "yhttp_resp_header: LOCATION was not unset");

	yhttp_requ_free(requ);
}

static void
test_resp_body(void)
{
	struct yhttp_requ_internal	*internal;
	struct yhttp_requ		*requ;
	const char			*body;

	if ((requ = yhttp_requ_init()) == NULL)
		errx(1, "yhttp_resp_body: yhttp_requ_init");
	internal = requ->internal;

	body = "foo";
	if (yhttp_resp_body(requ, (unsigned char *)body, strlen(body)) != YHTTP_OK)
		errx(1, "yhttp_resp_body: want YHTTP_OK");
	if (internal->resp->nbody != strlen(body))
		errx(1, "yhttp_resp_body: nbody was not set");
	if (memcmp(internal->resp->body, body, strlen(body)) != 0)
		errx(1, "yhttp_resp_body: body was not copied");

	/* Unset the body. */
	if (yhttp_resp_body(requ, NULL, 0) != YHTTP_OK)
		errx(1, "yhttp_resp_body: want YHTTP_OK");
	if (internal->resp->nbody != 0)
		errx(1, "yhttp_resp_body: nbody was not unset");
	if (internal->resp->body != NULL)
		errx(1, "yhttp_resp_body: body was not unset");

	yhttp_requ_free(requ);
}

int
main(int argc, char *argv[])
{
	test_resp_status();
	test_resp_header();
	test_resp_body();
	return (0);
}
