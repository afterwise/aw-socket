
#include "aw-socket.h"
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define REQUEST "GET /wiki/Main_Page http/1.1\nHost: en.wikipedia.org\n\n"

static char buf[4096];

int main(int argc, char *argv[]) {
	struct sockaddr_storage addr;
	socklen_t addrlen;
	char ipstr[46];
	int sd, port;
	ssize_t err;

	(void) argc;
	(void) argv;

	socket_init();

	socket_getaddr(&addr, &addrlen, "en.wikipedia.org", "http");

	if (addr.ss_family == AF_INET) {
		struct sockaddr_in *s = (struct sockaddr_in *) &addr;
		port = ntohs(s->sin_port);
		inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
	} else {
		struct sockaddr_in6 *s = (struct sockaddr_in6 *) &addr;
		port = ntohs(s->sin6_port);
		inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
	}

	fprintf(stderr, "addr: %s @ %d\n\n", ipstr, port);

	sd = socket_connect("en.wikipedia.org", "http", SOCKET_STREAM);

	strcpy(buf, REQUEST);
	err = socket_send(sd, buf, sizeof REQUEST - 1);

	err = socket_recv(sd, buf, sizeof buf);
	printf("%s\n", buf);

	err = socket_close(sd);

	return 0;
}

