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
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "buf.h"
#include "parser.h"
#include "yhttp.h"
#include "yhttp-internal.h"
#include "resp.h"
#include "net.h"

#define NGROW	128

struct poll_data {
	/*
	 * The parsers array is being kept alongside with the pfds array in
	 * terms of size and indices, meaning that the index of a connected
	 * client inside pfds also refers to its accompanying parser instance
	 * inside parsers.
	 */
	struct parser	**parsers;
	struct pollfd	 *pfds;
	size_t		  npfds;
	size_t		  used;
};

static int	 net_finish_requ(struct poll_data *, size_t);

static int	 net_handle_accept(struct poll_data *, size_t);
static int	 net_handle_client(struct poll_data *, size_t,
				   void (*)(struct yhttp_requ *, void *),
				   void *);
static char	*net_ip(int);
static int	 net_is_keep_alive(struct yhttp_requ *);
static int	 net_nonblock(int);
static void	 net_poll_init(struct poll_data *);
static void	 net_poll_free(struct poll_data *);
static int	 net_poll_grow(struct poll_data *);
static int	 net_poll_add(struct poll_data *, int, short);
static void	 net_poll_del(struct poll_data *, size_t);
static void	 net_poll_close(struct poll_data *, size_t);
static int	 net_socket(int, uint16_t);

static int
net_finish_requ(struct poll_data *pd, size_t index)
{
	if (net_is_keep_alive(pd->parsers[index]->requ)) {
		/* Connection is keep-alive, just reset it. */
		parser_free(pd->parsers[index]);

		if ((pd->parsers[index] = parser_init()) == NULL)
			return (YHTTP_ERRNO);
		return (YHTTP_OK);
	} else {
		/* Connection is close, close it. */
		net_poll_close(pd, index);
		return (YHTTP_OK);
	}
}

static int
net_handle_accept(struct poll_data *pd, size_t index)
{
	int	c, rc, s;

	s = pd->pfds[index].fd;

	/* The IP address is being obtained later by getpeername(2). */
	if ((c = accept(s, NULL, NULL)) == -1)
		return (YHTTP_OK);	/* Not a FATAL error. */

	if ((rc = net_poll_add(pd, c, POLLIN)) != YHTTP_OK)
		return (rc);

	return (YHTTP_OK);
}

static int
net_handle_client(struct poll_data *pd, size_t index,
		  void (*cb)(struct yhttp_requ *, void *), void *udata)
{
	struct yhttp_requ_internal	*internal;
	char				*client_ip;
	unsigned char			 msg[4096];
	size_t				 n;
	int				 rc, s;

	s = pd->pfds[index].fd;

	n = recv(s, msg, sizeof(msg), 0);
	if (n <= 0) {
		if (n < 0 && errno == EAGAIN)
			return (YHTTP_OK);

		/* Connection was closed or error occurred. */
		net_poll_close(pd, index);
	} else {
		rc = parser_parse(pd->parsers[index], msg, n);
		if (rc != YHTTP_OK) {
			net_poll_close(pd, index);
			return (rc);
		}

		if (pd->parsers[index]->err) {
			if (resp_err(s, pd->parsers[index]->err) != YHTTP_OK) {
				net_poll_close(pd, index);
				return (YHTTP_OK);
			}
			return (net_finish_requ(pd, index));
		} else if (pd->parsers[index]->state == PARSER_DONE) {
			/* Obtain the IP address. */
			if ((client_ip = net_ip(s)) == NULL)
				return (YHTTP_ERRNO);
			pd->parsers[index]->requ->client_ip = client_ip;

			internal = pd->parsers[index]->requ->internal;
			cb(pd->parsers[index]->requ, udata);
			if (resp(s, internal->resp) != YHTTP_OK) {
				net_poll_close(pd, index);
				return (YHTTP_OK);
			}
			return (net_finish_requ(pd, index));
		}
	}

	return (YHTTP_OK);
}

