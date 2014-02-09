#define default_interrupt(name) \
  extern __attribute__((weak,alias("UIRQ"))) void name (void)

default_interrupt(INT_MCA);
default_interrupt(INT_SD);
default_interrupt(INT_MHA);
/* default_interrupt(INT_USB); seems wrong */
default_interrupt(INT_DSP);
default_interrupt(INT_PCNT);
default_interrupt(INT_WD);
default_interrupt(INT_T1);
default_interrupt(INT_T0);
default_interrupt(INT_RTC);
default_interrupt(INT_DMA);
default_interrupt(INT_KEY);
default_interrupt(INT_EXT);
default_interrupt(INT_IIC2);
default_interrupt(INT_IIC1);
default_interrupt(INT_ADC);
default_interrupt(INT_DAC);
default_interrupt(INT_NAND);
default_interrupt(INT_YUV);

/* this will be panicf() on regular rockbox fw */
static void UIRQ(void)
{
    while(1);
}

/* TRICK ALERT !!!!
 * The table is organized in reversed order so
 * clz on INTC_PD returns the index in this table
 */
void (* const irqvector[])(void) __attribute__((used)) =
{
    UIRQ, UIRQ, UIRQ, UIRQ, UIRQ, INT_YUV, UIRQ, INT_NAND,
    UIRQ, INT_DAC, INT_ADC, UIRQ, UIRQ, INT_IIC1, INT_IIC2, UIRQ,
    UIRQ, INT_EXT, INT_KEY, INT_DMA, INT_RTC, INT_T0, INT_T1, INT_WD,
    INT_PCNT, UIRQ, INT_DSP, UIRQ, INT_MHA, INT_SD, UIRQ, INT_MCA
};

