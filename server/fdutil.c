#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include "fdutil.h"

int nprintf(char *format, ...)
{
        va_list args;
        char buf[1024];

        va_start(args, format);
        vsnprintf(buf, sizeof(buf), format, args);
        va_end(args);

        return writen(1, buf, strlen(buf));
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