static char *
net_ip(int s)
{
	char			*str;
	void			*addr;
	struct sockaddr_storage	 sa;
	socklen_t		 nsa;
	int			 af;

	/* Obtain the address information and socket domain. */
	nsa = sizeof(sa);
	if (getpeername(s, (struct sockaddr *)&sa, &nsa) == -1)
		return (NULL);
	af = sa.ss_family;

	if ((str = malloc(INET6_ADDRSTRLEN)) == NULL)
		return (NULL);
	memset(str, '\0', INET6_ADDRSTRLEN);

	assert(af == AF_INET || af == AF_INET6);
	if (af == AF_INET)
		addr = &(((struct sockaddr_in *)&sa)->sin_addr);
	else if (af == AF_INET6)
		addr = &(((struct sockaddr_in6 *)&sa)->sin6_addr);

	if (inet_ntop(af, addr, str, INET6_ADDRSTRLEN) == NULL) {
		free(str);
		return (NULL);
	}

	return (str);
}

static int
net_is_keep_alive(struct yhttp_requ *requ)
{
	char	*value;

	value = yhttp_header(requ, "Connection");
	if (value != NULL && strcmp(value, "keep-alive") == 0)
		return (1);
	else
		return (0);
}

static int
net_nonblock(int fd)
{
	int	flags;

	if ((flags = fcntl(fd, F_GETFL)) == -1)
		return (YHTTP_ERRNO);
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		return (YHTTP_ERRNO);
	return (YHTTP_OK);
}

static void
net_poll_init(struct poll_data *pd)
{
	pd->parsers = NULL;
	pd->pfds = NULL;
	pd->npfds = 0;
	pd->used = 0;
}

static void
net_poll_free(struct poll_data *pd)
{
	size_t	i;

	if (pd == NULL)
		return;

	/* Free parsers. */
	for (i = 0; i < pd->npfds; ++i)
		parser_free(pd->parsers[i]);
	free(pd->parsers);

	free(pd->pfds);
}

static int
net_poll_grow(struct poll_data *pd)
{
	struct parser	**n_parsers;
	struct pollfd	 *n_pfds;
	size_t		 n_npfds, i;

	/* Check for integer overflows before reallocation. */
	if (SIZE_MAX - NGROW < pd->npfds)
		return (YHTTP_EOVERFLOW);
	n_npfds = pd->npfds + 128;
	if (n_npfds > SIZE_MAX / sizeof(struct parser *))
		return (YHTTP_EOVERFLOW);
	if (n_npfds > SIZE_MAX / sizeof(struct pollfd))
		return (YHTTP_EOVERFLOW);

	/* Reallocate the arrays. */
	n_parsers = realloc(pd->parsers, sizeof(struct parser *) * n_npfds);
	if (n_parsers == NULL)
		return (YHTTP_ERRNO);
	pd->parsers = n_parsers;
	n_pfds = realloc(pd->pfds, sizeof(struct pollfd) * n_npfds);
	if (n_pfds == NULL)
		return (YHTTP_ERRNO);
	pd->pfds = n_pfds;

	pd->npfds = n_npfds;

	/* Initialize the new fields. */
	for (i = pd->used; i < pd->npfds; ++i) {
		pd->parsers[i] = NULL;

		pd->pfds[i].fd = -1;
		pd->pfds[i].events = 0;
		pd->pfds[i].revents = 0;
	}

	return (YHTTP_OK);
}

static int
net_poll_add(struct poll_data *pd, int fd, short events)
{
	size_t	i;
	int	rc;

	/* Check if we need to grow pd->pfds. */
	if (pd->npfds == pd->used) {
		if ((rc = net_poll_grow(pd)) != YHTTP_OK)
			return (rc);

		/* No need to search for a free slot in this case. */
		if ((pd->parsers[pd->used] = parser_init()) == NULL)
			return (YHTTP_ERRNO);

		pd->pfds[pd->used].fd = fd;
		pd->pfds[pd->used++].events = events;

		return (YHTTP_OK);
	} else {
		/* Search for a free slot. */
		for (i = 0; i < pd->npfds; ++i) {
			if (pd->pfds[i].fd == -1)
				break;
		}
		assert(i < pd->npfds);

		if ((pd->parsers[i] = parser_init()) == NULL)
			return (YHTTP_ERRNO);

		pd->pfds[i].fd = fd;
		pd->pfds[i].events = events;
		++pd->used;

		return (YHTTP_OK);
	}
}

static void
net_poll_del(struct poll_data *pd, size_t index)
{
	parser_free(pd->parsers[index]);
	pd->parsers[index] = NULL;

	pd->pfds[index].fd = -1;
	pd->pfds[index].events = 0;
	pd->pfds[index].revents = 0;
	--pd->used;
}

