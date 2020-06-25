/*
  copied from firmware/assert.h
*/

#undef assert

#ifdef NDEBUG           /* required by ANSI standard */
#define assert(p)       ((void)0)
#else

#define assert(e)       ((e) ? (void)0 : fatal("assertion failed on %s line %d: " #e, __FILE__, __LINE__))

#endif /* NDEBUG */
