#ifndef _FDUTIL_H
#define _FDUTIL_H

extern ssize_t writen(int fd, char *buffer, size_t count);
extern ssize_t readn(int fd, char *buffer, size_t count);
extern int nprintf(char *format, ...);

#endif