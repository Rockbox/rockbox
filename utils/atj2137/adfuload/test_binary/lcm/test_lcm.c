#include "mips.h"
#include "atj213x.h"

extern const unsigned short rockboxlogo[];

unsigned short framebuffer[320*240] __attribute__((aligned(32))) = {[0 ... 76799] = 0};

static void backlight_init(void)
{
    /* backlight clock enable, select backlight clock as 32kHz */
    CMU_FMCLK = (CMU_FMCLK & ~(CMU_FMCLK_BCLK_MASK)) | CMU_FMCLK_BCKE | CMU_FMCLK_BCLK_32K;

    /* baclight enable */
    PMU_CTL |= PMU_CTL_BL_EN;

    /* pwm output, phase high, some initial duty cycle set as 24/32 */
    PMU_CHG = ((PMU_CHG & ~PMU_CHG_PDOUT_MASK)| PMU_CHG_PBLS_PWM | PMU_CHG_PPHS_HIGH | PMU_CHG_PDUT(24));

}

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

int lcm_wait_fifo_empty(void)
{
    volatile int timeout = 100000;

    while (--timeout)
        if (YUV2RGB_CTL & 0x04)
            return 0;

    return -1;
}

static inline void lcm_rs_command(void)
{
    lcm_wait_fifo_empty();

    YUV2RGB_CTL = 0x802ae;
}

static inline void lcm_rs_data(void)
{
    lcm_wait_fifo_empty();

    YUV2RGB_CTL = 0x902ae;
}

static inline void lcm_fb_data(void)
{
    lcm_rs_command();
    YUV2RGB_FIFODATA = 0x22; /* write GRAM command */
    YUV2RGB_CTL = 0xa02ae; /* AHB bus write fifo */
}

static inline lcd_reg_write(int reg, int val)
{
     lcm_rs_command();
     YUV2RGB_FIFODATA = reg;
     lcm_rs_data();
     YUV2RGB_FIFODATA = val;
}

/* strips off MIPS segment coding */
#define PHYADDR(x) ((x) & 0x1fffffff)

/* rb compat defines */
#define LCD_WIDTH 240
#define LCD_HEIGHT 320
#define FBADDR(x,y) ((short *)framebuffer + LCD_WIDTH*y + x)

static void lcd_set_gram_area(int x_start, int y_start,
                       int x_end, int y_end)
{
    /* here we exploit the fact that width fits in LSB only */
    lcd_reg_write(0x02, 0x00); /* column start MSB */
    lcd_reg_write(0x03, x_start); /* column start LSB */
    lcd_reg_write(0x04, 0x00); /* column end MSB */
    lcd_reg_write(0x05, x_end); /* column end LSB */

    lcd_reg_write(0x06, y_start>>8); /* row start MSB */
    lcd_reg_write(0x07, y_start&0xff); /* row start LSB */
    lcd_reg_write(0x08, y_end>>8); /* row end MSB */
    lcd_reg_write(0x09, y_end&0xff); /* row end LSB */
}

void lcd_update_rect(int x, int y, int width, int height)
{
    unsigned int end_x, end_y, row, col, *fbptr;

    x &= ~1; /* we transfer 2px at once */
    width = (width + 1) & ~1; /* adjust the width */

    end_x = x + width - 1;
    end_y = y + height - 1;

    lcd_set_gram_area(x, y, end_x, end_y); /* set GRAM window */
    lcm_fb_data(); /* prepare for AHB write to fifo */

    for (row=y; row<=end_y; row++)
    {
        fbptr = (uint32_t *)FBADDR(x,row);
        for (col=x; col<end_x; col+=2)
            YUV2RGB_FIFODATA = *fbptr++;
    }
}

/* for reference
 * When in act213x_fb_data()
 * YUV2RGB_CTL = 0xa02ae; -> DMA0-3 (AHB bus)
 * YUV2RGB_CTL = 0xa00ae; -> DMA4-7 (special bus)
 */
