
#ifndef AW_SOCKET_H
#define AW_SOCKET_H

#if _WIN32
# include <winsock2.h>
# include <sys/types.h>
#else
# include <sys/socket.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if _WIN32
typedef int socklen_t;
#endif

void socket_init();
void socket_end();

enum {
	SOCKET_STREAM = 1 << 0,
	SOCKET_NONBLOCK = 1 << 1,
	SOCKET_REUSEADDR = 1 << 2
};

int socket_getaddr(
	struct sockaddr_storage *addr, socklen_t *addrlen,
	const char *node, const char *service);

int socket_connect(const char *node, const char *service, int flags);
int socket_listen(const char *service, int flags);
int socket_accept(int sd, struct sockaddr_storage *addr, socklen_t *addrlen);
int socket_close(int sd);

ssize_t socket_send(int sd, const void *p, size_t n);
ssize_t socket_recv(int sd, void *p, size_t n);

ssize_t socket_sendto(int sd, const void *p, size_t n, const struct sockaddr_storage *addr, socklen_t addrlen);
ssize_t socket_recvfrom(int sd, void *p, size_t n, struct sockaddr_storage *addr, socklen_t *addrlen);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AW_SOCKET_H */

