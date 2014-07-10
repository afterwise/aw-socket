
#include "aw-socket.h"

#if _WIN32
# include <ws2tcpip.h>
#else
# include <netinet/in.h>
# include <fcntl.h>
# include <netdb.h>
# include <unistd.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void socket_init() {
#if _WIN32
	WSADATA data;

	if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
		fprintf(stderr, "WSAStartup: %d\n", WSAGetLastError()), abort();
#endif
}

void socket_end() {
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

		if ((flags & SOCKET_STREAM) == 0 ||
				connect(sd, ai->ai_addr, ai->ai_addrlen) == 0)
			break;

		socket_close(sd);
		sd = -1;
	}

	freeaddrinfo(res);
	return sd;
}

int socket_listen(const char *node, const char *service, int flags) {
	struct addrinfo hints, *res, *ai;
	int reuse = 1;
	int sd = -1;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6;
	hints.ai_socktype = ((flags & SOCKET_STREAM) != 0 ? SOCK_STREAM : SOCK_DGRAM);
	hints.ai_flags = AI_PASSIVE | AI_V4MAPPED | AI_ADDRCONFIG;

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

		if ((flags & SOCKET_REUSEADDR) != 0)
			if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (void *) &reuse, sizeof reuse) < 0) {
				socket_close(sd);
				sd = -1;
				continue;
			}

		if (bind(sd, ai->ai_addr, ai->ai_addrlen) < 0) {
			socket_close(sd);
			sd = -1;
			continue;
		}

		if ((flags & SOCKET_STREAM) == 0 || listen(sd, 4) == 0)
			break;

		socket_close(sd);
		sd = -1;
	}

	freeaddrinfo(res);
	return sd;
}

int socket_accept(int sd, struct endpoint *ep) {
	int res;

	ep->addrlen = sizeof ep->addrbuf;

	while ((res = accept(sd, (struct sockaddr *) &ep->addrbuf, &ep->addrlen)) < 0)
		if (errno != EINTR)
			return -1;

	return res;
}

int socket_close(int sd) {
#if _WIN32
	while (closesocket(sd) == SOCKET_ERROR)
		if (WSAGetLastError() != WSAEINTR)
			return -1;
#else
	while (close(sd) < 0)
		if (errno != EINTR)
			return -1;
#endif

	return 0;
}

ssize_t socket_send(int sd, const void *p, size_t n) {
        ssize_t err, off, len;

        for (off = 0, len = n; len != 0; off += err > 0 ? err : 0, len = n - off)
                if ((err = send(sd, (const char *) p + off, len, 0)) < 0 && errno != EINTR)
                        return -1;

        return off;
}

ssize_t socket_recv(int sd, void *p, size_t n) {
        ssize_t err, off, len;

        for (off = 0, len = n; len != 0; off += err > 0 ? err : 0, len = n - off)
                if ((err = recv(sd, (char *) p + off, len, 0)) == 0)
                        break;
                else if (err < 0 && errno != EINTR)
                        return -1;

        return off;
}

ssize_t socket_sendto(int sd, const void *p, size_t n, const struct endpoint *ep) {
        ssize_t err;

	do err = sendto(sd, p, n, 0, (struct sockaddr *) &ep->addrbuf, ep->addrlen);
	while (err < 0 && errno != EINTR);

        return err;
}

ssize_t socket_recvfrom(int sd, void *p, size_t n, struct endpoint *ep) {
        ssize_t err;

	ep->addrlen = sizeof ep->addrbuf;

	do err = recvfrom(sd, p, n, 0, (struct sockaddr *) &ep->addrbuf, &ep->addrlen);
	while (err < 0 && errno == EINTR);

        return err;
}

