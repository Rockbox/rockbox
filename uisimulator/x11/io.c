
#include <sys/stat.h>
#include "dir.h"

#undef DIR

MYDIR *x11_opendir(char *name)
{
  char buffer[256]; /* sufficiently big */
  MYDIR *my = (MYDIR *)malloc(sizeof(MYDIR));

  if(name[0] == '/') {
    sprintf(buffer, "%s%s", SIMULATOR_ARCHOS_ROOT, name);    
    my->dir=(DIR *)opendir(buffer);
  }
  else
    my->dir=(DIR *)opendir(name);
  
  my->name = (char *)strdup(name);

  return my;
}

struct dirent *x11_readdir(MYDIR *dir)
{
  char buffer[512]; /* sufficiently big */
  static struct dirent secret;
  struct stat s;

  struct x11_dirent *x11 = (readdir)(dir->dir);

  strcpy(secret.d_name, x11->d_name);

  /* build file name */
  sprintf(buffer, SIMULATOR_ARCHOS_ROOT "%s/%s",
          dir->name, x11->d_name);
  stat(buffer, &s); /* get info */

  secret.attribute = S_ISDIR(s.st_mode)?ATTR_DIRECTORY:0;
  secret.size = s.st_size;

  return &secret;
}

void x11_closedir(MYDIR *dir)
{
  free(dir->name);
  (closedir)(dir->dir);

  free(dir);
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