static void lcd_update_dma(void)
{
    lcd_set_gram_area(0, 0, LCD_WIDTH-1, LCD_HEIGHT-1);

    /* AHB bus DMA channels are 0-3 */
    DMA_MODE(0) = (0x00<<29) |    /* dst burst length = single */
                  (0x00<<28) |    /* DMA no reload */
                  (0x00<<27) |    /* dst DSP mode disable */
                  (0x00<<26) |    /* dst row mode */
                  (0x00<<25) |    /* dst address count dir - don't care */
                  (0x01<<24) |    /* dst fixed address */
                  (0x18<<19) |    /* dst DRQ trig source LCM */
                  (0x02<<17) |    /* dst transfer width 32bits */
                  (0x01<<16) |    /* dst transfer fixed size chunks */
                  (0x05<<13) |    /* src burst length = incr8 */
                  (0x00<<12) |    /* reserved */
                  (0x00<<11) |    /* src DSP mode disable */
                  (0x00<<10) |    /* src row mode */
                  (0x00<<9)  |    /* src address count dir - increase */
                  (0x00<<8)  |    /* src address not fixed */
                  (0x10<<3)  |    /* src DRQ trig source SDRAM */
                  (0x02<<1)  |    /* src transfer width 32bit */
                  (0x00<<0);      /* src transfer not in fixed chunks */

    DMA_SRC(0) = PHYADDR((uint32_t)framebuffer);
    DMA_DST(0) = PHYADDR((uint32_t)&YUV2RGB_FIFODATA);

    DMA_CNT(0) = 240*320*2;       /* in bytes */
    DMA_CMD(0) = 1;               /* start dma operation */
}

static lcm_init(void)
{
    CMU_DEVCLKEN |= (1<<8)|(1<<1); /* dma clk, lcm clk */
    CMU_DMACLK = 1;                /* DMA4 clock enable */

    /* reset DMA4 */
    DMAC_CTL |= (1<<20);
    udelay(1);
    DMAC_CTL &= ~(1<<20);

    GPIO_MFCTL1 = (1<<31); // enable multifunction
    GPIO_MFCTL0 = (GPIO_MFCTL0 & ~(0x07<<22 | 0x03<<14 | 0x03<<6 | 0x07<<3 | 0x07)) |  // 0xfe3f3f00
                               0x02<<22 | 0x02<<14 | 0x02<<6 | 0x02<<3|0x02;       // 0x808092
    udelay(1);

    lcm_rs_command();   /* this has side effect of enabling whole block */
    YUV2RGB_CLKCTL = 0x102; /* lcm clock divider */
}

