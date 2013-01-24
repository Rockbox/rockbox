/*
** FreeBSD Alpha (64 bit) data types
*/

#define FICL_WANT_PLATFORM (1)

#define FICL_PLATFORM_BASIC_TYPES   (1)
#define FICL_PLATFORM_ALIGNMENT     (8)

#define FICL_PLATFORM_HAS_2INTEGER  (0)
#define FICL_PLATFORM_HAS_FTRUNCATE (1)
#define FICL_PLATFORM_INLINE        inline
#define FICL_PLATFORM_OS            "FreeBSD"
#define FICL_PLATFORM_ARCHITECTURE  "alpha"

typedef char ficlInteger8;
typedef unsigned char ficlUnsigned8;
typedef short ficlInteger16;
typedef unsigned short ficlUnsigned16;
typedef int ficlInteger32;
typedef unsigned int ficlUnsigned32;
typedef long ficlInteger64;
typedef unsigned long ficlUnsigned64;

typedef ficlInteger64 ficlInteger;
typedef ficlUnsigned64 ficlUnsigned;
typedef float ficlFloat;
