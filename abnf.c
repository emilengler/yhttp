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

#include "config.h"

#include <sys/types.h>

#include <ctype.h>

int	abnf_is_pct_encoded(const char *, size_t);
int	abnf_is_unreserved(int);
int	abnf_is_sub_delims(int);
int	abnf_is_tchar(int);

int
abnf_is_pct_encoded(const char *s, size_t ns)
{
	if (ns < 3)
		return (0);
	if (s[0] != '%')
		return (0);
	if (!isxdigit(s[1]) || !isxdigit(s[2]))
		return (0);
	if (s[1] == '0' && s[2] == '0')
		return (0);
	return (1);
}

int
abnf_is_unreserved(int c)
{
	return (isalnum(c) || c == '-' || c == '.' || c == '_' || c == '~');
}

int
abnf_is_sub_delims(int c)
{
	return (c == '!' || c == '$' || c == '&' || c == '\'' || c == '(' ||
		c == ')' || c == '*' || c == '+' || c == ',' || c == ';' ||
		c == '=');
}

int
abnf_is_tchar(int c)
{
	return (c == '!' || c == '#' || c == '$' || c == '%' || c == '&' ||
		c == '\'' || c == '*' || c == '+' || c == '-' || c == '.' ||
		c == '^' || c == '_' || c == '`' || c == '|' || c == '~' ||
		isalnum(c));
}