/* google says COG-IZT2298 is 2.8" lcd module */
static lcd_init(void)
{
    int x,y;

    /* lcd reset pin GPIOA16 */
    GPIO_AOUTEN |= (1<<16);
    GPIO_ADAT |= (1<<16);
    mdelay(10);
    GPIO_ADAT &= ~(1<<16);
    mdelay(10);
    GPIO_ADAT |= (1<<16);
    mdelay(10);

    /* lcd controller init sequence matching HX8347-D */
    lcd_reg_write(0xea, 0x00);
    lcd_reg_write(0xeb, 0x20);
    lcd_reg_write(0xec, 0x0f);
    lcd_reg_write(0xed, 0xc4);
    lcd_reg_write(0xe8, 0xc4);
    lcd_reg_write(0xe9, 0xc4);
    lcd_reg_write(0xf1, 0xc4);
    lcd_reg_write(0xf2, 0xc4);
    lcd_reg_write(0x27, 0xc4);
    lcd_reg_write(0x40, 0x00); /* gamma block start */
    lcd_reg_write(0x41, 0x00);
    lcd_reg_write(0x42, 0x01);
    lcd_reg_write(0x43, 0x13);
    lcd_reg_write(0x44, 0x10);
    lcd_reg_write(0x45, 0x26);
    lcd_reg_write(0x46, 0x08);
    lcd_reg_write(0x47, 0x51);
    lcd_reg_write(0x48, 0x02);
    lcd_reg_write(0x49, 0x12);
    lcd_reg_write(0x4a, 0x18);
    lcd_reg_write(0x4b, 0x19);
    lcd_reg_write(0x4c, 0x14);
    lcd_reg_write(0x50, 0x19);
    lcd_reg_write(0x51, 0x2f);
    lcd_reg_write(0x52, 0x2c);
    lcd_reg_write(0x53, 0x3e);
    lcd_reg_write(0x54, 0x3f);
    lcd_reg_write(0x55, 0x3f);
    lcd_reg_write(0x56, 0x2e);
    lcd_reg_write(0x57, 0x77);
    lcd_reg_write(0x58, 0x0b);
    lcd_reg_write(0x59, 0x06);
    lcd_reg_write(0x5a, 0x07);
    lcd_reg_write(0x5b, 0x0d);
    lcd_reg_write(0x5c, 0x1d);
    lcd_reg_write(0x5d, 0xcc); /* gamma block end */
    lcd_reg_write(0x1b, 0x1b);
    lcd_reg_write(0x1a, 0x01);
    lcd_reg_write(0x24, 0x2f);
    lcd_reg_write(0x25, 0x57);
    lcd_reg_write(0x23, 0x86);
    lcd_reg_write(0x18, 0x36); /* 70Hz framerate */
    lcd_reg_write(0x19, 0x01); /* osc enable */
    lcd_reg_write(0x01, 0x00);
    lcd_reg_write(0x1f, 0x88);
    mdelay(5);
    lcd_reg_write(0x1f, 0x80);
    mdelay(5);
    lcd_reg_write(0x1f, 0x90);
    mdelay(5);
    lcd_reg_write(0x1f, 0xd0);
    mdelay(5);
    lcd_reg_write(0x17, 0x05); /* 16bpp */
    lcd_reg_write(0x36, 0x00);
    lcd_reg_write(0x28, 0x38);
    mdelay(40);
    lcd_reg_write(0x28, 0x3c);

    lcd_reg_write(0x02, 0x00); /* column start MSB */
    lcd_reg_write(0x03, 0x00); /* column start LSB */
    lcd_reg_write(0x04, 0x00); /* column end MSB */
    lcd_reg_write(0x05, 0xef); /* column end LSB */
    lcd_reg_write(0x06, 0x00); /* row start MSB */
    lcd_reg_write(0x07, 0x00); /* row start LSB */
    lcd_reg_write(0x08, 0x01); /* row end MSB */
    lcd_reg_write(0x09, 0x3f); /* row end LSB */

    lcm_fb_data(); /* prepare for DMA write to fifo */

    /* clear lcd gram */
    for (y=0; y<LCD_HEIGHT; y++)
        for (x=0; x<(LCD_WIDTH>>1); x++)
            YUV2RGB_FIFODATA = 0;
}

int main(void)
{
    int i;

    RTC_WDCTL &= ~(1<<4); /* disable WDT */

    /* Configure T0 interrupt as IP6. IP6 is unmasked in crt0.S */
    INTC_CFG0 = 0;
    INTC_CFG1 = 0;
    INTC_CFG2 = (1<<10);

    /* unmask T0 source in INTC */
    INTC_MSK |= (1<<10);

    lcm_init();
    lcd_init();
    backlight_init();

        /* copy rb logo image */
    for (i=0; i<240*74; i++)
        framebuffer[i] = rockboxlogo[i];
    lcd_update_rect(0,0,240,74);

    /* ADEC_N63.BIN seems to setup P_CLK as 7.5MHz which is timer clk */
    RTC_T0 = (7500000*10/32); /* with this we should see transition every ~0.3125s and 'black' every ~10s */
    RTC_T0CTL = (1<<5) | (1<<2) | (1<<1) | (1<<0); /* timer enable, timer reload, timer irq, clear irq pending bit */

    while(1)
    {
        /* otherwise wdt will trigger reset */
        //wdt_feed();
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
