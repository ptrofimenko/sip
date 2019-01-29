all: simple_pjsua

simple_pjsua: pjsua.c
	$(CC) -g -O0 -o $@ $< `pkg-config --cflags --libs libpjproject`
clean:
	rm -f main.o main

