#include <puzzles.h>
#include "fixedpoint.h"

int sprintf_wrapper(char *str, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int ret = rb->vsnprintf(str, 9999, fmt, ap);
    va_end(ap);
    return ret;
}

char *getenv_wrapper(const char *c)
{
    if(!strcmp(c, "SIGNPOST_GEARS"))
        return "y";
    return NULL;
}

int puts_wrapper(const char *s)
{
    LOGF("%s", s);
    return 0;
}

/* fixed-point wrappers */
static long lastphase = 0, lastsin = 0, lastcos = 0x7fffffff;

double sin_wrapper(double rads)
{
    /* we want [0, 2*PI) */
    while(rads >= 2*PI)
        rads -= 2*PI;
    while(rads < 0)
        rads += 2*PI;

    unsigned long phase = rads/(2*PI) * 4294967296.0;

    /* caching */
    if(phase == lastphase)
    {
        return lastsin/(lastsin < 0 ? 2147483648.0 : 2147483647.0);
    }

    lastphase = phase;
    lastsin = fp_sincos(phase, &lastcos);
    return lastsin/(lastsin < 0 ? 2147483648.0 : 2147483647.0);
}

double cos_wrapper(double rads)
{
    /* we want [0, 2*PI) */
    while(rads >= 2*PI)
        rads -= 2*PI;
    while(rads < 0)
        rads += 2*PI;

    unsigned long phase = rads/(2*PI) * 4294967296.0;

    /* caching */
    if(phase == lastphase)
    {
        return lastcos/(lastcos < 0 ? 2147483648.0 : 2147483647.0);
    }

    lastphase = phase;
    lastsin = fp_sincos(phase, &lastcos);
    return lastcos/(lastcos < 0 ? 2147483648.0 : 2147483647.0);
}

int vsprintf_wrapper(char *s, const char *fmt, va_list ap)
{
    return rb->vsnprintf(s, 9999, fmt, ap);
}

/* Absolute value, simple calculus */
float fabs_wrapper(float x)
{
    return (x < 0.0f) ? -x : x;
}

float floor_wrapper(float n)
{
    if(n < 0.0f)
        return ((int)n - 1);
    else
        return (int)n;
}

static float rb_log(float x)
{
    long x_f = x * 65536.0f;
    return fp16_log(x_f) / 65536.0;
}

/* A union which permits us to convert between a float and a 32 bit
   int.  */

typedef union
{
  float value;
  uint32_t word;
} ieee_float_shape_type;

/* Get a 32 bit int from a float.  */

#define GET_FLOAT_WORD(i,d)                                     \
do {                                                            \
  ieee_float_shape_type gf_u;                                   \
  gf_u.value = (d);                                             \
  (i) = gf_u.word;                                              \
} while (0)

/* Set a float from a 32 bit int.  */

#define SET_FLOAT_WORD(d,i)                                     \
do {                                                            \
  ieee_float_shape_type sf_u;                                   \
  sf_u.word = (i);                                              \
  (d) = sf_u.value;                                             \
} while (0)

#ifdef ROCKBOX_LITTLE_ENDIAN
#define __HI(x) *(1+(int*)&x)
#define __LO(x) *(int*)&x
#define __HIp(x) *(1+(int*)x)
#define __LOp(x) *(int*)x
#else
#define __HI(x) *(int*)&x
#define __LO(x) *(1+(int*)&x)
#define __HIp(x) *(int*)x
#define __LOp(x) *(1+(int*)x)
#endif

union ieee754_double
  {
    double d;

    /* This is the IEEE 754 double-precision format.  */
    struct
      {
#ifdef ROCKBOX_BIG_ENDIAN
        unsigned int negative:1;
        unsigned int exponent:11;
        /* Together these comprise the mantissa.  */
        unsigned int mantissa0:20;
        unsigned int mantissa1:32;
#else /* ROCKBOX_LITTLE_ENDIAN */
        /* Together these comprise the mantissa.  */
        unsigned int mantissa1:32;
        unsigned int mantissa0:20;
        unsigned int exponent:11;
        unsigned int negative:1;
#endif /* ROCKBOX_LITTLE_ENDIAN */
      } ieee;

    /* This format makes it easier to see if a NaN is a signalling NaN.  */
    struct
      {
#ifdef ROCKBOX_BIG_ENDIAN
        unsigned int negative:1;
        unsigned int exponent:11;
        unsigned int quiet_nan:1;
        /* Together these comprise the mantissa.  */
        unsigned int mantissa0:19;
        unsigned int mantissa1:32;
#else /* ROCKBOX_LITTLE_ENDIAN */
        /* Together these comprise the mantissa.  */
        unsigned int mantissa1:32;
        unsigned int mantissa0:19;
        unsigned int quiet_nan:1;
        unsigned int exponent:11;
        unsigned int negative:1;
#endif /* ROCKBOX_LITTLE_ENDIAN */
      } ieee_nan;
  };

static const volatile float TWOM100 = 7.88860905e-31;
static const volatile float TWO127 = 1.7014118346e+38;

