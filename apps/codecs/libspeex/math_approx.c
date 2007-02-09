/* Copyright (C) 2002 Jean-Marc Valin 
   File: math_approx.c
   Various math approximation functions for Speex

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   
   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   
   - Neither the name of the Xiph.org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif



#include "math_approx.h"
#include "misc.h"

#ifdef FIXED_POINT

/* sqrt(x) ~= 0.22178 + 1.29227*x - 0.77070*x^2 + 0.25723*x^3 (for .25 < x < 1) */
#define C0 3634
#define C1 21173
#define C2 -12627
#define C3 4215

spx_word16_t spx_sqrt(spx_word32_t x)
{
   int k=0;
   spx_word32_t rt;

   if (x<=0)
      return 0;
#if 1
   if (x>=16777216)
   {
      x>>=10;
      k+=5;
   }
   if (x>=1048576)
   {
      x>>=6;
      k+=3;
   }
   if (x>=262144)
   {
      x>>=4;
      k+=2;
   }
   if (x>=32768)
   {
      x>>=2;
      k+=1;
   }
   if (x>=16384)
   {
      x>>=2;
      k+=1;
   }
#else
   while (x>=16384)
   {
      x>>=2;
      k++;
      }
#endif
   while (x<4096)
   {
      x<<=2;
      k--;
   }
   rt = ADD16(C0, MULT16_16_Q14(x, ADD16(C1, MULT16_16_Q14(x, ADD16(C2, MULT16_16_Q14(x, (C3)))))));
   if (rt > 16383)
      rt = 16383;
   if (k>0)
      rt <<= k;
   else
      rt >>= -k;
   rt >>=7;
   return rt;
}

  static int intSqrt(int x) {
    int xn;
   static int sqrt_table[256] = {
     0,    16,  22,  27,  32,  35,  39,  42,  45,  48,  50,  53,  55,  57,
     59,   61,  64,  65,  67,  69,  71,  73,  75,  76,  78,  80,  81,  83,
     84,   86,  87,  89,  90,  91,  93,  94,  96,  97,  98,  99, 101, 102,
     103, 104, 106, 107, 108, 109, 110, 112, 113, 114, 115, 116, 117, 118,
     119, 120, 121, 122, 123, 124, 125, 126, 128, 128, 129, 130, 131, 132,
     133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 144, 145,
     146, 147, 148, 149, 150, 150, 151, 152, 153, 154, 155, 155, 156, 157,
     158, 159, 160, 160, 161, 162, 163, 163, 164, 165, 166, 167, 167, 168,
     169, 170, 170, 171, 172, 173, 173, 174, 175, 176, 176, 177, 178, 178,
     179, 180, 181, 181, 182, 183, 183, 184, 185, 185, 186, 187, 187, 188,
     189, 189, 190, 191, 192, 192, 193, 193, 194, 195, 195, 196, 197, 197,
     198, 199, 199, 200, 201, 201, 202, 203, 203, 204, 204, 205, 206, 206,
     207, 208, 208, 209, 209, 210, 211, 211, 212, 212, 213, 214, 214, 215,
     215, 216, 217, 217, 218, 218, 219, 219, 220, 221, 221, 222, 222, 223,
     224, 224, 225, 225, 226, 226, 227, 227, 228, 229, 229, 230, 230, 231,
     231, 232, 232, 233, 234, 234, 235, 235, 236, 236, 237, 237, 238, 238,
     239, 240, 240, 241, 241, 242, 242, 243, 243, 244, 244, 245, 245, 246,
     246, 247, 247, 248, 248, 249, 249, 250, 250, 251, 251, 252, 252, 253,
     253, 254, 254, 255
  };
    if (x >= 0x10000) {
      if (x >= 0x1000000) {
        if (x >= 0x10000000) {
          if (x >= 0x40000000) {
            xn = sqrt_table[x >> 24] << 8;
          } else {
            xn = sqrt_table[x >> 22] << 7;
          }
        } else {
          if (x >= 0x4000000) {
            xn = sqrt_table[x >> 20] << 6;
          } else {
            xn = sqrt_table[x >> 18] << 5;
          }
        }

        xn = (xn + 1 + (x / xn)) >> 1;
        xn = (xn + 1 + (x / xn)) >> 1;
        return ((xn * xn) > x) ? --xn : xn;
      } else {
        if (x >= 0x100000) {
          if (x >= 0x400000) {
            xn = sqrt_table[x >> 16] << 4;
          } else {
            xn = sqrt_table[x >> 14] << 3;
          }
        } else {
          if (x >= 0x40000) {
            xn = sqrt_table[x >> 12] << 2;
          } else {
            xn = sqrt_table[x >> 10] << 1;
          }
        }

        xn = (xn + 1 + (x / xn)) >> 1;

        return ((xn * xn) > x) ? --xn : xn;
      }
    } else {
      if (x >= 0x100) {
        if (x >= 0x1000) {
          if (x >= 0x4000) {
            xn = (sqrt_table[x >> 8]) + 1;
          } else {
            xn = (sqrt_table[x >> 6] >> 1) + 1;
          }
        } else {
          if (x >= 0x400) {
            xn = (sqrt_table[x >> 4] >> 2) + 1;
          } else {
            xn = (sqrt_table[x >> 2] >> 3) + 1;
          }
        }

        return ((xn * xn) > x) ? --xn : xn;
      } else {
        if (x >= 0) {
          return sqrt_table[x] >> 4;
        }
      }
    }
    
    return -1;
  }

