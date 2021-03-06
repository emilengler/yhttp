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

#include "../buf.h"
#include "../yhttp.h"

static void	test_buf_init(void);
static void	test_buf_wipe(void);
static void	test_buf_append(void);
static void	test_buf_pop(void);

static void
test_buf_init(void)
{
	struct buf	buf;

	buf_init(&buf);
	if (buf.buf != NULL)
		errx(1, "buf_init: buf.buf is not NULL");
	if (buf.nbuf != 0)
		errx(1, "buf_init: buf.nbuf is not 0");
	if (buf.used != 0)
		errx(1, "buf_init: buf.used is not 0");
}

static void
test_buf_wipe(void)
{
	struct buf	buf;

	buf.buf = NULL;
	buf_wipe(&buf);
	if (buf.buf != NULL)
		errx(1, "buf_wipe: buf.buf is not NULL");
	if (buf.nbuf != 0)
		errx(1, "buf_wipe: buf.nbuf is not 0");
	if (buf.used != 0)
		errx(1, "buf_wipe: buf.used is not 0");

	buf_wipe(NULL);
}

static void
test_buf_append(void)
{
	unsigned char	data[8192];
	struct buf	buf;

	buf_init(&buf);
	arc4random_buf(data, 8192);

	/* Insert initial data. */
	if (buf_append(&buf, data, sizeof(data)) != YHTTP_OK)
		err(1, "buf_append");
	if (buf.nbuf != (sizeof(data) * 2))
		errx(1, "buf_append: have nbuf %zu, want %zu", buf.nbuf, sizeof(data) * 2);
	if (buf.used != sizeof(data))
		errx(1, "buf_append: have used %zu, want %zu", buf.used, sizeof(data));
	if (memcmp(buf.buf, data, sizeof(data)) != 0)
		errx(1, "buf_append: data does not match");

	/* Append it again. */
	if (buf_append(&buf, data, sizeof(data)) != YHTTP_OK)
		err(1, "buf_append");
	if (buf.nbuf != (sizeof(data) * 4))
		errx(1, "buf_append: have nbuf %zu, want %zu", buf.nbuf, sizeof(data) * 4);
	if (buf.used != (sizeof(data) * 2))
		errx(1, "buf_append: have used %zu, want %zu", buf.used, sizeof(data) * 2);
	if (memcmp(buf.buf + 8192, data, sizeof(data)) != 0)
		errx(1, "buf_append: data does not match");
	buf_wipe(&buf);

	/* Append without a reallaction. */
	if (buf_append(&buf, (const unsigned char *)"hello", 5) != YHTTP_OK)
		err(1, "buf_append");
	if (buf_append(&buf, (const unsigned char *)"foo", 3) != YHTTP_OK)
		err(1, "buf_append");
	if (buf.nbuf != 10)
		errx(1, "buf_append: have nbuf %zu, want 10", buf.nbuf);
	if (buf.used != 8)
		errx(1, "buf_append: have used %zu, want 8", buf.used);
	if (memcmp(buf.buf, "hellofoo", 8) != 0)
		errx(1, "buf_append: data does not match");
	buf_wipe(&buf);

	/* Test some integer overflows. */
	buf.used = SIZE_MAX - 10;
	if (buf_append(&buf, data, 11) != YHTTP_EOVERFLOW)
		errx(1, "buf_append: want YHTTP_EOVERFLOW");
	buf_wipe(&buf);

	if (buf_append(&buf, data, (SIZE_MAX / 2) + 1) != YHTTP_EOVERFLOW)
		errx(1, "buf_append: want YHTTP_EOVERFLOW");
	buf_wipe(&buf);

	buf.nbuf = SIZE_MAX - 32;
	buf.used = buf.nbuf;
	if (buf_append(&buf, data, 17) != YHTTP_EOVERFLOW)
		errx(1, "buf_append: want YHTTP_EOVERFLOW");
	buf_wipe(&buf);
}

static void
test_buf_pop(void)
{
	const char		*content;
	struct buf		 buf;

	content = "hello world";

	buf_init(&buf);
	if (buf_append(&buf, (const unsigned char *)content, strlen(content)) != YHTTP_OK)
		errx(1, "buf_append");

	if (buf_pop(&buf, 6) != YHTTP_OK)
		errx(1, "buf_pop");

	if (buf.nbuf != 10)
		errx(1, "buf_pop: have nbuf %zu, want 10", buf.nbuf);
	if (buf.used != 5)
		errx(1, "buf_pop: have used %zu, want 5", buf.used);
	if (memcmp(buf.buf, "world", 5) != 0)
		errx(1, "buf_pop: data does not match");

	buf_wipe(&buf);
}

int
main(int argc, char *argv[])
{
	test_buf_init();
	test_buf_wipe();
	test_buf_append();
	test_buf_pop();
	return (0);
}
