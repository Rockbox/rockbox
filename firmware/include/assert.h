/*
        assert.h
*/

#include <panic.h>

#undef assert

#ifdef NDEBUG           /* required by ANSI standard */
#define assert(p)       ((void)0)
#else
#define assert(e)       ((e) ? (void)0 : panicf("Assertion failed: %s (%s: %s: %d)", #e, __FILE__, __func__, __LINE__))
#endif /* NDEBUG */

