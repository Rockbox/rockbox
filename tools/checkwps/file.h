#ifndef MAX_PATH
#define MAX_PATH 260
#endif

/* Wrapper - required for O_RDONLY */

#include <fcntl.h>

extern ssize_t read(int fd, void *buf, size_t count);
extern ssize_t write(int fd, const void *buf, size_t count);
extern off_t lseek(int fildes, off_t offset, int whence);
extern int close(int fd);

/* strlcpy doesn't belong here (it's in string.h in the rockbox sources),
 * but this avoids complicated magic to override the system string.h */
size_t  strlcpy(char *dst, const char *src, size_t siz);
