/* This file is part of libmspack.
 * (C) 2003-2004 Stuart Caie.
 *
 * libmspack is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License (LGPL) version 2.1
 *
 * For further details, see the file COPYING.LIB distributed with libmspack
 */

#ifndef MSPACK_SYSTEM_H
#define MSPACK_SYSTEM_H 1

#ifdef __cplusplus
extern "C" {
#endif

/* ensure config.h is read before mspack.h */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "mspack.h"
#include "macros.h"

/* fix for problem with GCC 4 and glibc (thanks to Ville Skytta)
 * http://bugzilla.redhat.com/bugzilla/show_bug.cgi?id=150429
 */
#ifdef read
# undef read
#endif

/* CAB supports searching through files over 4GB in size, and the CHM file
 * format actively uses 64-bit offsets. These can only be fully supported
 * if the system the code runs on supports large files. If not, the library
 * will work as normal using only 32-bit arithmetic, but if an offset
 * greater than 2GB is detected, an error message indicating the library
 * can't support the file should be printed.
 */
#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif

extern struct mspack_system *mspack_default_system;

/* returns the length of a file opened for reading */
extern int mspack_sys_filelen(struct mspack_system *system,
			      struct mspack_file *file, off_t *length);

/* validates a system structure */
extern int mspack_valid_system(struct mspack_system *sys);

#if HAVE_STRINGS_H
# include <strings.h>
#endif

#if HAVE_STRING_H
# include <string.h>
#endif

#if HAVE_MEMCMP
# define mspack_memcmp memcmp
#else
/* inline memcmp() */
#ifdef _MSC_VER /* MSVC requires use of __inline instead of inline */
#define INLINE __inline
#else
#define INLINE inline
#endif
static INLINE int mspack_memcmp(const void *s1, const void *s2, size_t n) {
  unsigned char *c1 = (unsigned char *) s1;
  unsigned char *c2 = (unsigned char *) s2;
  if (n == 0) return 0;
  while (--n && (*c1 == *c2)) c1++, c2++;
  return *c1 - *c2;
}
#endif

#ifdef __cplusplus
}
#endif

#endif
