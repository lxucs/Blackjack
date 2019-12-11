OBJS = client.o deck.o util.o
CFLAGS = -Wall -O2

all: client

client: $(OBJS)
	$(CC) $(OBJS) -o client

deck.o: deck.h
util.o: util.h

.c.o:
	$(CC) $(CFLAGS) -c $*.c

clean:
	rm -f client *.o
