
#include "dir.h"

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

struct dirent *x11_readdir(DIR *dir)
{
  static struct dirent secret;

  struct x11_dirent *x11 = (readdir)(dir);

  strcpy(secret.d_name, x11->d_name);
  secret.attribute = (x11->d_type == DT_DIR)?ATTR_DIRECTORY:0;

  return &secret;
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