float rb_exp(float x)
{
    long x_f = x * 65536.0f;
    return fp16_exp(x_f) / 65536.0f;
}

/* Arc tangent,
   taken from glibc-2.8. */

static const float atanhi[] = {
  4.6364760399e-01, /* atan(0.5)hi 0x3eed6338 */
  7.8539812565e-01, /* atan(1.0)hi 0x3f490fda */
  9.8279368877e-01, /* atan(1.5)hi 0x3f7b985e */
  1.5707962513e+00, /* atan(inf)hi 0x3fc90fda */
};

static const float atanlo[] = {
  5.0121582440e-09, /* atan(0.5)lo 0x31ac3769 */
  3.7748947079e-08, /* atan(1.0)lo 0x33222168 */
  3.4473217170e-08, /* atan(1.5)lo 0x33140fb4 */
  7.5497894159e-08, /* atan(inf)lo 0x33a22168 */
};

static const float aT[] = {
  3.3333334327e-01, /* 0x3eaaaaaa */
 -2.0000000298e-01, /* 0xbe4ccccd */
  1.4285714924e-01, /* 0x3e124925 */
 -1.1111110449e-01, /* 0xbde38e38 */
  9.0908870101e-02, /* 0x3dba2e6e */
 -7.6918758452e-02, /* 0xbd9d8795 */
  6.6610731184e-02, /* 0x3d886b35 */
 -5.8335702866e-02, /* 0xbd6ef16b */
  4.9768779427e-02, /* 0x3d4bda59 */
 -3.6531571299e-02, /* 0xbd15a221 */
  1.6285819933e-02, /* 0x3c8569d7 */
};

static const float
huge = 1.0e+30,
tiny = 1.0e-30,
one  = 1.0f;

float atan_wrapper(float x)
{
        float w,s1,s2,z;
        int32_t ix,hx,id;

        GET_FLOAT_WORD(hx,x);
        ix = hx&0x7fffffff;
        if(ix>=0x50800000) {    /* if |x| >= 2^34 */
            if(ix>0x7f800000)
                return x+x;             /* NaN */
            if(hx>0) return  atanhi[3]+atanlo[3];
            else     return -atanhi[3]-atanlo[3];
        } if (ix < 0x3ee00000) {        /* |x| < 0.4375 */
            if (ix < 0x31000000) {      /* |x| < 2^-29 */
                if(huge+x>one) return x;        /* raise inexact */
            }
            id = -1;
        } else {
        x = fabs_wrapper(x);
        if (ix < 0x3f980000) {          /* |x| < 1.1875 */
            if (ix < 0x3f300000) {      /* 7/16 <=|x|<11/16 */
                id = 0; x = ((float)2.0*x-one)/((float)2.0+x);
            } else {                    /* 11/16<=|x|< 19/16 */
                id = 1; x  = (x-one)/(x+one);
            }
        } else {
            if (ix < 0x401c0000) {      /* |x| < 2.4375 */
                id = 2; x  = (x-(float)1.5)/(one+(float)1.5*x);
            } else {                    /* 2.4375 <= |x| < 2^66 */
                id = 3; x  = -(float)1.0/x;
            }
        }}
    /* end of argument reduction */
        z = x*x;
        w = z*z;
    /* break sum from i=0 to 10 aT[i]z**(i+1) into odd and even poly */
        s1 = z*(aT[0]+w*(aT[2]+w*(aT[4]+w*(aT[6]+w*(aT[8]+w*aT[10])))));
        s2 = w*(aT[1]+w*(aT[3]+w*(aT[5]+w*(aT[7]+w*aT[9]))));
        if (id<0) return x - x*(s1+s2);
        else {
            z = atanhi[id] - ((x*(s1+s2) - atanlo[id]) - x);
            return (hx<0)? -z:z;
        }
}

/* Arc tangent from two variables, original. */

static const float
pi_o_4  = 7.8539818525e-01,  /* 0x3f490fdb */
pi_o_2  = 1.5707963705e+00,  /* 0x3fc90fdb */
pi      = 3.1415927410e+00,  /* 0x40490fdb */
pi_lo   = -8.7422776573e-08; /* 0xb3bbbd2e */

static const float zero = 0.0f;

