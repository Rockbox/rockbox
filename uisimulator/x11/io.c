
#include <dirent.h>

#define SIMULATOR_ARCHOS_ROOT "archos"

DIR *x11_opendir(char *name)
{
  char buffer[256]; /* sufficiently big */

  if(name[0] == '/') {
    sprintf(buffer, "%s%s", SIMULATOR_ARCHOS_ROOT, name);    
    return opendir(buffer);
  }
  return opendir(name);
}

int x11_open(char *name, int opts)
{
  char buffer[256]; /* sufficiently big */

  if(name[0] == '/') {
    sprintf(buffer, "%s%s", SIMULATOR_ARCHOS_ROOT, name);

    debugf("We open the real file '%s'", buffer);
    return open(buffer, opts);
  }
  return open(name, opts);
}
