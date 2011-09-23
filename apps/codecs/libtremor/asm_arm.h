/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis 'TREMOR' CODEC SOURCE CODE.   *
 *                                                                  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis 'TREMOR' SOURCE CODE IS (C) COPYRIGHT 1994-2002    *
 * BY THE Xiph.Org FOUNDATION http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: arm7 and later wide math functions

 ********************************************************************/

#ifdef _ARM_ASSEM_
#if ARM_ARCH < 6
#define INCL_OPTIMIZED_VECTOR_FMUL_WINDOW
static inline void ff_vector_fmul_window_c(ogg_int32_t *dst, const ogg_int32_t *src0,
                                           const ogg_int32_t *src1, const ogg_int32_t *win, int len)
{
    /* len is always a power of 2 and always >= 16 so this is unrolled 2 times*/
    ogg_int32_t *dst0 = dst, *dst1 = dst + 2*len;
    const ogg_int32_t *win0 = win, *win1 = win + 2*len;
    src1 += len;
    asm volatile ("cmp %[len], #0\n\t"
                  "b 1f\n\t"
                  "0:\n\t"
                  "ldr r0, [%[src0]], #4\n\t"
                  "ldr r1, [%[src1], #-4]!\n\t"
                  "ldr r2, [%[win0]], #4\n\t"
                  "ldr r3, [%[win1], #-4]!\n\t"
                  "smull r4, r5, r0, r2\n\t"
                  "smlal r4, r5, r1, r3\n\t"
                  "rsb r2, r2, #0\n\t"
                  "lsl r5, r5, #1\n\t"
                  "str r5, [%[dst1], #-4]!\n\t"
                  "smull r4, r5, r0, r3\n\t"
                  "smlal r4, r5, r1, r2\n\t"
                  "ldr r0, [%[src0]], #4\n\t"
                  "ldr r1, [%[src1], #-4]!\n\t"
                  "lsl r5, r5, #1\n\t"
                  "str r5, [%[dst0]], #4\n\t"

                  "ldr r2, [%[win0]], #4\n\t"
                  "ldr r3, [%[win1], #-4]!\n\t"
                  "smull r4, r5, r0, r2\n\t"
                  "smlal r4, r5, r1, r3\n\t"
                  "rsb r2, r2, #0\n\t"
                  "lsl r5, r5, #1\n\t"
                  "str r5, [%[dst1], #-4]!\n\t"
                  "smull r4, r5, r0, r3\n\t"
                  "smlal r4, r5, r1, r2\n\t"
                  "subs %[len], %[len], #2\n\t"
                  "lsl r5, r5, #1\n\t"
                  "str r5, [%[dst0]], #4\n\t"
                  "1:\n\t"
                  "bgt 0b\n\t"
                  : [dst0] "+r" (dst0), [dst1] "+r" (dst1),
                    [src0] "+r" (src0), [src1] "+r" (src1),
                    [win0] "+r" (win0), [win1] "+r" (win1),
                    [len] "+r" (len)
                  :: "r0", "r1", "r2", "r3", "r4", "r5", "cc", "memory");
}
#endif
#ifndef _V_LSP_MATH_ASM
#define _V_LSP_MATH_ASM

