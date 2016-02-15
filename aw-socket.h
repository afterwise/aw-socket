
/*
   Copyright (c) 2014-2016 Malte Hildingsson, malte (at) afterwi.se

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
 */

#ifndef AW_SOCKET_H
#define AW_SOCKET_H

#if _WIN32
# include <winsock2.h>
# include <sys/types.h>
#else
# include <netinet/in.h>
# include <sys/socket.h>
# include <netdb.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __cplusplus
# define _socket_staticsize static
#else
# define _socket_staticsize
#endif

#if _WIN32
typedef int socklen_t;
#endif

struct endpoint {
	union {
		struct sockaddr_storage addrbuf;
		struct sockaddr_in addr;
		struct sockaddr_in6 addr6;
	};
	socklen_t addrlen;
};

void socket_init(void);
void socket_end(void);

#define SOCKET_MAXNODE (NI_MAXHOST)
#define SOCKET_MAXSERV (NI_MAXSERV)
#define SOCKET_MAXADDRSTRLEN (INET6_ADDRSTRLEN)
int socket_getname(
	char node[_socket_staticsize SOCKET_MAXNODE],
	char serv[_socket_staticsize SOCKET_MAXSERV],
	const struct endpoint *endpoint);
int socket_tohuman(char str[_socket_staticsize SOCKET_MAXADDRSTRLEN], struct endpoint *endpoint);

enum {
	SOCKET_STREAM = 0x1,
	SOCKET_NONBLOCK = 0x2,
	SOCKET_LINGER = 0x4,
	SOCKET_NODELAY = 0x8,
	SOCKET_REUSEADDR = 0x10,
	SOCKET_FASTOPEN = 0x20,
	SOCKET_DEFERACCEPT = 0x40
};
int socket_broadcast(void);
int socket_connect(const char *node, const char *service, struct endpoint *endpoint, int flags);
int socket_listen(const char *node, const char *service, struct endpoint *endpoint, int backlog, int flags);
int socket_accept(int sd, struct endpoint *endpoint, int flags);

enum {
	SOCKET_RECV = 0,
	SOCKET_SEND = 1,
	SOCKET_BOTH = 2
};
int socket_shutdown(int sd, int mode);
int socket_close(int sd);

enum {
	SOCKET_WAITALL = 0x1
};
ssize_t socket_send(int sd, const void *p, size_t n);
ssize_t socket_recv(int sd, void *p, size_t n, int flags);

ssize_t socket_sendto(int sd, const void *p, size_t n, const struct endpoint *endpoint);
ssize_t socket_recvfrom(int sd, void *p, size_t n, struct endpoint *endpoint);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AW_SOCKET_H */

