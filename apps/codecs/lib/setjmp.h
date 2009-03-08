#ifndef _SETJMP_H_
#define _SETJMP_H_

/* Combined parts of include/setjmp.h and include/machine/setjmp.h in 
 * newlib 1.17.0, with minor changes for Rockbox.
 */

#ifdef CPU_ARM
/*
 * All callee preserved registers:
 * v1 - v7, fp, ip, sp, lr, f4, f5, f6, f7
 */
#define _JBLEN 23
#endif

/* necv70 was 9 as well. */

#ifdef CPU_COLDFIRE
/*
 * onsstack,sigmask,sp,pc,psl,d2-d7,a2-a6,
 * fp2-fp7	for 68881.
 * All else recovered by under/over(flow) handling.
 */
#define	_JBLEN	34
#endif

#ifdef __mips__
#ifdef __mips64
#define _JBTYPE long long
#endif
#ifdef __mips_soft_float
#define _JBLEN 11
#else
#define _JBLEN 23
#endif
#endif

#ifdef  __sh__
#if __SH5__
#define _JBLEN 50
#define _JBTYPE long long
#else
#define _JBLEN 20
#endif /* __SH5__ */
#endif

#ifdef _JBLEN
#ifdef _JBTYPE
typedef	_JBTYPE jmp_buf[_JBLEN];
#else
typedef	int jmp_buf[_JBLEN];
#endif
#endif


extern void longjmp(jmp_buf __jmpb, int __retval);
extern int  setjmp(jmp_buf __jmpb);

#endif // _SETJMP_H_
