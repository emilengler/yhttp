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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "buf.h"
#include "yhttp.h"

static int	buf_grow(struct buf *, size_t);

static int
buf_grow(struct buf *buf, size_t ndata)
{
	unsigned char	*n_buf;
	size_t		 n_nbuf;

	/*
	 * If buf->used + ndata is smaller than buf->nbuf, it means
	 * that we do not need to perform any sort of reallocation,
	 * as the buffer is already big enough to hold the new data.
	 */
	if (SIZE_MAX - buf->used < ndata)	/* buf->used + ndata */
		return (YHTTP_EOVERFLOW);
	if ((buf->used + ndata) < buf->nbuf)
		return (YHTTP_OK);

	/*
	 * The new size of the buffer composites as follows:
	 * old space + (ndata * 2)
	 * The following will perform several integer overflow checks in
	 * order to determine if the reallocation can take place.
	 */
	if (ndata > SIZE_MAX / 2)		/* ndata * 2 */
		return (YHTTP_EOVERFLOW);
	if (SIZE_MAX - buf->nbuf < (ndata * 2))	/* buf->nbuf + (ndata * 2) */
		return (YHTTP_EOVERFLOW);
	n_nbuf = buf->nbuf + (ndata * 2);

	/* Now we can perform the actual reallocation. */
	if ((n_buf = realloc(buf->buf, n_nbuf)) == NULL)
		return (YHTTP_ERRNO);

	buf->buf = n_buf;
	buf->nbuf = n_nbuf;

	return (YHTTP_OK);
}

void
buf_init(struct buf *buf)
{
	buf->buf = NULL;
	buf->nbuf = 0;
	buf->used = 0;
}

void
buf_wipe(struct buf *buf)
{
	if (buf == NULL)
		return;

	free(buf->buf);
	buf_init(buf);
}

int
buf_append(struct buf *buf, const unsigned char *data, size_t ndata)
{
	int	rc;

	if ((rc = buf_grow(buf, ndata)) != YHTTP_OK)
		return (rc);

	memcpy(buf->buf + buf->used, data, ndata),
	buf->used += ndata;	/* Overflow was already done by buf_grow(). */

	return (YHTTP_OK);
}
