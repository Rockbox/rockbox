/*
  copied from firmware/assert.h
*/

#undef assert

#ifdef NDEBUG           /* required by ANSI standard */
#define assert(p)       ((void)0)
#else

#define assert(e)       ((e) ? (void)0 : fatal("assertion failed %s:%d", __FILE__, __LINE__))

#endif /* NDEBUG */
