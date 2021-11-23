#ifndef FILE_H
#define FILE_H

#include <sys/types.h>
#include <sys/statfs.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef MAX_PATH
# undef MAX_PATH
#endif

#define MAX_PATH 260

off_t filesize(int fd);

#endif
