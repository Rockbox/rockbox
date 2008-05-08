#include "kernel.h"
#include "system.h"
#include "panic.h"
#include "avic-imx31.h"
#include "gpio-imx31.h"
#include "mmu-imx31.h"
#include "system-target.h"
#include "lcd.h"
#include "serial-imx31.h"
#include "debug.h"
#include "clkctl-imx31.h"

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
    static const int disable_clocks[] =
    {
        /* CGR0 */
        CG_SD_MMC1,
        CG_SD_MMC2,
        CG_IIM,
        CG_CSPI3,
        CG_RNG,
        CG_UART1,
        CG_UART2,
        CG_SSI1,
        CG_I2C1,
        CG_I2C2,
        CG_I2C3,

        /* CGR1 */
        CG_HANTRO,
        CG_MEMSTICK1,
        CG_MEMSTICK2,
        CG_CSI,
        CG_PWM,
        CG_WDOG,
        CG_SIM,
        CG_ECT,
        CG_USBOTG,
        CG_KPP,
        CG_UART3,
        CG_UART4,
        CG_UART5,
        CG_1_WIRE,

        /* CGR2 */
        CG_SSI2,
        CG_CSPI1,
        CG_CSPI2,
        CG_GACC,
        CG_RTIC,
        CG_FIR
    };

    unsigned int i;

    /* MCR WFI enables wait mode */
    CLKCTL_CCMR &= ~(3 << 14);

    imx31_regmod32(&SDHC1_CLOCK_CONTROL, STOP_CLK, STOP_CLK);
    imx31_regmod32(&SDHC2_CLOCK_CONTROL, STOP_CLK, STOP_CLK);
    imx31_regmod32(&RNGA_CONTROL, RNGA_CONTROL_SLEEP, RNGA_CONTROL_SLEEP);
    imx31_regmod32(&UCR1_1, 0, EUARTUCR1_UARTEN);
    imx31_regmod32(&UCR1_2, 0, EUARTUCR1_UARTEN);
    imx31_regmod32(&UCR1_3, 0, EUARTUCR1_UARTEN);
    imx31_regmod32(&UCR1_4, 0, EUARTUCR1_UARTEN);
    imx31_regmod32(&UCR1_5, 0, EUARTUCR1_UARTEN);

    for (i = 0; i < ARRAYLEN(disable_clocks); i++)
        imx31_clkctl_module_clock_gating(disable_clocks[i], CGM_OFF);

    avic_init();
    gpio_init();
}

void imx31_regmod32(volatile uint32_t *reg_p, uint32_t value, uint32_t mask)
{
    value &= mask;
    mask = ~mask;

    int oldlevel = disable_interrupt_save(IRQ_FIQ_STATUS);
    *reg_p = (*reg_p & mask) | value;
    restore_interrupt(oldlevel);
}

#ifdef BOOTLOADER
void system_prepare_fw_start(void)
{
    disable_interrupt(IRQ_FIQ_STATUS);
    avic_disable_int(ALL);
    tick_stop();
}
#endif

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