float atan2_wrapper(float y, float x)
{
        float z;
        int32_t k,m,hx,hy,ix,iy;

        GET_FLOAT_WORD(hx,x);
        ix = hx&0x7fffffff;
        GET_FLOAT_WORD(hy,y);
        iy = hy&0x7fffffff;
        if((ix>0x7f800000)||
           (iy>0x7f800000))     /* x or y is NaN */
           return x+y;
        if(hx==0x3f800000) return atan_wrapper(y);   /* x=1.0 */
        m = ((hy>>31)&1)|((hx>>30)&2);  /* 2*sign(x)+sign(y) */

    /* when y = 0 */
        if(iy==0) {
            switch(m) {
                case 0:
                case 1: return y;       /* atan(+-0,+anything)=+-0 */
                case 2: return  pi+tiny;/* atan(+0,-anything) = pi */
                case 3: return -pi-tiny;/* atan(-0,-anything) =-pi */
            }
        }
    /* when x = 0 */
        if(ix==0) return (hy<0)?  -pi_o_2-tiny: pi_o_2+tiny;

    /* when x is INF */
        if(ix==0x7f800000) {
            if(iy==0x7f800000) {
                switch(m) {
                    case 0: return  pi_o_4+tiny;/* atan(+INF,+INF) */
                    case 1: return -pi_o_4-tiny;/* atan(-INF,+INF) */
                    case 2: return  (float)3.0*pi_o_4+tiny;/*atan(+INF,-INF)*/
                    case 3: return (float)-3.0*pi_o_4-tiny;/*atan(-INF,-INF)*/
                }
            } else {
                switch(m) {
                    case 0: return  zero  ;     /* atan(+...,+INF) */
                    case 1: return -zero  ;     /* atan(-...,+INF) */
                    case 2: return  pi+tiny  ;  /* atan(+...,-INF) */
                    case 3: return -pi-tiny  ;  /* atan(-...,-INF) */
                }
            }
        }
    /* when y is INF */
        if(iy==0x7f800000) return (hy<0)? -pi_o_2-tiny: pi_o_2+tiny;

    /* compute y/x */
        k = (iy-ix)>>23;
        if(k > 60) z=pi_o_2+(float)0.5*pi_lo;   /* |y/x| >  2**60 */
        else if(hx<0&&k<-60) z=0.0;     /* |y|/x < -2**60 */
        else z=atan_wrapper(fabs_wrapper(y/x));   /* safe to do y/x */
        switch (m) {
            case 0: return       z  ;   /* atan(+,+) */
            case 1: {
                      uint32_t zh;
                      GET_FLOAT_WORD(zh,z);
                      SET_FLOAT_WORD(z,zh ^ 0x80000000);
                    }
                    return       z  ;   /* atan(-,+) */
            case 2: return  pi-(z-pi_lo);/* atan(+,-) */
            default: /* case 3 */
                    return  (z-pi_lo)-pi;/* atan(-,-) */
        }
}

float sqrt_wrapper(float x)
{
    /* find inverse, Quake-style */
    float xhalf = .5f * x;
    int i = *(int*)&x;
    i = 0x5f3759df - (i >> 1);
    x = *(float*)&i;
    x = x * (1.5f - (xhalf * x * x));

    return 1.0f / x;
}

/* hack, simple trig */
double acos_wrapper(double x)
{
    return atan2_wrapper(sqrt_wrapper(1.0 - x * x), x);
}

/*
 * Copyright (C) 2004 by egnite Software GmbH. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY EGNITE SOFTWARE GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL EGNITE
 * SOFTWARE GMBH OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * For additional information see http://www.ethernut.de/
 *
 *-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * $Log$
 * Revision 1.4  2008/02/15 17:13:01  haraldkipp
 * Use configurable constant attribute.
 *
 * Revision 1.3  2006/10/08 16:48:08  haraldkipp
 * Documentation fixed
 *
 * Revision 1.2  2005/08/02 17:46:47  haraldkipp
 * Major API documentation update.
 *
 * Revision 1.1  2004/09/08 10:23:35  haraldkipp
 * Generic C stdlib added
 *
 */

#define CONST const
long strtol_wrapper(CONST char *nptr, char **endptr, int base)
{
    register CONST char *s;
    register long acc, cutoff;
    register int c;
    register int neg, any, cutlim;

    /*
     * Skip white space and pick up leading +/- sign if any.
     * If base is 0, allow 0x for hex and 0 for octal, else
     * assume decimal; if base is already 16, allow 0x.
     */
    s = nptr;
    do {
        c = (unsigned char) *s++;
    } while (isspace(c));
    if (c == '-') {
        neg = 1;
        c = *s++;
    } else {
        neg = 0;
        if (c == '+')
            c = *s++;
    }
    if ((base == 0 || base == 16) && c == '0' && (*s == 'x' || *s == 'X')) {
        c = s[1];
        s += 2;
        base = 16;
    }
    if (base == 0)
        base = c == '0' ? 8 : 10;

    /*
     * Compute the cutoff value between legal numbers and illegal
     * numbers.  That is the largest legal value, divided by the
     * base.  An input number that is greater than this value, if
     * followed by a legal input character, is too big.  One that
     * is equal to this value may be valid or not; the limit
     * between valid and invalid numbers is then based on the last
     * digit.  For instance, if the range for longs is
     * [-2147483648..2147483647] and the input base is 10,
     * cutoff will be set to 214748364 and cutlim to either
     * 7 (neg==0) or 8 (neg==1), meaning that if we have accumulated
     * a value > 214748364, or equal but the next digit is > 7 (or 8),
     * the number is too big, and we will return a range error.
     *
     * Set any if any `digits' consumed; make it negative to indicate
     * overflow.
     */
    cutoff = neg ? LONG_MIN : LONG_MAX;
    cutlim = cutoff % base;
    cutoff /= base;
    if (neg) {
        if (cutlim > 0) {
            cutlim -= base;
            cutoff += 1;
        }
        cutlim = -cutlim;
    }
    for (acc = 0, any = 0;; c = (unsigned char) *s++) {
        if (isdigit(c))
            c -= '0';
        else if (isalpha(c))
            c -= isupper(c) ? 'A' - 10 : 'a' - 10;
        else
            break;
        if (c >= base)
            break;
        if (any < 0)
            continue;
        if (neg) {
            if ((acc < cutoff || acc == cutoff) && c > cutlim) {
                any = -1;
                acc = LONG_MIN;
            } else {
                any = 1;
                acc *= base;
                acc -= c;
            }
        } else {
            if ((acc > cutoff || acc == cutoff) && c > cutlim) {
                any = -1;
                acc = LONG_MAX;
            } else {
                any = 1;
                acc *= base;
                acc += c;
            }
        }
    }
    if (endptr != 0)
        *endptr = (char *) (any ? s - 1 : nptr);
    return (acc);
}

