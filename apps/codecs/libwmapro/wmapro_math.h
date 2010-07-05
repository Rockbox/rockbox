#include <inttypes.h>
#include "types.h"

#define fixtof16(x)       (float)((float)(x) / (float)(1 << 16))
#define ftofix16(x)       ((int32_t)((x) * (float)(1 << 16) + ((x) < 0 ? -0.5:0.5)))

static inline FIXED fixmulshift(FIXED x, FIXED y, int shamt)
{
    int64_t temp;
    temp = x;
    temp *= y;

    temp >>= shamt;

    return (int32_t)temp;
}


static inline void vector_fixmul_window(FIXED *dst, const FIXED *src0, 
                                   const FIXED *src1, const FIXED *win, 
                                   FIXED add_bias, int len)
{
    int i, j;
    dst += len;
    win += len;
    src0+= len;
        for(i=-len, j=len-1; i<0; i++, j--) {
        FIXED s0 = src0[i];
        FIXED s1 = src1[j];
        FIXED wi = win[i];
        FIXED wj = win[j];
        dst[i] = fixmulshift(s0,-1*wj,31) - fixmulshift(s1,-1*wi,31) + (add_bias<<16);
        dst[j] = fixmulshift(s0,-1*wi,31) + fixmulshift(s1,-1*wj,31) + (add_bias<<16);
    }   
    
}

static inline void vector_fixmul_scalar(FIXED *dst, const FIXED *src, FIXED mul,
                                        int len)
{
    int i;
    for(i=0; i<len; i++) {
        dst[i] = fixmulshift(src[i],mul,32);
    }   
    
}
