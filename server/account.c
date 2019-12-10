#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include "fdutil.h"

int account_balance(char *user)
{
        char pathname[1024];
        char buf[1024];
        int fd;
        ssize_t n;

        snprintf(pathname, sizeof(pathname), "account/%s", user);

        fd = open(pathname, O_RDONLY, 0);

        if (fd < 0)
                return -1;

        if (flock(fd, LOCK_SH) < 0)
        {
                close(fd);
                return -1;
        }

        memset(buf, 0, sizeof(buf));

        n = readn(fd, buf, sizeof(buf) - 1);

        if (n < 0)
                return -1;

        if (flock(fd, LOCK_UN) < 0)
        {
                close(fd);
                return -1;
        }

        if (close(fd) < 0)
                return -1;

        return atoi(buf);
}

int account_update(char *user, int delta)
{
        char pathname[1024];
        char buf[1024];
        int fd;
        ssize_t n;
        int balance;

        snprintf(pathname, sizeof(pathname), "account/%s", user);

        fd = open(pathname, O_RDWR, 0);

        if (fd < 0)
                return -1;

        if (flock(fd, LOCK_EX) < 0)
        {
                close(fd);
                return -1;
        }

        memset(buf, 0, sizeof(buf));

        n = readn(fd, buf, sizeof(buf) - 1);

        if (n < 0)
        {
                close(fd);
                return -1;
        }

        balance = atoi(buf);
        balance = balance + delta;

        if (ftruncate(fd, 0) < 0)
        {
                close(fd);
                return -1;
        }

        if (lseek(fd, 0, SEEK_SET) < 0)
        {
                close(fd);
                return -1;
        }

        snprintf(buf, sizeof(buf), "%d\n", balance);

        if (writen(fd, buf, strlen(buf)) < 0)
        {
                close(fd);
                return -1;
        }

        if (flock(fd, LOCK_UN) < 0)
        {
                close(fd);
                return -1;
        }

        if (close(fd) < 0)
                return -1;

        return 0;
}

char *account_flag(void)
{
        static char buf[1024] = "Flag!";

        return buf;
}