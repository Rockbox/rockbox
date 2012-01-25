unsigned int __get_sp(void)
{
    unsigned int result;
    unsigned long cpsr_save, mode;

    asm volatile (
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
        "mov %[result], sp \n"                  /* get SP */
        "msr cpsr, %[cpsr_save] \n"             /* restore mode */
        : [result] "=r" (result),
          [cpsr_save] "=r" (cpsr_save),
          [mode] "=r" (mode)
    );

    return result;
}
