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

#ifndef YHTTP_INTERNAL_H
#define YHTTP_INTERNAL_H

struct yhttp {
	int		pipe[2];	/* pipe(2). */
	int		is_dispatched;	/* yhttp_dispatch() is running. */
	uint16_t	port;		/* The TCP port. */
};

struct yhttp_requ_internal {
	struct hash		**headers;	/* Header fields. */
	struct hash		**queries;	/* Query fields. */
	struct yhttp_resp	 *resp;
};

struct yhttp_resp {
	struct hash	**headers;	/* The header fields. */
	unsigned char	 *body;		/* The message body. */
	size_t		  nbody;	/* The length of the body. */
	int		  status;	/* The HTTP status code. */
};

struct yhttp_requ	*yhttp_requ_init(void);
void			 yhttp_requ_free(struct yhttp_requ *);

struct yhttp_resp	*yhttp_resp_init(void);
void			 yhttp_resp_free(struct yhttp_resp *);

#endif
