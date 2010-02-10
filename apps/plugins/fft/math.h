#ifndef __MATH_H_
#define __MATH_H_

#include <inttypes.h>
#include <math.h>
#include "lib/fixedpoint.h"

#define Q_MUL(a, b, bits) (( (int64_t) (a) * (int64_t) (b) ) >> (bits))
#define Q15_MUL(a, b) Q_MUL(a,b,15)
#define Q16_MUL(a, b) Q_MUL(a,b,16)

#define Q_DIV(a, b, bits) ( (((int64_t) (a)) << (bits)) / (b) )
#define Q15_DIV(a, b) Q_DIV(a,b,15)
#define Q16_DIV(a, b) Q_DIV(a,b,16)

#define float_q(a, bits) (int32_t)( ((float)(a)) *(1<<(bits)))
#define float_q15(a) float_q(a, 15)
#define float_q16(a) float_q(a, 16)

/**
 * Fixed point square root via Newton-Raphson.
 * @param a square root argument.
 * @param fracbits specifies number of fractional bits in argument.
 * @return Square root of argument in same fixed point format as input.
 */
int64_t fsqrt64(int64_t a, unsigned int fracbits);

#endif
