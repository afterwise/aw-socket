
export CFLAGS += -Wall -Wextra

test: test.o ../libaw-socket.a aw-thread/libaw-thread.a
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c aw-thread/aw-thread.h
	$(CC) $(CFLAGS) -I.. -c $< -o $@

aw-thread/libaw-thread.a:
	$(MAKE) -Caw-thread

../libaw-socket.a: CFLAGS += -Itest/aw-thread

../libaw-socket.a:
	$(MAKE) -C..

aw-thread/aw-thread.h:
	rm -rf aw-thread && \
	cp -r ../../aw-thread . && \
	touch -c $@
#	git clone git@github.com:afterwise/aw-fs.git && \
#	touch -c $@

.PHONY: clean
clean:
	rm -f test test.o
	rm -rf aw-thread

