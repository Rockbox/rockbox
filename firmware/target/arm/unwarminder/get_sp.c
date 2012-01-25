#include "config.h"
/* On native platform we protect ourself by disabling interrupts
 * then we check current processor mode. If we are called
 * from exception we need to save state and switch to SYS
 * after obtaining SP we restore everything from saved state.
 *
 * On RaaA we are called in USER mode most probably and
 * cpsr mangling is restricted. We simply copy SP value
 * in this situation
 */
unsigned int __get_sp(void)
{
    unsigned int result;
    unsigned long cpsr_save, mode;

    asm volatile (
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
        "mrs %[cpsr_save], cpsr \n"             /* save current state */
        "orr %[mode], %[cpsr_save], #0xc0 \n"
        "msr cpsr, %[mode] \n"                  /* disable IRQ and FIQ */
        "and %[mode], %[cpsr_save], #0x1f \n"   /* get current mode */
        "cmp %[mode], #0x1f \n"                 /* are we in sys mode? */
        "beq get_sp \n"       
        "call_from_exception: \n"
        "mrs %[mode], spsr \n"                  /* get saved state */
        "and %[mode], %[mode], #0x1f \n"        /* get mode bits */
        "orr %[mode], %[mode], #0xc0 \n"        /* no FIQ no IRQ */
        "msr cpsr, %[mode] \n"                  /* change mode */
        "get_sp: \n"
#endif
        "mov %[result], sp \n"                  /* get SP */
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
        "msr cpsr, %[cpsr_save] \n"             /* restore mode */
#endif
        : [result] "=r" (result),
          [cpsr_save] "=r" (cpsr_save),
          [mode] "=r" (mode)
    );

    return result;
}