int64_t strtoq_wrapper(CONST char *nptr, char **endptr, int base)
{
    return strtol(nptr, endptr, base);
}

uint64_t strtouq_wrapper(CONST char *nptr, char **endptr, int base)
{
    return strtol(nptr, endptr, base);
}

/* Power function, taken from glibc-2.8 and dietlibc-0.32 */
float pow_wrapper(float x, float y)
{
    unsigned int e;
    float result;

    /* Special cases 0^x */
    if(x == 0.0f)
    {
        if(y > 0.0f)
            return 0.0f;
        else if(y == 0.0f)
            return 1.0f;
        else
            return 1.0f / x;
    }

    /* Special case x^n where n is integer */
    if(y == (int) (e = (int) y))
    {
        if((int) e < 0)
        {
            e = -e;
            x = 1.0f / x;
        }

        result = 1.0f;

        while(1)
        {
            if(e & 1)
                result *= x;

            if((e >>= 1) == 0)
                break;

            x *= x;
        }

        return result;
    }

    /* Normal case */
    return rb_exp(rb_log(x) * y);
}

/* @(#)s_copysign.c 1.3 95/01/18 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunSoft, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

/*
 * copysign(double x, double y)
 * copysign(x,y) returns a value with the magnitude of x and
 * with the sign bit of y.
 */

double copysign_wrapper(double x, double y)
{
        __HI(x) = (__HI(x)&0x7fffffff)|(__HI(y)&0x80000000);
        return x;
}

/* @(#)s_scalbn.c 1.3 95/01/18 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunSoft, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

/*
 * scalbn (double x, int n)
 * scalbn(x,n) returns x* 2**n  computed by  exponent
 * manipulation rather than by actually performing an
 * exponentiation or a multiplication.
 */

#ifdef __STDC__
static const double
#else
static double
#endif
two54   =  1.80143985094819840000e+16, /* 0x43500000, 0x00000000 */
    twom54  =  5.55111512312578270212e-17; /* 0x3C900000, 0x00000000 */

double scalbn_wrapper (double x, int n)
{
        int  k,hx,lx;
        hx = __HI(x);
        lx = __LO(x);
        k = (hx&0x7ff00000)>>20;                /* extract exponent */
        if (k==0) {                             /* 0 or subnormal x */
            if ((lx|(hx&0x7fffffff))==0) return x; /* +-0 */
            x *= two54;
            hx = __HI(x);
            k = ((hx&0x7ff00000)>>20) - 54;
            if (n< -50000) return tiny*x;       /*underflow*/
            }
        if (k==0x7ff) return x+x;               /* NaN or Inf */
        k = k+n;
        if (k >  0x7fe) return huge*copysign_wrapper(huge,x); /* overflow  */
        if (k > 0)                              /* normal result */
            {__HI(x) = (hx&0x800fffff)|(k<<20); return x;}
        if (k <= -54)
        {
            if (n > 50000)      /* in case integer overflow in n+k */
                return huge*copysign_wrapper(huge,x);   /*overflow*/
            else
                return tiny*copysign_wrapper(tiny,x);  /*underflow*/
        }
        k += 54;                                /* subnormal result */
        __HI(x) = (hx&0x800fffff)|(k<<20);
        return x*twom54;
}

/* horrible hack */
float ceil_wrapper(float x)
{
    return floor_wrapper(x) + 1.0;
}

/* Implementation of strtod() and atof(),
   taken from SanOS (http://www.jbox.dk/sanos/). */
static int rb_errno = 0;

