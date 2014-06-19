
#include "aw-socket.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define REQUEST "GET /wiki/Main_Page http/1.1\nHost: en.wikipedia.org\n\n"

static char buf[4096];

int main(int argc, char *argv[]) {
	int sd;
	ssize_t err;

	(void) argc;
	(void) argv;

	socket_init();

	sd = socket_connect("en.wikipedia.org", "http", SOCKET_STREAM);

	strcpy(buf, REQUEST);
	err = socket_send(sd, buf, sizeof REQUEST - 1);

	err = socket_recv(sd, buf, sizeof buf);
	printf("%s\n", buf);

	err = socket_close(sd);

	return 0;
}

