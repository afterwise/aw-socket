
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

int socket_getaddr(
		struct sockaddr_storage *addr, socklen_t *addrlen,
		const char *node, const char *service) {
	struct addrinfo hints, *res;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;

	if (getaddrinfo(node, service, &hints, &res) < 0)
		goto bail;

	memcpy(addr, res->ai_addr, res->ai_addrlen);
	*addrlen = res->ai_addrlen;

	freeaddrinfo(res);
	return 0;

bail:
	freeaddrinfo(res);
	return -1;
}

int socket_connect(const char *node, const char *service, int flags) {
	struct addrinfo hints, *res;
	int sd;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = ((flags & SOCKET_STREAM) != 0 ? SOCK_STREAM : SOCK_DGRAM);

	if (getaddrinfo(node, service, &hints, &res) < 0)
		goto bail;

	if ((sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
		goto bail;

	if ((flags & SOCKET_NONBLOCK) != 0) {
#if _WIN32
		DWORD nonblock = 1;
		if (ioctlsocket(sd, FIONBIO, &nonblock) < 0)
			goto bail;
#else
		if (fcntl(sd, F_SETFL, O_NONBLOCK) < 0)
			goto bail;
#endif
	}

	if (node != NULL)
		if (connect(sd, res->ai_addr, res->ai_addrlen) < 0) {
			socket_close(sd);
			goto bail;
		}

	freeaddrinfo(res);
	return sd;

bail:
	freeaddrinfo(res);
	return -1;
}

int socket_listen(const char *service, int flags) {
	struct addrinfo hints, *res;
	int reuse = 1;
	int sd;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = ((flags & SOCKET_STREAM) != 0 ? SOCK_STREAM : SOCK_DGRAM);
	hints.ai_flags = AI_PASSIVE;

	if (getaddrinfo(NULL, service, &hints, &res) < 0)
		goto bail;

	if ((sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
		goto bail;

	if ((flags & SOCKET_NONBLOCK) != 0) {
#if _WIN32
		DWORD nonblock = 1;
		if (ioctlsocket(sd, FIONBIO, &nonblock) < 0)
			goto bail;
#else
		if (fcntl(sd, F_SETFL, O_NONBLOCK) < 0)
			goto bail;
#endif
	}

	if ((flags & SOCKET_REUSEADDR) != 0)
		if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (void *) &reuse, sizeof reuse) < 0)
			goto bail;

	if (bind(sd, res->ai_addr, res->ai_addrlen) < 0) {
		socket_close(sd);
		goto bail;
	}

	if (listen(sd, 4) < 0) {
		socket_close(sd);
		goto bail;
	}

	freeaddrinfo(res);
	return sd;

bail:
	freeaddrinfo(res);
	return -1;
}

int socket_accept(int sd, struct sockaddr_storage *addr, socklen_t *addrlen) {
	int res;

	*addrlen = sizeof *addr;

	while ((res = accept(sd, (struct sockaddr *) addr, addrlen)) < 0)
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

ssize_t socket_sendto(int sd, const void *p, size_t n, const struct sockaddr *addr, socklen_t addrlen) {
        ssize_t err, off, len;

        for (off = 0, len = n; len != 0; off += err > 0 ? err : 0, len = n - off)
                if ((err = sendto(sd, (const char *) p + off, len, 0, addr, addrlen)) < 0 && errno != EINTR)
                        return -1;

        return off;
}

ssize_t socket_recvfrom(int sd, void *p, size_t n, struct sockaddr_storage *addr, socklen_t *addrlen) {
        ssize_t err, off, len;

	*addrlen = sizeof *addr;

        for (off = 0, len = n; len != 0; off += err > 0 ? err : 0, len = n - off)
                if ((err = recvfrom(sd, (char *) p + off, len, 0, (struct sockaddr *) addr, addrlen)) == 0)
                        break;
                else if (err < 0 && errno != EINTR)
                        return -1;

        return off;
}