static double rb_strtod(const char *str, char **endptr)
{
  double number;
  int exponent;
  int negative;
  char *p = (char *) str;
  double p10;
  int n;
  int num_digits;
  int num_decimals;

    /* Reset Rockbox errno -- W.B. */
#ifdef ROCKBOX
    rb_errno = 0;
#endif

  // Skip leading whitespace
  while (isspace(*p)) p++;

  // Handle optional sign
  negative = 0;
  switch (*p)
  {
    case '-': negative = 1; // Fall through to increment position
    case '+': p++;
  }

  number = 0.;
  exponent = 0;
  num_digits = 0;
  num_decimals = 0;

  // Process string of digits
  while (isdigit(*p))
  {
    number = number * 10. + (*p - '0');
    p++;
    num_digits++;
  }

  // Process decimal part
  if (*p == '.')
  {
    p++;

    while (isdigit(*p))
    {
      number = number * 10. + (*p - '0');
      p++;
      num_digits++;
      num_decimals++;
    }

    exponent -= num_decimals;
  }

  if (num_digits == 0)
  {
#ifdef ROCKBOX
    rb_errno = 1;
#else
    errno = ERANGE;
#endif
    return 0.0;
  }

  // Correct for sign
  if (negative) number = -number;

  // Process an exponent string
  if (*p == 'e' || *p == 'E')
  {
    // Handle optional sign
    negative = 0;
    switch(*++p)
    {
      case '-': negative = 1;   // Fall through to increment pos
      case '+': p++;
    }

    // Process string of digits
    n = 0;
    while (isdigit(*p))
    {
      n = n * 10 + (*p - '0');
      p++;
    }

    if (negative)
      exponent -= n;
    else
      exponent += n;
  }

#ifndef ROCKBOX
  if (exponent < DBL_MIN_EXP  || exponent > DBL_MAX_EXP)
  {
    errno = ERANGE;
    return HUGE_VAL;
  }
#endif

  // Scale the result
  p10 = 10.;
  n = exponent;
  if (n < 0) n = -n;
  while (n)
  {
    if (n & 1)
    {
      if (exponent < 0)
        number /= p10;
      else
        number *= p10;
    }
    n >>= 1;
    p10 *= p10;
  }

#ifndef ROCKBOX
  if (number == HUGE_VAL) errno = ERANGE;
#endif
  if (endptr) *endptr = p;

  return number;
}

double atof_wrapper(const char *str)
{
    return rb_strtod(str, NULL);
}

/*
 * Copyright (c) 2004 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*-
 * Copyright (c) 1990, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "rbcompat.h"

#define BUF             32      /* Maximum length of numeric string. */

/*
 * Flags used during conversion.
 */
#define LONG            0x01    /* l: long or double */
#define SHORT           0x04    /* h: short */
#define SUPPRESS        0x08    /* *: suppress assignment */
#define POINTER         0x10    /* p: void * (as hex) */
#define NOSKIP          0x20    /* [ or c: do not skip blanks */
#define LONGLONG        0x400   /* ll: long long (+ deprecated q: quad) */
#define SHORTSHORT      0x4000  /* hh: char */
#define UNSIGNED        0x8000  /* %[oupxX] conversions */

/*
 * The following are used in numeric conversions only:
 * SIGNOK, NDIGITS, DPTOK, and EXPOK are for floating point;
 * SIGNOK, NDIGITS, PFXOK, and NZDIGITS are for integral.
 */
#define SIGNOK          0x40    /* +/- is (still) legal */
#define NDIGITS         0x80    /* no digits detected */

#define DPTOK           0x100   /* (float) decimal point is still legal */
#define EXPOK           0x200   /* (float) exponent (e+3, etc) still legal */

#define PFXOK           0x100   /* 0x prefix is (still) legal */
#define NZDIGITS        0x200   /* no zero digits detected */

/*
 * Conversion types.
 */
#define CT_CHAR         0       /* %c conversion */
#define CT_CCL          1       /* %[...] conversion */
#define CT_STRING       2       /* %s conversion */
#define CT_INT          3       /* %[dioupxX] conversion */

typedef unsigned char u_char;
typedef uint64_t      u_quad_t;

static const u_char *__sccl(char *, const u_char *);

static void bcopy_wrapper(const void *src, void *dst, size_t n)
{
    memmove(dst, src, n);
}

