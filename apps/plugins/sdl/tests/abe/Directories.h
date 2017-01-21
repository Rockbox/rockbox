#ifndef DIRECTORIES_H
#define DIRECTORIES_H

#ifndef WIN32
#include <stdio.h>
#endif

#include "Main.h"

#include "lib/pluginlib_exit.h"

// The macros xstr(s) and str(s) were extracted from 
// http://gcc.gnu.org/onlinedocs/cpp/Stringification.html
// They are used to expand BASE_DIR .
#define xstr(s) str(s)
#define str(s) #s

#ifndef PATH_SIZE               // to allow modifying it by the compiler option -D
#define PATH_SIZE 1024
#endif

#ifdef WIN32
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

// BASE_DIR has not " arround it.
#ifndef BASE_DIR
#define BASE_DIR /
#endif

#define IMAGES_DIR "images"
#define MAPS_DIR "maps"
#define SOUND_DIR "sounds"

char *getHomeUserAbe();

#ifndef WIN32
#define getSaveGameDir() getHomeUserAbe()
#else
#define getSaveGameDir() xstr(BASE_DIR) PATH_SEP "savegame" PATH_SEP
#endif

void mkshuae();

#endif
