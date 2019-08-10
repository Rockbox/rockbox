#ifndef _FIXEDPOINTMATH_H
#define _FIXEDPOINTMATH_H
//Fixed point math routines (16.16)
//Dan East
//01-24-2001

#define FPM_PI        205887L
#define FPM_2PI       411775L
#define FPM_E         178144L
#define FPM_ROOT2      74804L
#define FPM_ROOT3     113512L
#define FPM_GOLDEN    106039L
#define FPM_MAX    0x7fff0000


typedef long fixedpoint_t;
typedef long fixedpoint8_24_t;


fixedpoint_t fpm_FromFloat(double f);
float fpm_ToFloat(fixedpoint_t fxp);

fixedpoint_t fpm_FromLong(long l);
long fpm_ToLong(fixedpoint_t fxp);

fixedpoint_t fpm_Add(fixedpoint_t fxp1, fixedpoint_t fxp2);
fixedpoint_t fpm_Add3(fixedpoint_t fxp1, fixedpoint_t fxp2, fixedpoint_t fxp3);

fixedpoint_t fpm_Sub(fixedpoint_t fxp1, fixedpoint_t fxp2);

fixedpoint_t fpm_Mul(fixedpoint_t fxp1, fixedpoint_t fxp2);

fixedpoint_t fpm_Div(fixedpoint_t fxp1, fixedpoint_t fxp2);
fixedpoint_t fpm_DivInt(fixedpoint_t fxp1, long l);
fixedpoint_t fpm_Abs(fixedpoint_t fxp);

fixedpoint_t fpm_Ceil(fixedpoint_t fxp);
fixedpoint_t fpm_Floor(fixedpoint_t fxp);

fixedpoint_t fpm_Sqrt(fixedpoint_t fxp);
fixedpoint_t fpm_Sqr(fixedpoint_t fxp);

fixedpoint_t fpm_Inv(fixedpoint_t fxp);

fixedpoint_t fpm_Sin(fixedpoint_t fxp);
fixedpoint_t fpm_Cos(fixedpoint_t fxp);
fixedpoint_t fpm_Tan(fixedpoint_t fxp);
fixedpoint_t fpm_ATan(fixedpoint_t fxp);
//These take degrees
fixedpoint_t fpm_SinDeg(fixedpoint_t fxp);
fixedpoint_t fpm_CosDeg(fixedpoint_t fxp);
fixedpoint_t fpm_TanDeg(fixedpoint_t fxp);
fixedpoint_t fpm_ATanDeg(fixedpoint_t fxp);


#define FPM_FROMFLOAT(f)                fpm_FromFloat(f)
#define FPM_FROMFLOATC(f)               ((long)((f) * 65536.0 ))        //Constant version
#define FPM_TOFLOAT(fxp)                fpm_ToFloat(fxp)

#define FPM_FROMLONG(l)                 fpm_FromLong(l)
#define FPM_FROMLONGC(l)                ((l)<<16)                                       //Constant version
#define FPM_TOLONG(l)                   fpm_ToLong(l)

#define FPM_ADD(f1, f2)                 fpm_Add(f1, f2)
#define FPM_ADD3(f1, f2, f3)    fpm_Add3(f1, f2, f3)
#define FPM_INC(f1)                             ((f1)=FPM_ADD(f1, FPM_FROMLONG(1)))

#define FPM_SUB(f1, f2)                 fpm_Sub(f1, f2)
#define FPM_DEC(f1)                             ((f1)=FPM_SUB(f1, FPM_FROMLONG(1)))

#define FPM_MUL(f1, f2)                 fpm_Mul(f1, f2)

#define FPM_DIV(n, d)                   fpm_Div(n, d)
#define FPM_DIVINT(n, d)                fpm_DivInt(n, d)

#define FPM_ABS(fxp)                    fpm_Abs(fxp)

#define FPM_CEIL(fxp)                   fpm_Ceil(fxp)
#define FPM_FLOOR(fxp)                  fpm_Floor(fxp)

#define FPM_SQRT(fxp)                   fpm_Sqrt(fxp)
#define FPM_SQR(fxp)                    fpm_Sqr(fxp)

#define FPM_INV(fxp)                    fpm_Inv(fxp)

//These take radians
#define FPM_SIN(r)                              fpm_Sin(r)
#define FPM_COS(r)                              fpm_Cos(r)
#define FPM_TAN(r)                              fpm_Tan(r)
#define FPM_ATAN(r)                             fpm_ATan(r)

//These take degrees
#define FPM_SIN_DEG(d)                  fpm_SinDeg(d)
#define FPM_COS_DEG(d)                  fpm_CosDeg(d)
#define FPM_TAN_DEG(d)                  fpm_TanDeg(d)
#define FPM_ATAN_DEG(d)                 fpm_ATanDeg(d)