int
rb_vsscanf(const char *inp, char const *fmt0, va_list ap)
{
        int inr;
        const u_char *fmt = (const u_char *)fmt0;
        int c;                  /* character from format, or conversion */
        size_t width;           /* field width, or 0 */
        char *p;                /* points into all kinds of strings */
        int n;                  /* handy integer */
        int flags;              /* flags as defined above */
        char *p0;               /* saves original value of p when necessary */
        int nassigned;          /* number of fields assigned */
        int nconversions;       /* number of conversions */
        int nread;              /* number of characters consumed from fp */
        int base;               /* base argument to conversion function */
        char ccltab[256];       /* character class table for %[...] */
        char buf[BUF];          /* buffer for numeric conversions */

        /* `basefix' is used to avoid `if' tests in the integer scanner */
        static short basefix[17] =
                { 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };

        inr = strlen(inp);

        nassigned = 0;
        nconversions = 0;
        nread = 0;
        base = 0;               /* XXX just to keep gcc happy */
        for (;;) {
                c = *fmt++;
                if (c == 0)
                        return (nassigned);
                if (isspace(c)) {
                        while (inr > 0 && isspace(*inp))
                                nread++, inr--, inp++;
                        continue;
                }
                if (c != '%')
                        goto literal;
                width = 0;
                flags = 0;
                /*
                 * switch on the format.  continue if done;
                 * break once format type is derived.
                 */
again:          c = *fmt++;
                switch (c) {
                case '%':
literal:
                        if (inr <= 0)
                                goto input_failure;
                        if (*inp != c)
                                goto match_failure;
                        inr--, inp++;
                        nread++;
                        continue;

                case '*':
                        flags |= SUPPRESS;
                        goto again;
                case 'l':
                        if (flags & LONG) {
                                flags &= ~LONG;
                                flags |= LONGLONG;
                        } else
                                flags |= LONG;
                        goto again;
                case 'q':
                        flags |= LONGLONG;      /* not quite */
                        goto again;
                case 'h':
                        if (flags & SHORT) {
                                flags &= ~SHORT;
                                flags |= SHORTSHORT;
                        } else
                                flags |= SHORT;
                        goto again;

                case '0': case '1': case '2': case '3': case '4':
                case '5': case '6': case '7': case '8': case '9':
                        width = width * 10 + c - '0';
                        goto again;

                /*
                 * Conversions.
                 */
                case 'd':
                        c = CT_INT;
                        base = 10;
                        break;

                case 'i':
                        c = CT_INT;
                        base = 0;
                        break;

                case 'o':
                        c = CT_INT;
                        flags |= UNSIGNED;
                        base = 8;
                        break;

                case 'u':
                        c = CT_INT;
                        flags |= UNSIGNED;
                        base = 10;
                        break;

                case 'X':
                case 'x':
                        flags |= PFXOK; /* enable 0x prefixing */
                        c = CT_INT;
                        flags |= UNSIGNED;
                        base = 16;
                        break;

                case 's':
                        c = CT_STRING;
                        break;

                case '[':
                        fmt = __sccl(ccltab, fmt);
                        flags |= NOSKIP;
                        c = CT_CCL;
                        break;

                case 'c':
                        flags |= NOSKIP;
                        c = CT_CHAR;
                        break;

                case 'p':       /* pointer format is like hex */
                        flags |= POINTER | PFXOK;
                        c = CT_INT;
                        flags |= UNSIGNED;
                        base = 16;
                        break;

                case 'n':
                        nconversions++;
                        if (flags & SUPPRESS)   /* ??? */
                                continue;
                        if (flags & SHORTSHORT)
                                *va_arg(ap, char *) = nread;
                        else if (flags & SHORT)
                                *va_arg(ap, short *) = nread;
                        else if (flags & LONG)
                                *va_arg(ap, long *) = nread;
                        else if (flags & LONGLONG)
                                *va_arg(ap, long long *) = nread;
                        else
                                *va_arg(ap, int *) = nread;
                        continue;
                }

                /*
                 * We have a conversion that requires input.
                 */
                if (inr <= 0)
                        goto input_failure;

                /*
                 * Consume leading white space, except for formats
                 * that suppress this.
                 */
                if ((flags & NOSKIP) == 0) {
                        while (isspace(*inp)) {
                                nread++;
                                if (--inr > 0)
                                        inp++;
                                else
                                        goto input_failure;
                        }
                        /*
                         * Note that there is at least one character in
                         * the buffer, so conversions that do not set NOSKIP
                         * can no longer result in an input failure.
                         */
                }

                /*
                 * Do the conversion.
                 */
                switch (c) {

                case CT_CHAR:
                        /* scan arbitrary characters (sets NOSKIP) */
                        if (width == 0)
                                width = 1;
                        if (flags & SUPPRESS) {
                                size_t sum = 0;
                                for (;;) {
                                        if ((n = inr) < (int)width) {
                                                sum += n;
                                                width -= n;
                                                inp += n;
                                                if (sum == 0)
                                                        goto input_failure;
                                                break;
                                        } else {
                                                sum += width;
                                                inr -= width;
                                                inp += width;
                                                break;
                                        }
                                }
                                nread += sum;
                        } else {
                                bcopy_wrapper(inp, va_arg(ap, char *), width);
                                inr -= width;
                                inp += width;
                                nread += width;
                                nassigned++;
                        }
                        nconversions++;
                        break;

                case CT_CCL:
                        /* scan a (nonempty) character class (sets NOSKIP) */
                        if (width == 0)
                                width = (size_t)~0;     /* `infinity' */
                        /* take only those things in the class */
                        if (flags & SUPPRESS) {
                                n = 0;
                                while (ccltab[(unsigned char)*inp]) {
                                        n++, inr--, inp++;
                                        if (--width == 0)
                                                break;
                                        if (inr <= 0) {
                                                if (n == 0)
                                                        goto input_failure;
                                                break;
                                        }
                                }
                                if (n == 0)
                                        goto match_failure;
                        } else {
                                p0 = p = va_arg(ap, char *);
                                while (ccltab[(unsigned char)*inp]) {
                                        inr--;
                                        *p++ = *inp++;
                                        if (--width == 0)
                                                break;
                                        if (inr <= 0) {
                                                if (p == p0)
                                                        goto input_failure;
                                                break;
                                        }
                                }
                                n = p - p0;
                                if (n == 0)
                                        goto match_failure;
                                *p = 0;
                                nassigned++;
                        }
                        nread += n;
                        nconversions++;
                        break;

                case CT_STRING:
                        /* like CCL, but zero-length string OK, & no NOSKIP */
                        if (width == 0)
                                width = (size_t)~0;
                        if (flags & SUPPRESS) {
                                n = 0;
                                while (!isspace(*inp)) {
                                        n++, inr--, inp++;
                                        if (--width == 0)
                                                break;
                                        if (inr <= 0)
                                                break;
                                }
                                nread += n;
                        } else {
                                p0 = p = va_arg(ap, char *);
                                while (!isspace(*inp)) {
                                        inr--;
                                        *p++ = *inp++;
                                        if (--width == 0)
                                                break;
                                        if (inr <= 0)
                                                break;
                                }
                                *p = 0;
                                nread += p - p0;
                                nassigned++;
                        }
                        nconversions++;
                        continue;

                case CT_INT:
                        /* scan an integer as if by the conversion function */
#ifdef hardway
                        if (width == 0 || width > sizeof(buf) - 1)
                                width = sizeof(buf) - 1;
#else
                        /* size_t is unsigned, hence this optimisation */
                        if (--width > sizeof(buf) - 2)
                                width = sizeof(buf) - 2;
                        width++;
#endif
                        flags |= SIGNOK | NDIGITS | NZDIGITS;
                        for (p = buf; width; width--) {
                                c = *inp;
                                /*
                                 * Switch on the character; `goto ok'
                                 * if we accept it as a part of number.
                                 */
                                switch (c) {

                                /*
                                 * The digit 0 is always legal, but is
                                 * special.  For %i conversions, if no
                                 * digits (zero or nonzero) have been
                                 * scanned (only signs), we will have
                                 * base==0.  In that case, we should set
                                 * it to 8 and enable 0x prefixing.
                                 * Also, if we have not scanned zero digits
                                 * before this, do not turn off prefixing
                                 * (someone else will turn it off if we
                                 * have scanned any nonzero digits).
                                 */
                                case '0':
                                        if (base == 0) {
                                                base = 8;
                                                flags |= PFXOK;
                                        }
                                        if (flags & NZDIGITS)
                                            flags &= ~(SIGNOK|NZDIGITS|NDIGITS);
                                        else
                                            flags &= ~(SIGNOK|PFXOK|NDIGITS);
                                        goto ok;

                                /* 1 through 7 always legal */
                                case '1': case '2': case '3':
                                case '4': case '5': case '6': case '7':
                                        base = basefix[base];
                                        flags &= ~(SIGNOK | PFXOK | NDIGITS);
                                        goto ok;

                                /* digits 8 and 9 ok iff decimal or hex */
                                case '8': case '9':
                                        base = basefix[base];
                                        if (base <= 8)
                                                break;  /* not legal here */
                                        flags &= ~(SIGNOK | PFXOK | NDIGITS);
                                        goto ok;

                                /* letters ok iff hex */
                                case 'A': case 'B': case 'C':
                                case 'D': case 'E': case 'F':
                                case 'a': case 'b': case 'c':
                                case 'd': case 'e': case 'f':
                                        /* no need to fix base here */
                                        if (base <= 10)
                                                break;  /* not legal here */
                                        flags &= ~(SIGNOK | PFXOK | NDIGITS);
                                        goto ok;

                                /* sign ok only as first character */
                                case '+': case '-':
                                        if (flags & SIGNOK) {
                                                flags &= ~SIGNOK;
                                                goto ok;
                                        }
                                        break;

                                /* x ok iff flag still set & 2nd char */
                                case 'x': case 'X':
                                        if (flags & PFXOK && p == buf + 1) {
                                                base = 16;      /* if %i */
                                                flags &= ~PFXOK;
                                                goto ok;
                                        }
                                        break;
                                }

                                /*
                                 * If we got here, c is not a legal character
                                 * for a number.  Stop accumulating digits.
                                 */
                                break;
                ok:
                                /*
                                 * c is legal: store it and look at the next.
                                 */
                                *p++ = c;
                                if (--inr > 0)
                                        inp++;
                                else
                                        break;          /* end of input */
                        }
                        /*
                         * If we had only a sign, it is no good; push
                         * back the sign.  If the number ends in `x',
                         * it was [sign] '0' 'x', so push back the x
                         * and treat it as [sign] '0'.
                         */
                        if (flags & NDIGITS) {
                                if (p > buf) {
                                        inp--;
                                        inr++;
                                }
                                goto match_failure;
                        }
                        c = ((u_char *)p)[-1];
                        if (c == 'x' || c == 'X') {
                                --p;
                                inp--;
                                inr++;
                        }
                        if ((flags & SUPPRESS) == 0) {
                                u_quad_t res;

                                *p = 0;
                                if ((flags & UNSIGNED) == 0)
                                    res = strtoq(buf, (char **)NULL, base);
                                else
                                    res = strtouq(buf, (char **)NULL, base);
                                if (flags & POINTER)
                                        *va_arg(ap, void **) =
                                                (void *)(uintptr_t)res;
                                else if (flags & SHORTSHORT)
                                        *va_arg(ap, char *) = res;
                                else if (flags & SHORT)
                                        *va_arg(ap, short *) = res;
                                else if (flags & LONG)
                                        *va_arg(ap, long *) = res;
                                else if (flags & LONGLONG)
                                        *va_arg(ap, long long *) = res;
                                else
                                        *va_arg(ap, int *) = res;
                                nassigned++;
                        }
                        nread += p - buf;
                        nconversions++;
                        break;

                }
        }
input_failure:
        return (nconversions != 0 ? nassigned : -1);
match_failure:
        return (nassigned);
}

