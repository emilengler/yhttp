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
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "yhttp.h"

static struct yhttp	*yh = NULL;

static void		parse_args(int, char *[]);
static void		sighdlr(int);
static __dead void	usage(void);
static void		yhttp_cb(struct yhttp_requ *, void *);

static int		 virtual_hosts = 0;
static uint16_t		 port = 8080;

static void
parse_args(int argc, char *argv[])
{
	int	ch, port_arg;

	/* Parse the command line arguments. */
	while ((ch = getopt(argc, argv, "hp:")) != -1) {
		switch (ch) {
		case 'h':
			virtual_hosts = 1;
			break;
		case 'p':
			port_arg = atoi(optarg);
			if (port_arg < 1024)
				errx(1, "port must be > 1023");
			else if (port_arg > 65535)
				errx(1, "port must be < 65536");
			port = (uint16_t) port_arg;
			break;
		default:
			usage();
		}
	}

	if (optind == argc) {
		/* DIRECTORY not supplied. */
		usage();
	}
}

static void
sighdlr(int sig)
{
	int	rc;

	fprintf(stderr, "Received signal %d, exiting...\n", sig);
	if ((rc = yhttp_stop(yh)) != YHTTP_OK)
		errx(1, "yhttp_stop: %d\n", rc);
}

static __dead void
usage(void)
{
	errx(1, "[-h] [-p PORT] DIRECTORY");
}

static void
yhttp_cb(struct yhttp_requ *requ, void *udata)
{
}

int
main(int argc, char *argv[])
{
	int	rc;

	parse_args(argc, argv);

	signal(SIGINT, sighdlr);
	signal(SIGTERM, sighdlr);

	if ((yh = yhttp_init(port)) == NULL)
		err(1, "yhttp_init");

	fprintf(stderr, "Listening on TCP port %d\n", port);
	if ((rc = yhttp_dispatch(yh, yhttp_cb, NULL)) != YHTTP_OK)
		errx(1, "yhttp_dispatch: %d\n", rc);

	yhttp_free(&yh);

	return (0);
}
