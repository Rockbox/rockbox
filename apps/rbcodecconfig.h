#ifndef RBCODECCONFIG_H_INCLUDED
#define RBCODECCONFIG_H_INCLUDED

#include "config.h"

#ifndef HAVE_MAS35XX
#define RBCODEC_ENABLE_CHANNEL_MODES
#endif

#define HAVE_ROCKBOX_FIXEDPOINT
#define HAVE_STRLCPY

#ifndef __ASSEMBLER__

/* NULL, offsetof, size_t */
#include <stddef.h>

/* ssize_t, off_t, open, close, read, lseek, SEEK_SET, SEEK_CUR, SEEK_END,
 * O_RDONLY, O_WRONLY, O_CREAT, O_APPEND, MAX_PATH, filesize */
#include "file.h"

/* {,u}int{8,16,32,64}_t, , intptr_t, uintptr_t, bool, true, false, swap16,
 * swap32, hto{be,le}{16,32}, {be,le}toh{16,32}, ROCKBOX_{BIG,LITTLE}_ENDIAN,
 * {,U}INT{8,16,32,64}_{MIN,MAX} */
#include "system.h"

#endif

#endif
