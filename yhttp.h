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

#ifndef YHTTP_H
#define YHTTP_H

enum yhttp_err {
	YHTTP_ERRNO = -1,
	YHTTP_OK = 0,
	YHTTP_ENOENT,
	YHTTP_EINVAL,
	YHTTP_EOVERFLOW
};

enum yhttp_method {
	YHTTP_GET,
	YHTTP_HEAD,
	YHTTP_POST,
	YHTTP_PUT,
	YHTTP_DELETE,
	YHTTP_PATCH
};

struct yhttp_requ {
	char			*path;
	char			*client_ip;
	unsigned char		*body;
	size_t			 nbody;
	enum yhttp_method	 method;

	void			*internal;
};

struct yhttp	*yhttp_init(void);
void		 yhttp_free(struct yhttp **);

char		*yhttp_header(struct yhttp_requ *, const char *);
char		*yhttp_query(struct yhttp_requ *, const char *);

char		*yhttp_url_enc(const char *);
char		*yhttp_url_dec(const char *);

int		 yhttp_resp_status(struct yhttp *, int);
int		 yhttp_resp_header(struct yhttp *, const char *,
				   const char *);
int		 yhttp_resp_body(struct yhttp *, const unsigned char *,
				 size_t);

int		 yhttp_dispatch(struct yhttp *,
				void (*)(struct yhttp_requ *, void *), void *);
int		 yhttp_stop(struct yhttp *);

#endif
