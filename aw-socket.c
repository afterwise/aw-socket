
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

#include "aw-socket.h"
#include "aw-thread.h"

#if _WIN32
# include <ws2tcpip.h>
#else
# include <netinet/in.h>
# include <fcntl.h>
# include <netdb.h>
# include <unistd.h>
#endif

#if __linux__ || __APPLE__
# include <netinet/tcp.h>
#endif

#if __linux__
# include <sys/epoll.h>
#elif __APPLE__
# include <sys/event.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void socket_init(void) {
#if _WIN32
	WSADATA data;

	if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
		fprintf(stderr, "WSAStartup: %d\n", WSAGetLastError());
		abort();
	}
#endif
}

void socket_end(void) {
#if _WIN32
	WSACleanup();
#endif
}

int socket_getaddr(struct endpoint *ep, const char *node, const char *service) {
	struct addrinfo hints, *res;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG;

	if (getaddrinfo(node, service, &hints, &res) < 0)
		return -1;

	memcpy(&ep->addrbuf, res->ai_addr, res->ai_addrlen);
	ep->addrlen = res->ai_addrlen;

	freeaddrinfo(res);
	return 0;
}

int socket_getname(
		char node[_socket_staticsize SOCKET_MAXNODE],
		char serv[_socket_staticsize SOCKET_MAXSERV],
		const struct endpoint *ep) {
	return getnameinfo(
		(const struct sockaddr *) &ep->addrbuf, ep->addrlen,
		node, SOCKET_MAXNODE, serv, SOCKET_MAXSERV, 0);
}

int socket_broadcast(void) {
	int sd, broadcast = 1;

	if ((sd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		return -1;

	if (setsockopt(sd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast) < 0) {
		socket_close(sd);
		return -1;
	}

	return sd;
}

int socket_connect(const char *node, const char *service, int flags) {
	struct addrinfo hints, *res, *ai;
	int sd = -1;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6;
	hints.ai_socktype = ((flags & SOCKET_STREAM) != 0 ? SOCK_STREAM : SOCK_DGRAM);
	hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG;

	if (getaddrinfo(node, service, &hints, &res) < 0)
		return -1;

	for (ai = res; ai != NULL; ai = ai->ai_next) {
		if ((sd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) < 0)
			continue;

		if ((flags & SOCKET_NONBLOCK) != 0) {
#if _WIN32
			DWORD nonblock = 1;
			if (ioctlsocket(sd, FIONBIO, &nonblock) < 0) {
				socket_close(sd);
				sd = -1;
				continue;
			}
#else
			if (fcntl(sd, F_SETFL, O_NONBLOCK) < 0) {
				socket_close(sd);
				sd = -1;
				continue;
			}
#endif
		}

		if ((flags & SOCKET_STREAM) == 0)
			break;

		if ((flags & SOCKET_LINGER) == 0) {
			struct linger l;
			l.l_onoff = 1;
			l.l_linger = 0;
			if (setsockopt(sd, SOL_SOCKET, SO_LINGER, &l, sizeof l) < 0) {
				socket_close(sd);
				sd = -1;
				continue;
			}
		}

		if ((flags & SOCKET_NODELAY) != 0) {
			int yes = 1;
			if (setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof yes) < 0) {
				socket_close(sd);
				sd = -1;
				continue;
			}
		}

#if TCP_FASTOPEN
		if ((flags & SOCKET_FASTOPEN) != 0) {
# if __APPLE__
			sa_endpoints_t ep;
			memset(&ep, 0, sizeof ep);
			ep.sae_dstaddr = (struct sockaddr *) ai->ai_addr;
			ep.sae_dstaddrlen = ai->ai_addrlen;
			if (connectx(
					sd, &ep, SAE_ASSOCID_ANY,
					CONNECT_RESUME_ON_READ_WRITE | CONNECT_DATA_IDEMPOTENT,
					NULL, 0, NULL, NULL) == 0 || errno == EINPROGRESS)
				break;
# elif MSG_FASTOPEN
			if (sendto(sd, "", 0, MSG_FASTOPEN, ai->ai_addr, ai->ai_addrlen) == 0)
				break;
# endif
		}
#endif
		if (connect(sd, ai->ai_addr, ai->ai_addrlen) == 0 || errno == EINPROGRESS)
			break;

		socket_close(sd);
		sd = -1;
	}

	freeaddrinfo(res);
	return sd;
}

int socket_listen(const char *node, const char *service, int flags) {
	struct addrinfo hints, *res, *ai;
	int sd = -1;
	int val;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6;
	hints.ai_socktype = ((flags & SOCKET_STREAM) != 0 ? SOCK_STREAM : SOCK_DGRAM);
	hints.ai_flags = AI_PASSIVE | AI_V4MAPPED | AI_ADDRCONFIG;

	if (getaddrinfo(node, service, &hints, &res) < 0)
		return -1;

	for (ai = res; ai != NULL; ai = ai->ai_next) {
		if ((sd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) < 0)
			continue;

#if __linux__
		if ((flags & SOCKET_DEFERACCEPT) != 0) {
			val = 5; /* secs */
			if (setsockopt(sd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &val, sizeof val) < 0) {
				socket_close(sd);
				sd = -1;
				continue;
			}
		}
#endif

		if ((flags & SOCKET_NONBLOCK) != 0) {
#if _WIN32
			DWORD nonblock = 1;
			if (ioctlsocket(sd, FIONBIO, &nonblock) < 0) {
				socket_close(sd);
				sd = -1;
				continue;
			}
#else
			if (fcntl(sd, F_SETFL, O_NONBLOCK) < 0) {
				socket_close(sd);
				sd = -1;
				continue;
			}
#endif
		}

		if ((flags & SOCKET_REUSEADDR) != 0) {
			val = 1; /* reuse */
			if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof val) < 0) {
				socket_close(sd);
				sd = -1;
				continue;
			}
		}

		if (bind(sd, ai->ai_addr, ai->ai_addrlen) < 0) {
			socket_close(sd);
			sd = -1;
			continue;
		}

		if ((flags & SOCKET_STREAM) == 0)
			break;

		if (listen(sd, 4) == 0) {
#if TCP_FASTOPEN
			if ((flags & SOCKET_FASTOPEN) != 0) {
# if __APPLE__
				val = 1; /* enable */
# else
				val = 5; /* qlen */
# endif
				if (setsockopt(sd, IPPROTO_TCP, TCP_FASTOPEN, &val, sizeof val) < 0) {
					socket_close(sd);
					sd = -1;
					continue;
				}
			}
#endif
			break;
		}

		socket_close(sd);
		sd = -1;
	}

	freeaddrinfo(res);
	return sd;
}

