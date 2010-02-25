/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Wincent Balin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "plugin.h"
#include "pdbox.h"
#include "ctype.h"

#include "PDa/src/m_pd.h"
#include "PDa/src/s_stuff.h"


/* This implementation of strncat is taken from lua plug-in. */

/* gcc is broken and has a non-SUSv2 compliant internal prototype.
 * This causes it to warn about a type mismatch here.  Ignore it. */
char *rb_strncat(char *s, const char *t, size_t n)
{
    char *dest = s;
    register char *max;
    s += strlen(s);

    if((max = s + n) == s)
        goto strncat_fini;

    while(true)
    {
        if(!(*s = *t))
            break;
        if(++s == max)
            break;
        ++t;

#ifndef WANT_SMALL_STRING_ROUTINES
    if(!(*s = *t))
        break;
    if(++s == max)
        break;
    ++t;
    if(!(*s = *t))
        break;
    if(++s == max)
        break;
    ++t;
    if(!(*s = *t))
        break;
    if(++s == max)
        break;
    ++t;
#endif
    }

    *s = 0;

strncat_fini:
    return dest;
}


/* Implementation of floor, original. */
float rb_floor(float value)
{
    /* If value is negative, decrement value to match function's definition. */
    if(value < 0.0)
    {
        value -= 1.0;
    }

    /* Truncate fractional part (convert to integer)
       and afterwards convert back to double. */
    return (float) ((int) value);
}


/* Implementation of strtod() and atof(),
   taken from SanOS (http://www.jbox.dk/sanos/). */
static int rb_errno = 0;

double rb_strtod(const char *str, char **endptr)
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

double rb_atof(const char *str)
{
    return rb_strtod(str, NULL);
}


/* Implementation of ftoa(), original. */
void rb_ftoan(float f, char* out, int size)
{
    #define SBUFSIZE 12
    char sbuf[SBUFSIZE];

    /* Zero out string. */
    *out = '\0';
    size--;

    /* Handle negative numbers. */
    if(f < 0.0)
    {
        f = -f;
        strcat(out, "-");
        size--;
    }

    /* Find and convert integer part. */
    int int_part = (int) f;
    snprintf(sbuf, SBUFSIZE, "%d", int_part);
    int int_part_len = strlen(sbuf);
    if(size < int_part_len)
        return;

    /* Append integral part to output string. */
    strcat(out, sbuf);
    size -= int_part_len;

    /* Check whether further content is possible. */
    if(size <= 0)
        return;

    /* Append decimal point. */
    strcat(out, ".");
    size--;

    /* Calculate first rest. */
    float rest1 = (f - (float) int_part) * 1000000000.0;

    /* If there is no fractional part, return here. */
    if(rest1 == 0.0f)
        return;

    /* Convert the first rest to string. */
    int irest1 = (int) rest1;
    snprintf(sbuf, SBUFSIZE, "%09d", irest1);

    /* Append first rest to output string. */
    int rest1_len = strlen(sbuf);
    int rest1_minlen = MIN(size, rest1_len);
    strncat(out, sbuf, rest1_minlen);
    size -= rest1_minlen;

    /* Check whether output string still has enough space. */
    if(size <= 0)
        return;

    /* Calculate second rest. */
    float rest2 = (rest1 - (float) irest1) * 1000000000.0;

    /* If no second rest, check whether
       the output string has unwanted zero trail,
       remove it and end processing here. */
    if(rest2 == 0.0f)
    {
        char* zerotrail = out + strlen(out) - 1;

        for(; zerotrail >= out; zerotrail--)
        {
            if(*zerotrail == '0')
                *zerotrail = '\0';
            else
                return;
        }
    }

    /* Convert second rest. */
    int irest2 = (int) rest2;
    snprintf(sbuf, SBUFSIZE, "%09d", irest2);

    /* Append second rest to the output string. */
    int rest2_len = strlen(sbuf);
    int rest2_minlen = MIN(size, rest2_len);
    strncat(out, sbuf, rest2_minlen);

    /* Cut trailing zeroes. */
    char* zerotrail = out + strlen(out) - 1;
    for(;zerotrail >= out; zerotrail--)
    {
            if(*zerotrail == '0')
                *zerotrail = '\0';
            else
                return;
    }
}


/* Implementation of atol(), adapted from
   the atoi() implementation in Rockbox. */
long rb_atol(const char* str)
{
    long value = 0L;
    long sign = 1L;

    while (isspace(*str))
    {
        str++;
    }

    if ('-' == *str)
    {
        sign = -1L;
        str++;
    }
    else if ('+' == *str)
    {
        str++;
    }

    while ('0' == *str)
    {
        str++;
    }

    while (isdigit(*str))
    {
        value = (value * 10L) + (*str - '0');
        str++;
    }

    return value * sign;
}


/* Implementation of sin() and cos(),
   adapted from http://lab.polygonal.de/2007/07/18/fast-and-accurate-sinecosine-approximation/
*/

float rb_sin(float rad)
{
    int cycles;

    /* Trim input value to -PI..PI interval. */
    if(rad > 3.14159265)
    {
        cycles = rad / 6.28318531;
        rad -= (6.28318531 * (float) cycles);
    }
    else if(rad < -3.14159265)
    {
        cycles = rad / -6.28318531;
        rad += (6.28318531 * (float) cycles);
    }

    if(rad < 0)
        return (1.27323954 * rad + 0.405284735 * rad * rad);
    else
        return (1.27323954 * rad - 0.405284735 * rad * rad);
}

float rb_cos(float rad)
{
    /* Compute cosine: sin(x + PI/2) = cos(x) */
    return rb_sin(rad + 1.57079632);
}


/* Emulation of fscanf(fd, "%f", (float*) xxx);
   Basically a reimplementation of rb_strtod() above. */
int rb_fscanf_f(int fd, float* f)
{
    #define FSCANF_F_BUFSIZE 64
    char buf[FSCANF_F_BUFSIZE];

    /* Read line from file. */
    int bytes_read = rb->read_line(fd, buf, FSCANF_F_BUFSIZE-1);

    /* Terminate string. */
    if(bytes_read >= FSCANF_F_BUFSIZE)
        buf[FSCANF_F_BUFSIZE-1] = '\0';
    else
        buf[bytes_read-1] = '\0';

    /* Convert buffer to float. */
    *f = rb_atof(buf);

    /* If there was an error, no float was read. */
    if(rb_errno)
        return 0;

    return 1;
}

/* Emulation of fprintf(fd, "%f\n", (float*) xxx); */
int rb_fprintf_f(int fd, float f)
{
    #define FPRINTF_F_BUFSIZE 64
    char buf[FPRINTF_F_BUFSIZE];
    const char* next_line = "\n";

    /* Convert float to string. */
    rb_ftoan(f, buf, sizeof(buf)-1);

    /* Add next line character. */
    strcat(buf, next_line);

    /* Write string into file. */
    return write(fd, buf, strlen(buf));
}


/* Natural logarithm.
   Taken from glibc-2.8 */
static const float
ln2_hi =   6.9313812256e-01,	/* 0x3f317180 */
ln2_lo =   9.0580006145e-06,	/* 0x3717f7d1 */
two25 =    3.355443200e+07,	/* 0x4c000000 */
Lg1 = 6.6666668653e-01,	/* 3F2AAAAB */
Lg2 = 4.0000000596e-01,	/* 3ECCCCCD */
Lg3 = 2.8571429849e-01, /* 3E924925 */
Lg4 = 2.2222198546e-01, /* 3E638E29 */
Lg5 = 1.8183572590e-01, /* 3E3A3325 */
Lg6 = 1.5313838422e-01, /* 3E1CD04F */
Lg7 = 1.4798198640e-01; /* 3E178897 */