fixedpoint8_24_t fpm_FromFloat(double f);
float fpm_ToFloat8_24(fixedpoint8_24_t fxp);

fixedpoint8_24_t fpm_FromLong8_24(long l);
long fpm_ToLong8_24(fixedpoint8_24_t fxp);

fixedpoint8_24_t fpm_FromFixedPoint(fixedpoint_t fxp);
fixedpoint_t fpm_ToFixedPoint(fixedpoint8_24_t fxp);

fixedpoint8_24_t fpm_Add8_24(fixedpoint8_24_t fxp1, fixedpoint8_24_t fxp2);
fixedpoint8_24_t fpm_Add38_24(fixedpoint8_24_t fxp1, fixedpoint8_24_t fxp2, fixedpoint8_24_t fxp3);

fixedpoint8_24_t fpm_Sub8_24(fixedpoint8_24_t fxp1, fixedpoint8_24_t fxp2);

fixedpoint8_24_t fpm_Mul8_24(fixedpoint8_24_t fxp1, fixedpoint8_24_t fxp2);
fixedpoint_t fpm_MulMixed8_24(fixedpoint8_24_t fxp1, fixedpoint_t fxp2);

fixedpoint8_24_t fpm_Div8_24(fixedpoint8_24_t fxp1, fixedpoint8_24_t fxp2);
fixedpoint8_24_t fpm_DivInt8_24(fixedpoint8_24_t fxp1, long l);
fixedpoint8_24_t fpm_DivInt64_8_24(fixedpoint8_24_t fxp1, long long l);

fixedpoint8_24_t fpm_Abs8_24(fixedpoint8_24_t fxp);

fixedpoint8_24_t fpm_Ceil8_24(fixedpoint8_24_t fxp);
fixedpoint8_24_t fpm_Floor8_24(fixedpoint8_24_t fxp);

fixedpoint8_24_t fpm_Sqrt8_24(fixedpoint8_24_t fxp);
fixedpoint8_24_t fpm_Sqr8_24(fixedpoint8_24_t fxp);

fixedpoint8_24_t fpm_Inv8_24(fixedpoint8_24_t fxp);


#define FPM_FROMFLOAT8_24(f)            fpm_FromFloat8_24(f)
#define FPM_FROMFLOATC8_24(f)           ((long)((f) * 16777216.0 ))     //Constant version
#define FPM_TOFLOAT8_24(fxp)            fpm_ToFloat8_24(fxp)

#define FPM_FROMLONG8_24(l)                     fpm_FromLong8_24(l)
#define FPM_FROMLONGC8_24(l)            ((l)<<24)                                       //Constant version
#define FPM_TOLONG8_24(l)                       fpm_ToLong8_24(l)


