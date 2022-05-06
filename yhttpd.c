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
#include <sys/stat.h>

#include <err.h>
#include <pwd.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "yhttp.h"

static struct yhttp	*yh = NULL;

static void		parse_args(int, char *[]);
static void		sandbox(void);
static void		sighdlr(int);
static __dead void	usage(void);
static void		yhttp_cb(struct yhttp_requ *, void *);

static char		*directory = NULL;
static uint16_t		 port = 8080;

static void
parse_args(int argc, char *argv[])
{
	int	ch, port_arg;

	/* Parse the command line arguments. */
	while ((ch = getopt(argc, argv, "hp:")) != -1) {
		switch (ch) {
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
	directory = argv[optind];
}

static void
sandbox(void)
{
	struct passwd	*pw;
	struct stat	 st;

	/*
	 * Obtain the uid of the owner of directory, because we will setuid()
	 * to it afterwards.
	 */
	if (stat(directory, &st) == -1)
		err(1, "stat %s", directory);
	if (st.st_uid == 0)
		errx(1, "%s cannot be owned by root", directory);

	/* Obtain the gid of the uid. */
	if ((pw = getpwuid(st.st_uid)) == NULL)
		err(1, "getpwuid %d", st.st_uid);

	if (chroot(directory) == -1)
		err(1, "chroot %s", directory);

	/* Drop the privileges. */
	if (setgroups(1, &pw->pw_gid) == -1 ||
	    setresgid(pw->pw_gid, pw->pw_gid, pw->pw_gid) == -1 ||
	    setresuid(pw->pw_uid, pw->pw_uid, pw->pw_uid) == -1)
		err(1, "cannot drop root privileges");

#ifdef __OpenBSD__
	if (pledge("stdio inet rpath", "") == -1)
		err(1, "pledge");
#endif
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
	errx(1, "[-p PORT] DIRECTORY");
}

static void
yhttp_cb(struct yhttp_requ *requ, void *udata)
{
}

int
main(int argc, char *argv[])
{
	int	rc;

	if (geteuid() != 0)
		errx(1, "need root privileges");

	/* Setup the process. */
	parse_args(argc, argv);
	sandbox();
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