float spx_sqrtf(float arg)
{
	if(arg==0.0)
		return 0.0;
	else if(arg==1.0)
		return 1.0;
	else if(arg==2.0)
		return 1.414;
	else if(arg==3.27)
		return 1.8083;
	//printf("Sqrt:%f:%f:%f\n",arg,(((float)intSqrt((int)(arg*10000)))/100)+0.0055,(float)spx_sqrt((spx_word32_t)arg));
	//return ((float)fastSqrt((int)(arg*2500)))/50;
	//LOGF("Sqrt:%d:%d\n",arg,(intSqrt((int)(arg*2500)))/50);
	return (((float)intSqrt((int)(arg*10000)))/100)+0.0055;//(float)spx_sqrt((spx_word32_t)arg);
	//return 1;
}


/* log(x) ~= -2.18151 + 4.20592*x - 2.88938*x^2 + 0.86535*x^3 (for .5 < x < 1) */


#define A1 16469
#define A2 2242
#define A3 1486

spx_word16_t spx_acos(spx_word16_t x)
{
   int s=0;
   spx_word16_t ret;
   spx_word16_t sq;
   if (x<0)
   {
      s=1;
      x = NEG16(x);
   }
   x = SUB16(16384,x);
   
   x = x >> 1;
   sq = MULT16_16_Q13(x, ADD16(A1, MULT16_16_Q13(x, ADD16(A2, MULT16_16_Q13(x, (A3))))));
   ret = spx_sqrt(SHL32(EXTEND32(sq),13));
   
   /*ret = spx_sqrt(67108864*(-1.6129e-04 + 2.0104e+00*f + 2.7373e-01*f*f + 1.8136e-01*f*f*f));*/
   if (s)
      ret = SUB16(25736,ret);
   return ret;
}


#define K1 8192
#define K2 -4096
#define K3 340
#define K4 -10

spx_word16_t spx_cos(spx_word16_t x)
{
   spx_word16_t x2;

   if (x<12868)
   {
      x2 = MULT16_16_P13(x,x);
      return ADD32(K1, MULT16_16_P13(x2, ADD32(K2, MULT16_16_P13(x2, ADD32(K3, MULT16_16_P13(K4, x2))))));
   } else {
      x = SUB16(25736,x);
      x2 = MULT16_16_P13(x,x);
      return SUB32(-K1, MULT16_16_P13(x2, ADD32(K2, MULT16_16_P13(x2, ADD32(K3, MULT16_16_P13(K4, x2))))));
   }
}