static const float zero   =  0.0;

/* A union which permits us to convert between a float and a 32 bit
   int.  */

typedef union
{
  float value;
  uint32_t word;
} ieee_float_shape_type;

/* Get a 32 bit int from a float.  */

#define GET_FLOAT_WORD(i,d)					\
do {								\
  ieee_float_shape_type gf_u;					\
  gf_u.value = (d);						\
  (i) = gf_u.word;						\
} while (0)

/* Set a float from a 32 bit int.  */

#define SET_FLOAT_WORD(d,i)					\
do {								\
  ieee_float_shape_type sf_u;					\
  sf_u.word = (i);						\
  (d) = sf_u.value;						\
} while (0)


float rb_log(float x)
{
    float hfsq, f, s, z, R, w, t1, t2, dk;
    int32_t k, ix, i, j;

    GET_FLOAT_WORD(ix,x);

    k=0;
    if (ix < 0x00800000) {          /* x < 2**-126  */
        if ((ix&0x7fffffff)==0)
            return -two25/(x-x);		/* log(+-0)=-inf */
        if (ix<0) return (x-x)/(x-x);	/* log(-#) = NaN */
        k -= 25; x *= two25; /* subnormal number, scale up x */
        GET_FLOAT_WORD(ix,x);
    }
    if (ix >= 0x7f800000) return x+x;
    k += (ix>>23)-127;
    ix &= 0x007fffff;
    i = (ix+(0x95f64<<3))&0x800000;
    SET_FLOAT_WORD(x,ix|(i^0x3f800000));	/* normalize x or x/2 */
    k += (i>>23);
    f = x-(float)1.0;
    if((0x007fffff&(15+ix))<16) {	/* |f| < 2**-20 */
        if(f==zero) {
            if(k==0)
                return zero;
            else
            {
                dk=(float)k;
                return dk*ln2_hi+dk*ln2_lo;
            }
        }
        R = f*f*((float)0.5-(float)0.33333333333333333*f);
        if(k==0)
            return f-R;
        else
        {
            dk=(float)k;
            return dk*ln2_hi-((R-dk*ln2_lo)-f);
        }
    }
    s = f/((float)2.0+f);
    dk = (float)k;
    z = s*s;
    i = ix-(0x6147a<<3);
    w = z*z;
    j = (0x6b851<<3)-ix;
    t1= w*(Lg2+w*(Lg4+w*Lg6));
    t2= z*(Lg1+w*(Lg3+w*(Lg5+w*Lg7)));
    i |= j;
    R = t2+t1;
    if(i>0) {
        hfsq=(float)0.5*f*f;
        if(k==0)
            return f-(hfsq-s*(hfsq+R));
        else
            return dk*ln2_hi-((hfsq-(s*(hfsq+R)+dk*ln2_lo))-f);
    } else {
        if(k==0)
            return f-s*(f-R);
        else
            return dk*ln2_hi-((s*(f-R)-dk*ln2_lo)-f);
    }
}


/* Logarithm for 10th base,
   taken from glibc-2.8 */

static const float
ivln10     =  4.3429449201e-01, /* 0x3ede5bd9 */
log10_2hi  =  3.0102920532e-01, /* 0x3e9a2080 */
log10_2lo  =  7.9034151668e-07; /* 0x355427db */

float rb_log10(float x)
{
	float y,z;
	int32_t i,k,hx;

	GET_FLOAT_WORD(hx,x);

        k=0;
        if (hx < 0x00800000) {			/* x < 2**-126  */
            if ((hx&0x7fffffff)==0)
                return -two25/(x-x);		/* log(+-0)=-inf */
            if (hx<0) return (x-x)/(x-x);	/* log(-#) = NaN */
            k -= 25; x *= two25; /* subnormal number, scale up x */
	    GET_FLOAT_WORD(hx,x);
        }
	if (hx >= 0x7f800000) return x+x;
	k += (hx>>23)-127;
	i  = ((uint32_t)k&0x80000000)>>31;
        hx = (hx&0x007fffff)|((0x7f-i)<<23);
        y  = (float)(k+i);
	SET_FLOAT_WORD(x,hx);
	z  = y*log10_2lo + ivln10*rb_log(x);
	return  z+y*log10_2hi;
}


/* Power function, taken from glibc-2.8 and dietlibc-0.32 */
static const float
huge = 1.0e+30,
tiny = 1.0e-30,
one  = 1.0f;

float rb_pow(float x, float y)
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



/* Square root function, original. */
float rb_sqrt(float x)
{
	float z;
	int32_t sign = (int)0x80000000; 
	int32_t ix,s,q,m,t,i;
	uint32_t r;

	GET_FLOAT_WORD(ix,x);

    /* take care of Inf and NaN */
	if((ix&0x7f800000)==0x7f800000) {			
	    return x*x+x;		/* sqrt(NaN)=NaN, sqrt(+inf)=+inf
					   sqrt(-inf)=sNaN */
	} 
    /* take care of zero */
	if(ix<=0) {
	    if((ix&(~sign))==0) return x;/* sqrt(+-0) = +-0 */
	    else if(ix<0)
		return (x-x)/(x-x);		/* sqrt(-ve) = sNaN */
	}
    /* normalize x */
	m = (ix>>23);
	if(m==0) {				/* subnormal x */
	    for(i=0;(ix&0x00800000)==0;i++) ix<<=1;
	    m -= i-1;
	}
	m -= 127;	/* unbias exponent */
	ix = (ix&0x007fffff)|0x00800000;
	if(m&1)	/* odd m, double x to make it even */
	    ix += ix;
	m >>= 1;	/* m = [m/2] */

    /* generate sqrt(x) bit by bit */
	ix += ix;
	q = s = 0;		/* q = sqrt(x) */
	r = 0x01000000;		/* r = moving bit from right to left */

	while(r!=0) {
	    t = s+r; 
	    if(t<=ix) { 
		s    = t+r; 
		ix  -= t; 
		q   += r; 
	    } 
	    ix += ix;
	    r>>=1;
	}

    /* use floating add to find out rounding direction */
	if(ix!=0) {
	    z = one-tiny; /* trigger inexact flag */
	    if (z>=one) {
	        z = one+tiny;
		if (z>one)
		    q += 2;
		else
		    q += (q&1);
	    }
	}
	ix = (q>>1)+0x3f000000;
	ix += (m <<23);
	SET_FLOAT_WORD(z,ix);
	return z;
}

