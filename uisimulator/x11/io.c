
#include <sys/stat.h>
#include <dirent.h>

#define DIRFUNCTIONS_DEFINED /* prevent those prototypes */
#define dirent x11_dirent
#include "../../firmware/common/dir.h"
#undef dirent

#define SIMULATOR_ARCHOS_ROOT "archos"

struct mydir {
  DIR *dir;
  char *name;
};

typedef struct mydir MYDIR;

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

struct x11_dirent *x11_readdir(MYDIR *dir)
{
  char buffer[512]; /* sufficiently big */
  static struct x11_dirent secret;
  struct stat s;
  struct dirent *x11 = (readdir)(dir->dir);

  if(!x11)
    return NULL;

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
