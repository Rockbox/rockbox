//
//  rockbox_compat.h
//  Duke3D
//
//  Based on unix_compat.h
//  Copyright Wed, Jul 31, 2013, Juan Manuel Borges Caño (GPLv3+)
//

/* all we need */
#include "SDL.h"
#include "plugin.h"

#ifndef Duke3D_rockbox_compat_h
#define Duke3D_rockbox_compat_h

//#define BYTE_ORDER LITTLE_ENDIAN
#define PLATFORM_SUPPORTS_SDL

#define kmalloc(x) malloc(x)
#define kkmalloc(x) malloc(x)
#define kfree(x) free(x)
#define kkfree(x) free(x)

#ifdef FP_OFF
#undef FP_OFF
#endif

// Horrible horrible macro: Watcom allowed memory pointer to be cast
// to a 32bits integer. The code is unfortunately stuffed with this :( !
#define FP_OFF(x) ((int32_t)(x))

#ifndef max
#define max(x, y)  (((x) > (y)) ? (x) : (y))
#endif

#ifndef min
#define min(x, y)  (((x) < (y)) ? (x) : (y))
#endif

#include <inttypes.h>
#define __int64 int64_t

#define O_BINARY 0

#define UDP_NETWORKING 1

#define PLATFORM_ROCKBOX 1

#define read(a, b, c) rb->read((a), (b), (c))
#define write(a, b, c) rb->write((a), (b), (c))
#define close(a) rb->close(a)


/*
#define SOL_IP SOL_SOCKET
#define IP_RECVERR  SO_BROADCAST
*/

#ifndef stricmp
#define stricmp strcasecmp
#endif
#ifndef strcmpi
#define strcmpi strcasecmp
#endif

#define S_IREAD S_IRUSR

#define USER_DUMMY_NETWORK 1
#define STUB_NETWORKING 1

#endif
