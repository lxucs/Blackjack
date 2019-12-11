#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include "deck.h"

int card_value(char c)
{
    switch (c)
    {
        case 'A':
            return 1;
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return c - '0';
        case 'T':
        case 'J':
        case 'Q':
        case 'K':
            return 10;
    }

    return 0;
}

int get_hand_value(struct hand *h)
{
    int i;
    int aces = 0;
    int sum = 0;

    for (i = 0; i < h->ncards; i++)
    {
        if (h->cards[i] == 'A')
            aces++;
        else
            sum += card_value(h->cards[i]);
    }

    if (aces == 0)
        return sum;

    /* Give 11 points for aces, unless it causes us to go bust, so treat those as 1 point */
    while (aces && (sum + aces*11) > 21)
    {
        sum++;
        aces--;
    }

    sum += 11 * aces;

    return sum;
}

ssize_t readn(int fd, char *buffer, size_t count)
{
    int offset, block;

    offset = 0;
    while (count > 0)
    {
        block = read(fd, &buffer[offset], count);

        if (block < 0) return block;
        if (!block) return offset;

        offset += block;
        count -= block;
    }

    return offset;
}

ssize_t writen(int fd, char *buffer, size_t count)
{
    int offset, block;

    offset = 0;
    while (count > 0)
    {
        block = write(fd, &buffer[offset], count);

        if (block < 0) return block;
        if (!block) return offset;

        offset += block;
        count -= block;
    }

    return offset;
}

int nprintf(int fd, char *format, ...)
{
    va_list args;
    char buf[1024];

    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    return writen(fd, buf, strlen(buf));
}

int readline(int fd, char *buf, size_t maxlen)
{
    int offset;
    ssize_t n;
    char c;

    offset = 0;
    while (1)
    {
        n = read(fd, &c, 1);

        if (n < 0 && n == EINTR)
            continue;

        if (c == '\n')
        {
            buf[offset++] = 0;
            break;
        }

        buf[offset++] = c;

        if (offset >= maxlen)
            return 0;
    }

    return 1;
}