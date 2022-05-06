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
#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "yhttp.h"

#define MAX_FS	1000 * 1000 * 512	/* 512 MB */

struct mime_type {
	const char	*ext;
	const char	*type;
};

static const char	*get_content_type(const char *);
static long		 get_file_sz(FILE *);
static void		 parse_args(int, char *[]);
static void		 sandbox(void);
static void		 sighdlr(int);
static __dead void	 usage(void);
static void		 yhttp_cb(struct yhttp_requ *, void *);

static const struct mime_type	types[] = {
	{ "html", "text/html" },
	{ "htm", "text/html" },
	{ "css", "text/css" },
	{ "txt", "text/plain" },

	{ "gif", "image/gif" },
	{ "jpg", "image/jpeg" },
	{ "jpeg", "image/jpeg" },
	{ "png", "image/png" },
	{ "pdf", "application/pdf" },

	{ NULL, "application/octet-stream" }
};

static struct yhttp	*yh = NULL;
static char		*directory = NULL;
static uint16_t		 port = 8080;

static const char *
get_content_type(const char *path)
{
	const char	*ext;
	size_t		 i, len;

	/* Find the last dot. */
	len = strlen(path);
	for (i = len - 1; i != 0; --i) {
		if (path[i] == '.')
			break;
	}
	if (path[i] != '.')
		return ("application/octet-stream");
	ext = path + i + 1;

	for (i = 0; types[i].ext != NULL; ++i) {
		if (strcasecmp(ext, types[i].ext) == 0)
			break;
	}

	return (types[i].type);
}

static long
get_file_sz(FILE *fp)
{
	long	sz;

	fseek(fp, 0L, SEEK_END);
	sz = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	return (sz);
}

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
	const char	*path, *body, *content_type;
	unsigned char	*buf;
	FILE		*fp;
	long		 fsize;
	int		 status;

	/* Support for index. */
	if (strcmp(requ->path, "/") == 0)
		path = "/index.html";
	else
		path = requ->path;

	/* Prepare the response. */
	status = 200;
	content_type = get_content_type(path);

	/* Open the file. */
	if ((fp = fopen(path, "r")) == NULL) {
		switch (errno) {
		case ENOTDIR:	/* FALLTHROUGH */
		case ENOENT:
			status = 404;	/* Not Found */
			break;
		case EISDIR:	/* FALLTHROUGH */
		case EACCES:
			status = 403;	/* Forbidden */
			break;
		default:
			status = 500;	/* Internal Server Error */
			break;
		}
		goto end;
	}

	/* Check the file size. */
	fsize = get_file_sz(fp);
	if (fsize == -1 || fsize > MAX_FS) {
		fclose(fp);
		status = 500;
		goto end;
	}

	/* Read the file into a temporary buffer. */
	if ((buf = malloc(fsize)) == NULL) {
		fclose(fp);
		status = 500;
		goto end;
	}
	fread(buf, fsize, 1, fp);
	fclose(fp);

	/* Make it the message body. */
	if (yhttp_resp_body(requ, buf, fsize) != YHTTP_OK) {
		free(buf);
		status = 500;
		goto end;
	}
	free(buf);

end:
	yhttp_resp_status(requ, status);
	if (status == 200)
		yhttp_resp_header(requ, "Content-Type", content_type);
	else {
		switch (status) {
		case 403:
			body = "<h1>403 Forbidden</h1>";
			break;
		case 404:
			body = "<h1>404 Not Found</h1>";
			break;
		case 500:
			body = "<h1>500 Internal Server Error</h1>";
			break;
		}
		yhttp_resp_header(requ, "Content-Type", "text/html");
		yhttp_resp_body(requ, (const unsigned char *)body,
				strlen(body));
	}
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
