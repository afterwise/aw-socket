
#include "aw-socket.h"
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define REQUEST "GET /wiki/Main_Page http/1.1\nHost: en.wikipedia.org\n\n"

static char buf[4096];

int main(int argc, char *argv[]) {
	struct endpoint ep;
	char ipstr[46];
	int sd, port;
	ssize_t err;

	(void) argc;
	(void) argv;

	socket_init();

	socket_getaddr(&ep, "en.wikipedia.org", "http");

	if (ep.addr.sin_family == AF_INET) {
		port = ntohs(ep.addr.sin_port);
		inet_ntop(AF_INET, &ep.addr.sin_addr, ipstr, sizeof ipstr);
	} else {
		port = ntohs(ep.addr6.sin6_port);
		inet_ntop(AF_INET6, &ep.addr6.sin6_addr, ipstr, sizeof ipstr);
	}

	fprintf(stderr, "addr: %s @ %d\n\n", ipstr, port);

	sd = socket_connect("en.wikipedia.org", "http", SOCKET_STREAM);

	strcpy(buf, REQUEST);
	err = socket_send(sd, buf, sizeof REQUEST - 1);

	err = socket_recv(sd, buf, sizeof buf, SOCKET_WAITALL);
	printf("%s\n", buf);

	err = socket_close(sd);

	return 0;
}

