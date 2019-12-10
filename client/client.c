#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

int fd = -1;
char buf[4096];

char hand1, hand2, faceup;
int hand_value, face_value, balance;
int win = 0, lose = 0, push=0, game_state = 0;
char last_result = 0;  // W: win; L: lose; P: push; B: bust

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

typedef struct {
    int bet;
    char* hand_string;
    int hand_value;
    char dealer_faceup;
    int dealer_value;
} bet_response;

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

void play_one_game(int epo) {
    printf("----------------Game %d----------------\n", epo);
    action_balance();
    int balance_start = balance;

    action_bet(1);
    while(game_state == 1 && hand_value < 12)
        action_hit();

    if(game_state == 1)
        action_stand();

    action_balance();

    printf("*** Result: %c\tWin: %d\t Lose: %d\t Push: %d\t Balance: %d -> %d\n", last_result, win, lose, push, balance_start, balance);
}

void play_games() {
    int epoch = 50;
    for(int epo = 1; epo <= epoch; ++epo) {
        play_one_game(epo);
        sleep(3);
    }
}

int main(void)
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
        printf("buf = %s\n", buf);

        play_games();

        return 0;
}