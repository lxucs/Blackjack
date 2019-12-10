OBJS = blackjack.o account.o misc.o deck.o fdutil.o
CFLAGS = -Wall -O2

all: blackjack

blackjack: $(OBJS)
        $(CC) $(OBJS) -o blackjack

deck.o: deck.h
fdutil.o: fdutil.h
account.o: account.h
misc.o: misc.h

.c.o:
        $(CC) $(CFLAGS) -c $*.c

clean:
        rm -f blackjack *.o