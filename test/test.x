
#ifndef _nofeatures
# if _WIN32
#  define WIN32_LEAN_AND_MEAN 1
# elif __linux__
#  define _BSD_SOURCE 1
#  define _DEFAULT_SOURCE 1
#  define _POSIX_C_SOURCE 200809L
#  define _SVID_SOURCE 1
# elif __APPLE__
#  define _DARWIN_C_SOURCE 1
# endif
#endif /* _nofeatures */

#include "aw-socket.h"
#include <stdio.h>
#include <string.h>

#define REQUEST "GET /wiki/Main_Page http/1.1\nHost: en.wikipedia.org\n\n"

static char buf[4096];

int main(int argc, char *argv[]) {
	struct endpoint ep;
	char ipstr[SOCKET_MAXADDRSTRLEN];
	socket_t sd;
	int port;

	(void) argc;
	(void) argv;

	socket_init();

	if (socket_connect(&sd, "en.wikipedia.org", "http", &ep, SOCKET_STREAM) < 0)
		return 1;

	port = socket_tohuman(ipstr, &ep);
	fprintf(stderr, "connect: %s#%d\n\n", ipstr, port);

	strcpy(buf, REQUEST);
	socket_send(sd, buf, sizeof REQUEST - 1);

	socket_recv(sd, buf, sizeof buf, 0);
	printf("%s\n", buf);

	socket_shutdown(sd, SOCKET_BOTH);

	while (socket_recv(sd, buf, sizeof buf, 0) > 0)
		;

	socket_close(sd);

	return 0;
}