int
sscanf_wrapper(const char *ibuf, const char *fmt, ...)
{
        va_list ap;
        int ret;

        va_start(ap, fmt);
        ret = rb_vsscanf(ibuf, fmt, ap);
        va_end(ap);
        return(ret);
}

/*
 * Fill in the given table from the scanset at the given format
 * (just after `[').  Return a pointer to the character past the
 * closing `]'.  The table has a 1 wherever characters should be
 * considered part of the scanset.
 */
static const u_char *
__sccl(char *tab, const u_char *fmt)
{
        int c, n, v;

        /* first `clear' the whole table */
        c = *fmt++;             /* first char hat => negated scanset */
        if (c == '^') {
                v = 1;          /* default => accept */
                c = *fmt++;     /* get new first char */
        } else
                v = 0;          /* default => reject */

        /* XXX: Will not work if sizeof(tab*) > sizeof(char) */
        (void) memset(tab, v, 256);

        if (c == 0)
                return (fmt - 1);/* format ended before closing ] */

        /*
         * Now set the entries corresponding to the actual scanset
         * to the opposite of the above.
         *
         * The first character may be ']' (or '-') without being special;
         * the last character may be '-'.
         */
        v = 1 - v;
        for (;;) {
                tab[c] = v;             /* take character c */
doswitch:
                n = *fmt++;             /* and examine the next */
                switch (n) {

                case 0:                 /* format ended too soon */
                        return (fmt - 1);

                case '-':
                        /*
                         * A scanset of the form
                         *      [01+-]
                         * is defined as `the digit 0, the digit 1,
                         * the character +, the character -', but
                         * the effect of a scanset such as
                         *      [a-zA-Z0-9]
                         * is implementation defined.  The V7 Unix
                         * scanf treats `a-z' as `the letters a through
                         * z', but treats `a-a' as `the letter a, the
                         * character -, and the letter a'.
                         *
                         * For compatibility, the `-' is not considerd
                         * to define a range if the character following
                         * it is either a close bracket (required by ANSI)
                         * or is not numerically greater than the character
                         * we just stored in the table (c).
                         */
                        n = *fmt;
                        if (n == ']' || n < c) {
                                c = '-';
                                break;  /* resume the for(;;) */
                        }
                        fmt++;
                        /* fill in the range */
                        do {
                            tab[++c] = v;
                        } while (c < n);
                        c = n;
                        /*
                         * Alas, the V7 Unix scanf also treats formats
                         * such as [a-c-e] as `the letters a through e'.
                         * This too is permitted by the standard....
                         */
                        goto doswitch;
                        break;

                case ']':               /* end of scanset */
                        return (fmt);

                default:                /* just another character */
                        c = n;
                        break;
                }
        }
        /* NOTREACHED */
}