#else

#ifndef M_PI
#define M_PI           3.14159265358979323846  /* pi */
#endif

#define C1 0.9999932946f
#define C2 -0.4999124376f
#define C3 0.0414877472f
#define C4 -0.0012712095f


#define SPX_PI_2 1.5707963268
spx_word16_t spx_cos(spx_word16_t x)
{
   if (x<SPX_PI_2)
   {
      x *= x;
      return C1 + x*(C2+x*(C3+C4*x));
   } else {
      x = M_PI-x;
      x *= x;
      return NEG16(C1 + x*(C2+x*(C3+C4*x)));
   }
}
#endif

inline float spx_floor(float x){
	return ((float)(((int)x)));
}

#define FP_BITS         (14)
#define FP_MASK         ((1 << FP_BITS) - 1)
#define FP_ONE          (1 << FP_BITS)
#define FP_TWO          (2 << FP_BITS)
#define FP_HALF         (1 << (FP_BITS - 1))
#define FP_LN2          ( 45426 >> (16 - FP_BITS))
#define FP_LN2_INV      ( 94548 >> (16 - FP_BITS))
#define FP_EXP_ZERO     ( 10922 >> (16 - FP_BITS))
#define FP_EXP_ONE      (  -182 >> (16 - FP_BITS))
#define FP_EXP_TWO      (     4 >> (16 - FP_BITS))
// #define FP_INF          (0x7fffffff)
// #define FP_LN10         (150902 >> (16 - FP_BITS))

#define FP_MAX_DIGITS       (4)
#define FP_MAX_DIGITS_INT   (10000)


// #define FP_FAST_MUL_DIV

// #ifdef FP_FAST_MUL_DIV

/* These macros can easily overflow, but they are good enough for our uses,
 * and saves some code.
 */
#define fp_mul(x, y) (((x) * (y)) >> FP_BITS)
#define fp_div(x, y) (((x) << FP_BITS) / (y))

#ifndef abs
	#define abs(x) (((x)<0)?((x)*-1):(x))
#endif

float spx_sqrt2(float xf) {
	long x=(xf*(2.0*FP_BITS));
	int i=0, s = (x + FP_ONE) >> 1;
	for (; i < 8; i++) {
		s = (s + fp_div(x, s)) >> 1;
	}
	return s/((float)(2*FP_BITS));
}

