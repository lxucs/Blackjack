#ifndef _BLACKJACK_ACCOUNT_H
#define _BLACKJACK_ACCOUNT_H

extern int account_balance(char *user);
extern int account_update(char *user, int delta);
extern char *account_flag(void);

#endif