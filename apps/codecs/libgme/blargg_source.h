// Included at the beginning of library source files, after all other #include lines
#ifndef BLARGG_SOURCE_H
#define BLARGG_SOURCE_H

// If debugging is enabled and expr is false, abort program. Meant for checking
// caller-supplied parameters and operations that are outside the control of the
// module. A failed requirement indicates a bug outside the module.
// void require( bool expr );
#if defined(ROCKBOX)
#undef assert
#define assert( expr )
#undef require
#define require( expr )
#else
#undef require
#define require( expr ) assert( expr )
#endif

// Like printf() except output goes to debug log file. Might be defined to do
// nothing (not even evaluate its arguments).
// void dprintf( const char* format, ... );
#if defined(ROCKBOX)
#define dprintf DEBUGF
#else
static inline void blargg_dprintf_( const char* fmt, ... ) { }
#undef dprintf
#define dprintf (1) ? (void) 0 : blargg_dprintf_
#endif

// If enabled, evaluate expr and if false, make debug log entry with source file
// and line. Meant for finding situations that should be examined further, but that
// don't indicate a problem. In all cases, execution continues normally.
#undef check
#define check( expr ) ((void) 0)

/* If expr yields non-NULL error string, returns it from current function,
otherwise continues normally. */
#undef  RETURN_ERR
#define RETURN_ERR( expr ) \
	do {\
		blargg_err_t blargg_return_err_ = (expr);\
		if ( blargg_return_err_ )\
			return blargg_return_err_;\
	} while ( 0 )

/* If ptr is NULL, returns out-of-memory error, otherwise continues normally. */
#undef  CHECK_ALLOC
#define CHECK_ALLOC( ptr ) \
	do {\
		if ( !(ptr) )\
			return "Out of memory";\
	} while ( 0 )

#ifndef max
	#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
	#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

// typedef unsigned char byte;
typedef unsigned char blargg_byte;
#undef  byte
#define byte blargg_byte

// deprecated
#define BLARGG_CHECK_ALLOC CHECK_ALLOC
#define BLARGG_RETURN_ERR RETURN_ERR

// BLARGG_SOURCE_BEGIN: If defined, #included, allowing redefition of dprintf and check
#ifdef BLARGG_SOURCE_BEGIN
	#include BLARGG_SOURCE_BEGIN
#endif

#endif
