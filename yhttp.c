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
		return (strdup(""));	/* Return empty allocated string. */

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

		/* Convert octet to percent-encoded sequence. */
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