/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * The contents of this file constitute Original Code as defined in and
 * are subject to the Apple Public Source License Version 1.1 (the
 * "License").  You may not use this file except in compliance with the
 * License.  Please obtain a copy of the License at
 * http://www.apple.com/publicsource and read it before using this file.
 *
 * This Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 * Copyright (c) 1990, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "plugin.h"

/*
 * Span the complement of string s2.
 */
size_t
strcspn_wrapper(const char *s1, const char *s2)
{
        register const char *p, *spanp;
        register char c, sc;

        /*
         * Stop as soon as we find any character from s2.  Note that there
         * must be a NUL in s2; it suffices to stop when we find that, too.
         */
        for (p = s1;;) {
                c = *p++;
                spanp = s2;
                do {
                        if ((sc = *spanp++) == c)
                                return (p - 1 - s1);
                } while (sc != 0);
        }
        /* NOTREACHED */
}

/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * The contents of this file constitute Original Code as defined in and
 * are subject to the Apple Public Source License Version 1.1 (the
 * "License").  You may not use this file except in compliance with the
 * License.  Please obtain a copy of the License at
 * http://www.apple.com/publicsource and read it before using this file.
 *
 * This Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 * Copyright (c) 1989, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "rbcompat.h"

/*
 * Span the string s2 (skip characters that are in s2).
 */
size_t
strspn_wrapper(const char *s1, const char *s2)
{
        register const char *p = s1, *spanp;
        register char c, sc;

        /*
         * Skip any characters in s2, excluding the terminating \0.
         */
cont:
        c = *p++;
        for (spanp = s2; (sc = *spanp++) != 0;)
                if (sc == c)
                        goto cont;
        return (p - 1 - s1);
}
