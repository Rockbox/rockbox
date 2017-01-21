#ifndef _INCLUDE_PLATFORM_DUKE_H_
#define _INCLUDE_PLATFORM_DUKE_H_

#if (defined PLATFORM_WIN32)
    #include "win32_compat.h"
#elif (defined PLATFORM_UNIX)
    #include "unix_compat.h"
#elif (defined PLATFORM_DOS)
    #include "doscmpat.h"
#elif (defined __APPLE__)
    #include "macos_compat.h"
#elif (defined ROCKBOX)
    #include "rockbox_compat.h"
#else
#error Define your platform!
#endif

#if (!defined __EXPORT__)
    #define __EXPORT__
#endif

uint16_t _swap16(uint16_t D);
unsigned int _swap32(unsigned int D);
#if defined(PLATFORM_MACOSX) && defined(__ppc__)
#define PLATFORM_BIGENDIAN 1
#define BUILDSWAP_INTEL16(x) _swap16(x)
#define BUILDSWAP_INTEL32(x) _swap32(x)
#else
#define PLATFORM_LITTLEENDIAN 1
#define BUILDSWAP_INTEL16(x) (x)
#define BUILDSWAP_INTEL32(x) (x)
#endif

#endif  /* !defined _INCLUDE_PLATFORM_H_ */

/* end of platform.h ... */


