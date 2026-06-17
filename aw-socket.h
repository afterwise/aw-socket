
/*
   Copyright (c) 2014-2025 Malte Hildingsson, malte (at) afterwi.se

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

#if !defined(__NINTENDO__) || !__has_include(<socket_Config.h>)

#if defined(_WIN32)
# include <winsock2.h>
# include <ws2tcpip.h>
# include <sys/types.h>
#elif defined(__linux__) || defined(__APPLE__) || defined(__SCE__) || defined(__NINTENDO__)
# include <netinet/in.h>
# include <sys/socket.h>
# if defined(__SCE__)
#  include <net6.h>
#  if !defined(__ORBIS__)
#   include <netinet6/in6.h>
#  endif
# else
#  include <netdb.h>
# endif
#endif

#if !defined(_MSC_VER) || _MSC_VER >= 1600
# include <stdint.h>
#endif

#if defined(_socket_dllexport)
# if defined(_MSC_VER)
#  define _socket_api extern __declspec(dllexport)
# elif defined(__GNUC__)
#  define _socket_api __attribute__((visibility("default"))) extern
# endif
#elif defined(_socket_dllimport)
# if defined(_MSC_VER)
#  define _socket_api extern __declspec(dllimport)
# endif
#endif
#ifndef _socket_api
# define _socket_api extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __cplusplus
# define _socket_staticsize static
#else
# define _socket_staticsize
#endif

#if defined(_MSC_VER)
typedef signed __int64 socket_ssize_t;
#else
typedef ssize_t socket_ssize_t;
#endif

#if defined(_WIN32)
typedef int socklen_t;
typedef intptr_t socket_t;
#else
typedef int socket_t;
#endif

#if defined(_MSC_VER)
# pragma warning(push)
# pragma warning(disable : 4201)
#endif

struct socket_endpoint {
	union {
		struct sockaddr_storage addrbuf;
		struct sockaddr_in addr;
#if !defined(__ORBIS__)
		struct sockaddr_in6 addr6;
#endif
	};
	socklen_t addrlen;
};

#if defined(_MSC_VER)
# pragma warning(pop)
#endif

_socket_api void socket_init(void);
_socket_api void socket_end(void);

#define SOCKET_MAXNODE (NI_MAXHOST)
#define SOCKET_MAXSERV (NI_MAXSERV)
#define SOCKET_MAXADDRSTRLEN (INET6_ADDRSTRLEN)
#if !defined(__SCE__)
_socket_api int socket_getname(
	char node[/*_socket_staticsize SOCKET_MAXNODE*/],
	char serv[/*_socket_staticsize SOCKET_MAXSERV*/],
	const struct socket_endpoint *endpoint);
#endif
_socket_api int socket_tohuman(char str[/*_socket_staticsize SOCKET_MAXADDRSTRLEN*/], struct socket_endpoint *endpoint);

enum {
	SOCKET_STREAM = 0x1,
	SOCKET_NONBLOCK = 0x2,
	SOCKET_LINGER = 0x4,
	SOCKET_NODELAY = 0x8,
	SOCKET_REUSEADDR = 0x10,
	SOCKET_FASTOPEN = 0x20,
	SOCKET_DEFERACCEPT = 0x40
};
_socket_api int socket_broadcast(socket_t* out_sd);
_socket_api int socket_connect(socket_t *out_sd, const char *node, const char *service, struct socket_endpoint *endpoint, int flags);
#if !defined(__SCE__)
_socket_api int socket_listen(socket_t *out_sd, const char *node, const char *service, struct socket_endpoint *endpoint, int backlog, int flags);
_socket_api int socket_accept(socket_t *out_sd, socket_t sd, struct socket_endpoint *endpoint, int flags);
#endif

enum {
	SOCKET_RECV = 0,
	SOCKET_SEND = 1,
	SOCKET_BOTH = 2
};
_socket_api int socket_shutdown(socket_t sd, int mode);
_socket_api int socket_close(socket_t sd);

enum {
	SOCKET_WAITALL = 0x1
};
_socket_api socket_ssize_t socket_send(socket_t sd, const void *p, size_t n);
_socket_api socket_ssize_t socket_recv(socket_t sd, void *p, size_t n, int flags);

_socket_api socket_ssize_t socket_sendto(socket_t sd, const void *p, size_t n, const struct socket_endpoint *endpoint);
_socket_api socket_ssize_t socket_recvfrom(socket_t sd, void *p, size_t n, struct socket_endpoint *endpoint);

#if !defined(_GAMING_XBOX) && !defined(__SCE__) && !defined(__NINTENDO__)
_socket_api socket_ssize_t socket_sendfile(socket_t sd, intptr_t fd, size_t n);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // !defined(__NINTENDO__) || !__has_include(<socket_Config.h>)

#endif /* AW_SOCKET_H */

