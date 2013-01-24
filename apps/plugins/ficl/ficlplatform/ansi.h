#include <stdint.h>
#include "config.h"

typedef int8_t ficlInteger8;
typedef uint8_t ficlUnsigned8;
typedef int16_t ficlInteger16;
typedef uint16_t ficlUnsigned16;
typedef int32_t ficlInteger32;
typedef uint32_t ficlUnsigned32;
typedef int64_t ficlInteger64;
typedef uint64_t ficlUnsigned64;

#if UINTPTR_MAX == 0xffffffffffffffff
/* 64bit system */
typedef ficlInteger64 ficlInteger;
typedef ficlUnsigned64 ficlUnsigned;
#define FICL_PLATFORM_ALIGNMENT       (8)
#else
/* 32bit system */
typedef intptr_t ficlInteger;
typedef uintptr_t ficlUnsigned;
#define FICL_PLATFORM_ALIGNMENT       (4)
#endif

typedef float ficlFloat;

#define FICL_PLATFORM_INLINE          static inline

#define FICL_PLATFORM_BASIC_TYPES   (1)
#define FICL_PLATFORM_HAS_2INTEGER  (0)
#define FICL_PLATFORM_HAS_FTRUNCATE (0)

#define FICL_PLATFORM_OS            "rockbox"

#if (ARCH == ARCH_ARM)
#define FICL_PLATFORM_ARCHITECTURE  "arm"
#elif (ARCH == ARCH_MIPS)
#define FICL_PLATFORM_ARCHITECTURE  "mips"
#elif (ARCH == ARCH_M68K)
#define FICL_PLATFORM_ARCHITECTURE  "coldfire"
#elif (ARCH == ARCH_SH)
#define FICL_PLATFORM_ARCHITECTURE  "sh"
#elif UINTPTR_MAX == 0xffffffffffffffff
#define FICL_PLATFORM_ARCHITECTURE  "x86-64"
#else
#define FICL_PLATFORM_ARCHITECTURE  "x86"
#endif 
