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

#ifndef PARSER_H
#define PARSER_H

enum parser_state {
	PARSER_RLINE,	/* Request line. */
	PARSER_HEADERS,	/* Header fields. */
	PARSER_BODY,	/* Message body. */
	PARSER_DONE,	/* Request has been parsed successfully. */
	PARSER_ERR	/* Request is malformatted. */
};

struct parser {
	struct yhttp_requ	*requ;
	struct buf		 buf;
	enum parser_state	 state;
	int			 err_code;
};

struct parser	*parser_init(void);
void		 parser_free(struct parser *);

int		 parser_parse(struct parser *, const unsigned char *, size_t);

#endif