/*
extern  __int64 FPM_TMPVAR_INT64;

  #define FPM_FROMFLOAT(f)              ((long)((f) * 65536.0 ))        //+0.5
#define FPM_TOFLOAT(fxp)                (((float)(fxp)) / 65536.0)

#define FPM_FROMLONG(l)                 ((l)<<16)
#define FPM_TOLONG(l)                   ((l)<0?(-(long)((l)^0xffffffff)>>16):(((l)>>16)&0x0000ffff))

#define FPM_ADD(f1, f2)                 ((f1)+(f2))
#define FPM_ADD3(f1, f2, f3)    ((f1)+(f2)+(f3))
#define FPM_INC(f1)                             ((f1)=FPM_ADD(f1, FPM_FROMLONG(1)))

#define FPM_SUB(f1, f2)                 ((f1)-(f2))
#define FPM_DEC(f1)                             ((f1)=FPM_SUB(f1, FPM_FROMLONG(1)))

#define FPM_MUL(f1, f2)                 (((f1)>>8)*((f2)>>8))
//#define FPM_MUL(f1, f2)                       ((fixedpoint_t)((FPM_TMPVAR_INT64=(f1))*(f2))>>16)
//#define FPM_MUL(f1, f2)                       (((f1)*(f2))>>16)
//TODO: This needs to be done without copying to another var
#define FPM_DIV(n, d)                   ((fixedpoint_t)(((FPM_TMPVAR_INT64=(n))<<16)/d))
#define FPM_DIVINT(n, d)                ((fixedpoint_t)((n)/(d)))
//#define FPM_DIV(n, d)                 ((long)(((__int64)n)<<16)/(d))

#define FPM_ABS(fxp)                    (abs(fxp))

//TODO: could be more effecient
#define FPM_CEIL(fxp)                   ((fxp)&0x0000ffff?((fxp)<=0?((fxp)&0xffff0000):(((fxp)&0xffff0000)+FPM_FROMLONG(1))):(fxp))
#define FPM_FLOOR(fxp)                  ((fxp)&0x0000ffff?((fxp)<0?(((fxp)&0xffff0000)-FPM_FROMLONG(1)):((fxp)&0xffff0000)):(fxp))

//TODO: Implement sqrt mathematically instead of converting to float and back
#define FPM_SQRT(fxp)                   (FPM_FROMFLOAT(sqrt(FPM_TOFLOAT(fxp))))
#define FPM_SQR(fxp)                    (FPM_MUL(fxp,fxp)>>16)

#define FPM_INV(fxp)                    (FPM_DIV(0x10000, fxp))
//TODO: Calc trig functions (or lookup) instead of converting to float and back
//These take radians
#define FPM_SIN(r)      (FPM_FROMFLOAT(sin(FPM_TOFLOAT(r))))
#define FPM_COS(r)      (FPM_FROMFLOAT(cos(FPM_TOFLOAT(r))))
#define FPM_TAN(r)      (FPM_FROMFLOAT(tan(FPM_TOFLOAT(r))))
#define FPM_ATAN(r)     (FPM_FROMFLOAT(atan(FPM_TOFLOAT(r))))

//These take degrees
#define FPM_SIN_DEG(d)  (FPM_SIN(FPM_DIV(FPM_MUL(d,FPM_PI),0xB40000)))  //0xB40000 = 180.0
#define FPM_COS_DEG(d)  (FPM_COS(FPM_DIV(FPM_MUL(d,FPM_PI),0xB40000)))
#define FPM_TAN_DEG(d)  (FPM_TAN(FPM_DIV(FPM_MUL(d,FPM_PI),0xB40000)))
#define FPM_ATAN_DEG(d) (FPM_ATAN(FPM_DIV(FPM_MUL(d,FPM_PI),0xB40000)))

*/
/*
#define FPM_PI        3.14
#define FPM_2PI       (3.14*2)
#define FPM_E         178144L
#define FPM_ROOT2      74804L
#define FPM_ROOT3     113512L
#define FPM_GOLDEN    106039L


typedef float fixedpoint_t;
//This variable must be declared in one of the implementation files.
extern  __int64 FPM_TMPVAR_INT64;

#define FPM_FROMFLOAT(f)                (f)
#define FPM_TOFLOAT(fxp)                (fxp)

#define FPM_FROMLONG(l)                 ((float)l)
#define FPM_TOLONG(fxp)                 ((long)fxp)

#define FPM_ADD(f1, f2)                 ((f1)+(f2))
#define FPM_ADD3(f1, f2, f3)    ((f1)+(f2)+(f3))
#define FPM_INC(f1)                             ((f1)++)

#define FPM_SUB(f1, f2)                 ((f1)-(f2))
#define FPM_DEC(f1)                             ((f1)--)

#define FPM_MUL(f1, f2)                 ((f1)*(f2))
//#define FPM_MUL(f1, f2)                       ((fixedpoint_t)((FPM_TMPVAR_INT64=(f1))*(f2))>>16)
//#define FPM_MUL(f1, f2)                       (((f1)*(f2))>>16)
//TODO: This needs to be done without copying to another var
#define FPM_DIV(n, d)                   ((n)/(d))
//#define FPM_DIV(n, d)                 ((long)(((__int64)n)<<16)/(d))

#define FPM_ABS(fxp)                    (abs(fxp))

//TODO: Implement ceil mathematically instead of converting to float and back
#define FPM_CEIL(fxp)                   (ceil(fxp))
#define FPM_FLOOR(fxp)                  (floor(fxp))

//TODO: Implement sqrt mathematically instead of converting to float and back
#define FPM_SQRT(fxp)                   (sqrt(fxp))
#define FPM_SQR(fxp)                    ((fxp)*(fxp))

#define FPM_INV(fxp)                    (1/(fxp))
//TODO: Calc trig functions (or lookup) instead of converting to float and back
//These take radians
#define FPM_SIN(r)      (sin(r))
#define FPM_COS(r)      (cos(r))
#define FPM_TAN(r)      (tan(r))
#define FPM_ATAN(r)     (atan(r))

//These take degrees
#define FPM_SIN_DEG(d)  FPM_SIN(((d)*FPM_PI)/180.0)     //0xB40000 = 180.0
#define FPM_COS_DEG(d)  FPM_COS(((d)*FPM_PI)/180.0)
#define FPM_TAN_DEG(d)  FPM_TAN(((d)*FPM_PI)/180.0)
#define FPM_ATAN_DEG(d) FPM_ATAN(((d)*FPM_PI)/180.0)
*/

#endif //_FIXEDPOINTMATH_H
