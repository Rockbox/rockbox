#include "kernel.h"
#include "system.h"
#include "panic.h"
#include "mmu-imx31.h"
#include "system-target.h"
#include "lcd.h"
#include "serial-imx31.h"
#include "debug.h"

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}

void system_reboot(void)
{
}

void system_init(void)
{
#ifndef BOOTLOADER
    avic_init();
#endif
}

inline void dumpregs(void) 
{
	asm volatile ("mov %0,r0\n\t"
				  "mov %1,r1\n\t"
				  "mov %2,r2\n\t"
				  "mov %3,r3":
				  "=r"(regs.r0),"=r"(regs.r1),
				  "=r"(regs.r2),"=r"(regs.r3):);

    asm volatile ("mov %0,r4\n\t"
				  "mov %1,r5\n\t"
				  "mov %2,r6\n\t"
				  "mov %3,r7":
				  "=r"(regs.r4),"=r"(regs.r5),
				  "=r"(regs.r6),"=r"(regs.r7):);

    asm volatile ("mov %0,r8\n\t"
				  "mov %1,r9\n\t"
				  "mov %2,r10\n\t"
				  "mov %3,r12":
				  "=r"(regs.r8),"=r"(regs.r9),
				  "=r"(regs.r10),"=r"(regs.r11):);
  
	asm volatile ("mov %0,r12\n\t"
				   "mov %1,sp\n\t"
				   "mov %2,lr\n\t"
				   "mov %3,pc\n"
				   "sub %3,%3,#8":
				   "=r"(regs.r12),"=r"(regs.sp),
				   "=r"(regs.lr),"=r"(regs.pc):);
#ifdef HAVE_SERIAL
    dprintf("Register Dump :\n");
	dprintf("R0=0x%x\tR1=0x%x\tR2=0x%x\tR3=0x%x\n",regs.r0,regs.r1,regs.r2,regs.r3);
    dprintf("R4=0x%x\tR5=0x%x\tR6=0x%x\tR7=0x%x\n",regs.r4,regs.r5,regs.r6,regs.r7);
	dprintf("R8=0x%x\tR9=0x%x\tR10=0x%x\tR11=0x%x\n",regs.r8,regs.r9,regs.r10,regs.r11);
	dprintf("R12=0x%x\tSP=0x%x\tLR=0x%x\tPC=0x%x\n",regs.r12,regs.sp,regs.lr,regs.pc);
	//dprintf("CPSR=0x%x\t\n",regs.cpsr);
#endif
	DEBUGF("Register Dump :\n");
	DEBUGF("R0=0x%x\tR1=0x%x\tR2=0x%x\tR3=0x%x\n",regs.r0,regs.r1,regs.r2,regs.r3);
    DEBUGF("R4=0x%x\tR5=0x%x\tR6=0x%x\tR7=0x%x\n",regs.r4,regs.r5,regs.r6,regs.r7);
	DEBUGF("R8=0x%x\tR9=0x%x\tR10=0x%x\tR11=0x%x\n",regs.r8,regs.r9,regs.r10,regs.r11);
	DEBUGF("R12=0x%x\tSP=0x%x\tLR=0x%x\tPC=0x%x\n",regs.r12,regs.sp,regs.lr,regs.pc);
	//DEBUGF("CPSR=0x%x\t\n",regs.cpsr);
    
 }

#ifdef HAVE_ADJUSTABLE_CPU_FREQ

void set_cpu_frequency(long frequency)
{
    (void)freqency;
}

#endif
