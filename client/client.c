#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include "deck.h"
#include "util.h"

typedef struct deck my_deck;
typedef struct hand my_hand;

int fd = -1;
char buf[4096];
static const char CARDS[] = "A23456789TJQK";

char hand1, hand2, faceup;
int hand_value, face_value, balance;
int win = 0, lose = 0, push=0, game_state = 0;
char last_result = 0;  // W: win; L: lose; P: push; B: bust

char debug = 1;

void exit_error(char* str) {
    printf("%s\n", str);
    exit(EXIT_FAILURE);
}

void action_balance() {
    nprintf(fd, "BALANCE\n");
    if (!readline(fd, buf, sizeof(buf)))
        exit_error("balance: readline");
    if (sscanf(buf, "+OK %d", &balance) != 1)
        exit_error(("balance: sscanf"));
}

void action_bet(int amount) {
    if(game_state == 1) {
        printf("Error: bet during a game\n");
        return;
    }

    int bet;
    nprintf(fd, "BET %d\n", amount);
    printf("** BET %d\n", amount);

    if (!readline(fd, buf, sizeof(buf)))
        exit_error("bet: readline");
    if (sscanf(buf, "+OK BET %d HAND %c%c %d FACEUP %c %d", &bet, &hand1, &hand2, &hand_value, &faceup, &face_value) != 6)
        exit_error(("bet: sscanf"));
    printf("%s\n", buf);

    if(bet != amount)
        exit_error("bet: amount error");
    game_state = 1;
}

void action_hit() {
    if(game_state == 0) {
        printf("Error: hit without a game\n");
        return;
    }

    nprintf(fd, "HIT\n");
    printf("** HIT\n");

    if (!readline(fd, buf, sizeof(buf)))
        exit_error("hit: readline");
    printf("%s\n", buf);

    if(buf[4] == 'B') {
        game_state = 0;
        lose++;
        last_result = 'B';
    } else if(buf[4] == 'G') {
        char tmp[10];
        if (sscanf(buf, "+OK GOT %s %d", tmp, &hand_value) != 2)
            exit_error(("hit: sscanf"));
    } else {
        printf("hit: buf is %s\n", buf);
        exit(EXIT_FAILURE);
    }
}

void action_stand() {
    if(game_state == 0) {
        printf("Error: stand without a game\n");
        return;
    }

    nprintf(fd, "STAND\n");
    printf("** STAND\n");

    if (!readline(fd, buf, sizeof(buf)))
        exit_error("stand: readline");
    printf("%s\n", buf);

    if(buf[4] == 'W') {
        win++;
    } else if(buf[4] == 'L') {
        lose++;
    } else if(buf[4] == 'P') {
        push++;
    } else {
        printf("stand: buf is %s\n", buf);
        exit(EXIT_FAILURE);
    }
    game_state = 0;
    last_result = buf[4];
}

void action_double() {
    if(game_state == 0) {
        printf("Error: double without a game\n");
        return;
    }

    nprintf(fd, "DOUBLE\n");
    printf("** DOUBLE\n");

    if (!readline(fd, buf, sizeof(buf)))
        exit_error("stand: readline");
    printf("%s\n", buf);

    if(buf[4] == 'W') {
        win++;
    } else if(buf[4] == 'L') {
        lose++;
    } else if(buf[4] == 'B') {
        lose++;
    } else if(buf[4] == 'P') {
        push++;
    } else {
        printf("stand: buf is %s\n", buf);
        exit(EXIT_FAILURE);
    }
    game_state = 0;
    last_result = buf[4];
}

void play_games_dummy(int epoch) {
    printf("Playing %d games...\n", epoch);
    action_balance();

    for(int epo = 1; epo <= epoch; ++epo) {
        printf("-----------------------Game %d-----------------------\n", epo);
        int balance_start = balance;

        action_bet(1);
        while(game_state == 1 && hand_value < 12)
            action_hit();

        if(game_state == 1)
            action_stand();

        action_balance();

        printf("*** Result: %c\tWin: %d\t Lose: %d\t Push: %d\t Balance: $%d -> $%d\n", last_result, win, lose, push, balance_start, balance);

        sleep(3);
    }
}

