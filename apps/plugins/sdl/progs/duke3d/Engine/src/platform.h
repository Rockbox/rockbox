#ifndef _INCLUDE_PLATFORM_DUKE_H_
#define _INCLUDE_PLATFORM_DUKE_H_

#if (defined ROCKBOX)
    #include "rockbox_compat.h"
#else
#error Define your platform!
#endif

#if (!defined __EXPORT__)
    #define __EXPORT__
#endif

uint16_t _swap16(uint16_t D);
unsigned int _swap32(unsigned int D);
#define PLATFORM_LITTLEENDIAN 1
#define BUILDSWAP_INTEL16(x) (x)
#define BUILDSWAP_INTEL32(x) (x)

#endif  /* !defined _INCLUDE_PLATFORM_H_ */

/* end of platform.h ... */


