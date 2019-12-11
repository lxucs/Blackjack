OBJS = client.o deck.o
CFLAGS = -Wall -O2

all: client

client: $(OBJS)
	$(CC) $(OBJS) -o client

deck.o: deck.h

.c.o:
	$(CC) $(CFLAGS) -c $*.c

clean:
	rm -f client *.o
