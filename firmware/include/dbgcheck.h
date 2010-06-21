#ifndef __DBGCHECK_H__
#define __DBGCHECK_H__

#include <stdbool.h>

#ifdef DEBUG
 #if (CONFIG_PLATFORM & PLATFORM_NATIVE)
  /* check whether a function is inside the valid memory location */
  #define IS_FUNCPTR(fp) ({/
     extern char _text[];/
     extern char _etext[];/
     ((((char *)(fp)) >= _text) && (((char *)(fp)) < _etext)/
  })
 #else
  /* check whether a function is inside the valid memory location */
  #define IS_FUNCPTR(fp) (((char*)(fp)) != NULL)
 #endif
#else
 /* check whether a function is inside the valid memory location */
 #define IS_FUNCPRT (fp) true
#endif


#endif // #ifndef __DBGCHECK_H__
