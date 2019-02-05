all: pjsua

simple_pjsua: pjsua.c
	$(CC) -g -O0 cb.c pjsua.c -o $@ $< `pkg-config --cflags --libs libpjproject`
pjsua: pjsua.c cb.c
	gcc -g -O0 -lasound cb.c pjsua.c -o pjsua `pkg-config --cflags --libs libpjproject`
clean:
	rm -f pjsua.o pjsua

