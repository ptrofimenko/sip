all: simple_pjsua

simple_pjsua: main.c
	$(CC) -o $@ $< `pkg-config --cflags --libs libpjproject`
clean:
	rm -f main.o main

