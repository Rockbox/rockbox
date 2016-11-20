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
    return NULL;
}

int puts_wrapper(const char *s)
{
    LOGF("%s", s);
    return 0;
}

double sin_wrapper(double rads)
{
    int degs = rads * 180/PI;
    long fixed = fp14_sin(degs);
    return fixed / (16384.0);
}

double cos_wrapper(double rads)
{
    int degs = rads * 180/PI;
    long fixed = fp14_cos(degs);
    return fixed / (16384.0);
}

int vsprintf_wrapper(char *s, const char *fmt, va_list ap)
{
    return rb->vsnprintf(s, 9999, fmt, ap);
}

double fabs_wrapper(double n)
{
    if(n < 0)
        return -n;
    else
        return n;
}

double floor_wrapper(double n)
{
    if(n < 0)
        return ((int)n) - 1;
    else
        return (int)n;
}

double atan_wrapper(double x);

/* @(#)e_atan2.c 1.3 95/01/18 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunSoft, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 *
 */

/* __ieee754_atan2(y,x)
 * Method :
 *      1. Reduce y to positive by atan2(y,x)=-atan2(-y,x).
 *      2. Reduce x to positive by (if x and y are unexceptional):
 *              ARG (x+iy) = arctan(y/x)           ... if x > 0,
 *              ARG (x+iy) = pi - arctan[y/(-x)]   ... if x < 0,
 *
 * Special cases:
 *
 *      ATAN2((anything), NaN ) is NaN;
 *      ATAN2(NAN , (anything) ) is NaN;
 *      ATAN2(+-0, +(anything but NaN)) is +-0  ;
 *      ATAN2(+-0, -(anything but NaN)) is +-pi ;
 *      ATAN2(+-(anything but 0 and NaN), 0) is +-pi/2;
 *      ATAN2(+-(anything but INF and NaN), +INF) is +-0 ;
 *      ATAN2(+-(anything but INF and NaN), -INF) is +-pi;
 *      ATAN2(+-INF,+INF ) is +-pi/4 ;
 *      ATAN2(+-INF,-INF ) is +-3pi/4;
 *      ATAN2(+-INF, (anything but,0,NaN, and INF)) is +-pi/2;
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following
 * constants. The decimal values may be used, provided that the
 * compiler will convert from decimal to binary accurately enough
 * to produce the hexadecimal values shown.
 */

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

#ifdef __STDC__
static const double
#else
static double
#endif
tiny  = 1.0e-300,
zero  = 0.0,
pi_o_4  = 7.8539816339744827900E-01, /* 0x3FE921FB, 0x54442D18 */
pi_o_2  = 1.5707963267948965580E+00, /* 0x3FF921FB, 0x54442D18 */
pi      = 3.1415926535897931160E+00, /* 0x400921FB, 0x54442D18 */
pi_lo   = 1.2246467991473531772E-16; /* 0x3CA1A626, 0x33145C07 */