static void
net_poll_close(struct poll_data *pd, size_t index)
{
	int	s;

	s = pd->pfds[index].fd;
	net_poll_del(pd, index);
	close(s);
}

static int
net_socket(int domain, uint16_t port)
{
	struct sockaddr_in6	sa6;
	struct sockaddr_in	sa4;
	const int		true = 1;
	int			rc, s;

	assert(domain == AF_INET || domain == AF_INET6);

	/* Create the socket. */
	if ((s = socket(domain, SOCK_STREAM, 0)) == -1)
		return (YHTTP_ERRNO);

	/* Change the socket settings. */
	if (net_nonblock(s) != YHTTP_OK)
		goto err;
	rc = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &true, sizeof(true));
	if (rc == -1)
		goto err;

	/* Bind the socket. */
	if (domain == AF_INET) {
		memset(&sa4, 0, sizeof(sa4));
		sa4.sin_family = AF_INET;
		sa4.sin_port = htons(port);
		sa4.sin_addr.s_addr = INADDR_ANY;

		if (bind(s, (struct sockaddr *)&sa4, sizeof(sa4)) == -1)
			goto err;
	} else if (domain == AF_INET6) {
		memset(&sa6, 0, sizeof(sa6));
		sa6.sin6_family = AF_INET6;
		sa6.sin6_port = htons(port);
		sa6.sin6_addr = in6addr_any;

		if (bind(s, (struct sockaddr *)&sa6, sizeof(sa6)) == -1)
			goto err;
	}

	if (listen(s, 128) == -1)
		goto err;

	return (s);
err:
	close(s);
	return (YHTTP_ERRNO);
}

int
net_dispatch(uint16_t port, int read_pipe,
	     void (*cb)(struct yhttp_requ *, void *), void *udata)
{
	struct poll_data	pd;
	size_t			i;
	int			quit, rc, s4, s6;

	net_poll_init(&pd);
	s4 = -1;
	s6 = -1;

	if ((s4 = net_socket(AF_INET, port)) == YHTTP_ERRNO) {
		rc = YHTTP_ERRNO;
		goto end;
	}
	if ((s6 = net_socket(AF_INET6, port)) == YHTTP_ERRNO) {
		rc = YHTTP_ERRNO;
		goto end;
	}

	/* Add the sockets and pipe to poll(2). */
	if ((rc = net_poll_add(&pd, read_pipe, POLLIN)) != YHTTP_OK)
		goto end;
	if ((rc = net_poll_add(&pd, s4, POLLIN)) != YHTTP_OK)
		goto end;
	if ((rc = net_poll_add(&pd, s6, POLLIN)) != YHTTP_OK)
		goto end;

	/* The actual event loop. */
	quit = 0;
	while(!quit) {
		rc = poll(pd.pfds, pd.npfds, INFTIM);
		if (rc == -1) {
			if (errno == EINTR)
				continue;
			else
				goto end;
		}

		for (i = 0; i < pd.npfds; ++i) {
			if (!(pd.pfds[i].revents & POLLIN))
				continue;

			if (pd.pfds[i].fd == s4 || pd.pfds[i].fd == s6) {
				/* Handle incoming connection. */
				rc = net_handle_accept(&pd, i);
				if (rc != YHTTP_OK)
					goto end;
			} else if (pd.pfds[i].fd == read_pipe) {
				/* Terminate execution. */
				quit = 1;
				break;
			} else {
				/* Handle connected client. */
				rc = net_handle_client(&pd, i, cb, udata);
				if (rc != YHTTP_OK)
					goto end;
			}
		}
	}

	rc = YHTTP_OK;
end:
	/* Close all file descriptors. */
	for (i = 0; i < pd.npfds; ++i) {
		if (pd.pfds[i].fd == read_pipe)
			continue;
		else
			close(pd.pfds[i].fd);
	}
	net_poll_free(&pd);

	return (rc);
}

ssize_t
net_send(int s, const unsigned char *data, size_t ndata)
{
	ssize_t	n;
	size_t	sent;

	sent = 0;
	do {
		n = send(s, data + sent, ndata - sent, 0);
		if (n <= 0)
			return (n);

		/* Integer overflow check. */
		if (SIZE_MAX - (size_t)n < sent)
			return (-1);
		sent += n;
	} while (sent != ndata);

	/* Unlikely but it's better to go safe. */
	if (sent > SSIZE_MAX)
		return (-1);

	return (sent);
}
