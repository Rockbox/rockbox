#include "file.h"

off_t filesize(int osfd)
{
    struct stat sb;

    if (!fstat(osfd, &sb))
        return sb.st_size;
    else
        return -1;
}