static int exp_s16p16(int x)
{
	int t;
	int y = 0x00010000;
	
	if (x < 0) x += 0xb1721,            y >>= 16;
	t = x - 0x58b91; if (t >= 0) x = t, y <<= 8;
	t = x - 0x2c5c8; if (t >= 0) x = t, y <<= 4;
	t = x - 0x162e4; if (t >= 0) x = t, y <<= 2;
	t = x - 0x0b172; if (t >= 0) x = t, y <<= 1;
	t = x - 0x067cd; if (t >= 0) x = t, y += y >> 1;
	t = x - 0x03920; if (t >= 0) x = t, y += y >> 2;
	t = x - 0x01e27; if (t >= 0) x = t, y += y >> 3;
	t = x - 0x00f85; if (t >= 0) x = t, y += y >> 4;
	t = x - 0x007e1; if (t >= 0) x = t, y += y >> 5;
	t = x - 0x003f8; if (t >= 0) x = t, y += y >> 6;
	t = x - 0x001fe; if (t >= 0) x = t, y += y >> 7;
	y += ((y >> 8) * x) >> 8;
	
	return y;
}
float spx_expB(float xf) {
	return exp_s16p16(xf*32)/32;
}
float spx_expC(float xf){
  long x=xf*(2*FP_BITS);
/*
static long fp_exp(long x)
{*/
    long k;
    long z;
    long R;
    long xp;

    if (x == 0)
    {
        return FP_ONE;
    }

    k = (fp_mul(abs(x), FP_LN2_INV) + FP_HALF) & ~FP_MASK;

    if (x < 0)
    {
        k = -k;
    }

    x -= fp_mul(k, FP_LN2);
    z = fp_mul(x, x);
    R = FP_TWO + fp_mul(z, FP_EXP_ZERO + fp_mul(z, FP_EXP_ONE
        + fp_mul(z, FP_EXP_TWO)));
    xp = FP_ONE + fp_div(fp_mul(FP_TWO, x), R - x);

    if (k < 0)
    {
        k = FP_ONE >> (-k >> FP_BITS);
    }
    else
    {
        k = FP_ONE << (k >> FP_BITS);
    }

    return fp_mul(k, xp)/(2*FP_BITS);
}
/*To generate (ruby code): (0...33).each { |idx| puts Math.exp((idx-10) / 8.0).to_s + "," } */
const float exp_lookup_int[33]={0.28650479686019,0.32465246735835,0.367879441171442,0.416862019678508,0.472366552741015,0.53526142851899,0.606530659712633,0.687289278790972,0.778800783071405,0.882496902584595,1.0,1.13314845306683,1.28402541668774,1.4549914146182,1.64872127070013,1.86824595743222,2.11700001661267,2.3988752939671,2.71828182845905,3.08021684891803,3.49034295746184,3.95507672292058,4.48168907033806,5.07841903718008,5.75460267600573,6.52081912033011,7.38905609893065,8.37289748812726,9.48773583635853,10.7510131860764,12.1824939607035,13.8045741860671,15.6426318841882};
/*To generate (ruby code): (0...32).each { |idx| puts Math.exp((idx-16.0) / 4.0).to_s+","} */
static const float exp_table[32]={0.0183156388887342,0.0235177458560091,0.0301973834223185,0.038774207831722,0.0497870683678639,0.0639278612067076,0.0820849986238988,0.105399224561864,0.135335283236613,0.173773943450445,0.22313016014843,0.28650479686019,0.367879441171442,0.472366552741015,0.606530659712633,0.778800783071405,1.0,1.28402541668774,1.64872127070013,2.11700001661267,2.71828182845905,3.49034295746184,4.48168907033806,5.75460267600573,7.38905609893065,9.48773583635853,12.1824939607035,15.6426318841882,20.0855369231877,25.7903399171931,33.1154519586923,42.5210820000628};
/**Returns exp(x) Range x=-4-+4 {x.0,x.25,x.5,x.75} */
float spx_exp(float xf){
	float flt=spx_floor(xf);
	if(-4<xf&&4>xf&&(abs(xf-flt)==0.0||abs(xf-flt)==0.25||abs(xf-flt)==0.5||abs(xf-flt)==0.75||abs(xf-flt)==1.0)){
#ifdef SIMULATOR 				
/*		printf("NtbSexp:%f,%d,%f:%f,%f,%f:%d,%d:%d\n",
			exp_sqrt_table[(int)((xf+4.0)*4.0)],
			(int)((xf-4.0)*4.0),
			(xf-4.0)*4.0,
			xf,
			flt,
			xf-flt,
			-4<xf,
			4>xf,
			abs(xf-flt)
		);*/
#endif
		return exp_table[(int)((xf+4.0)*4.0)];
	} else if (-4<xf&&4>xf){
#ifdef SIMULATOR 		
		printf("NtbLexp:%f,%f,%f:%d,%d:%d\n",xf,flt,xf-flt,-4<xf,4>xf,abs(xf-flt));
#endif

		return exp_table[(int)((xf+4.0)*4.0)];
	}
	
#ifdef SIMULATOR 		
	printf("NTBLexp:%f,%f,%f:%d,%d:%d\n",xf,flt,xf-flt,-4<xf,4>xf,abs(xf-flt));
#endif
	return spx_expB(xf);	
	//return exp(xf);
}
//Placeholders (not fixed point, only used when encoding):
float pow(float a,float b){
	return 0;
}
float log(float l){
	return 0;
}
float fabs(float a){
	return 0;
}
float sin(float a){
	return 0;
}
