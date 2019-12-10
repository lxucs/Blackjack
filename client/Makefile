all: client

.c.o:
        gcc -Wall -c -o $@ $<

client: client.c client.o
        gcc -Wall -o client client.c

clean:
        rm -f client *.o