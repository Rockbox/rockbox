#include <sys/types.h>
#include <dirent.h>

#define opendir(x) x11_opendir(x)

#define DIRENT_DEFINED /* prevent it from getting defined again */
#include "../../firmware/common/dir.h"
