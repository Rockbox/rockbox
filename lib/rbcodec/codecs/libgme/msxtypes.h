#ifndef MSX_TYPES
#define MSX_TYPES

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __GNUC__
#define __int64 long long
#endif

#ifdef _WIN32
#define DIR_SEPARATOR "\\"
#else
#define DIR_SEPARATOR "/"
#endif

/* So far, only support for MSVC types
 */
typedef unsigned char    UInt8;
#ifndef __CARBON__
typedef unsigned short   UInt16;
typedef unsigned int     UInt32;
typedef unsigned __int64 UInt64;
#endif
typedef signed   char    Int8;
typedef signed   short   Int16;
typedef signed   int     Int32;

#ifdef __cplusplus
}
#endif


#endif
