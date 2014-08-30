#undef MAX_PATH
#define MAX_PATH 260
#include <unistd.h>
#include <fcntl.h>

off_t filesize(int fd);
