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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"

/*
 * Return an allocated string in the printf(3) style.
 * It must be passed to free(3) afterwards.
 * See asprintf(3).
 */
char *
util_aprintf(const char *fmt, ...)
{
	char	*s;
	va_list	 v1, v2;
	int	 sz;

	s = NULL;
	va_start(v1, fmt);
	va_copy(v2, v1);

	sz = vsnprintf(NULL, 0, fmt, v1);
	if (sz < 0)
		goto end;

	if ((s = malloc(sz + 1)) == NULL)
		goto end;

	if (vsnprintf(s, sz + 1, fmt, v2) < 0) {
		free(s);
		s = NULL;
	}
end:
	va_end(v1);
	va_end(v2);
	return (s);
}