int socket_accept(int sd, struct endpoint *ep, int flags) {
	int res;

	ep->addrlen = sizeof ep->addrbuf;

	if ((res = accept(sd, (struct sockaddr *) &ep->addrbuf, &ep->addrlen)) < 0)
		return -1;

	if ((flags & SOCKET_NONBLOCK) != 0) {
#if _WIN32
		DWORD nonblock = 1;
		if (ioctlsocket(res, FIONBIO, &nonblock) < 0) {
			socket_close(res);
			res = -1;
		}
#else
		if (fcntl(res, F_SETFL, O_NONBLOCK) < 0) {
			socket_close(res);
			res = -1;
		}
#endif
	}

	if ((flags & SOCKET_LINGER) == 0) {
		struct linger l;
		l.l_onoff = 1;
		l.l_linger = 0;
		if (setsockopt(res, SOL_SOCKET, SO_LINGER, &l, sizeof l) < 0) {
			socket_close(res);
			return -1;
		}
	}

	if ((flags & SOCKET_NODELAY) != 0) {
		int yes = 1;
		if (setsockopt(res, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof yes) < 0) {
			socket_close(res);
			return -1;
		}
	}

	return res;
}

int socket_shutdown(int sd, int mode) {
	return shutdown(sd, mode);
}

int socket_close(int sd) {
#if _WIN32
	if (closesocket(sd) == SOCKET_ERROR)
		return -1;
#else
	if (close(sd) < 0)
		return -1;
#endif

	return 0;
}

ssize_t socket_send(int sd, const void *p, size_t n) {
        ssize_t err, off, len;

        for (off = 0, len = n; len != 0; off += err > 0 ? err : 0, len = n - off)
                if ((err = send(sd, (const char *) p + off, len, 0)) < 0)
                        return err;

        return off;
}

ssize_t socket_recv(int sd, void *p, size_t n, int flags) {
	return recv(sd, p, n, (flags & SOCKET_WAITALL) ? MSG_WAITALL : 0);
}

ssize_t socket_sendto(int sd, const void *p, size_t n, const struct endpoint *ep) {
	return sendto(sd, p, n, 0, (struct sockaddr *) &ep->addrbuf, ep->addrlen);
}

ssize_t socket_recvfrom(int sd, void *p, size_t n, struct endpoint *ep) {
	ep->addrlen = sizeof ep->addrbuf;
	return recvfrom(sd, p, n, 0, (struct sockaddr *) &ep->addrbuf, &ep->addrlen);
}