my_deck shuffle_new_deck() {
    my_deck deck;
    deck.top = 0;

    for(int i = 0; i < 4; ++i)
        for(int j = 0; j < 13; ++j) {
            deck.cards[i * 13 + j] = CARDS[j];
        }
    for(int i = 1; i < 52; ++i) {
        int r = rand() % i;
        char tmp = deck.cards[i];
        deck.cards[i] = deck.cards[r];
        deck.cards[r] = tmp;
    }

    return deck;
}

int verify_seed(unsigned int seed, char card1, char card2, char card3,
                char card1_r2, char card2_r2, char card3_r2) {
    srand(seed);
    my_deck deck = shuffle_new_deck();
    if(deck.cards[0] == card1 && deck.cards[1] == card2 && deck.cards[2] == card3) {
        deck = shuffle_new_deck();
        if(deck.cards[0] == card1_r2 && deck.cards[1] == card2_r2 && deck.cards[2] == card3_r2) {
            printf("!!!!!Found seed: %d\n", seed);
            return 1;
        }
    }
    return 0;
}

unsigned int crack_and_set_seed(int seed_search_range, char card1, char card2, char card3,
                                char card1_r2, char card2_r2, char card3_r2) {
    time_t curr_t = time(NULL);
    time_t range_high = curr_t + seed_search_range;
    time_t range_low = curr_t - seed_search_range;
    for(time_t t = range_high; t >= range_low; --t) {
        unsigned int seed = t;
        if(verify_seed(seed, card1, card2, card3, card1_r2, card2_r2, card3_r2)) {
            return seed;
        }
    }
    printf("Didn't find seed in range [%ld - %ld]\n", range_low, range_high);
    return 0;
}

void check_security(char hand1_foreseen, char hand2_foreseen, char faceup_foreseen) {
    /* Should be called after action_bet() */
    if(hand1_foreseen == hand1 && hand2_foreseen == hand2 && faceup_foreseen == faceup)
        return;
    else {
        printf("Error: foreseen values don't match actual card values: %c%c, %c\n", hand1, hand2, faceup);
        exit(EXIT_FAILURE);
    }
}

int bet_big() {
    int amount = balance / 5;
    return amount > 50000? 50000 : amount;
}

