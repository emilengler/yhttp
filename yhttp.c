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

#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "hash.h"
#include "yhttp.h"
#include "yhttp-internal.h"
#include "net.h"

struct yhttp *
yhttp_init(uint16_t port)
{
	struct yhttp	*yh;

	/*
	 * Running on a well-known port requires running with root privileges,
	 * which is strongly discouraged.  Therefore, only ports outside of
	 * the well-known range are permitted for usage.
	 */
	if (port < 1024)
		return (NULL);

	if ((yh = malloc(sizeof(struct yhttp))) == NULL)
		return (NULL);

	memset(yh->pipe, -1, sizeof(yh->pipe));
	yh->is_dispatched = 0;
	yh->port = port;

	return (yh);
}

void
yhttp_free(struct yhttp **yh)
{
	if (yh == NULL || *yh == NULL)
		return;

	free(*yh);
	*yh = NULL;
}

char *
yhttp_header(struct yhttp_requ *requ, const char *name)
{
	struct yhttp_requ_internal	*internal;
	struct hash			*node;

	internal = requ->internal;
	if ((node = hash_get(internal->headers, name)) == NULL)
		return (NULL);
	else
		return (node->value);
}

char *
yhttp_query(struct yhttp_requ *requ, const char *key)
{
	struct yhttp_requ_internal	*internal;
	struct hash			*node;

	internal = requ->internal;
	if ((node = hash_get(internal->queries, key)) == NULL)
		return (NULL);
	else
		return (node->value);
}

struct yhttp_requ *
yhttp_requ_init(void)
{
	struct yhttp_requ_internal	*internal;
	struct yhttp_requ		*requ;

	if ((requ = malloc(sizeof(struct yhttp_requ))) == NULL)
		return (NULL);

	requ->path = NULL;
	requ->client_ip = NULL;

	/*
	 * The request body will be stored inside the buffer of the parser
	 * rather than in a separately allocated space, meaning the clean-up
	 * is not in our scope.
	 */
	requ->body = NULL;
	requ->nbody = 0;

	requ->method = YHTTP_GET;

	/* Initialize the internal field. */
	if ((internal = malloc(sizeof(struct yhttp_requ_internal))) == NULL)
		goto err;
	requ->internal = internal;

	internal->headers = NULL;
	internal->queries = NULL;

	if ((internal->headers = hash_init()) == NULL)
		goto err;
	if ((internal->queries = hash_init()) == NULL)
		goto err;

	return (requ);
err:
	free(requ);
	if (internal != NULL) {
		hash_free(internal->headers);
		hash_free(internal->queries);
		free(internal);
	}
	return (NULL);
}

void
yhttp_requ_free(struct yhttp_requ *requ)
{
	struct yhttp_requ_internal	*internal;

	if (requ == NULL)
		return;

	internal = requ->internal;

	/* See the comment regarding requ->body inside yhttp_requ_init(). */
	free(requ->path);
	free(requ->client_ip);
	free(requ);

	hash_free(internal->headers);
	hash_free(internal->queries);
	free(internal);
}

/*
 * The following function is largely based upon kcgi(3)s khttp_urlencode(),
 * and is distributed under the following license:
 *
 * Copyright (c) 2022 Emil Engler <engler+yhttp@unveil2.org>
 * Copyright (c) 2012, 2014--2017 Kristaps Dzonsons <kristaps@bsd.lv>
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
char *
yhttp_url_enc(const char *s)
{
	char	*res;
	char	 ch;
	size_t	 sz, i;
	int	 rc;

	if (s == NULL)
		return (NULL);

	/*
	 * In the worst case, the resulting string will be three times larger
	 * than the original, because every percent-encoded sequence is three
	 * bytes long.  Instead of performing several calls to realloc(3), we
	 * assume the worst case and simply make the resulting string three
	 * times larger, so we do not have to worry about any reallocations.
	 */

	/* Check for multiplication overflow. */
	sz = strlen(s) + 1;
	if (sz > SIZE_MAX / 3)
		return (NULL);

	if ((res = malloc(sz * 3)) == NULL)
		return (NULL);

	i = 0;
	while ((ch = *s++) != '\0') {
		if (isalnum((unsigned char)ch) || ch == '-' || ch == '_' ||
		    ch == '.' || ch == '~') {
			res[i++] = ch;
			continue;
		}
		else if (ch == ' ') {
			res[i++] = '+';
			continue;
		}

		/* Convert byte to percent-encoded sequence. */
		rc = snprintf(res + i, 4, "%%%.2hhX", (unsigned char)ch);
		if (rc != 3) {
			free(res);
			return (NULL);
		}
		i += 3;
	}
	res[i] = '\0';

	return (res);
}

char *
yhttp_url_dec(const char *s)
{
	char	*res;
	char	 tmp[3];
	char	 ch;
	long	 val;
	size_t	 i;

	if (s == NULL)
		return (NULL);

	/* The result cannot be bigger than the original string. */
	if ((res = malloc(strlen(s) + 1)) == NULL)
		return (NULL);

	i = 0;
	while ((ch = *s) != '\0') {
		if (ch == '%') {
			/*
			 * Check if this is actually a valid and legal
			 * percent-encoded sequence, by checking if the
			 * following two bytes are valid hexadecimal
			 * characters.
			 */
			if (!isxdigit(s[1]) || !isxdigit(s[2]))
				goto err;

			/* Convert the ASCII chars into an integer. */
			strlcpy(tmp, s + 1, sizeof(tmp));
			val = strtol(tmp, NULL, 16);
			assert(val >= 0x0 && val <= 0xFF);

			/* %00 is forbidden for security reasons. */
			if (val == 0)
				goto err;

			/* Append it to the result. */
			res[i++] = val;
			s += 3;
			continue;
		} else if (ch == '+')
			res[i++] = ' ';
		else
			res[i++] = ch;
		++s;
	}
	res[i] = '\0';

	return (res);
err:
	free(res);
	return (NULL);
}

int
yhttp_dispatch(struct yhttp *yh, void (*cb)(struct yhttp_requ *, void *),
	       void *udata)
{
	int	rc;

	if (yh == NULL || cb == NULL)
		return (YHTTP_EINVAL);

	/* The instance has already been dispatched. */
	if (yh->is_dispatched)
		return (YHTTP_EBUSY);
	yh->is_dispatched = 1;

	/*
	 * We are going to create a pipe(2) for the server, so that shutdown
	 * can be communicated somehow (by sending EOF through the pipe(2)).
	 */
	if (pipe(yh->pipe) == -1)
		return (YHTTP_ERRNO);

	rc = net_dispatch(yh->port, yh->pipe[0], cb, udata);

	/* Destroy the pipe. */
	close(yh->pipe[0]);
	close(yh->pipe[1]);
	memset(yh->pipe, -1, sizeof(yh->pipe));

	yh->is_dispatched = 0;

	return (YHTTP_OK);
}

int
yhttp_stop(struct yhttp *yh)
{
	if (yh == NULL)
		return (YHTTP_EINVAL);
	if (!yh->is_dispatched)
		return (YHTTP_ENOENT);

	/* We tell the server to shutdown by closing its pipe(2). */
	close(yh->pipe[1]);
	yh->pipe[1] = -1;

	return (YHTTP_OK);
}
