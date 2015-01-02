#ifndef METRONOME2_LOG_H
#define METRONOME2_LOG_H

/* This is a nasty hack: Ripped out atof() from pdbox plugin,
   Copyright (C) 2009 Wincent Balin (and glibc folks, actually) */

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
#endif
