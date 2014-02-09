#include "mips.h"
#include "atj213x.h"

static void backlight_set(int level)
{
    /* set duty cycle in 1/32 units */
    PMU_CHG = ((PMU_CHG & ~PMU_CHG_PDOUT_MASK) | PMU_CHG_PDUT(level));
}

static void wdt_feed(void)
{
    RTC_WDCTL |= RTC_WDCTL_CLR;
}

void set_sw_interrupt0(void)
{
   unsigned int val;
   asm volatile("mfc0 %0,$13" : "=r" (val));
   val |= 0x100;
   asm volatile("mtc0 %0,$13" : "+r" (val));
}

int main(void)
{
    /* backlight clock enable, select backlight clock as 32kHz */
    CMU_FMCLK = (CMU_FMCLK & ~(CMU_FMCLK_BCLK_MASK)) | CMU_FMCLK_BCKE | CMU_FMCLK_BCLK_32K;

    /* baclight enable */
    PMU_CTL |= PMU_CTL_BL_EN;

    /* pwm output, phase high, some initial duty cycle set as 24/32 */
    PMU_CHG = ((PMU_CHG & ~PMU_CHG_PDOUT_MASK)| PMU_CHG_PBLS_PWM | PMU_CHG_PPHS_HIGH | PMU_CHG_PDUT(24));

    /* ADEC_N63.BIN seems to setup P_CLK as 7.5MHz which is timer clk */
    RTC_T0 = (7500000*10/32); /* with this we should see transition every ~0.3125s and 'black' every ~10s */
    RTC_T0CTL = (1<<5) | (1<<2) | (1<<1) | (1<<0); /* timer enable, timer reload, timer irq, clear irq pending bit */

    /* Configure T0 interrupt as IP6. IP6 is unmasked in crt0.S */
    INTC_CFG0 = 0;
    INTC_CFG1 = 0;
    INTC_CFG2 = (1<<10);

    /* unmask T0 source in INTC */
    INTC_MSK |= (1<<10);

    while(1)
    {
        /* otherwise wdt will trigger reset */
        wdt_feed();
    }

    return 0;
}

/* Timer T0 interrupt service routine */ 
INT_T0()
{
    static int j = 0;

    /* clear pending bit in timer module */
    RTC_T0CTL |= 1;

    /* change backligh */
    backlight_set(++j);
}
