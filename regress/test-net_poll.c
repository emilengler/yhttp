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
#include <stdlib.h>

#include "../net.c"

static void	test_net_poll_init(void);
static void	test_net_poll_free(void);
static void	test_net_poll_grow(void);
static void	test_net_poll_add(void);
static void	test_net_poll_grow(void);

static void
test_net_poll_init(void)
{
	struct poll_data	pd;

	net_poll_init(&pd);

	if (pd.parsers != NULL)
		errx(1, "net_poll_init: have pd.parsers not NULL, want NULL");
	if (pd.pfds != NULL)
		errx(1, "net_poll_init: have pd.pfds not NULL, want NULL");
	if (pd.npfds != 0)
		errx(1, "net_poll_init: have pd.npfds %zu, want 0", pd.npfds);
	if (pd.used != 0)
		errx(1, "net_poll_init: have pd.used %zu, want 0", pd.used);

	net_poll_free(&pd);
}

static void
test_net_poll_free(void)
{
	net_poll_free(NULL);
}

static void
test_net_poll_grow(void)
{
	struct poll_data	pd;
	size_t			i;
	int			rc;

	net_poll_init(&pd);

	if ((rc = net_poll_grow(&pd)) != YHTTP_OK)
		errx(1, "net_poll_grow: have %d, want YHTTP_OK", rc);
	if (pd.npfds != NGROW)
		errx(1, "net_poll_grow: have pd.npfds %zu, want %d", pd.npfds, NGROW);
	if (pd.used != 0)
		errx(1, "net_poll_grow: have pd.used %zu, want 0", pd.used);
	for (i = 0; i < pd.npfds; ++i) {
		if (pd.parsers[i] != NULL)
			errx(1, "net_poll_grow: have pd.parsers[%zu] not NULL, want NULL", i);
		if (pd.pfds[i].fd != -1)
			errx(1, "net_poll_grow: have pd.pfds[%zu].fd not -1, want -1", i);
		if (pd.pfds[i].events != 0)
			errx(1, "net_poll_grow: have pd.pfds[%zu].events %hd, want 0", i, pd.pfds[i].events);
		if (pd.pfds[i].revents != 0)
			errx(1, "net_poll_grow: have pd.pfds[%zu].revents %hd, want 0", i, pd.pfds[i].revents);

		/* For the next test. */
		pd.pfds[i].fd = 0;
	}
	pd.used = NGROW;

	if ((rc = net_poll_grow(&pd)) != YHTTP_OK)
		errx(1, "net_poll_grow: have %d, want YHTTP_OK", rc);
	if (pd.npfds != NGROW * 2)
		errx(1, "net_poll_grow: have pd.npfds %zu, want %d", pd.npfds, NGROW * 2);
	if (pd.used != NGROW)
		errx(1, "net_poll_grow: have pd.used %zu, want %d", pd.used, NGROW);
	for (i = 0; i < NGROW; ++i) {
		if (pd.pfds[i].fd != 0)
			errx(1, "net_poll_grow: have pd.pfds[%zu].fd not 0, want 0", i);
	}
	for (i = NGROW; i < pd.npfds; ++i) {
		if (pd.parsers[i] != NULL)
			errx(1, "net_poll_grow: have pd.parsers[%zu] not NULL, want NULL", i);
		if (pd.pfds[i].fd != -1)
			errx(1, "net_poll_grow: have pd.pfds[%zu].fd not -1, want -1", i);
		if (pd.pfds[i].events != 0)
			errx(1, "net_poll_grow: have pd.pfds[%zu].events %hd, want 0", i, pd.pfds[i].events);
		if (pd.pfds[i].revents != 0)
			errx(1, "net_poll_grow: have pd.pfds[%zu].revents %hd, want 0", i, pd.pfds[i].revents);
	}

	net_poll_free(&pd);
}

static void
test_net_poll_add(void)
{
	struct poll_data	pd;
	size_t			i;
	int			rc;

	net_poll_init(&pd);

	/* Test in combination with net_poll_grow(). */
	if ((rc = net_poll_add(&pd, 1, POLLIN)) != YHTTP_OK)
		errx(1, "net_poll_add: have %d, want YHTTP_OK", rc);

	if (pd.pfds == NULL)
		errx(1, "net_poll_add: have pd.pfds NULL, want not NULL");
	if (pd.npfds != NGROW)
		errx(1, "net_poll_add: have pd.npfds %zu, want %d", pd.npfds, NGROW);
	if (pd.used != 1)
		errx(1, "net_poll_add: have pd.used %zu, want 1", pd.used);

	if (pd.parsers[0]->state != PARSER_RLINE)
		errx(1, "net_poll_add: have pd.parsers[0]->state %d, want PARSER_RLINE", pd.parsers[0]->state);
	if (pd.pfds[0].fd != 1)
		errx(1, "net_poll_add: have pd.pfds[0].fd %d, want 1", pd.pfds[0].fd);
	if (pd.pfds[0].events != POLLIN)
		errx(1, "net_poll_add: have pd.pfds[0].events %d, want POLLIN", pd.pfds[0].events);
	if (pd.pfds[0].revents != 0)
		errx(1, "net_poll_add: have pd.pfds[0].revents %d, want 0", pd.pfds[0].revents);

	/* Test the search for a free slot. */
	for (i = 1; i < pd.npfds; ++i)
		pd.pfds[i].fd = 5;
	pd.pfds[55].fd = -1;

	if ((rc = net_poll_add(&pd, 55, POLLOUT)) != YHTTP_OK)
		errx(1, "net_poll_add: have %d, want YHTTP_OK", rc);

	if (pd.pfds[55].fd != 55)
		errx(1, "net_poll_add: have pd.pfds[55].fd %d, want 55", pd.pfds[55].fd);
	if (pd.pfds[55].events != POLLOUT)
		errx(1, "net_poll_add: have pd.pfds[55].events %hd, want POLLOUT", pd.pfds[55].events);

	net_poll_free(&pd);
}

static void
test_net_poll_del(void)
{
	struct poll_data	pd;
	size_t			i;
	int			rc;

	net_poll_init(&pd);

	for (i = 0; i < NGROW + 1; ++i) {
		if ((rc = net_poll_add(&pd, i, POLLIN)) != YHTTP_OK)
			errx(1, "net_poll_del: net_poll_add %d", rc);
	}

	net_poll_del(&pd, 55);
	if (pd.parsers[55] != NULL)
		errx(1, "net_poll_del: pd.parsers[55] is not NULL, want NULL");
	if (pd.pfds[55].fd != -1)
		errx(1, "net_poll_del: pd.pfds[55].fd is not -1");
	if (pd.pfds[55].events != 0)
		errx(1, "net_poll_del: pd.pfds[55].events is not 0");
	if (pd.pfds[55].revents != 0)
		errx(1, "net_poll_del: pd.pfds[55].revents is not 0");

	net_poll_free(&pd);
}

int
main(int argc, char *argv[])
{
	test_net_poll_init();
	test_net_poll_free();
	test_net_poll_grow();
	test_net_poll_add();
	test_net_poll_del();

	return (0);
}
