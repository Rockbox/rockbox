/*
  Ballerburg

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file COPYING.txt for details.
*/

#include <SDL.h>

#ifndef BALLER_PATHS_H
#define BALLER_PATHS_H

extern void Paths_Init(const char *argv0);
extern const char *Paths_GetWorkingDir(void);
extern const char *Paths_GetDataDir(void);
extern const char *Paths_GetLocaleDir(void);
extern const char *Paths_GetUserHome(void);
extern const char *Paths_GetProgHome(void);

#endif