void play_games_cheating(int stop_threshold, int seed_search_range) {
    printf("Playing games by cheating... (until reaching $%d)\n\n", stop_threshold);
    action_balance();

    printf("Probing random seed (bet minimum twice)...\n");
    action_bet(1);
    char hand1_r1 = hand1, hand2_r1 = hand2, faceup_r1 = faceup;
    action_stand();
    printf("Finished first bet; result: %c; balance: $%d\n", last_result, balance);
    action_bet(1);
    char hand1_r2 = hand1, hand2_r2 = hand2, faceup_r2 = faceup;
    action_stand();
    printf("Finished second bet; result: %c; balance: $%d\n\n", last_result, balance);

    unsigned int seed = crack_and_set_seed(seed_search_range, hand1_r1, hand2_r1, faceup_r1, hand1_r2, hand2_r2, faceup_r2);
    if(seed == 0)
        return;

    // Play game optimally
    int epo = 1;
    while(balance < stop_threshold) {
        printf("-----------------------Game %d-----------------------\n", epo++);
        int balance_start = balance;

        my_deck deck = shuffle_new_deck();  // Foresee entire deck
        my_hand self_hand, dealer_hand;
        hand_init(&self_hand);
        hand_init(&dealer_hand);
        deck_deal(&deck, &self_hand);
        deck_deal(&deck, &self_hand);
        deck_deal(&deck, &dealer_hand);
        deck_deal(&deck, &dealer_hand);
        printf("##### Foreseen deck (first 10 cards): ");
        for(int i = 0; i < 10; ++i)
            printf("%c", deck.cards[i]);
        printf("\n");

        char won_this_game = 0;
        // If blackjack, we win
        if(get_hand_value(&self_hand) == 21) {
            printf("##### Analysis: we will win by blackjack, bet big!\n");
            action_bet(bet_big());
            check_security(deck.cards[0], deck.cards[1], deck.cards[2]);
            action_stand();
            won_this_game = 1;
        }
        // Try if we can win by double (whenever we can win by larger point)
        if(!won_this_game) {
            my_deck deck_tmp = deck;
            my_hand self_hand_tmp = self_hand;
            deck_deal(&deck_tmp, &self_hand_tmp);
            while(get_hand_value(&self_hand_tmp) <= 21 && !won_this_game) {
                if(get_hand_value(&self_hand_tmp) > get_hand_value(&dealer_hand)) {
                    printf("##### Analysis: we can win by double, bet big!\n");
                    action_bet(bet_big());
                    check_security(deck.cards[0], deck.cards[1], deck.cards[2]);
                    for(int k = 0; k < self_hand_tmp.ncards - 3; ++k)
                        action_hit();
                    action_double();
                    won_this_game = 1;
                    break;
                }
                deck_deal(&deck_tmp, &self_hand_tmp);
            }
        }
        // If cannot win by larger point, try to let dealer bust
        if(!won_this_game) {
            for(int hit = 0; ; ++hit) {
                my_deck deck_tmp = deck;
                my_hand self_hand_tmp = self_hand;
                my_hand dealer_hand_tmp = dealer_hand;
                for(int k = 0; k < hit; ++k)
                    deck_deal(&deck_tmp, &self_hand_tmp);
                if(get_hand_value(&self_hand_tmp) > 21)
                    break;
                while(get_hand_value(&dealer_hand_tmp) < 17)
                    deck_deal(&deck_tmp, &dealer_hand_tmp);
                if(get_hand_value(&dealer_hand_tmp) > 21) {
                    printf("##### Analysis: we can win by busting dealer, bet big!\n");
                    action_bet(bet_big());
                    check_security(deck.cards[0], deck.cards[1], deck.cards[2]);
                    for(int i = 0; i < hit; ++i)
                        action_hit();
                    action_stand();
                    won_this_game = 1;
                    break;
                }
            }
        }
        // Otherwise, just bet minimum and stand directly
        if(!won_this_game) {
            printf("##### Analysis: hard to win this game; bet minimum..\n");
            action_bet(1);
            action_stand();
        }

        if(game_state == 1)
            exit_error("Error: game is supposed to be over; sth went wrong...");
        action_balance();
        printf("*** Result: %c\tWin: %d\t Lose: %d\t Push: %d\t Balance: $%d -> $%d\n", last_result, win, lose, push, balance_start, balance);

        sleep(1);
    }
    printf("Reached threshold; terminate.\n");
}

int main(int argc, char* argv[])
{
    char host[] = "127.0.0.1";
    int port = 5555;
    struct sockaddr_in sa;

    if (!inet_aton(host, &sa.sin_addr))
    {
        perror("inet_aton");
        return 1;
    }

    if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        perror("socket");
        return 1;
    }

    sa.sin_port = htons(port);
    sa.sin_family = AF_INET;

    if (connect(fd, (struct sockaddr *)&sa, sizeof(struct sockaddr_in)) < 0)
    {
        perror("inet_aton");
        return 1;
    }

    /* "Welcome" message */
    if (!readline(fd, buf, sizeof(buf)))
        return 1;
    printf("%s\n\n", buf);

    int stop_threshold = 1000000;
    int seed_search_range = 50000;
    play_games_cheating(stop_threshold, seed_search_range);

    return 0;
}