#include "plugin.h"

#include "fixedpoint.h"

#include <SDL.h>

extern bool printf_enabled;

/* fixed-point wrappers */
static unsigned long lastphase = 0;
static long lastsin = 0, lastcos = 0x7fffffff;

#define PI 3.1415926535897932384626433832795

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

float tan_wrapper(float f)
{
    return sin_wrapper(f)/cos_wrapper(f);
}

/* stolen from doom */
// Here is a hacked up printf command to get the output from the game.
int printf_wrapper(const char *fmt, ...)
{
    static int p_xtpt;
    char p_buf[50];
    rb->yield();
    va_list ap;

    va_start(ap, fmt);
    rb->vsnprintf(p_buf,sizeof(p_buf), fmt, ap);
    va_end(ap);

    rb->lcd_setfont(FONT_SYSFIXED);
    rb->lcd_putsxy(1,p_xtpt, (unsigned char *)p_buf);
    if (printf_enabled)
        rb->lcd_update();

    p_xtpt+=8;
    if(p_xtpt>LCD_HEIGHT-8)
    {
        p_xtpt=0;
        if (printf_enabled)
        {
            rb->lcd_set_backdrop(NULL);
            rb->lcd_clear_display();
        }
    }
    return 1;
}

int sprintf_wrapper(char *str, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int ret = rb->vsnprintf(str, 9999, fmt, ap);
    va_end(ap);
    return ret;
}

char *strcpy_wrapper(char *dest, const char *src)
{
    rb->strlcpy(dest, src, 999);
    return dest;
}

char *strdup_wrapper(const char *s) {
    char *r = malloc(1+strlen(s));
    strcpy(r,s);
    return r;
}

char *strcat_wrapper(char *dest, const char *src)
{
    rb->strlcat(dest, src, 999);
    return dest;
}

char *strpbrk_wrapper(const char *s1, const char *s2)
{
    while(*s1)
        if(strchr(s2, *s1++))
            return (char*)--s1;
    return 0;
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

/* Absolute value, simple calculus */
float fabs_wrapper(float x)
{
    return (x < 0.0f) ? -x : x;
}

float fmod(float x, float y)
{
    return x - (int) (x / y) * y;
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

static const float zero   =  0.0;

static const float
huge = 1.0e+30,
tiny = 1.0e-30,
one  = 1.0f;

/* Square root function, original. */
float sqrt_wrapper(float x)
{
        float z;
        int32_t sign = (int)0x80000000;
        int32_t ix,s,q,m,t,i;
        uint32_t r;

        GET_FLOAT_WORD(ix,x);

    /* take care of Inf and NaN */
        if((ix&0x7f800000)==0x7f800000) {
            return x*x+x;               /* sqrt(NaN)=NaN, sqrt(+inf)=+inf
                                           sqrt(-inf)=sNaN */
        }
    /* take care of zero */
        if(ix<=0) {
            if((ix&(~sign))==0) return x;/* sqrt(+-0) = +-0 */
            else if(ix<0)
                return (x-x)/(x-x);             /* sqrt(-ve) = sNaN */
        }
    /* normalize x */
        m = (ix>>23);
        if(m==0) {                              /* subnormal x */
            for(i=0;(ix&0x00800000)==0;i++) ix<<=1;
            m -= i-1;
        }
        m -= 127;       /* unbias exponent */
        ix = (ix&0x007fffff)|0x00800000;
        if(m&1) /* odd m, double x to make it even */
            ix += ix;
        m >>= 1;        /* m = [m/2] */

    /* generate sqrt(x) bit by bit */
        ix += ix;
        q = s = 0;              /* q = sqrt(x) */
        r = 0x01000000;         /* r = moving bit from right to left */

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