double atan2_wrapper(double y, double x)
{
        double z;
        int k,m,hx,hy,ix,iy;
        unsigned lx,ly;

        hx = __HI(x); ix = hx&0x7fffffff;
        lx = __LO(x);
        hy = __HI(y); iy = hy&0x7fffffff;
        ly = __LO(y);
        if(((ix|((lx|-lx)>>31))>0x7ff00000)||
           ((iy|((ly|-ly)>>31))>0x7ff00000))    /* x or y is NaN */
           return x+y;
        if((hx-0x3ff00000|lx)==0) return atan_wrapper(y);   /* x=1.0 */
        m = ((hy>>31)&1)|((hx>>30)&2);  /* 2*sign(x)+sign(y) */

    /* when y = 0 */
        if((iy|ly)==0) {
            switch(m) {
                case 0:
                case 1: return y;       /* atan(+-0,+anything)=+-0 */
                case 2: return  pi+tiny;/* atan(+0,-anything) = pi */
                case 3: return -pi-tiny;/* atan(-0,-anything) =-pi */
            }
        }
    /* when x = 0 */
        if((ix|lx)==0) return (hy<0)?  -pi_o_2-tiny: pi_o_2+tiny;

    /* when x is INF */
        if(ix==0x7ff00000) {
            if(iy==0x7ff00000) {
                switch(m) {
                    case 0: return  pi_o_4+tiny;/* atan(+INF,+INF) */
                    case 1: return -pi_o_4-tiny;/* atan(-INF,+INF) */
                    case 2: return  3.0*pi_o_4+tiny;/*atan(+INF,-INF)*/
                    case 3: return -3.0*pi_o_4-tiny;/*atan(-INF,-INF)*/
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
        if(iy==0x7ff00000) return (hy<0)? -pi_o_2-tiny: pi_o_2+tiny;

    /* compute y/x */
        k = (iy-ix)>>20;
        if(k > 60) z=pi_o_2+0.5*pi_lo;  /* |y/x| >  2**60 */
        else if(hx<0&&k<-60) z=0.0;     /* |y|/x < -2**60 */
        else z=atan_wrapper(fabs_wrapper(y/x));         /* safe to do y/x */
        switch (m) {
            case 0: return       z  ;   /* atan(+,+) */
            case 1: __HI(z) ^= 0x80000000;
                    return       z  ;   /* atan(-,+) */
            case 2: return  pi-(z-pi_lo);/* atan(+,-) */
            default: /* case 3 */
                    return  (z-pi_lo)-pi;/* atan(-,-) */
        }
}

/* @(#)s_atan.c 1.3 95/01/18 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunSoft, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 *
 */

/* atan(x)
 * Method
 *   1. Reduce x to positive by atan(x) = -atan(-x).
 *   2. According to the integer k=4t+0.25 chopped, t=x, the argument
 *      is further reduced to one of the following intervals and the
 *      arctangent of t is evaluated by the corresponding formula:
 *
 *      [0,7/16]      atan(x) = t-t^3*(a1+t^2*(a2+...(a10+t^2*a11)...)
 *      [7/16,11/16]  atan(x) = atan(1/2) + atan( (t-0.5)/(1+t/2) )
 *      [11/16.19/16] atan(x) = atan( 1 ) + atan( (t-1)/(1+t) )
 *      [19/16,39/16] atan(x) = atan(3/2) + atan( (t-1.5)/(1+1.5t) )
 *      [39/16,INF]   atan(x) = atan(INF) + atan( -1/t )
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following
 * constants. The decimal values may be used, provided that the
 * compiler will convert from decimal to binary accurately enough
 * to produce the hexadecimal values shown.
 */

#ifdef __STDC__
static const double atanhi[] = {
#else
static double atanhi[] = {
#endif
  4.63647609000806093515e-01, /* atan(0.5)hi 0x3FDDAC67, 0x0561BB4F */
  7.85398163397448278999e-01, /* atan(1.0)hi 0x3FE921FB, 0x54442D18 */
  9.82793723247329054082e-01, /* atan(1.5)hi 0x3FEF730B, 0xD281F69B */
  1.57079632679489655800e+00, /* atan(inf)hi 0x3FF921FB, 0x54442D18 */
};

#ifdef __STDC__
static const double atanlo[] = {
#else
static double atanlo[] = {
#endif
  2.26987774529616870924e-17, /* atan(0.5)lo 0x3C7A2B7F, 0x222F65E2 */
  3.06161699786838301793e-17, /* atan(1.0)lo 0x3C81A626, 0x33145C07 */
  1.39033110312309984516e-17, /* atan(1.5)lo 0x3C700788, 0x7AF0CBBD */
  6.12323399573676603587e-17, /* atan(inf)lo 0x3C91A626, 0x33145C07 */
};

#ifdef __STDC__
static const double aT[] = {
#else
static double aT[] = {
#endif
  3.33333333333329318027e-01, /* 0x3FD55555, 0x5555550D */
 -1.99999999998764832476e-01, /* 0xBFC99999, 0x9998EBC4 */
  1.42857142725034663711e-01, /* 0x3FC24924, 0x920083FF */
 -1.11111104054623557880e-01, /* 0xBFBC71C6, 0xFE231671 */
  9.09088713343650656196e-02, /* 0x3FB745CD, 0xC54C206E */
 -7.69187620504482999495e-02, /* 0xBFB3B0F2, 0xAF749A6D */
  6.66107313738753120669e-02, /* 0x3FB10D66, 0xA0D03D51 */
 -5.83357013379057348645e-02, /* 0xBFADDE2D, 0x52DEFD9A */
  4.97687799461593236017e-02, /* 0x3FA97B4B, 0x24760DEB */
 -3.65315727442169155270e-02, /* 0xBFA2B444, 0x2C6A6C2F */
  1.62858201153657823623e-02, /* 0x3F90AD3A, 0xE322DA11 */
};

#ifdef __STDC__
        static const double
#else
        static double
#endif
one   = 1.0,
huge   = 1.0e300;

double atan_wrapper(double x)
{
        double w,s1,s2,z;
        int ix,hx,id;

        hx = __HI(x);
        ix = hx&0x7fffffff;
        if(ix>=0x44100000) {    /* if |x| >= 2^66 */
            if(ix>0x7ff00000||
                (ix==0x7ff00000&&(__LO(x)!=0)))
                return x+x;             /* NaN */
            if(hx>0) return  atanhi[3]+atanlo[3];
            else     return -atanhi[3]-atanlo[3];
        } if (ix < 0x3fdc0000) {        /* |x| < 0.4375 */
            if (ix < 0x3e200000) {      /* |x| < 2^-29 */
                if(huge+x>one) return x;        /* raise inexact */
            }
            id = -1;
        } else {
        x = fabs(x);
        if (ix < 0x3ff30000) {          /* |x| < 1.1875 */
            if (ix < 0x3fe60000) {      /* 7/16 <=|x|<11/16 */
                id = 0; x = (2.0*x-one)/(2.0+x);
            } else {                    /* 11/16<=|x|< 19/16 */
                id = 1; x  = (x-one)/(x+one);
            }
        } else {
            if (ix < 0x40038000) {      /* |x| < 2.4375 */
                id = 2; x  = (x-1.5)/(one+1.5*x);
            } else {                    /* 2.4375 <= |x| < 2^66 */
                id = 3; x  = -1.0/x;
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
/* @(#)e_sqrt.c 1.3 95/01/18 */
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

/* __ieee754_sqrt(x)
 * Return correctly rounded sqrt.
 *           ------------------------------------------
 *           |  Use the hardware sqrt if you have one |
 *           ------------------------------------------
 * Method:
 *   Bit by bit method using integer arithmetic. (Slow, but portable)
 *   1. Normalization
 *      Scale x to y in [1,4) with even powers of 2:
 *      find an integer k such that  1 <= (y=x*2^(2k)) < 4, then
 *              sqrt(x) = 2^k * sqrt(y)
 *   2. Bit by bit computation
 *      Let q  = sqrt(y) truncated to i bit after binary point (q = 1),
 *           i                                                   0
 *                                     i+1         2
 *          s  = 2*q , and      y  =  2   * ( y - q  ).         (1)
 *           i      i            i                 i
 *
 *      To compute q    from q , one checks whether
 *                  i+1       i
 *
 *                            -(i+1) 2
 *                      (q + 2      ) <= y.                     (2)
 *                        i
 *                                                            -(i+1)
 *      If (2) is false, then q   = q ; otherwise q   = q  + 2      .
 *                             i+1   i             i+1   i
 *
 *      With some algebric manipulation, it is not difficult to see
 *      that (2) is equivalent to
 *                             -(i+1)
 *                      s  +  2       <= y                      (3)
 *                       i                i
 *
 *      The advantage of (3) is that s  and y  can be computed by
 *                                    i      i
 *      the following recurrence formula:
 *          if (3) is false
 *
 *          s     =  s  ,       y    = y   ;                    (4)
 *           i+1      i          i+1    i
 *
 *          otherwise,
 *                         -i                     -(i+1)
 *          s     =  s  + 2  ,  y    = y  -  s  - 2             (5)
 *           i+1      i          i+1    i     i
 *
 *      One may easily use induction to prove (4) and (5).
 *      Note. Since the left hand side of (3) contain only i+2 bits,
 *            it does not necessary to do a full (53-bit) comparison
 *            in (3).
 *   3. Final rounding
 *      After generating the 53 bits result, we compute one more bit.
 *      Together with the remainder, we can decide whether the
 *      result is exact, bigger than 1/2ulp, or less than 1/2ulp
 *      (it will never equal to 1/2ulp).
 *      The rounding mode can be detected by checking whether
 *      huge + tiny is equal to huge, and whether huge - tiny is
 *      equal to huge for some floating point number "huge" and "tiny".
 *
 * Special cases:
 *      sqrt(+-0) = +-0         ... exact
 *      sqrt(inf) = inf
 *      sqrt(-ve) = NaN         ... with invalid signal
 *      sqrt(NaN) = NaN         ... with invalid signal for signaling NaN
 *
 * Other methods : see the appended file at the end of the program below.
 *---------------
 */

double sqrt_wrapper(double x)
{
        double z;
        int     sign = (int)0x80000000;
        unsigned r,t1,s1,ix1,q1;
        int ix0,s0,q,m,t,i;

        ix0 = __HI(x);                  /* high word of x */
        ix1 = __LO(x);          /* low word of x */

    /* take care of Inf and NaN */
        if((ix0&0x7ff00000)==0x7ff00000) {
            return x*x+x;               /* sqrt(NaN)=NaN, sqrt(+inf)=+inf
                                           sqrt(-inf)=sNaN */
        }
    /* take care of zero */
        if(ix0<=0) {
            if(((ix0&(~sign))|ix1)==0) return x;/* sqrt(+-0) = +-0 */
            else if(ix0<0)
                return (x-x)/(x-x);             /* sqrt(-ve) = sNaN */
        }
    /* normalize x */
        m = (ix0>>20);
        if(m==0) {                              /* subnormal x */
            while(ix0==0) {
                m -= 21;
                ix0 |= (ix1>>11); ix1 <<= 21;
            }
            for(i=0;(ix0&0x00100000)==0;i++) ix0<<=1;
            m -= i-1;
            ix0 |= (ix1>>(32-i));
            ix1 <<= i;
        }
        m -= 1023;      /* unbias exponent */
        ix0 = (ix0&0x000fffff)|0x00100000;
        if(m&1){        /* odd m, double x to make it even */
            ix0 += ix0 + ((ix1&sign)>>31);
            ix1 += ix1;
        }
        m >>= 1;        /* m = [m/2] */

    /* generate sqrt(x) bit by bit */
        ix0 += ix0 + ((ix1&sign)>>31);
        ix1 += ix1;
        q = q1 = s0 = s1 = 0;   /* [q,q1] = sqrt(x) */
        r = 0x00200000;         /* r = moving bit from right to left */

        while(r!=0) {
            t = s0+r;
            if(t<=ix0) {
                s0   = t+r;
                ix0 -= t;
                q   += r;
            }
            ix0 += ix0 + ((ix1&sign)>>31);
            ix1 += ix1;
            r>>=1;
        }

        r = sign;
        while(r!=0) {
            t1 = s1+r;
            t  = s0;
            if((t<ix0)||((t==ix0)&&(t1<=ix1))) {
                s1  = t1+r;
                if(((t1&sign)==sign)&&(s1&sign)==0) s0 += 1;
                ix0 -= t;
                if (ix1 < t1) ix0 -= 1;
                ix1 -= t1;
                q1  += r;
            }
            ix0 += ix0 + ((ix1&sign)>>31);
            ix1 += ix1;
            r>>=1;
        }

    /* use floating add to find out rounding direction */
        if((ix0|ix1)!=0) {
            z = one-tiny; /* trigger inexact flag */
            if (z>=one) {
                z = one+tiny;
                if (q1==(unsigned)0xffffffff) { q1=0; q += 1;}
                else if (z>one) {
                    if (q1==(unsigned)0xfffffffe) q+=1;
                    q1+=2;
                } else
                    q1 += (q1&1);
            }
        }
        ix0 = (q>>1)+0x3fe00000;
        ix1 =  q1>>1;
        if ((q&1)==1) ix1 |= sign;
        ix0 += (m <<20);
        __HI(z) = ix0;
        __LO(z) = ix1;
        return z;
}

/*
Other methods  (use floating-point arithmetic)
-------------
(This is a copy of a drafted paper by Prof W. Kahan
and K.C. Ng, written in May, 1986)

        Two algorithms are given here to implement sqrt(x)
        (IEEE double precision arithmetic) in software.
        Both supply sqrt(x) correctly rounded. The first algorithm (in
        Section A) uses newton iterations and involves four divisions.
        The second one uses reciproot iterations to avoid division, but
        requires more multiplications. Both algorithms need the ability
        to chop results of arithmetic operations instead of round them,
        and the INEXACT flag to indicate when an arithmetic operation
        is executed exactly with no roundoff error, all part of the
        standard (IEEE 754-1985). The ability to perform shift, add,
        subtract and logical AND operations upon 32-bit words is needed
        too, though not part of the standard.

A.  sqrt(x) by Newton Iteration

   (1)  Initial approximation

        Let x0 and x1 be the leading and the trailing 32-bit words of
        a floating point number x (in IEEE double format) respectively

            1    11                  52                           ...widths
           ------------------------------------------------------
        x: |s|    e     |             f                         |
           ------------------------------------------------------
              msb    lsb  msb                                 lsb ...order


             ------------------------        ------------------------
        x0:  |s|   e    |    f1     |    x1: |          f2           |
             ------------------------        ------------------------

        By performing shifts and subtracts on x0 and x1 (both regarded
        as integers), we obtain an 8-bit approximation of sqrt(x) as
        follows.

                k  := (x0>>1) + 0x1ff80000;
                y0 := k - T1[31&(k>>15)].       ... y ~ sqrt(x) to 8 bits
        Here k is a 32-bit integer and T1[] is an integer array containing
        correction terms. Now magically the floating value of y (y's
        leading 32-bit word is y0, the value of its trailing word is 0)
        approximates sqrt(x) to almost 8-bit.

        Value of T1:
        static int T1[32]= {
        0,      1024,   3062,   5746,   9193,   13348,  18162,  23592,
        29598,  36145,  43202,  50740,  58733,  67158,  75992,  85215,
        83599,  71378,  60428,  50647,  41945,  34246,  27478,  21581,
        16499,  12183,  8588,   5674,   3403,   1742,   661,    130,};

    (2) Iterative refinement

        Apply Heron's rule three times to y, we have y approximates
        sqrt(x) to within 1 ulp (Unit in the Last Place):

                y := (y+x/y)/2          ... almost 17 sig. bits
                y := (y+x/y)/2          ... almost 35 sig. bits
                y := y-(y-x/y)/2        ... within 1 ulp


        Remark 1.
            Another way to improve y to within 1 ulp is:

                y := (y+x/y)            ... almost 17 sig. bits to 2*sqrt(x)
                y := y - 0x00100006     ... almost 18 sig. bits to sqrt(x)

                                2
                            (x-y )*y
                y := y + 2* ----------  ...within 1 ulp
                               2
                             3y  + x


        This formula has one division fewer than the one above; however,
        it requires more multiplications and additions. Also x must be
        scaled in advance to avoid spurious overflow in evaluating the
        expression 3y*y+x. Hence it is not recommended uless division
        is slow. If division is very slow, then one should use the
        reciproot algorithm given in section B.

    (3) Final adjustment

        By twiddling y's last bit it is possible to force y to be
        correctly rounded according to the prevailing rounding mode
        as follows. Let r and i be copies of the rounding mode and
        inexact flag before entering the square root program. Also we
        use the expression y+-ulp for the next representable floating
        numbers (up and down) of y. Note that y+-ulp = either fixed
        point y+-1, or multiply y by nextafter(1,+-inf) in chopped
        mode.

                I := FALSE;     ... reset INEXACT flag I
                R := RZ;        ... set rounding mode to round-toward-zero
                z := x/y;       ... chopped quotient, possibly inexact
                If(not I) then {        ... if the quotient is exact
                    if(z=y) {
                        I := i;  ... restore inexact flag
                        R := r;  ... restore rounded mode
                        return sqrt(x):=y.
                    } else {
                        z := z - ulp;   ... special rounding
                    }
                }
                i := TRUE;              ... sqrt(x) is inexact
                If (r=RN) then z=z+ulp  ... rounded-to-nearest
                If (r=RP) then {        ... round-toward-+inf
                    y = y+ulp; z=z+ulp;
                }
                y := y+z;               ... chopped sum
                y0:=y0-0x00100000;      ... y := y/2 is correctly rounded.
                I := i;                 ... restore inexact flag
                R := r;                 ... restore rounded mode
                return sqrt(x):=y.

    (4) Special cases

        Square root of +inf, +-0, or NaN is itself;
        Square root of a negative number is NaN with invalid signal.


B.  sqrt(x) by Reciproot Iteration

   (1)  Initial approximation

        Let x0 and x1 be the leading and the trailing 32-bit words of
        a floating point number x (in IEEE double format) respectively
        (see section A). By performing shifs and subtracts on x0 and y0,
        we obtain a 7.8-bit approximation of 1/sqrt(x) as follows.

            k := 0x5fe80000 - (x0>>1);
            y0:= k - T2[63&(k>>14)].    ... y ~ 1/sqrt(x) to 7.8 bits

        Here k is a 32-bit integer and T2[] is an integer array
        containing correction terms. Now magically the floating
        value of y (y's leading 32-bit word is y0, the value of
        its trailing word y1 is set to zero) approximates 1/sqrt(x)
        to almost 7.8-bit.

        Value of T2:
        static int T2[64]= {
        0x1500, 0x2ef8, 0x4d67, 0x6b02, 0x87be, 0xa395, 0xbe7a, 0xd866,
        0xf14a, 0x1091b,0x11fcd,0x13552,0x14999,0x15c98,0x16e34,0x17e5f,
        0x18d03,0x19a01,0x1a545,0x1ae8a,0x1b5c4,0x1bb01,0x1bfde,0x1c28d,
        0x1c2de,0x1c0db,0x1ba73,0x1b11c,0x1a4b5,0x1953d,0x18266,0x16be0,
        0x1683e,0x179d8,0x18a4d,0x19992,0x1a789,0x1b445,0x1bf61,0x1c989,
        0x1d16d,0x1d77b,0x1dddf,0x1e2ad,0x1e5bf,0x1e6e8,0x1e654,0x1e3cd,
        0x1df2a,0x1d635,0x1cb16,0x1be2c,0x1ae4e,0x19bde,0x1868e,0x16e2e,
        0x1527f,0x1334a,0x11051,0xe951, 0xbe01, 0x8e0d, 0x5924, 0x1edd,};

    (2) Iterative refinement

        Apply Reciproot iteration three times to y and multiply the
        result by x to get an approximation z that matches sqrt(x)
        to about 1 ulp. To be exact, we will have
                -1ulp < sqrt(x)-z<1.0625ulp.

        ... set rounding mode to Round-to-nearest
           y := y*(1.5-0.5*x*y*y)       ... almost 15 sig. bits to 1/sqrt(x)
           y := y*((1.5-2^-30)+0.5*x*y*y)... about 29 sig. bits to 1/sqrt(x)
        ... special arrangement for better accuracy
           z := x*y                     ... 29 bits to sqrt(x), with z*y<1
           z := z + 0.5*z*(1-z*y)       ... about 1 ulp to sqrt(x)

        Remark 2. The constant 1.5-2^-30 is chosen to bias the error so that
        (a) the term z*y in the final iteration is always less than 1;
        (b) the error in the final result is biased upward so that
                -1 ulp < sqrt(x) - z < 1.0625 ulp
            instead of |sqrt(x)-z|<1.03125ulp.

    (3) Final adjustment

        By twiddling y's last bit it is possible to force y to be
        correctly rounded according to the prevailing rounding mode
        as follows. Let r and i be copies of the rounding mode and
        inexact flag before entering the square root program. Also we
        use the expression y+-ulp for the next representable floating
        numbers (up and down) of y. Note that y+-ulp = either fixed
        point y+-1, or multiply y by nextafter(1,+-inf) in chopped
        mode.

        R := RZ;                ... set rounding mode to round-toward-zero
        switch(r) {
            case RN:            ... round-to-nearest
               if(x<= z*(z-ulp)...chopped) z = z - ulp; else
               if(x<= z*(z+ulp)...chopped) z = z; else z = z+ulp;
               break;
            case RZ:case RM:    ... round-to-zero or round-to--inf
               R:=RP;           ... reset rounding mod to round-to-+inf
               if(x<z*z ... rounded up) z = z - ulp; else
               if(x>=(z+ulp)*(z+ulp) ...rounded up) z = z+ulp;
               break;
            case RP:            ... round-to-+inf
               if(x>(z+ulp)*(z+ulp)...chopped) z = z+2*ulp; else
               if(x>z*z ...chopped) z = z+ulp;
               break;
        }

        Remark 3. The above comparisons can be done in fixed point. For
        example, to compare x and w=z*z chopped, it suffices to compare
        x1 and w1 (the trailing parts of x and w), regarding them as
        two's complement integers.

        ...Is z an exact square root?
        To determine whether z is an exact square root of x, let z1 be the
        trailing part of z, and also let x0 and x1 be the leading and
        trailing parts of x.

        If ((z1&0x03ffffff)!=0) ... not exact if trailing 26 bits of z!=0
            I := 1;             ... Raise Inexact flag: z is not exact
        else {
            j := 1 - [(x0>>20)&1]       ... j = logb(x) mod 2
            k := z1 >> 26;              ... get z's 25-th and 26-th
                                            fraction bits
            I := i or (k&j) or ((k&(j+j+1))!=(x1&3));
        }
        R:= r           ... restore rounded mode
        return sqrt(x):=z.

        If multiplication is cheaper then the foregoing red tape, the
        Inexact flag can be evaluated by

            I := i;
            I := (z*z!=x) or I.

        Note that z*z can overwrite I; this value must be sensed if it is
        True.

        Remark 4. If z*z = x exactly, then bit 25 to bit 0 of z1 must be
        zero.

                    --------------------
                z1: |        f2        |
                    --------------------
                bit 31             bit 0

        Further more, bit 27 and 26 of z1, bit 0 and 1 of x1, and the odd
        or even of logb(x) have the following relations:

        -------------------------------------------------
        bit 27,26 of z1         bit 1,0 of x1   logb(x)
        -------------------------------------------------
        00                      00              odd and even
        01                      01              even
        10                      10              odd
        10                      00              even
        11                      01              even
        -------------------------------------------------

    (4) Special cases (see (4) of Section A).

 */


/* @(#)e_acos.c 1.3 95/01/18 */
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

/* __ieee754_acos(x)
 * Method :
 *      acos(x)  = pi/2 - asin(x)
 *      acos(-x) = pi/2 + asin(x)
 * For |x|<=0.5
 *      acos(x) = pi/2 - (x + x*x^2*R(x^2))     (see asin.c)
 * For x>0.5
 *      acos(x) = pi/2 - (pi/2 - 2asin(sqrt((1-x)/2)))
 *              = 2asin(sqrt((1-x)/2))
 *              = 2s + 2s*z*R(z)        ...z=(1-x)/2, s=sqrt(z)
 *              = 2f + (2c + 2s*z*R(z))
 *     where f=hi part of s, and c = (z-f*f)/(s+f) is the correction term
 *     for f so that f+c ~ sqrt(z).
 * For x<-0.5
 *      acos(x) = pi - 2asin(sqrt((1-|x|)/2))
 *              = pi - 0.5*(s+s*z*R(z)), where z=(1-|x|)/2,s=sqrt(z)
 *
 * Special cases:
 *      if x is NaN, return x itself;
 *      if |x|>1, return NaN with invalid signal.
 *
 * Function needed: sqrt
 */

#ifdef __STDC__
static const double
#else
static double
#endif
pio2_hi =  1.57079632679489655800e+00, /* 0x3FF921FB, 0x54442D18 */
pio2_lo =  6.12323399573676603587e-17, /* 0x3C91A626, 0x33145C07 */
pS0 =  1.66666666666666657415e-01, /* 0x3FC55555, 0x55555555 */
pS1 = -3.25565818622400915405e-01, /* 0xBFD4D612, 0x03EB6F7D */
pS2 =  2.01212532134862925881e-01, /* 0x3FC9C155, 0x0E884455 */
pS3 = -4.00555345006794114027e-02, /* 0xBFA48228, 0xB5688F3B */
pS4 =  7.91534994289814532176e-04, /* 0x3F49EFE0, 0x7501B288 */
pS5 =  3.47933107596021167570e-05, /* 0x3F023DE1, 0x0DFDF709 */
qS1 = -2.40339491173441421878e+00, /* 0xC0033A27, 0x1C8A2D4B */
qS2 =  2.02094576023350569471e+00, /* 0x40002AE5, 0x9C598AC8 */
qS3 = -6.88283971605453293030e-01, /* 0xBFE6066C, 0x1B8D0159 */
qS4 =  7.70381505559019352791e-02; /* 0x3FB3B8C5, 0xB12E9282 */

double acos_wrapper(double x)
{
        double z,p,q,r,w,s,c,df;
        int hx,ix;
        hx = __HI(x);
        ix = hx&0x7fffffff;
        if(ix>=0x3ff00000) {    /* |x| >= 1 */
            if(((ix-0x3ff00000)|__LO(x))==0) {  /* |x|==1 */
                if(hx>0) return 0.0;            /* acos(1) = 0  */
                else return pi+2.0*pio2_lo;     /* acos(-1)= pi */
            }
            return (x-x)/(x-x);         /* acos(|x|>1) is NaN */
        }
        if(ix<0x3fe00000) {     /* |x| < 0.5 */
            if(ix<=0x3c600000) return pio2_hi+pio2_lo;/*if|x|<2**-57*/
            z = x*x;
            p = z*(pS0+z*(pS1+z*(pS2+z*(pS3+z*(pS4+z*pS5)))));
            q = one+z*(qS1+z*(qS2+z*(qS3+z*qS4)));
            r = p/q;
            return pio2_hi - (x - (pio2_lo-x*r));
        } else  if (hx<0) {             /* x < -0.5 */
            z = (one+x)*0.5;
            p = z*(pS0+z*(pS1+z*(pS2+z*(pS3+z*(pS4+z*pS5)))));
            q = one+z*(qS1+z*(qS2+z*(qS3+z*qS4)));
            s = sqrt_wrapper(z);
            r = p/q;
            w = r*s-pio2_lo;
            return pi - 2.0*(s+w);
        } else {                        /* x > 0.5 */
            z = (one-x)*0.5;
            s = sqrt_wrapper(z);
            df = s;
            __LO(df) = 0;
            c  = (z-df*df)/(s+df);
            p = z*(pS0+z*(pS1+z*(pS2+z*(pS3+z*(pS4+z*pS5)))));
            q = one+z*(qS1+z*(qS2+z*(qS3+z*qS4)));
            r = p/q;
            w = r*s+c;
            return 2.0*(df+w);
        }

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

/*
 * ====================================================
 * Copyright (C) 2004 by Sun Microsystems, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

/* __ieee754_pow(x,y) return x**y
 *
 *                    n
 * Method:  Let x =  2   * (1+f)
 *      1. Compute and return log2(x) in two pieces:
 *              log2(x) = w1 + w2,
 *         where w1 has 53-24 = 29 bit trailing zeros.
 *      2. Perform y*log2(x) = n+y' by simulating muti-precision
 *         arithmetic, where |y'|<=0.5.
 *      3. Return x**y = 2**n*exp(y'*log2)
 *
 * Special cases:
 *      1.  (anything) ** 0  is 1
 *      2.  (anything) ** 1  is itself
 *      3.  (anything) ** NAN is NAN
 *      4.  NAN ** (anything except 0) is NAN
 *      5.  +-(|x| > 1) **  +INF is +INF
 *      6.  +-(|x| > 1) **  -INF is +0
 *      7.  +-(|x| < 1) **  +INF is +0
 *      8.  +-(|x| < 1) **  -INF is +INF
 *      9.  +-1         ** +-INF is NAN
 *      10. +0 ** (+anything except 0, NAN)               is +0
 *      11. -0 ** (+anything except 0, NAN, odd integer)  is +0
 *      12. +0 ** (-anything except 0, NAN)               is +INF
 *      13. -0 ** (-anything except 0, NAN, odd integer)  is +INF
 *      14. -0 ** (odd integer) = -( +0 ** (odd integer) )
 *      15. +INF ** (+anything except 0,NAN) is +INF
 *      16. +INF ** (-anything except 0,NAN) is +0
 *      17. -INF ** (anything)  = -0 ** (-anything)
 *      18. (-anything) ** (integer) is (-1)**(integer)*(+anything**integer)
 *      19. (-anything except 0 and inf) ** (non-integer) is NAN
 *
 * Accuracy:
 *      pow(x,y) returns x**y nearly rounded. In particular
 *                      pow(integer,integer)
 *      always returns the correct integer provided it is
 *      representable.
 *
 * Constants :
 * The hexadecimal values are the intended ones for the following
 * constants. The decimal values may be used, provided that the
 * compiler will convert from decimal to binary accurately enough
 * to produce the hexadecimal values shown.
 */

double scalbn_wrapper (double x, int n);

#ifdef __STDC__
static const double
#else
static double
#endif
bp[] = {1.0, 1.5,},
dp_h[] = { 0.0, 5.84962487220764160156e-01,}, /* 0x3FE2B803, 0x40000000 */
dp_l[] = { 0.0, 1.35003920212974897128e-08,}, /* 0x3E4CFDEB, 0x43CFD006 */
two     =  2.0,
two53   =  9007199254740992.0,  /* 0x43400000, 0x00000000 */
        /* poly coefs for (3/2)*(log(x)-2s-2/3*s**3 */
L1  =  5.99999999999994648725e-01, /* 0x3FE33333, 0x33333303 */
L2  =  4.28571428578550184252e-01, /* 0x3FDB6DB6, 0xDB6FABFF */
L3  =  3.33333329818377432918e-01, /* 0x3FD55555, 0x518F264D */
L4  =  2.72728123808534006489e-01, /* 0x3FD17460, 0xA91D4101 */
L5  =  2.30660745775561754067e-01, /* 0x3FCD864A, 0x93C9DB65 */
L6  =  2.06975017800338417784e-01, /* 0x3FCA7E28, 0x4A454EEF */
P1   =  1.66666666666666019037e-01, /* 0x3FC55555, 0x5555553E */
P2   = -2.77777777770155933842e-03, /* 0xBF66C16C, 0x16BEBD93 */
P3   =  6.61375632143793436117e-05, /* 0x3F11566A, 0xAF25DE2C */
P4   = -1.65339022054652515390e-06, /* 0xBEBBBD41, 0xC5D26BF1 */
P5   =  4.13813679705723846039e-08, /* 0x3E663769, 0x72BEA4D0 */
lg2  =  6.93147180559945286227e-01, /* 0x3FE62E42, 0xFEFA39EF */
lg2_h  =  6.93147182464599609375e-01, /* 0x3FE62E43, 0x00000000 */
lg2_l  = -1.90465429995776804525e-09, /* 0xBE205C61, 0x0CA86C39 */
ovt =  8.0085662595372944372e-0017, /* -(1024-log2(ovfl+.5ulp)) */
cp    =  9.61796693925975554329e-01, /* 0x3FEEC709, 0xDC3A03FD =2/(3ln2) */
cp_h  =  9.61796700954437255859e-01, /* 0x3FEEC709, 0xE0000000 =(float)cp */
cp_l  = -7.02846165095275826516e-09, /* 0xBE3E2FE0, 0x145B01F5 =tail of cp_h*/
ivln2    =  1.44269504088896338700e+00, /* 0x3FF71547, 0x652B82FE =1/ln2 */
ivln2_h  =  1.44269502162933349609e+00, /* 0x3FF71547, 0x60000000 =24b 1/ln2*/
ivln2_l  =  1.92596299112661746887e-08; /* 0x3E54AE0B, 0xF85DDF44 =1/ln2 tail*/

double pow_wrapper(double x, double y)
{
        double z,ax,z_h,z_l,p_h,p_l;
        double y1,t1,t2,r,s,t,u,v,w;
        int i0,i1,i,j,k,yisint,n;
        int hx,hy,ix,iy;
        unsigned lx,ly;

        i0 = ((*(int*)&one)>>29)^1; i1=1-i0;
        hx = __HI(x); lx = __LO(x);
        hy = __HI(y); ly = __LO(y);
        ix = hx&0x7fffffff;  iy = hy&0x7fffffff;

    /* y==zero: x**0 = 1 */
        if((iy|ly)==0) return one;

    /* +-NaN return x+y */
        if(ix > 0x7ff00000 || ((ix==0x7ff00000)&&(lx!=0)) ||
           iy > 0x7ff00000 || ((iy==0x7ff00000)&&(ly!=0)))
                return x+y;

    /* determine if y is an odd int when x < 0
     * yisint = 0       ... y is not an integer
     * yisint = 1       ... y is an odd int
     * yisint = 2       ... y is an even int
     */
        yisint  = 0;
        if(hx<0) {
            if(iy>=0x43400000) yisint = 2; /* even integer y */
            else if(iy>=0x3ff00000) {
                k = (iy>>20)-0x3ff;        /* exponent */
                if(k>20) {
                    j = ly>>(52-k);
                    if((j<<(52-k))==ly) yisint = 2-(j&1);
                } else if(ly==0) {
                    j = iy>>(20-k);
                    if((j<<(20-k))==iy) yisint = 2-(j&1);
                }
            }
        }

    /* special value of y */
        if(ly==0) {
            if (iy==0x7ff00000) {       /* y is +-inf */
                if(((ix-0x3ff00000)|lx)==0)
                    return  y - y;      /* inf**+-1 is NaN */
                else if (ix >= 0x3ff00000)/* (|x|>1)**+-inf = inf,0 */
                    return (hy>=0)? y: zero;
                else                    /* (|x|<1)**-,+inf = inf,0 */
                    return (hy<0)?-y: zero;
            }
            if(iy==0x3ff00000) {        /* y is  +-1 */
                if(hy<0) return one/x; else return x;
            }
            if(hy==0x40000000) return x*x; /* y is  2 */
            if(hy==0x3fe00000) {        /* y is  0.5 */
                if(hx>=0)       /* x >= +0 */
                return sqrt(x);
            }
        }

        ax   = fabs(x);
    /* special value of x */
        if(lx==0) {
            if(ix==0x7ff00000||ix==0||ix==0x3ff00000){
                z = ax;                 /*x is +-0,+-inf,+-1*/
                if(hy<0) z = one/z;     /* z = (1/|x|) */
                if(hx<0) {
                    if(((ix-0x3ff00000)|yisint)==0) {
                        z = (z-z)/(z-z); /* (-1)**non-int is NaN */
                    } else if(yisint==1)
                        z = -z;         /* (x<0)**odd = -(|x|**odd) */
                }
                return z;
            }
        }

        n = (hx>>31)+1;

    /* (x<0)**(non-int) is NaN */
        if((n|yisint)==0) return (x-x)/(x-x);

        s = one; /* s (sign of result -ve**odd) = -1 else = 1 */
        if((n|(yisint-1))==0) s = -one;/* (-ve)**(odd int) */

    /* |y| is huge */
        if(iy>0x41e00000) { /* if |y| > 2**31 */
            if(iy>0x43f00000){  /* if |y| > 2**64, must o/uflow */
                if(ix<=0x3fefffff) return (hy<0)? huge*huge:tiny*tiny;
                if(ix>=0x3ff00000) return (hy>0)? huge*huge:tiny*tiny;
            }
        /* over/underflow if x is not close to one */
            if(ix<0x3fefffff) return (hy<0)? s*huge*huge:s*tiny*tiny;
            if(ix>0x3ff00000) return (hy>0)? s*huge*huge:s*tiny*tiny;
        /* now |1-x| is tiny <= 2**-20, suffice to compute
           log(x) by x-x^2/2+x^3/3-x^4/4 */
            t = ax-one;         /* t has 20 trailing zeros */
            w = (t*t)*(0.5-t*(0.3333333333333333333333-t*0.25));
            u = ivln2_h*t;      /* ivln2_h has 21 sig. bits */
            v = t*ivln2_l-w*ivln2;
            t1 = u+v;
            __LO(t1) = 0;
            t2 = v-(t1-u);
        } else {
            double ss,s2,s_h,s_l,t_h,t_l;
            n = 0;
        /* take care subnormal number */
            if(ix<0x00100000)
                {ax *= two53; n -= 53; ix = __HI(ax); }
            n  += ((ix)>>20)-0x3ff;
            j  = ix&0x000fffff;
        /* determine interval */
            ix = j|0x3ff00000;          /* normalize ix */
            if(j<=0x3988E) k=0;         /* |x|<sqrt(3/2) */
            else if(j<0xBB67A) k=1;     /* |x|<sqrt(3)   */
            else {k=0;n+=1;ix -= 0x00100000;}
            __HI(ax) = ix;

        /* compute ss = s_h+s_l = (x-1)/(x+1) or (x-1.5)/(x+1.5) */
            u = ax-bp[k];               /* bp[0]=1.0, bp[1]=1.5 */
            v = one/(ax+bp[k]);
            ss = u*v;
            s_h = ss;
            __LO(s_h) = 0;
        /* t_h=ax+bp[k] High */
            t_h = zero;
            __HI(t_h)=((ix>>1)|0x20000000)+0x00080000+(k<<18);
            t_l = ax - (t_h-bp[k]);
            s_l = v*((u-s_h*t_h)-s_h*t_l);
        /* compute log(ax) */
            s2 = ss*ss;
            r = s2*s2*(L1+s2*(L2+s2*(L3+s2*(L4+s2*(L5+s2*L6)))));
            r += s_l*(s_h+ss);
            s2  = s_h*s_h;
            t_h = 3.0+s2+r;
            __LO(t_h) = 0;
            t_l = r-((t_h-3.0)-s2);
        /* u+v = ss*(1+...) */
            u = s_h*t_h;
            v = s_l*t_h+t_l*ss;
        /* 2/(3log2)*(ss+...) */
            p_h = u+v;
            __LO(p_h) = 0;
            p_l = v-(p_h-u);
            z_h = cp_h*p_h;             /* cp_h+cp_l = 2/(3*log2) */
            z_l = cp_l*p_h+p_l*cp+dp_l[k];
        /* log2(ax) = (ss+..)*2/(3*log2) = n + dp_h + z_h + z_l */
            t = (double)n;
            t1 = (((z_h+z_l)+dp_h[k])+t);
            __LO(t1) = 0;
            t2 = z_l-(((t1-t)-dp_h[k])-z_h);
        }

    /* split up y into y1+y2 and compute (y1+y2)*(t1+t2) */
        y1  = y;
        __LO(y1) = 0;
        p_l = (y-y1)*t1+y*t2;
        p_h = y1*t1;
        z = p_l+p_h;
        j = __HI(z);
        i = __LO(z);
        if (j>=0x40900000) {                            /* z >= 1024 */
            if(((j-0x40900000)|i)!=0)                   /* if z > 1024 */
                return s*huge*huge;                     /* overflow */
            else {
                if(p_l+ovt>z-p_h) return s*huge*huge;   /* overflow */
            }
        } else if((j&0x7fffffff)>=0x4090cc00 ) {        /* z <= -1075 */
            if(((j-0xc090cc00)|i)!=0)           /* z < -1075 */
                return s*tiny*tiny;             /* underflow */
            else {
                if(p_l<=z-p_h) return s*tiny*tiny;      /* underflow */
            }
        }
    /*
     * compute 2**(p_h+p_l)
     */
        i = j&0x7fffffff;
        k = (i>>20)-0x3ff;
        n = 0;
        if(i>0x3fe00000) {              /* if |z| > 0.5, set n = [z+0.5] */
            n = j+(0x00100000>>(k+1));
            k = ((n&0x7fffffff)>>20)-0x3ff;     /* new k for n */
            t = zero;
            __HI(t) = (n&~(0x000fffff>>k));
            n = ((n&0x000fffff)|0x00100000)>>(20-k);
            if(j<0) n = -n;
            p_h -= t;
        }
        t = p_l+p_h;
        __LO(t) = 0;
        u = t*lg2_h;
        v = (p_l-(t-p_h))*lg2+t*lg2_l;
        z = u+v;
        w = v-(z-u);
        t  = z*z;
        t1  = z - t*(P1+t*(P2+t*(P3+t*(P4+t*P5))));
        r  = (z*t1)/(t1-two)-(w+z*w);
        z  = one-(r-z);
        j  = __HI(z);
        j += (n<<20);
        if((j>>20)<=0) z = scalbn_wrapper(z,n); /* subnormal output */
        else __HI(z) += (n<<20);
        return s*z;
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
            if (n > 50000)      /* in case integer overflow in n+k */
                return huge*copysign_wrapper(huge,x);   /*overflow*/
            else return tiny*copysign_wrapper(tiny,x);  /*underflow*/
        k += 54;                                /* subnormal result */
        __HI(x) = (hx&0x800fffff)|(k<<20);
        return x*twom54;
}

/* @(#)s_ceil.c 1.3 95/01/18 */
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
 * ceil(x)
 * Return x rounded toward -inf to integral value
 * Method:
 *      Bit twiddling.
 * Exception:
 *      Inexact flag raised if x not equal to ceil(x).
 */

double ceil_wrapper(double x)
{
        int i0,i1,j0;
        unsigned i,j;
        i0 =  __HI(x);
        i1 =  __LO(x);
        j0 = ((i0>>20)&0x7ff)-0x3ff;
        if(j0<20) {
            if(j0<0) {  /* raise inexact if x != 0 */
                if(huge+x>0.0) {/* return 0*sign(x) if |x|<1 */
                    if(i0<0) {i0=0x80000000;i1=0;}
                    else if((i0|i1)!=0) { i0=0x3ff00000;i1=0;}
                }
            } else {
                i = (0x000fffff)>>j0;
                if(((i0&i)|i1)==0) return x; /* x is integral */
                if(huge+x>0.0) {        /* raise inexact flag */
                    if(i0>0) i0 += (0x00100000)>>j0;
                    i0 &= (~i); i1=0;
                }
            }
        } else if (j0>51) {
            if(j0==0x400) return x+x;   /* inf or NaN */
            else return x;              /* x is integral */
        } else {
            i = ((unsigned)(0xffffffff))>>(j0-20);
            if((i1&i)==0) return x;     /* x is integral */
            if(huge+x>0.0) {            /* raise inexact flag */
                if(i0>0) {
                    if(j0==20) i0+=1;
                    else {
                        j = i1 + (1<<(52-j0));
                        if(j<i1) i0+=1; /* got a carry */
                        i1 = j;
                    }
                }
                i1 &= (~i);
            }
        }
        __HI(x) = i0;
        __LO(x) = i1;
        return x;
}

/* originally from python */

/* Just in case you haven't got an atof() around...
   This one doesn't check for bad syntax or overflow,
   and is slow and inaccurate.
   But it's good enough for the occasional string literal... */

#include <ctype.h>

double atof_wrapper(char *s)
{
        double a = 0.0;
        int e = 0;
        int c;
        while ((c = *s++) != '\0' && isdigit(c)) {
                a = a*10.0 + (c - '0');
        }
        if (c == '.') {
                while ((c = *s++) != '\0' && isdigit(c)) {
                        a = a*10.0 + (c - '0');
                        e = e-1;
                }
        }
        if (c == 'e' || c == 'E') {
                int sign = 1;
                int i = 0;
                c = *s++;
                if (c == '+')
                        c = *s++;
                else if (c == '-') {
                        c = *s++;
                        sign = -1;
                }
                while (isdigit(c)) {
                        i = i*10 + (c - '0');
                        c = *s++;
                }
                e += i*sign;
        }
        while (e > 0) {
                a *= 10.0;
                e--;
        }
        while (e < 0) {
                a *= 0.1;
                e++;
        }
        return a;
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

void bcopy(const void *src, void *dst, size_t n)
{
    memmove(dst, src, n);
}

int
sscanf_wrapper(const char *ibuf, const char *fmt, ...)
{
        va_list ap;
        int ret;

        va_start(ap, fmt);
        ret = vsscanf(ibuf, fmt, ap);
        va_end(ap);
        return(ret);
}

int
vsscanf(const char *inp, char const *fmt0, va_list ap)
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
                                bcopy(inp, va_arg(ap, char *), width);
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
