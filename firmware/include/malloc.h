/* malloc.h -- header file for memory routines.  */

#ifndef _INCLUDE_MALLOC_H_
#define _INCLUDE_MALLOC_H_

#include <_ansi.h>

#define __need_size_t
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The routines.  */

extern _PTR malloc _PARAMS ((size_t));
extern _VOID free _PARAMS ((_PTR));
extern _PTR realloc _PARAMS ((_PTR, size_t));
extern _PTR calloc _PARAMS ((size_t, size_t));

#ifdef __cplusplus
}
#endif

#endif /* _INCLUDE_MALLOC_H_ */