/* Absolute value, simple calculus */
float rb_fabs(float x)
{
    return (x < 0.0f) ? -x : x;
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


float rb_atan(float x)
{
	float w,s1,s2,z;
	int32_t ix,hx,id;

	GET_FLOAT_WORD(hx,x);
	ix = hx&0x7fffffff;
	if(ix>=0x50800000) {	/* if |x| >= 2^34 */
	    if(ix>0x7f800000)
		return x+x;		/* NaN */
	    if(hx>0) return  atanhi[3]+atanlo[3];
	    else     return -atanhi[3]-atanlo[3];
	} if (ix < 0x3ee00000) {	/* |x| < 0.4375 */
	    if (ix < 0x31000000) {	/* |x| < 2^-29 */
		if(huge+x>one) return x;	/* raise inexact */
	    }
	    id = -1;
	} else {
	x = rb_fabs(x);
	if (ix < 0x3f980000) {		/* |x| < 1.1875 */
	    if (ix < 0x3f300000) {	/* 7/16 <=|x|<11/16 */
		id = 0; x = ((float)2.0*x-one)/((float)2.0+x); 
	    } else {			/* 11/16<=|x|< 19/16 */
		id = 1; x  = (x-one)/(x+one); 
	    }
	} else {
	    if (ix < 0x401c0000) {	/* |x| < 2.4375 */
		id = 2; x  = (x-(float)1.5)/(one+(float)1.5*x);
	    } else {			/* 2.4375 <= |x| < 2^66 */
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

float rb_atan2(float x, float y)
{
	float z;
	int32_t k,m,hx,hy,ix,iy;

	GET_FLOAT_WORD(hx,x);
	ix = hx&0x7fffffff;
	GET_FLOAT_WORD(hy,y);
	iy = hy&0x7fffffff;
	if((ix>0x7f800000)||
	   (iy>0x7f800000))	/* x or y is NaN */
	   return x+y;
	if(hx==0x3f800000) return rb_atan(y);   /* x=1.0 */
	m = ((hy>>31)&1)|((hx>>30)&2);	/* 2*sign(x)+sign(y) */

    /* when y = 0 */
	if(iy==0) {
	    switch(m) {
		case 0:
		case 1: return y; 	/* atan(+-0,+anything)=+-0 */
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
		    case 0: return  zero  ;	/* atan(+...,+INF) */
		    case 1: return -zero  ;	/* atan(-...,+INF) */
		    case 2: return  pi+tiny  ;	/* atan(+...,-INF) */
		    case 3: return -pi-tiny  ;	/* atan(-...,-INF) */
		}
	    }
	}
    /* when y is INF */
	if(iy==0x7f800000) return (hy<0)? -pi_o_2-tiny: pi_o_2+tiny;

    /* compute y/x */
	k = (iy-ix)>>23;
	if(k > 60) z=pi_o_2+(float)0.5*pi_lo; 	/* |y/x| >  2**60 */
	else if(hx<0&&k<-60) z=0.0; 	/* |y|/x < -2**60 */
	else z=rb_atan(rb_fabs(y/x));	/* safe to do y/x */
	switch (m) {
	    case 0: return       z  ;	/* atan(+,+) */
	    case 1: {
	    	      uint32_t zh;
		      GET_FLOAT_WORD(zh,z);
		      SET_FLOAT_WORD(z,zh ^ 0x80000000);
		    }
		    return       z  ;	/* atan(-,+) */
	    case 2: return  pi-(z-pi_lo);/* atan(+,-) */
	    default: /* case 3 */
	    	    return  (z-pi_lo)-pi;/* atan(-,-) */
	}
}


/* Sine hyperbolic, taken from dietlibc-0.32 */
float rb_sinh(float x)
{
    float y = rb_exp(x);
    return (y - 1.0f / y) * 0.5f;
}


/* Tangent, simple calculus solution. */
float rb_tan(float x)
{
    return rb_sin(x) / rb_cos(x);
}



/* Exponential function,
   taken from glibc-2.8
   As it uses double values and udefines some symbols,
   it was moved to the end of the source code */

#define W52 (2.22044605e-16)
#define W55 (2.77555756e-17)
#define W58 (3.46944695e-18)
#define W59 (1.73472348e-18)
#define W60 (8.67361738e-19)
const float __exp_deltatable[178] = {
         0*W60,  16558714*W60, -10672149*W59,   1441652*W60,
 -15787963*W55,    462888*W60,   7291806*W60,   1698880*W60,
 -14375103*W58,  -2021016*W60,    728829*W60,  -3759654*W60,
   3202123*W60, -10916019*W58,   -251570*W60,  -1043086*W60,
   8207536*W60,   -409964*W60,  -5993931*W60,   -475500*W60,
   2237522*W60,    324170*W60,   -244117*W60,     32077*W60,
    123907*W60,  -1019734*W60,      -143*W60,    813077*W60,
    743345*W60,    462461*W60,    629794*W60,   2125066*W60,
  -2339121*W60,   -337951*W60,   9922067*W60,   -648704*W60,
    149407*W60,  -2687209*W60,   -631608*W60,   2128280*W60,
  -4882082*W60,   2001360*W60,    175074*W60,   2923216*W60,
   -538947*W60,  -1212193*W60,  -1920926*W60,  -1080577*W60,
   3690196*W60,   2643367*W60,   2911937*W60,    671455*W60,
  -1128674*W60,    593282*W60,  -5219347*W60,  -1941490*W60,
  11007953*W60,    239609*W60,  -2969658*W60,  -1183650*W60,
    942998*W60,    699063*W60,    450569*W60,   -329250*W60,
  -7257875*W60,   -312436*W60,     51626*W60,    555877*W60,
   -641761*W60,   1565666*W60,    884327*W60, -10960035*W60,
  -2004679*W60,   -995793*W60,  -2229051*W60,   -146179*W60,
   -510327*W60,   1453482*W60,  -3778852*W60,  -2238056*W60,
  -4895983*W60,   3398883*W60,   -252738*W60,   1230155*W60,
    346918*W60,   1109352*W60,    268941*W60,  -2930483*W60,
  -1036263*W60,  -1159280*W60,   1328176*W60,   2937642*W60,
  -9371420*W60,  -6902650*W60,  -1419134*W60,   1442904*W60,
  -1319056*W60,    -16369*W60,    696555*W60,   -279987*W60,
  -7919763*W60,    252741*W60,    459711*W60,  -1709645*W60,
    354913*W60,   6025867*W60,   -421460*W60,   -853103*W60,
   -338649*W60,    962151*W60,    955965*W60,    784419*W60,
  -3633653*W60,   2277133*W60,  -8847927*W52,   1223028*W60,
   5907079*W60,    623167*W60,   5142888*W60,   2599099*W60,
   1214280*W60,   4870359*W60,    593349*W60,    -57705*W60,
   7761209*W60,  -5564097*W60,   2051261*W60,   6216869*W60,
   4692163*W60,    601691*W60,  -5264906*W60,   1077872*W60,
  -3205949*W60,   1833082*W60,   2081746*W60,   -987363*W60,
  -1049535*W60,   2015244*W60,    874230*W60,   2168259*W60,
  -1740124*W60, -10068269*W60,    -18242*W60,  -3013583*W60,
    580601*W60,  -2547161*W60,   -535689*W60,   2220815*W60,
   1285067*W60,   2806933*W60,   -983086*W60,  -1729097*W60,
  -1162985*W60,  -2561904*W60,    801988*W60,    244351*W60,
   1441893*W60,  -7517981*W60,    271781*W60, -15021588*W60,
  -2341588*W60,   -919198*W60,   1642232*W60,   4771771*W60,
  -1220099*W60,  -3062372*W60,    628624*W60,   1278114*W60,
  13083513*W60, -10521925*W60,   3180310*W60,  -1659307*W60,
   3543773*W60,   2501203*W60,      4151*W60,   -340748*W60,
  -2285625*W60,   2495202*W60
};

const double __exp_atable[355] /* __attribute__((mode(DF))) */ = {
 0.707722561055888932371, /* 0x0.b52d4e46605c27ffd */
 0.709106182438804188967, /* 0x0.b587fb96f75097ffb */
 0.710492508843861281234, /* 0x0.b5e2d649899167ffd */
 0.711881545564593931623, /* 0x0.b63dde74d36bdfffe */
 0.713273297897442870573, /* 0x0.b699142f945f87ffc */
 0.714667771153751463236, /* 0x0.b6f477909c4ea0001 */
 0.716064970655995725059, /* 0x0.b75008aec758f8004 */
 0.717464901723956938193, /* 0x0.b7abc7a0eea7e0002 */
 0.718867569715736398602, /* 0x0.b807b47e1586c7ff8 */
 0.720272979947266023271, /* 0x0.b863cf5d10e380003 */
 0.721681137825144314297, /* 0x0.b8c01855195c37ffb */
 0.723092048691992950199, /* 0x0.b91c8f7d213740004 */
 0.724505717938892290800, /* 0x0.b97934ec5002d0007 */
 0.725922150953176470431, /* 0x0.b9d608b9c92ea7ffc */
 0.727341353138962865022, /* 0x0.ba330afcc29e98003 */
 0.728763329918453162104, /* 0x0.ba903bcc8618b7ffc */
 0.730188086709957051568, /* 0x0.baed9b40591ba0000 */
 0.731615628948127705309, /* 0x0.bb4b296f931e30002 */
 0.733045962086486091436, /* 0x0.bba8e671a05617ff9 */
 0.734479091556371366251, /* 0x0.bc06d25dd49568001 */
 0.735915022857225542529, /* 0x0.bc64ed4bce8f6fff9 */
 0.737353761441304711410, /* 0x0.bcc33752f915d7ff9 */
 0.738795312814142124419, /* 0x0.bd21b08af98e78005 */
 0.740239682467211168593, /* 0x0.bd80590b65e9a8000 */
 0.741686875913991849885, /* 0x0.bddf30ebec4a10000 */
 0.743136898669507939299, /* 0x0.be3e38443c84e0007 */
 0.744589756269486091620, /* 0x0.be9d6f2c1d32a0002 */
 0.746045454254026796384, /* 0x0.befcd5bb59baf8004 */
 0.747503998175051087583, /* 0x0.bf5c6c09ca84c0003 */
 0.748965393601880857739, /* 0x0.bfbc322f5b18b7ff8 */
 0.750429646104262104698, /* 0x0.c01c2843f776fffff */
 0.751896761271877989160, /* 0x0.c07c4e5fa18b88002 */
 0.753366744698445112140, /* 0x0.c0dca49a5fb18fffd */
 0.754839601988627206827, /* 0x0.c13d2b0c444db0005 */
 0.756315338768691947122, /* 0x0.c19de1cd798578006 */
 0.757793960659406629066, /* 0x0.c1fec8f623723fffd */
 0.759275473314173443536, /* 0x0.c25fe09e8a0f47ff8 */
 0.760759882363831851927, /* 0x0.c2c128dedc88f8000 */
 0.762247193485956486805, /* 0x0.c322a1cf7d6e7fffa */
 0.763737412354726363781, /* 0x0.c3844b88cb9347ffc */
 0.765230544649828092739, /* 0x0.c3e626232bd8f7ffc */
 0.766726596071518051729, /* 0x0.c44831b719bf18002 */
 0.768225572321911687194, /* 0x0.c4aa6e5d12d078001 */
 0.769727479119219348810, /* 0x0.c50cdc2da64a37ffb */
 0.771232322196981678892, /* 0x0.c56f7b41744490001 */
 0.772740107296721268087, /* 0x0.c5d24bb1259e70004 */
 0.774250840160724651565, /* 0x0.c6354d95640dd0007 */
 0.775764526565368872643, /* 0x0.c6988106fec447fff */
 0.777281172269557396602, /* 0x0.c6fbe61eb1bd0ffff */
 0.778800783068235302750, /* 0x0.c75f7cf560942fffc */
 0.780323364758801041312, /* 0x0.c7c345a3f1983fffe */
 0.781848923151573727006, /* 0x0.c8274043594cb0002 */
 0.783377464064598849602, /* 0x0.c88b6cec94b3b7ff9 */
 0.784908993312207869935, /* 0x0.c8efcbb89cba27ffe */
 0.786443516765346961618, /* 0x0.c9545cc0a88c70003 */
 0.787981040257604625744, /* 0x0.c9b9201dc643bfffa */
 0.789521569657452682047, /* 0x0.ca1e15e92a5410007 */
 0.791065110849462849192, /* 0x0.ca833e3c1ae510005 */
 0.792611669712891875319, /* 0x0.cae8992fd84667ffd */
 0.794161252150049179450, /* 0x0.cb4e26ddbc207fff8 */
 0.795713864077794763584, /* 0x0.cbb3e75f301b60003 */
 0.797269511407239561694, /* 0x0.cc19dacd978cd8002 */
 0.798828200086368567220, /* 0x0.cc8001427e55d7ffb */
 0.800389937624300440456, /* 0x0.cce65ade24d360006 */
 0.801954725261124767840, /* 0x0.cd4ce7a5de839fffb */
 0.803522573691593189330, /* 0x0.cdb3a7c79a678fffd */
 0.805093487311204114563, /* 0x0.ce1a9b563965ffffc */
 0.806667472122675088819, /* 0x0.ce81c26b838db8000 */
 0.808244534127439906441, /* 0x0.cee91d213f8428002 */
 0.809824679342317166307, /* 0x0.cf50ab9144d92fff9 */
 0.811407913793616542005, /* 0x0.cfb86dd5758c2ffff */
 0.812994243520784198882, /* 0x0.d0206407c20e20005 */
 0.814583674571603966162, /* 0x0.d0888e4223facfff9 */
 0.816176213022088536960, /* 0x0.d0f0ec9eb3f7c8002 */
 0.817771864936188586101, /* 0x0.d1597f377d6768002 */
 0.819370636400374108252, /* 0x0.d1c24626a46eafff8 */
 0.820972533518165570298, /* 0x0.d22b41865ff1e7ff9 */
 0.822577562404315121269, /* 0x0.d2947170f32ec7ff9 */
 0.824185729164559344159, /* 0x0.d2fdd60097795fff8 */
 0.825797039949601741075, /* 0x0.d3676f4fb796d0001 */
 0.827411500902565544264, /* 0x0.d3d13d78b5f68fffb */
 0.829029118181348834154, /* 0x0.d43b40960546d8001 */
 0.830649897953322891022, /* 0x0.d4a578c222a058000 */
 0.832273846408250750368, /* 0x0.d50fe617a3ba78005 */
 0.833900969738858188772, /* 0x0.d57a88b1218e90002 */
 0.835531274148056613016, /* 0x0.d5e560a94048f8006 */
 0.837164765846411529371, /* 0x0.d6506e1aac8078003 */
 0.838801451086016225394, /* 0x0.d6bbb1204074e0001 */
 0.840441336100884561780, /* 0x0.d72729d4c28518004 */
 0.842084427144139224814, /* 0x0.d792d8530e12b0001 */
 0.843730730487052604790, /* 0x0.d7febcb61273e7fff */
 0.845380252404570153833, /* 0x0.d86ad718c308dfff9 */
 0.847032999194574087728, /* 0x0.d8d727962c69d7fff */
 0.848688977161248581090, /* 0x0.d943ae49621ce7ffb */
 0.850348192619261200615, /* 0x0.d9b06b4d832ef8005 */
 0.852010651900976245816, /* 0x0.da1d5ebdc22220005 */
 0.853676361342631029337, /* 0x0.da8a88b555baa0006 */
 0.855345327311054837175, /* 0x0.daf7e94f965f98004 */
 0.857017556155879489641, /* 0x0.db6580a7c98f7fff8 */
 0.858693054267390953857, /* 0x0.dbd34ed9617befff8 */
 0.860371828028939855647, /* 0x0.dc4153ffc8b65fff9 */
 0.862053883854957292436, /* 0x0.dcaf90368bfca8004 */
 0.863739228154875360306, /* 0x0.dd1e0399328d87ffe */
 0.865427867361348468455, /* 0x0.dd8cae435d303fff9 */
 0.867119807911702289458, /* 0x0.ddfb9050b1cee8006 */
 0.868815056264353846599, /* 0x0.de6aa9dced8448001 */
 0.870513618890481399881, /* 0x0.ded9fb03db7320006 */
 0.872215502247877139094, /* 0x0.df4983e1380657ff8 */
 0.873920712852848668986, /* 0x0.dfb94490ffff77ffd */
 0.875629257204025623884, /* 0x0.e0293d2f1cb01fff9 */
 0.877341141814212965880, /* 0x0.e0996dd786fff0007 */
 0.879056373217612985183, /* 0x0.e109d6a64f5d57ffc */
 0.880774957955916648615, /* 0x0.e17a77b78e72a7ffe */
 0.882496902590150900078, /* 0x0.e1eb5127722cc7ff8 */
 0.884222213673356738383, /* 0x0.e25c63121fb0c8006 */
 0.885950897802399772740, /* 0x0.e2cdad93ec5340003 */
 0.887682961567391237685, /* 0x0.e33f30c925fb97ffb */
 0.889418411575228162725, /* 0x0.e3b0ecce2d05ffff9 */
 0.891157254447957902797, /* 0x0.e422e1bf727718006 */
 0.892899496816652704641, /* 0x0.e4950fb9713fc7ffe */
 0.894645145323828439008, /* 0x0.e50776d8b0e60fff8 */
 0.896394206626591749641, /* 0x0.e57a1739c8fadfffc */
 0.898146687421414902124, /* 0x0.e5ecf0f97c5798007 */
 0.899902594367530173098, /* 0x0.e660043464e378005 */
 0.901661934163603406867, /* 0x0.e6d3510747e150006 */
 0.903424713533971135418, /* 0x0.e746d78f06cd97ffd */
 0.905190939194458810123, /* 0x0.e7ba97e879c91fffc */
 0.906960617885092856864, /* 0x0.e82e92309390b0007 */
 0.908733756358986566306, /* 0x0.e8a2c6845544afffa */
 0.910510361377119825629, /* 0x0.e9173500c8abc7ff8 */
 0.912290439722343249336, /* 0x0.e98bddc30f98b0002 */
 0.914073998177417412765, /* 0x0.ea00c0e84bc4c7fff */
 0.915861043547953501680, /* 0x0.ea75de8db8094fffe */
 0.917651582652244779397, /* 0x0.eaeb36d09d3137ffe */
 0.919445622318405764159, /* 0x0.eb60c9ce4ed3dffff */
 0.921243169397334638073, /* 0x0.ebd697a43995b0007 */
 0.923044230737526172328, /* 0x0.ec4ca06fc7768fffa */
 0.924848813220121135342, /* 0x0.ecc2e44e865b6fffb */
 0.926656923710931002014, /* 0x0.ed39635df34e70006 */
 0.928468569126343790092, /* 0x0.edb01dbbc2f5b7ffa */
 0.930283756368834757725, /* 0x0.ee2713859aab57ffa */
 0.932102492359406786818, /* 0x0.ee9e44d9342870004 */
 0.933924784042873379360, /* 0x0.ef15b1d4635438005 */
 0.935750638358567643520, /* 0x0.ef8d5a94f60f50007 */
 0.937580062297704630580, /* 0x0.f0053f38f345cffff */
 0.939413062815381727516, /* 0x0.f07d5fde3a2d98001 */
 0.941249646905368053689, /* 0x0.f0f5bca2d481a8004 */
 0.943089821583810716806, /* 0x0.f16e55a4e497d7ffe */
 0.944933593864477061592, /* 0x0.f1e72b028a2827ffb */
 0.946780970781518460559, /* 0x0.f2603cd9fb5430001 */
 0.948631959382661205081, /* 0x0.f2d98b497d2a87ff9 */
 0.950486566729423554277, /* 0x0.f353166f63e3dffff */
 0.952344799896018723290, /* 0x0.f3ccde6a11ae37ffe */
 0.954206665969085765512, /* 0x0.f446e357f66120000 */
 0.956072172053890279009, /* 0x0.f4c12557964f0fff9 */
 0.957941325265908139014, /* 0x0.f53ba48781046fffb */
 0.959814132734539637840, /* 0x0.f5b66106555d07ffa */
 0.961690601603558903308, /* 0x0.f6315af2c2027fffc */
 0.963570739036113010927, /* 0x0.f6ac926b8aeb80004 */
 0.965454552202857141381, /* 0x0.f728078f7c5008002 */
 0.967342048278315158608, /* 0x0.f7a3ba7d66a908001 */
 0.969233234469444204768, /* 0x0.f81fab543e1897ffb */
 0.971128118008140250896, /* 0x0.f89bda33122c78007 */
 0.973026706099345495256, /* 0x0.f9184738d4cf97ff8 */
 0.974929006031422851235, /* 0x0.f994f284d3a5c0008 */
 0.976835024947348973265, /* 0x0.fa11dc35bc7820002 */
 0.978744770239899142285, /* 0x0.fa8f046b4fb7f8007 */
 0.980658249138918636210, /* 0x0.fb0c6b449ab1cfff9 */
 0.982575468959622777535, /* 0x0.fb8a10e1088fb7ffa */
 0.984496437054508843888, /* 0x0.fc07f5602d79afffc */
 0.986421160608523028820, /* 0x0.fc8618e0e55e47ffb */
 0.988349647107594098099, /* 0x0.fd047b83571b1fffa */
 0.990281903873210800357, /* 0x0.fd831d66f4c018002 */
 0.992217938695037382475, /* 0x0.fe01fead3320bfff8 */
 0.994157757657894713987, /* 0x0.fe811f703491e8006 */
 0.996101369488558541238, /* 0x0.ff007fd5744490005 */
 0.998048781093141101932, /* 0x0.ff801ffa9b9280007 */
 1.000000000000000000000, /* 0x1.00000000000000000 */
 1.001955033605393285965, /* 0x1.0080200565d29ffff */
 1.003913889319761887310, /* 0x1.0100802aa0e80fff0 */
 1.005876574715736104818, /* 0x1.01812090377240007 */
 1.007843096764807100351, /* 0x1.020201541aad7fff6 */
 1.009813464316352327214, /* 0x1.0283229c4c9820007 */
 1.011787683565730677817, /* 0x1.030484836910a000e */
 1.013765762469146736174, /* 0x1.0386272b9c077fffe */
 1.015747708536026694351, /* 0x1.04080ab526304fff0 */
 1.017733529475172815584, /* 0x1.048a2f412375ffff0 */
 1.019723232714418781378, /* 0x1.050c94ef7ad5e000a */
 1.021716825883923762690, /* 0x1.058f3be0f1c2d0004 */
 1.023714316605201180057, /* 0x1.06122436442e2000e */
 1.025715712440059545995, /* 0x1.06954e0fec63afff2 */
 1.027721021151397406936, /* 0x1.0718b98f41c92fff6 */
 1.029730250269221158939, /* 0x1.079c66d49bb2ffff1 */
 1.031743407506447551857, /* 0x1.082056011a9230009 */
 1.033760500517691527387, /* 0x1.08a487359ebd50002 */
 1.035781537016238873464, /* 0x1.0928fa93490d4fff3 */
 1.037806524719013578963, /* 0x1.09adb03b3e5b3000d */
 1.039835471338248051878, /* 0x1.0a32a84e9e5760004 */
 1.041868384612101516848, /* 0x1.0ab7e2eea5340ffff */
 1.043905272300907460835, /* 0x1.0b3d603ca784f0009 */
 1.045946142174331239262, /* 0x1.0bc3205a042060000 */
 1.047991002016745332165, /* 0x1.0c4923682a086fffe */
 1.050039859627715177527, /* 0x1.0ccf698898f3a000d */
 1.052092722826109660856, /* 0x1.0d55f2dce5d1dfffb */
 1.054149599440827866881, /* 0x1.0ddcbf86b09a5fff6 */
 1.056210497317612961855, /* 0x1.0e63cfa7abc97fffd */
 1.058275424318780855142, /* 0x1.0eeb23619c146fffb */
 1.060344388322010722446, /* 0x1.0f72bad65714bffff */
 1.062417397220589476718, /* 0x1.0ffa9627c38d30004 */
 1.064494458915699715017, /* 0x1.1082b577d0eef0003 */
 1.066575581342167566880, /* 0x1.110b18e893a90000a */
 1.068660772440545025953, /* 0x1.1193c09c267610006 */
 1.070750040138235936705, /* 0x1.121cacb4959befff6 */
 1.072843392435016474095, /* 0x1.12a5dd543cf36ffff */
 1.074940837302467588937, /* 0x1.132f529d59552000b */
 1.077042382749654914030, /* 0x1.13b90cb250d08fff5 */
 1.079148036789447484528, /* 0x1.14430bb58da3dfff9 */
 1.081257807444460983297, /* 0x1.14cd4fc984c4a000e */
 1.083371702785017154417, /* 0x1.1557d910df9c7000e */
 1.085489730853784307038, /* 0x1.15e2a7ae292d30002 */
 1.087611899742884524772, /* 0x1.166dbbc422d8c0004 */
 1.089738217537583819804, /* 0x1.16f9157586772ffff */
 1.091868692357631731528, /* 0x1.1784b4e533cacfff0 */
 1.094003332327482702577, /* 0x1.18109a360fc23fff2 */
 1.096142145591650907149, /* 0x1.189cc58b155a70008 */
 1.098285140311341168136, /* 0x1.1929370751ea50002 */
 1.100432324652149906842, /* 0x1.19b5eecdd79cefff0 */
 1.102583706811727015711, /* 0x1.1a42ed01dbdba000e */
 1.104739294993289488947, /* 0x1.1ad031c69a2eafff0 */
 1.106899097422573863281, /* 0x1.1b5dbd3f66e120003 */
 1.109063122341542140286, /* 0x1.1beb8f8fa8150000b */
 1.111231377994659874592, /* 0x1.1c79a8dac6ad0fff4 */
 1.113403872669181282605, /* 0x1.1d0809445a97ffffc */
 1.115580614653132185460, /* 0x1.1d96b0effc9db000e */
 1.117761612217810673898, /* 0x1.1e25a001332190000 */
 1.119946873713312474002, /* 0x1.1eb4d69bdb2a9fff1 */
 1.122136407473298902480, /* 0x1.1f4454e3bfae00006 */
 1.124330221845670330058, /* 0x1.1fd41afcbb48bfff8 */
 1.126528325196519908506, /* 0x1.2064290abc98c0001 */
 1.128730725913251964394, /* 0x1.20f47f31c9aa7000f */
 1.130937432396844410880, /* 0x1.21851d95f776dfff0 */
 1.133148453059692917203, /* 0x1.2216045b6784efffa */
 1.135363796355857157764, /* 0x1.22a733a6692ae0004 */
 1.137583470716100553249, /* 0x1.2338ab9b3221a0004 */
 1.139807484614418608939, /* 0x1.23ca6c5e27aadfff7 */
 1.142035846532929888057, /* 0x1.245c7613b7f6c0004 */
 1.144268564977221958089, /* 0x1.24eec8e06b035000c */
 1.146505648458203463465, /* 0x1.258164e8cea85fff8 */
 1.148747105501412235671, /* 0x1.26144a5180d380009 */
 1.150992944689175123667, /* 0x1.26a7793f5de2efffa */
 1.153243174560058870217, /* 0x1.273af1d712179000d */
 1.155497803703682491111, /* 0x1.27ceb43d81d42fff1 */
 1.157756840726344771440, /* 0x1.2862c097a3d29000c */
 1.160020294239811677834, /* 0x1.28f7170a74cf4fff1 */
 1.162288172883275239058, /* 0x1.298bb7bb0faed0004 */
 1.164560485298402170388, /* 0x1.2a20a2ce920dffff4 */
 1.166837240167474476460, /* 0x1.2ab5d86a4631ffff6 */
 1.169118446164539637555, /* 0x1.2b4b58b36d5220009 */
 1.171404112007080167155, /* 0x1.2be123cf786790002 */
 1.173694246390975415341, /* 0x1.2c7739e3c0aac000d */
 1.175988858069749065617, /* 0x1.2d0d9b15deb58fff6 */
 1.178287955789017793514, /* 0x1.2da4478b627040002 */
 1.180591548323240091978, /* 0x1.2e3b3f69fb794fffc */
 1.182899644456603782686, /* 0x1.2ed282d76421d0004 */
 1.185212252993012693694, /* 0x1.2f6a11f96c685fff3 */
 1.187529382762033236513, /* 0x1.3001ecf60082ffffa */
 1.189851042595508889847, /* 0x1.309a13f30f28a0004 */
 1.192177241354644978669, /* 0x1.31328716a758cfff7 */
 1.194507987909589896687, /* 0x1.31cb4686e1e85fffb */
 1.196843291137896336843, /* 0x1.32645269dfd04000a */
 1.199183159977805113226, /* 0x1.32fdaae604c39000f */
 1.201527603343041317132, /* 0x1.339750219980dfff3 */
 1.203876630171082595692, /* 0x1.3431424300e480007 */
 1.206230249419600664189, /* 0x1.34cb8170b3fee000e */
 1.208588470077065268869, /* 0x1.35660dd14dbd4fffc */
 1.210951301134513435915, /* 0x1.3600e78b6bdfc0005 */
 1.213318751604272271958, /* 0x1.369c0ec5c38ebfff2 */
 1.215690830512196507537, /* 0x1.373783a718d29000f */
 1.218067546930756250870, /* 0x1.37d3465662f480007 */
 1.220448909901335365929, /* 0x1.386f56fa770fe0008 */
 1.222834928513994334780, /* 0x1.390bb5ba5fc540004 */
 1.225225611877684750397, /* 0x1.39a862bd3c7a8fff3 */
 1.227620969111500981433, /* 0x1.3a455e2a37bcafffd */
 1.230021009336254911271, /* 0x1.3ae2a8287dfbefff6 */
 1.232425741726685064472, /* 0x1.3b8040df76f39fffa */
 1.234835175450728295084, /* 0x1.3c1e287682e48fff1 */
 1.237249319699482263931, /* 0x1.3cbc5f151b86bfff8 */
 1.239668183679933477545, /* 0x1.3d5ae4e2cc0a8000f */
 1.242091776620540377629, /* 0x1.3df9ba07373bf0006 */
 1.244520107762172811399, /* 0x1.3e98deaa0d8cafffe */
 1.246953186383919165383, /* 0x1.3f3852f32973efff0 */
 1.249391019292643401078, /* 0x1.3fd816ffc72b90001 */
 1.251833623164381181797, /* 0x1.40782b17863250005 */
 1.254280999953110153911, /* 0x1.41188f42caf400000 */
 1.256733161434815393410, /* 0x1.41b943b42945bfffd */
 1.259190116985283935980, /* 0x1.425a4893e5f10000a */
 1.261651875958665236542, /* 0x1.42fb9e0a2df4c0009 */
 1.264118447754797758244, /* 0x1.439d443f608c4fff9 */
 1.266589841787181258708, /* 0x1.443f3b5bebf850008 */
 1.269066067469190262045, /* 0x1.44e183883e561fff7 */
 1.271547134259576328224, /* 0x1.45841cecf7a7a0001 */
 1.274033051628237434048, /* 0x1.462707b2c43020009 */
 1.276523829025464573684, /* 0x1.46ca44023aa410007 */
 1.279019475999373156531, /* 0x1.476dd2045d46ffff0 */
 1.281520002043128991825, /* 0x1.4811b1e1f1f19000b */
 1.284025416692967214122, /* 0x1.48b5e3c3edd74fff4 */
 1.286535729509738823464, /* 0x1.495a67d3613c8fff7 */
 1.289050950070396384145, /* 0x1.49ff3e396e19d000b */
 1.291571087985403654081, /* 0x1.4aa4671f5b401fff1 */
 1.294096152842774794011, /* 0x1.4b49e2ae56d19000d */
 1.296626154297237043484, /* 0x1.4befb10fd84a3fff4 */
 1.299161101984141142272, /* 0x1.4c95d26d41d84fff8 */
 1.301701005575179204100, /* 0x1.4d3c46f01d9f0fff3 */
 1.304245874766450485904, /* 0x1.4de30ec21097d0003 */
 1.306795719266019562007, /* 0x1.4e8a2a0ccce3d0002 */
 1.309350548792467483458, /* 0x1.4f3198fa10346fff5 */
 1.311910373099227200545, /* 0x1.4fd95bb3be8cffffd */
 1.314475201942565174546, /* 0x1.50817263bf0e5fffb */
 1.317045045107389400535, /* 0x1.5129dd3418575000e */
 1.319619912422941299109, /* 0x1.51d29c4f01c54ffff */
 1.322199813675649204855, /* 0x1.527bafde83a310009 */
 1.324784758729532718739, /* 0x1.5325180cfb8b3fffd */
 1.327374757430096474625, /* 0x1.53ced504b2bd0fff4 */
 1.329969819671041886272, /* 0x1.5478e6f02775e0001 */
 1.332569955346704748651, /* 0x1.55234df9d8a59fff8 */
 1.335175174370685002822, /* 0x1.55ce0a4c5a6a9fff6 */
 1.337785486688218616860, /* 0x1.56791c1263abefff7 */
 1.340400902247843806217, /* 0x1.57248376aef21fffa */
 1.343021431036279800211, /* 0x1.57d040a420c0bfff3 */
 1.345647083048053138662, /* 0x1.587c53c5a630f0002 */
 1.348277868295411074918, /* 0x1.5928bd063fd7bfff9 */
 1.350913796821875845231, /* 0x1.59d57c9110ad60006 */
 1.353554878672557082439, /* 0x1.5a8292913d68cfffc */
 1.356201123929036356254, /* 0x1.5b2fff3212db00007 */
 1.358852542671913132777, /* 0x1.5bddc29edcc06fff3 */
 1.361509145047255398051, /* 0x1.5c8bdd032ed16000f */
 1.364170941142184734180, /* 0x1.5d3a4e8a5bf61fff4 */
 1.366837941171020309735, /* 0x1.5de9176042f1effff */
 1.369510155261156381121, /* 0x1.5e9837b062f4e0005 */
 1.372187593620959988833, /* 0x1.5f47afa69436cfff1 */
 1.374870266463378287715, /* 0x1.5ff77f6eb3f8cfffd */
 1.377558184010425845733, /* 0x1.60a7a734a9742fff9 */
 1.380251356531521533853, /* 0x1.6158272490016000c */
 1.382949794301995272203, /* 0x1.6208ff6a8978a000f */
 1.385653507605306700170, /* 0x1.62ba3032c0a280004 */
 1.388362506772382154503, /* 0x1.636bb9a994784000f */
 1.391076802081129493127, /* 0x1.641d9bfb29a7bfff6 */
 1.393796403973427855412, /* 0x1.64cfd7545928b0002 */
 1.396521322756352656542, /* 0x1.65826be167badfff8 */
 1.399251568859207761660, /* 0x1.663559cf20826000c */
 1.401987152677323100733, /* 0x1.66e8a14a29486fffc */
 1.404728084651919228815, /* 0x1.679c427f5a4b6000b */
 1.407474375243217723560, /* 0x1.68503d9ba0add000f */
 1.410226034922914983815, /* 0x1.690492cbf6303fff9 */
 1.412983074197955213304, /* 0x1.69b9423d7b548fff6 */
};

/* All floating-point numbers can be put in one of these categories.  */
enum
  {
    FP_NAN,
# define FP_NAN FP_NAN
    FP_INFINITE,
# define FP_INFINITE FP_INFINITE
    FP_ZERO,
# define FP_ZERO FP_ZERO
    FP_SUBNORMAL,
# define FP_SUBNORMAL FP_SUBNORMAL
    FP_NORMAL
# define FP_NORMAL FP_NORMAL
  };


int
__fpclassifyf (float x)
{
  uint32_t wx;
  int retval = FP_NORMAL;

  GET_FLOAT_WORD (wx, x);
  wx &= 0x7fffffff;
  if (wx == 0)
    retval = FP_ZERO;
  else if (wx < 0x800000)
    retval = FP_SUBNORMAL;
  else if (wx >= 0x7f800000)
    retval = wx > 0x7f800000 ? FP_NAN : FP_INFINITE;

  return retval;
}


int
__isinff (float x)
{
	int32_t ix,t;
	GET_FLOAT_WORD(ix,x);
	t = ix & 0x7fffffff;
	t ^= 0x7f800000;
	t |= -t;
	return ~(t >> 31) & (ix >> 30);
}

/* Return nonzero value if arguments are unordered.  */
#  define fpclassify(x) \
     (sizeof (x) == sizeof (float) ? __fpclassifyf (x) : __fpclassifyf (x))

# ifndef isunordered
#  define isunordered(u, v) \
  (__extension__							      \
   ({ __typeof__(u) __u = (u); __typeof__(v) __v = (v);			      \
      fpclassify (__u) == FP_NAN || fpclassify (__v) == FP_NAN; }))
# endif

/* Return nonzero value if X is less than Y.  */
# ifndef isless
#  define isless(x, y) \
  (__extension__							      \
   ({ __typeof__(x) __x = (x); __typeof__(y) __y = (y);			      \
      !isunordered (__x, __y) && __x < __y; }))
# endif

/* Return nonzero value if X is greater than Y.  */
# ifndef isgreater
#  define isgreater(x, y) \
  (__extension__							      \
   ({ __typeof__(x) __x = (x); __typeof__(y) __y = (y);			      \
      !isunordered (__x, __y) && __x > __y; }))
# endif

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
  static const float himark = 88.72283935546875;
  static const float lomark = -103.972084045410;
  /* Check for usual case.  */
  if (isless (x, himark) && isgreater (x, lomark))
    {
      static const float THREEp42 = 13194139533312.0;
      static const float THREEp22 = 12582912.0;
      /* 1/ln(2).  */
#undef M_1_LN2
      static const float M_1_LN2 = 1.44269502163f;
      /* ln(2) */
#undef M_LN2
      static const double M_LN2 = .6931471805599452862;

      int tval;
      double x22, t, result, dx;
      float n, delta;
      union ieee754_double ex2_u;
#ifndef ROCKBOX
      fenv_t oldenv;

      feholdexcept (&oldenv);
#endif

#ifdef FE_TONEAREST
      fesetround (FE_TONEAREST);
#endif

      /* Calculate n.  */
      n = x * M_1_LN2 + THREEp22;
      n -= THREEp22;
      dx = x - n*M_LN2;

      /* Calculate t/512.  */
      t = dx + THREEp42;
      t -= THREEp42;
      dx -= t;

      /* Compute tval = t.  */
      tval = (int) (t * 512.0);

      if (t >= 0)
	delta = - __exp_deltatable[tval];
      else
	delta = __exp_deltatable[-tval];

      /* Compute ex2 = 2^n e^(t/512+delta[t]).  */
      ex2_u.d = __exp_atable[tval+177];
      ex2_u.ieee.exponent += (int) n;

      /* Approximate e^(dx+delta) - 1, using a second-degree polynomial,
	 with maximum error in [-2^-10-2^-28,2^-10+2^-28]
	 less than 5e-11.  */
      x22 = (0.5000000496709180453 * dx + 1.0000001192102037084) * dx + delta;

      /* Return result.  */
#ifndef ROCKBOX
      fesetenv (&oldenv);
#endif

      result = x22 * ex2_u.d + ex2_u.d;
      return (float) result;
    }
  /* Exceptional cases:  */
  else if (isless (x, himark))
    {
      if (__isinff (x))
	/* e^-inf == 0, with no error.  */
	return 0;
      else
	/* Underflow */
	return TWOM100 * TWOM100;
    }
  else
    /* Return x, if x is a NaN or Inf; or overflow, otherwise.  */
    return TWO127*x;
}


/* Division with rest, original. */
div_t div(int x, int y)
{
    div_t result;

    result.quot = x / y;
    result.rem = x % y;

    /* Addition from glibc-2.8: */
    if(x >= 0 && result.rem < 0)
    {
        result.quot += 1;
        result.rem -= y;
    }

    return result;
}


/* Placeholder function. */
/* Originally defined in s_midi.c */
void sys_listmididevs(void)
{
}

/* Placeholder function,
   as we do sleep in the core thread
   and not in the scheduler function. */
/* Originally defined in s_inter.c */
void sys_microsleep(int microsec)
{
    (void) microsec;
}

/* Get running time in milliseconds. */
/* Originally defined in s_inter.c */
extern uint64_t runningtime;
t_time sys_getrealtime(void)
{
    return runningtime;
}

/* Place holder, as we do no IPC. */
/* Originally defined in s_inter.c */
void glob_ping(void* dummy)
{
    (void) dummy;
}

/* Call to quit. */
/* Originally defined in s_inter.c */
extern bool quit;
void glob_quit(void* dummy)
{
    (void) dummy;

    static bool reentered = false;

    if(!reentered)
    {
        reentered = true;

        /* Close audio subsystem. */
        /* Will be done by the main program: sys_close_audio(); */

        /* Stop main loop. */
        quit = true;
    }
}

/* Open file. Originally in s_main.c */
void glob_evalfile(t_pd *ignore, t_symbol *name, t_symbol *dir);
void openit(const char *dirname, const char *filename)
{
    char* nameptr;
    char dirbuf[MAXPDSTRING];

    /* Workaround: If the file resides in the root directory,
                   add a trailing slash to prevent directory part
                   of the filename from being removed -- W.B. */
    char ffilename[MAXPDSTRING];
    ffilename[0] = '/';
    ffilename[1] = '\0';
    strcat(ffilename, filename);

    int fd = open_via_path(dirname, ffilename, "", dirbuf, &nameptr,
    	MAXPDSTRING, 0);
    if (fd)
    {
    	close (fd);
    	glob_evalfile(0, gensym(nameptr), gensym(dirbuf));
    }
    else
    	error("%s: can't open", filename);
}


/* Get current working directory. */
extern char* filename;
char* rb_getcwd(char* buf, ssize_t size)
{
    /* Initialize buffer. */
    buf[0] = '\0';

    /* Search for the last slash. */
    char* end_of_dir = strrchr(filename, '/');
    int dirlen = end_of_dir - filename;

    /* Check whether length of directory path is correct.
       If not, abort. */
    if(size < dirlen || dirlen == 0)
        return NULL;

    /* Copy current working directory to buffer. */
    strncat(buf, filename, dirlen);
    return buf;
}


/* Execute instructions supplied on command-line.
   Basically a placeholder, because the only
   command line argument we get is the name of the .pd file. */
/* Originally defined in s_main.c */
extern t_namelist* sys_openlist;
void glob_initfromgui(void *dummy, t_symbol *s, int argc, t_atom *argv)
{
    t_namelist *nl;
    char cwd[MAXPDSTRING];

    (void) dummy;
    (void) s;
    (void) argc;
    (void) argv;

    /* Get current working directory. */
    rb_getcwd(cwd, MAXPDSTRING);

    /* open patches specifies with "-open" args */
    for(nl = sys_openlist; nl; nl = nl->nl_next)
        openit(cwd, nl->nl_string);

    namelist_free(sys_openlist);
    sys_openlist = 0;
}

/* Fake GUI start. Originally in s_inter.c */
static int defaultfontshit[] = {
    8, 5, 9, 10, 6, 10, 12, 7, 13, 14, 9, 17, 16, 10, 19, 24, 15, 28,
    	24, 15, 28};
extern t_binbuf* inbinbuf;
int sys_startgui(const char *guidir)
{
    unsigned int i;
    t_atom zz[23];
    char cmdbuf[4*MAXPDSTRING];

    (void) guidir;

    inbinbuf = binbuf_new();

    if(!rb_getcwd(cmdbuf, MAXPDSTRING))
        strcpy(cmdbuf, ".");

    SETSYMBOL(zz, gensym(cmdbuf));
    for (i = 1; i < 22; i++)
        SETFLOAT(zz + i, defaultfontshit[i-1]);
    SETFLOAT(zz+22,0);
    glob_initfromgui(0, 0, 23, zz);

    return 0;
}

/* Return default DAC block size. */
/* Originally defined in s_main.c */
int sys_getblksize(void)
{
    return (DEFDACBLKSIZE);
}

/* Find library directory and set it. */
void sys_findlibdir(const char* filename)
{
    char sbuf[MAXPDSTRING];

    (void) filename;

    /* Make current working directory the system library directory. */
    rb_getcwd(sbuf, MAXPDSTRING);
    sys_libdir = gensym(sbuf);
}
