#ifndef BLACKJACK_UTIL_H
#define BLACKJACK_UTIL_H

#include <unistd.h>
#include "deck.h"

extern int card_value(char c);
extern int get_hand_value(struct hand *h);
extern ssize_t readn(int fd, char *buffer, size_t count);
extern ssize_t writen(int fd, char *buffer, size_t count);
extern int nprintf(int fd, char *format, ...);
extern int readline(int fd, char *buf, size_t maxlen);

#endif //BLACKJACK_UTIL_H
