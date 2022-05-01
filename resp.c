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

#include <stdlib.h>
#include <string.h>

#include "hash.h"
#include "yhttp.h"
#include "yhttp-internal.h"
#include "net.h"
#include "resp.h"
#include "util.h"

struct status_rp {
	int		 status;
	const char	*reason_phrase;
};

static const char	*resp_find_rp(int);
static char		*resp_fmt_rline(int);
static char		*resp_fmt_header(struct hash *);
static char		*resp_fmt_err(int);
static int		 resp_transmit_rline(int, int);
static int		 resp_transmit_headers(int, struct yhttp_resp *);
static int		 resp_transmit_body(int, struct yhttp_resp *);

static struct status_rp	CODES[] = {
	{ 100, "Continue" },
	{ 101, "Switching Protocols" },
	{ 102, "Processing" },
	{ 103, "Early Hints" },

	{ 200, "OK" },
	{ 201, "Created" },
	{ 202, "Accepted" },
	{ 203, "Non-Authoritative Information" },
	{ 204, "No Content" },
	{ 205, "Reset Content" },
	{ 206, "Partial Content" },
	{ 207, "Multi-Status" },
	{ 208, "Already Reported" },
	{ 226, "IM Used" },

	{ 300, "Multiple Choices" },
	{ 301, "Moved Permanently" },
	{ 302, "Found" },
	{ 303, "See Other" },
	{ 304, "Not Modified" },
	{ 305, "Use Proxy" },
	{ 306, "Switch Proxy" },
	{ 307, "Temporary Redirect" },
	{ 308, "Permanent Redirect" },

	{ 400, "Bad Request" },
	{ 401, "Unauthorized" },
	{ 402, "Payment Required" },
	{ 403, "Forbidden" },
	{ 404, "Not Found" },
	{ 405, "Method Not Allowed" },
	{ 406, "Not Acceptable" },
	{ 407, "Proxy Authentication Required" },
	{ 408, "Request Timeout" },
	{ 409, "Conflict" },
	{ 410, "Gone" },
	{ 411, "Length Required" },
	{ 412, "Precondition Failed" },
	{ 413, "Payload Too Large" },
	{ 414, "URI Too Long" },
	{ 415, "Unsupported Media Type" },
	{ 416, "Range Not Satisfiable" },
	{ 417, "Expectation Failed" },
	{ 418, "I'm a teapot" },
	{ 421, "Misdirected Request" },
	{ 422, "Unprocessable Entity" },
	{ 423, "Locked" },
	{ 424, "Failed Dependency" },
	{ 425, "Too Early" },
	{ 426, "Upgrade Required" },
	{ 428, "Precondition Required" },
	{ 429, "Too Many Requests" },
	{ 431, "Request Header Fields Too Large" },
	{ 451, "Unavailable For Legal Reasons" },

	{ 500, "Internal Server Error" },
	{ 501, "Not Implemented" },
	{ 502, "Bad Gateway" },
	{ 503, "Service Unavailable" },
	{ 504, "Gateway Timeout" },
	{ 505, "HTTP Version Not Supported" },
	{ 506, "Variant Also Negotiates" },
	{ 507, "Insufficient Storage" },
	{ 508, "Loop Detected" },
	{ 510, "Not Extended" },
	{ 511, "Network Authentication Required" },

	{ 0, "NULL" }
};

static const char *
resp_find_rp(int status)
{
	size_t	i;

	for (i = 0; CODES[i].status != 0; ++i) {
		if (CODES[i].status == status)
			break;
	}

	return (CODES[i].reason_phrase);
}

static char *
resp_fmt_rline(int status)
{
	const char	*rp;

	rp = resp_find_rp(status);

	return (util_aprintf("HTTP/1.1 %d %s\r\n", status, rp));
}

static char *
resp_fmt_header(struct hash *node)
{
	return (util_aprintf("%s: %s\r\n", node->name, node->value));
}

static char *
resp_fmt_err(int status)
{
	const char	*rp;

	rp = resp_find_rp(status);

	return (util_aprintf("HTTP/1.1 %d %s\r\n"
			     "Content-Length: %zu\r\n"
			     "\r\n"
			     "%s",
			     status, rp, strlen(rp), rp));
}

static int
resp_transmit_rline(int s, int status)
{
	char	*rline;
	ssize_t	 n;
	size_t	 len;

	if ((rline = resp_fmt_rline(status)) == NULL)
		return (YHTTP_ERRNO);
	len = strlen(rline);
	n = net_send(s, (unsigned char *)rline, len);
	if (n <= 0 || (size_t)n != len)
		return (YHTTP_ERRNO);

	return (YHTTP_OK);
}

static int
resp_transmit_headers(int s, struct yhttp_resp *resp)
{
	struct hash	**headers;
	char		 *header;
	ssize_t		  n;
	size_t		  i, len;

	if ((headers = hash_dump(resp->headers)) == NULL)
		return (YHTTP_ERRNO);

	/* Format and transmit all header fields. */
	for (i = 0; headers[i] != NULL; ++i) {
		if ((header = resp_fmt_header(headers[i])) == NULL) {
			free(headers);
			return (YHTTP_ERRNO);
		}
		len = strlen(header);

		n = net_send(s, (unsigned char *)header, len);
		free(header);
		if (n <= 0 || (size_t)n != len) {
			free(headers);
			return (YHTTP_ERRNO);
		}
	}
	free(headers);

	/* Format and transmit the Content-Length header field. */
	header = util_aprintf("Content-Length: %zu\r\n\r\n", resp->nbody);
	if (header == NULL)
		return (YHTTP_ERRNO);
	len = strlen(header);

	n = net_send(s, (unsigned char *)header, len);
	free(header);
	if (n <= 0 || (size_t)n != len)
		return (YHTTP_ERRNO);

	return (YHTTP_OK);
}

static int
resp_transmit_body(int s, struct yhttp_resp *resp)
{
	ssize_t	n;

	n = net_send(s, resp->body, resp->nbody);
	if (n <= 0 || (size_t)n != resp->nbody)
		return (YHTTP_ERRNO);

	return (YHTTP_OK);
}

int
resp(int s, struct yhttp_resp *resp)
{
	int	rc;

	if ((rc = resp_transmit_rline(s, resp->status)) != YHTTP_OK)
		return (rc);
	if ((rc = resp_transmit_headers(s, resp)) != YHTTP_OK)
		return (rc);
	if ((rc = resp_transmit_body(s, resp)) != YHTTP_OK)
		return (rc);

	return (YHTTP_OK);
}

int
resp_err(int s, int status)
{
	char	*resp;
	ssize_t	 n;
	size_t	 len;

	if ((resp = resp_fmt_err(status)) == NULL)
		return (YHTTP_ERRNO);
	len = strlen(resp);

	n = net_send(s, (unsigned char *)resp, len);
	free(resp);
	if (n <= 0 || (size_t)n != len)
		return (YHTTP_ERRNO);

	return (YHTTP_OK);
}
