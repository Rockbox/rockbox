//
//  dukerockbox.h
//  Duke3D
//
//  Created by fabien sanglard on 12-12-12.
//  Copyright (c) 2012 fabien sanglard. All rights reserved.
//

#ifndef Duke3D_dukerb_h
#define Duke3D_dukerb_h

#include "SDL.h"

#define cdecl
#define __far
#define __interrupt


#define STUBBED(x)
#ifdef __SUNPRO_C
//#define STUBBED(x) fprintf(stderr,"STUB: %s (??? %s:%d)\n",x,__FILE__,__LINE__)
#else
//#define STUBBED(x) fprintf(stderr,"STUB: %s (%s, %s:%d)\n",x,__FUNCTION__,__FILE__,__LINE__)
#endif

#define PATH_SEP_CHAR '/'
#define PATH_SEP_STR  "/"
#define ROOTDIR       "/"
#define CURDIR        "/"

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef min
#define min(x, y) ((x) < (y) ? (x) : (y))
#endif

#ifndef max
#define max(x, y) ((x) > (y) ? (x) : (y))
#endif



#ifndef strcmpi
#define strcmpi(x, y) strcasecmp(x, y)
#endif

#ifdef DC
#undef stderr
#undef stdout
#undef getchar
/* kos compat */
#define stderr ((FILE*)2)
#define stdout ((FILE*)2)
#define Z_AvailHeap() ((10 * 1024) * 1024)
#else
// 64 megs should be enough for anybody.  :)  --ryan.
#define Z_AvailHeap() ((64 * 1024) * 1024)
#endif

#define printchrasm(x,y,ch) printf("%c", (uint8_t ) (ch & 0xFF))

#ifdef __GNUC__
#define GCC_PACK1_EXT __attribute__((packed,aligned(1)))
#endif


// FCS: Game.c features calls to mkdir without the proper flags.
// Giving all access is ugly but it is just game OK !
//#define mkdir(X) mkdir(X,0777)

#define getch getchar

#endif
