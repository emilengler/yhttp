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

#include "yhttp.h"
#include "yhttp-internal.h"

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
