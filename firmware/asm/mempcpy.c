/*
FUNCTION
        <<mempcpy>>---copy memory regions and return end pointer

ANSI_SYNOPSIS
        #include <string.h>
        void* mempcpy(void *<[out]>, const void *<[in]>, size_t <[n]>);

TRAD_SYNOPSIS
        void *mempcpy(<[out]>, <[in]>, <[n]>
        void *<[out]>;
        void *<[in]>;
        size_t <[n]>;

DESCRIPTION
        This function copies <[n]> bytes from the memory region
        pointed to by <[in]> to the memory region pointed to by
        <[out]>.

        If the regions overlap, the behavior is undefined.

RETURNS
        <<mempcpy>> returns a pointer to the byte following the
        last byte copied to the <[out]> region.

PORTABILITY
<<mempcpy>> is a GNU extension.

<<mempcpy>> requires no supporting OS subroutines.

        */

#include "config.h"
#include "_ansi.h" /* for _DEFUN */
#include <string.h>

/* This may be conjoined with memcpy in <cpu>/memcpy.S to get it nearly for
   free */

_PTR
_DEFUN (mempcpy, (dst0, src0, len0),
        _PTR dst0 _AND
        _CONST _PTR src0 _AND
        size_t len0)
{
    return memcpy(dst0, src0, len0) + len0;
}
