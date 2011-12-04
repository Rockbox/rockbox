/*
 * string.h
 *
 * Definitions for memory and string functions.
 */

#ifndef _STRING_H_
#define _STRING_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "_ansi.h"

#define __need_size_t
#define __need_NULL
#include <stddef.h>

_PTR     _EXFUN(memchr,(const _PTR, int, size_t));
int      _EXFUN(memcmp,(const _PTR, const _PTR, size_t));
_PTR     _EXFUN(memcpy,(_PTR, const _PTR, size_t));
_PTR     _EXFUN(memmove,(_PTR, const _PTR, size_t));
_PTR     _EXFUN(memset,(_PTR, int, size_t));
char    *_EXFUN(strcat,(char *, const char *));
char    *_EXFUN(strchr,(const char *, int));
int      _EXFUN(strcmp,(const char *, const char *));
int      _EXFUN(strcoll,(const char *, const char *));
char    *_EXFUN(strcpy,(char *, const char *));
size_t   _EXFUN(strcspn,(const char *, const char *));
char    *_EXFUN(strerror,(int));
size_t   _EXFUN(strlen,(const char *));
char    *_EXFUN(strncat,(char *, const char *, size_t));
int      _EXFUN(strncmp,(const char *, const char *, size_t));
char    *_EXFUN(strpbrk,(const char *, const char *));
char    *_EXFUN(strrchr,(const char *, int));
size_t   _EXFUN(strspn,(const char *, const char *));
char    *_EXFUN(strstr,(const char *, const char *));
char    *_EXFUN(strcasestr,(const char *, const char *));

size_t  strlcpy(char *dst, const char *src, size_t siz);
size_t  strlcat(char *dst, const char *src, size_t siz);

#ifndef _REENT_ONLY
char    *_EXFUN(strtok,(char *, const char *));
#endif

size_t   _EXFUN(strxfrm,(char *, const char *, size_t));

#ifndef __STRICT_ANSI__
char    *_EXFUN(strtok_r,(char *, const char *, char **));

_PTR     _EXFUN(memccpy,(_PTR, const _PTR, int, size_t));
int      _EXFUN(strcasecmp,(const char *, const char *));
int      _EXFUN(strncasecmp,(const char *, const char *, size_t));

#ifdef __CYGWIN__
#ifndef DEFS_H  /* Kludge to work around problem compiling in gdb */
const char  *_EXFUN(strsignal, (int __signo));
#endif
int     _EXFUN(strtosigno, (const char *__name));
#endif

#endif /* ! __STRICT_ANSI__ */

#ifdef __cplusplus
}
#endif
#endif /* _STRING_H_ */
