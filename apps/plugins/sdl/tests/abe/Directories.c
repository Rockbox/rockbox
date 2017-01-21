#include "Directories.h"

// Returns a char* to "/home/username/.abe/" or ".\"
// It does not mean that that directory exists.
char *
getHomeUserAbe()
{

  static char path[PATH_SIZE];

  sprintf(path, "/.abe/");

  return path;
}

// MaKe Sure Home User Abe Exists.
// See above getHomeUserAbe() .
// On Windows it does nothing.
void
mkshuae()
{
#ifndef WIN32
  char *hua = getHomeUserAbe();

  mkdir(hua);
#endif
}
