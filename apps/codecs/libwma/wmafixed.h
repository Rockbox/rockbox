/* 	fixed precision code.  We use a combination of Sign 15.16 and Sign.31
	precision here.

	The WMA decoder does not always follow this convention, and occasionally
	renormalizes values to other formats in order to maximize precision.
	However, only the two precisions above are provided in this file.

*/


#define PRECISION       16
#define PRECISION64     16


#define fixtof64(x)       (float)((float)(x) / (float)(1 << PRECISION64))        //does not work on int64_t!
#define ftofix32(x)       ((fixed32)((x) * (float)(1 << PRECISION) + ((x) < 0 ? -0.5 : 0.5)))
#define itofix64(x)       (IntTo64(x))
#define itofix32(x)       ((x) << PRECISION)
#define fixtoi32(x)       ((x) >> PRECISION)
#define fixtoi64(x)       (IntFrom64(x))


/*fixed functions*/

fixed64 IntTo64(int x);
int IntFrom64(fixed64 x);
fixed32 Fixed32From64(fixed64 x);
fixed64 Fixed32To64(fixed32 x);
fixed64 fixmul64byfixed(fixed64 x, fixed32 y);
fixed32 fixdiv32(fixed32 x, fixed32 y);
fixed64 fixdiv64(fixed64 x, fixed64 y);
fixed32 fixsqrt32(fixed32 x);
fixed32 fixsin32(fixed32 x);
fixed32 fixcos32(fixed32 x);
long fsincos(unsigned long phase, fixed32 *cos);





#ifdef CPU_ARM

/*
	Fixed precision multiply code ASM.

*/

/*Sign-15.16 format */

#define fixmul32(x, y)  \
    ({ int32_t __hi;  \
       uint32_t __lo;  \
       int32_t __result;  \
       asm ("smull   %0, %1, %3, %4\n\t"  \
            "movs    %0, %0, lsr %5\n\t"  \
            "adc    %2, %0, %1, lsl %6"  \
            : "=&r" (__lo), "=&r" (__hi), "=r" (__result)  \
            : "%r" (x), "r" (y),  \
              "M" (PRECISION), "M" (32 - PRECISION)  \
            : "cc");  \
       __result;  \
    })

/*
	Special fixmul32 that does a 16.16 x 1.31 multiply that returns a 16.16 value.
	this is needed because the fft constants are all normalized to be less then 1
	and can't fit into a 16 bit number without excessive rounding


*/


#  define fixmul32b(x, y)  \
    ({ int32_t __hi;  \
       uint32_t __lo;  \
       int32_t __result;  \
       asm ("smull    %0, %1, %3, %4\n\t"  \
        "movs    %0, %0, lsr %5\n\t"  \
        "adc    %2, %0, %1, lsl %6"  \
        : "=&r" (__lo), "=&r" (__hi), "=r" (__result)  \
        : "%r" (x), "r" (y),  \
          "M" (31), "M" (1)  \
        : "cc");  \
       __result;  \
    })


#else
fixed32 fixmul32(fixed32 x, fixed32 y);
fixed32 fixmul32b(fixed32 x, fixed32 y);
#endif

