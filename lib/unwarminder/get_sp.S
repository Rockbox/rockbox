#include "config.h"
/* On native platform we protect ourself by disabling interrupts
 * then we check current processor mode. If we are called
 * from exception we need to save state and switch to SYS and
 * after obtaining SP we restore everything from saved state.
 *
 * On RaaA we are called in USER mode most probably and
 * cpsr mangling is restricted. We simply copy SP value
 * in this situation
 */
.section .text
.type __get_sp,%function
.global __get_sp

__get_sp:
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
    mrs r1, cpsr         /* save current state */
    orr r0, r1, #0xc0
    msr cpsr, r0         /* disable IRQ and FIQ */
    and r0, r1, #0x1f    /* get current mode */
    cmp r0, #0x1f        /* are we in sys mode? */
    beq get_sp       
call_from_exception:
    mrs r0, spsr         /* get saved state */
    and r0, r0, #0x1f    /* get mode bits */
    orr r0, r0, #0xc0    /* no FIQ no IRQ */
    msr cpsr, r0         /* change mode */
get_sp:
#endif
    mov r0, sp           /* get SP */
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
    msr cpsr, r1         /* restore mode */
#endif
    bx  lr
.size __get_sp, . - __get_sp
