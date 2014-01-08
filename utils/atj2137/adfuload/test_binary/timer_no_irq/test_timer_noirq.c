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

int main(void)
{
    int i = 0, j = 0;

    /* backlight clock enable, select backlight clock as 32kHz */
    CMU_FMCLK = (CMU_FMCLK & ~(CMU_FMCLK_BCLK_MASK)) | CMU_FMCLK_BCKE | CMU_FMCLK_BCLK_32K;

    /* baclight enable */
    PMU_CTL |= PMU_CTL_BL_EN;

    /* pwm output, phase high, some initial duty cycle set as 24/32 */
    PMU_CHG = ((PMU_CHG & ~PMU_CHG_PDOUT_MASK)| PMU_CHG_PBLS_PWM | PMU_CHG_PPHS_HIGH | PMU_CHG_PDUT(24));

    /* ADEC_N63.BIN seems to setup P_CLK as 7.5MHz which is timer clk */
    RTC_T0 = (7500000*10/32); /* with this we should see transition every ~0.3125s and 'black' every ~10s */
    RTC_T0CTL = (1<<5) | (1<<2) | (1<<0); /* timer enable, timer reload, clear irq pending bit */

    while(1)
    {
        /* otherwise wdt will trigger reset */
        wdt_feed();

        /* check if irq pending bit is set */
        if (RTC_T0CTL & 1)
        {
            /* clear irq pending bit */
            RTC_T0CTL |= 1;

            j++;
            backlight_set(j);
        }
    }

    return 0;
} 