static inline void lsp_loop_asm(ogg_uint32_t *qip,ogg_uint32_t *pip,
                                ogg_int32_t *qexpp,
                                ogg_int32_t *ilsp,ogg_int32_t wi,
                                ogg_int32_t m){
  
  ogg_uint32_t qi=*qip,pi=*pip;
  ogg_int32_t qexp=*qexpp;

  asm("mov     r0,%3;"
      "movs    r1,%5,asr#1;"
      "add     r0,r0,r1,lsl#3;"
      "beq 2f;\n"
      "1:"
      
      "ldmdb   r0!,{r1,r3};"
      "subs    r1,r1,%4;"          //ilsp[j]-wi
      "rsbmi   r1,r1,#0;"          //labs(ilsp[j]-wi)
      "umull   %0,r2,r1,%0;"       //qi*=labs(ilsp[j]-wi)
      
      "subs    r1,r3,%4;"          //ilsp[j+1]-wi
      "rsbmi   r1,r1,#0;"          //labs(ilsp[j+1]-wi)
      "umull   %1,r3,r1,%1;"       //pi*=labs(ilsp[j+1]-wi)
      
      "cmn     r2,r3;"             // shift down 16?
      "beq     0f;"
      "add     %2,%2,#16;"
      "mov     %0,%0,lsr #16;"
      "orr     %0,%0,r2,lsl #16;"
      "mov     %1,%1,lsr #16;"
      "orr     %1,%1,r3,lsl #16;"
      "0:"
      "cmp     r0,%3;\n"
      "bhi     1b;\n"
      
      "2:"
      // odd filter assymetry
      "ands    r0,%5,#1;\n"
      "beq     3f;\n"
      "add     r0,%3,%5,lsl#2;\n"
      
      "ldr     r1,[r0,#-4];\n"
      "mov     r0,#0x4000;\n"
      
      "subs    r1,r1,%4;\n"          //ilsp[j]-wi
      "rsbmi   r1,r1,#0;\n"          //labs(ilsp[j]-wi)
      "umull   %0,r2,r1,%0;\n"       //qi*=labs(ilsp[j]-wi)
      "umull   %1,r3,r0,%1;\n"       //pi*=labs(ilsp[j+1]-wi)
      
      "cmn     r2,r3;\n"             // shift down 16?
      "beq     3f;\n"
      "add     %2,%2,#16;\n"
      "mov     %0,%0,lsr #16;\n"
      "orr     %0,%0,r2,lsl #16;\n"
      "mov     %1,%1,lsr #16;\n"
      "orr     %1,%1,r3,lsl #16;\n"
      
      //qi=(pi>>shift)*labs(ilsp[j]-wi);
      //pi=(qi>>shift)*labs(ilsp[j+1]-wi);
      //qexp+=shift;

      //}

      /* normalize to max 16 sig figs */
      "3:"
      "mov     r2,#0;"
      "orr     r1,%0,%1;"
      "tst     r1,#0xff000000;"
      "addne   r2,r2,#8;"
      "movne   r1,r1,lsr #8;"
      "tst     r1,#0x00f00000;"
      "addne   r2,r2,#4;"
      "movne   r1,r1,lsr #4;"
      "tst     r1,#0x000c0000;"
      "addne   r2,r2,#2;"
      "movne   r1,r1,lsr #2;"
      "tst     r1,#0x00020000;"
      "addne   r2,r2,#1;"
      "movne   r1,r1,lsr #1;"
      "tst     r1,#0x00010000;"
      "addne   r2,r2,#1;"
      "mov     %0,%0,lsr r2;"
      "mov     %1,%1,lsr r2;"
      "add     %2,%2,r2;"
      
      : "+r"(qi),"+r"(pi),"+r"(qexp)
      : "r"(ilsp),"r"(wi),"r"(m)
      : "r0","r1","r2","r3","cc");
  
  *qip=qi;
  *pip=pi;
  *qexpp=qexp;
}

static inline void lsp_norm_asm(ogg_uint32_t *qip,ogg_int32_t *qexpp){

  ogg_uint32_t qi=*qip;
  ogg_int32_t qexp=*qexpp;

  asm("tst     %0,#0x0000ff00;"
      "moveq   %0,%0,lsl #8;"
      "subeq   %1,%1,#8;"
      "tst     %0,#0x0000f000;"
      "moveq   %0,%0,lsl #4;"
      "subeq   %1,%1,#4;"
      "tst     %0,#0x0000c000;"
      "moveq   %0,%0,lsl #2;"
      "subeq   %1,%1,#2;"
      "tst     %0,#0x00008000;"
      "moveq   %0,%0,lsl #1;"
      "subeq   %1,%1,#1;"
      : "+r"(qi),"+r"(qexp)
      :
      : "cc");
  *qip=qi;
  *qexpp=qexp;
}

#endif
#endif

