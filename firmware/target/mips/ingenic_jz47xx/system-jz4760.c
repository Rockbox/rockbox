/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Vortex
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
 
#include "config.h"
#include "cpu.h"
#include "mips.h"
#include "mmu-mips.h"
#include "panic.h"
#include "system.h"
#include "kernel.h"
#include "power.h"

static int irq;
static void UIRQ(void)
{
    panicf("Unhandled interrupt occurred: %d", irq);
}

#define intr(name) extern __attribute__((weak,alias("UIRQ"))) void name (void)

intr(I2C1);intr(I2C0);intr(UART3);intr(UART2);intr(UART1);intr(UART0);intr(GPU);
intr(SSI1);intr(SSI0);intr(TSSI);intr(KBC);intr(SADC);intr(ETH);intr(UHC);
intr(OTG);intr(TCU2);intr(TCU1);intr(TCU0);intr(GPS);intr(IPU);intr(CIM);
intr(LCD);intr(RTC);intr(OWI);intr(AIC);intr(MSC2);intr(MSC1);intr(MSC0);
intr(SCC);intr(BCH);intr(PCM);intr(HARB0);intr(HARB2);intr(AOSD);intr(CPM);

intr(DMA0);intr(DMA1);intr(DMA2);intr(DMA3);intr(DMA4);intr(DMA5);
intr(DMA6);intr(DMA7);intr(DMA8);intr(DMA9);intr(DMA10);intr(DMA11);
intr(MDMA0);intr(MDMA1);intr(MDMA2);
intr(BDMA0);intr(BDMA1);intr(BDMA2);

intr(GPIO0);intr(GPIO1);intr(GPIO2);intr(GPIO3);intr(GPIO4);intr(GPIO5);
intr(GPIO6);intr(GPIO7);intr(GPIO8);intr(GPIO9);intr(GPIO10);intr(GPIO11);
intr(GPIO12);intr(GPIO13);intr(GPIO14);intr(GPIO15);intr(GPIO16);intr(GPIO17);
intr(GPIO18);intr(GPIO19);intr(GPIO20);intr(GPIO21);intr(GPIO22);intr(GPIO23);
intr(GPIO24);intr(GPIO25);intr(GPIO26);intr(GPIO27);intr(GPIO28);intr(GPIO29);
intr(GPIO30);intr(GPIO31);intr(GPIO32);intr(GPIO33);intr(GPIO34);intr(GPIO35);
intr(GPIO36);intr(GPIO37);intr(GPIO38);intr(GPIO39);intr(GPIO40);intr(GPIO41);
intr(GPIO42);intr(GPIO43);intr(GPIO44);intr(GPIO45);intr(GPIO46);intr(GPIO47);
intr(GPIO48);intr(GPIO49);intr(GPIO50);intr(GPIO51);intr(GPIO52);intr(GPIO53);
intr(GPIO54);intr(GPIO55);intr(GPIO56);intr(GPIO57);intr(GPIO58);intr(GPIO59);
intr(GPIO60);intr(GPIO61);intr(GPIO62);intr(GPIO63);intr(GPIO64);intr(GPIO65);
intr(GPIO66);intr(GPIO67);intr(GPIO68);intr(GPIO69);intr(GPIO70);intr(GPIO71);
intr(GPIO72);intr(GPIO73);intr(GPIO74);intr(GPIO75);intr(GPIO76);intr(GPIO77);
intr(GPIO78);intr(GPIO79);intr(GPIO80);intr(GPIO81);intr(GPIO82);intr(GPIO83);
intr(GPIO84);intr(GPIO85);intr(GPIO86);intr(GPIO87);intr(GPIO88);intr(GPIO89);
intr(GPIO90);intr(GPIO91);intr(GPIO92);intr(GPIO93);intr(GPIO94);intr(GPIO95);
intr(GPIO96);intr(GPIO97);intr(GPIO98);intr(GPIO99);intr(GPIO100);intr(GPIO101);
intr(GPIO102);intr(GPIO103);intr(GPIO104);intr(GPIO105);intr(GPIO106);
intr(GPIO107);intr(GPIO108);intr(GPIO109);intr(GPIO110);intr(GPIO111);
intr(GPIO112);intr(GPIO113);intr(GPIO114);intr(GPIO115);intr(GPIO116);
intr(GPIO117);intr(GPIO118);intr(GPIO119);intr(GPIO120);intr(GPIO121);
intr(GPIO122);intr(GPIO123);intr(GPIO124);intr(GPIO125);intr(GPIO126);
intr(GPIO127);intr(GPIO128);intr(GPIO129);intr(GPIO130);intr(GPIO131);
intr(GPIO132);intr(GPIO133);intr(GPIO134);intr(GPIO135);intr(GPIO136);
intr(GPIO137);intr(GPIO138);intr(GPIO139);intr(GPIO140);intr(GPIO141);
intr(GPIO142);intr(GPIO143);intr(GPIO144);intr(GPIO145);intr(GPIO146);
intr(GPIO147);intr(GPIO148);intr(GPIO149);intr(GPIO150);intr(GPIO151);
intr(GPIO152);intr(GPIO153);intr(GPIO154);intr(GPIO155);intr(GPIO156);
intr(GPIO157);intr(GPIO158);intr(GPIO159);intr(GPIO160);intr(GPIO161);
intr(GPIO162);intr(GPIO163);intr(GPIO164);intr(GPIO165);intr(GPIO166);
intr(GPIO167);intr(GPIO168);intr(GPIO169);intr(GPIO170);intr(GPIO171);
intr(GPIO172);intr(GPIO173);intr(GPIO174);intr(GPIO175);intr(GPIO176);
intr(GPIO177);intr(GPIO178);intr(GPIO179);intr(GPIO180);intr(GPIO181);
intr(GPIO182);intr(GPIO183);intr(GPIO184);intr(GPIO185);intr(GPIO186);
intr(GPIO187);intr(GPIO188);intr(GPIO189);intr(GPIO190);intr(GPIO191);

static void (* const irqvector[])(void) =
{
    I2C1,I2C0,UART3,UART2,UART1,UART0,GPU,SSI1,
    SSI0,TSSI,UIRQ,KBC,UIRQ,UIRQ,UIRQ,UIRQ,
    UIRQ,UIRQ,SADC,ETH,UHC,OTG,UIRQ,UIRQ,
    UIRQ,TCU2,TCU1,TCU0,GPS,IPU,CIM,LCD,

    RTC,OWI,AIC,MSC2,MSC1,MSC0,SCC,BCH, // 32
    PCM,HARB0,HARB2,AOSD,CPM,UIRQ,

    DMA0,DMA1,DMA2,DMA3,DMA4,DMA5,DMA6,DMA7, // 46
    DMA8,DMA9,DMA10,DMA11,MDMA0,MDMA1,MDMA2,BDMA0,
    BDMA1,BDMA2,

    GPIO0,GPIO1,GPIO2,GPIO3,GPIO4,GPIO5,GPIO6,GPIO7, // 64
    GPIO8,GPIO9,GPIO10,GPIO11,GPIO12,GPIO13,GPIO14,GPIO15,
    GPIO16,GPIO17,GPIO18,GPIO19,GPIO20,GPIO21,GPIO22,GPIO23,
    GPIO24,GPIO25,GPIO26,GPIO27,GPIO28,GPIO29,GPIO30,GPIO31,
    GPIO32,GPIO33,GPIO34,GPIO35,GPIO36,GPIO37,GPIO38,GPIO39,
    GPIO40,GPIO41,GPIO42,GPIO43,GPIO44,GPIO45,GPIO46,GPIO47,
    GPIO48,GPIO49,GPIO50,GPIO51,GPIO52,GPIO53,GPIO54,GPIO55,
    GPIO56,GPIO57,GPIO58,GPIO59,GPIO60,GPIO61,GPIO62,GPIO63,
    GPIO64,GPIO65,GPIO66,GPIO67,GPIO68,GPIO69,GPIO70,GPIO71,
    GPIO72,GPIO73,GPIO74,GPIO75,GPIO76,GPIO77,GPIO78,GPIO79,
    GPIO80,GPIO81,GPIO82,GPIO83,GPIO84,GPIO85,GPIO86,GPIO87,
    GPIO88,GPIO89,GPIO90,GPIO91,GPIO92,GPIO93,GPIO94,GPIO95,
    GPIO96,GPIO97,GPIO98,GPIO99,GPIO100,GPIO101,GPIO102,GPIO103,
    GPIO104,GPIO105,GPIO106,GPIO107,GPIO108,GPIO109,GPIO110,GPIO111,
    GPIO112,GPIO113,GPIO114,GPIO115,GPIO116,GPIO117,GPIO118,GPIO119,
    GPIO120,GPIO121,GPIO122,GPIO123,GPIO124,GPIO125,GPIO126,GPIO127,
    GPIO128,GPIO129,GPIO130,GPIO131,GPIO132,GPIO133,GPIO134,GPIO135,
    GPIO136,GPIO137,GPIO138,GPIO139,GPIO140,GPIO141,GPIO142,GPIO143,
    GPIO144,GPIO145,GPIO146,GPIO147,GPIO148,GPIO149,GPIO150,GPIO151,
    GPIO152,GPIO153,GPIO154,GPIO155,GPIO156,GPIO157,GPIO158,GPIO159,
    GPIO160,GPIO161,GPIO162,GPIO163,GPIO164,GPIO165,GPIO166,GPIO167,
    GPIO168,GPIO169,GPIO170,GPIO171,GPIO172,GPIO173,GPIO174,GPIO175,
    GPIO176,GPIO177,GPIO178,GPIO179,GPIO180,GPIO181,GPIO182,GPIO183,
    GPIO184,GPIO185,GPIO186,GPIO187,GPIO188,GPIO189,GPIO190,GPIO191
};

static unsigned int dma_irq_mask = 0;
static unsigned char mdma_irq_mask = 0;
static unsigned char bdma_irq_mask = 0;
static unsigned int gpio_irq_mask[6] = {0};

void system_enable_irq(unsigned int irq)
{
    register unsigned int t;
    if ((irq >= IRQ_GPIO_0) && (irq <= IRQ_GPIO_0 + NUM_GPIO))
    {
        __gpio_unmask_irq(irq - IRQ_GPIO_0);
        t = (irq - IRQ_GPIO_0) >> 5;
        gpio_irq_mask[t] |= (1 << ((irq - IRQ_GPIO_0) & 0x1f));
        __intc_unmask_irq(IRQ_GPIO0 - t);
    }
    else if ((irq >= IRQ_DMA_0) && (irq <= IRQ_DMA_0 + NUM_DMA))
    {
        __dmac_channel_enable_irq(irq - IRQ_DMA_0);
        t = (irq - IRQ_DMA_0) / HALF_DMA_NUM;
        dma_irq_mask |= (1 << (irq - IRQ_DMA_0));
        __intc_unmask_irq(IRQ_DMAC0 - t);
    }
    else if ((irq >= IRQ_MDMA_0) && (irq <= IRQ_MDMA_0 + NUM_MDMA))
    {
        __mdmac_channel_enable_irq(irq - IRQ_MDMA_0);
        mdma_irq_mask |= (1 << (irq - IRQ_MDMA_0));
        __intc_unmask_irq(IRQ_MDMA);
    }
    else if ((irq >= IRQ_BDMA_0) && (irq <= IRQ_BDMA_0 + NUM_BDMA))
    {
        __bdmac_channel_enable_irq(irq - IRQ_BDMA_0);
        bdma_irq_mask |= (1 << (irq - IRQ_BDMA_0));
        __intc_unmask_irq(IRQ_BDMA);
    }
    else if (irq < IRQ_INTC_MAX)
        __intc_unmask_irq(irq);
}

static void dis_irq(unsigned int irq)
{
    register unsigned int t;
    if ((irq >= IRQ_GPIO_0) && (irq <= IRQ_GPIO_0 + NUM_GPIO))
    {
        __gpio_mask_irq(irq - IRQ_GPIO_0);
        t = (irq - IRQ_GPIO_0) >> 5;
        gpio_irq_mask[t] &= ~(1 << ((irq - IRQ_GPIO_0) & 0x1f));
        if (!gpio_irq_mask[t])
            __intc_mask_irq(IRQ_GPIO0 - t);
    }
    else if ((irq >= IRQ_DMA_0) && (irq < IRQ_DMA_0 + NUM_DMA))
    {
        __dmac_channel_disable_irq(irq - IRQ_DMA_0);
        dma_irq_mask &= ~(1 << (irq - IRQ_DMA_0));
        if (!(dma_irq_mask & 0x003F))
            __intc_mask_irq(IRQ_DMAC0);
        if (!(dma_irq_mask & 0x0FC0))
            __intc_mask_irq(IRQ_DMAC1);
    }
    else if ((irq >= IRQ_MDMA_0) && (irq < IRQ_MDMA_0 + NUM_MDMA))
    {
        __mdmac_channel_disable_irq(irq - IRQ_MDMA_0);
        mdma_irq_mask &= ~(1 << (irq - IRQ_MDMA_0));
        if (!mdma_irq_mask)
            __intc_mask_irq(IRQ_MDMA);
    }
    else if ((irq >= IRQ_BDMA_0) && (irq < IRQ_BDMA_0 + NUM_BDMA))
    {
        __bdmac_channel_disable_irq(irq - IRQ_BDMA_0);
        bdma_irq_mask &= ~(1 << (irq - IRQ_BDMA_0));
        if (!bdma_irq_mask)
            __intc_mask_irq(IRQ_BDMA);
    }
    else if (irq < IRQ_INTC_MAX)
        __intc_mask_irq(irq);
}

static void ack_irq(unsigned int irq)
{
    if ((irq >= IRQ_GPIO_0) && (irq <= IRQ_GPIO_0 + NUM_GPIO))
    {
        __gpio_ack_irq(irq - IRQ_GPIO_0);
    }
}

static int get_irq_number(void)
{
    static unsigned long ipl0, ipl1;
    register int irq0, irq1;

    ipl0 |= REG_INTC_ICPR(0);
    ipl1 |= REG_INTC_ICPR(1);

    if (!(ipl0 || ipl1))
        return -1;

    __asm__ __volatile__("negu  $8, %0        \n"
                         "and   $8, %0, $8    \n"
                         "clz   %0, %1        \n"
                         "li    $8, 31        \n"
                         "subu  %0, $8, %0    \n"
                         : "=r" (irq0)
                         : "r" (ipl0)
                         : "t0"
                        );

    __asm__ __volatile__("negu  $8, %0        \n"
                         "and   $8, %0, $8    \n"
                         "clz   %0, %1        \n"
                         "li    $8, 31        \n"
                         "subu  %0, $8, %0    \n"
                         : "=r" (irq1)
                         : "r" (ipl1)
                         : "t0"
                        );

    if (UNLIKELY(irq0 < 0) && UNLIKELY(irq1 < 0))
        return -1;

    if (!(ipl0 & 3)) {
        if (ipl0) {
            irq = irq0;
            ipl0 &= ~(1<<irq0);
        } else {
            irq = irq1 + 32;
            ipl1 &= ~(1<<irq1);
        }
    } else  {
        if (ipl0 & 2) {
            irq = 1;
            ipl0 &= ~(1<<irq);
        } else {
            irq = 0;
            ipl0 &= ~(1<<irq);
        }
    }

    switch (irq)
    {
        case IRQ_GPIO0:
        case IRQ_GPIO1:
        case IRQ_GPIO2:
        case IRQ_GPIO3:
        case IRQ_GPIO4:
        case IRQ_GPIO5:
            irq = __gpio_get_irq() + IRQ_GPIO_0;
            break;
        case IRQ_DMAC0:
        case IRQ_DMAC1:
            irq = __dmac_get_irq() + IRQ_DMA_0;
            break;
        case IRQ_MDMA:
            irq = __mdmac_get_irq() + IRQ_MDMA_0;
            break;
        case IRQ_BDMA:
            irq = __bdmac_get_irq() + IRQ_BDMA_0;
            break;
    }

    return irq;
}

void intr_handler(void)
{
    register int irq = get_irq_number();
    if(UNLIKELY(irq < 0))
        return;
    
    ack_irq(irq);
    if(LIKELY(irq >= 0))
        irqvector[irq]();
}

#define EXC(x,y) case (x): return (y);
static char* parse_exception(unsigned int cause)
{
    switch(cause & M_CauseExcCode)
    {
        EXC(EXC_INT, "Interrupt");
        EXC(EXC_MOD, "TLB Modified");
        EXC(EXC_TLBL, "TLB Exception (Load or Ifetch)");
        EXC(EXC_ADEL, "Address Error (Load or Ifetch)");
        EXC(EXC_ADES, "Address Error (Store)");
        EXC(EXC_TLBS, "TLB Exception (Store)");
        EXC(EXC_IBE, "Instruction Bus Error");
        EXC(EXC_DBE, "Data Bus Error");
        EXC(EXC_SYS, "Syscall");
        EXC(EXC_BP, "Breakpoint");
        EXC(EXC_RI, "Reserved Instruction");
        EXC(EXC_CPU, "Coprocessor Unusable");
        EXC(EXC_OV, "Overflow");
        EXC(EXC_TR, "Trap Instruction");
        EXC(EXC_FPE, "Floating Point Exception");
        EXC(EXC_C2E, "COP2 Exception");
        EXC(EXC_MDMX, "MDMX Exception");
        EXC(EXC_WATCH, "Watch Exception");
        EXC(EXC_MCHECK, "Machine Check Exception");
        EXC(EXC_CacheErr, "Cache error caused re-entry to Debug Mode");
        default:
            return NULL;
    }
}

void exception_handler(void* stack_ptr, unsigned int cause, unsigned int epc)
{
    panicf("Exception occurred: %s [0x%08x] at 0x%08x (stack at 0x%08x)", parse_exception(cause), read_c0_badvaddr(), epc, (unsigned int)stack_ptr);
}

void tlb_refill_handler(void)
{
    panicf("TLB refill handler at 0x%08lx! [0x%x]", read_c0_epc(), read_c0_badvaddr());
}

void udelay(unsigned int usec)
{
    unsigned int i = usec * (__cpm_get_cclk() / 2000000);
    __asm__ __volatile__ (
                          ".set noreorder    \n"
                          "1:                \n"
                          "bne  %0, $0, 1b   \n"
                          "addi %0, %0, -1   \n"
                          ".set reorder      \n"
                          : "=r" (i)
                          : "0" (i)
                          );
}

void mdelay(unsigned int msec)
{
    unsigned int i;
    for(i=0; i<msec; i++)
        udelay(1000);
}

#define MHZ (1000 * 1000)
static inline unsigned int pll_calc_m_n_od(unsigned int speed, unsigned int xtal)
{
	const int pll_m_max = 0x7f, pll_m_min = 4;
	const int pll_n_max = 0x0f, pll_n_min = 2;

	int od[] = {1, 2, 4, 8};

	unsigned int plcr_m_n_od = 0;
	unsigned int distance;
	unsigned int tmp, raw;

	int i, j, k;
	int m, n;

	distance = 0xFFFFFFFF;

	for (i = 0; i < (int)sizeof (od) / (int)sizeof(int); i++) {
		/* Limit: 500MHZ <= CLK_OUT * OD <= 1500MHZ */
		if ((speed * od[i]) < 500 * MHZ || (speed * od[i]) > 1500 * MHZ)
			continue;
		for (k = pll_n_min; k <= pll_n_max; k++) {
			n = k;
			
			/* Limit: 1MHZ <= XIN/N <= 50MHZ */
			if ((xtal / n) < (1 * MHZ))
				break;
			if ((xtal / n) > (15 * MHZ))
				continue;

			for (j = pll_m_min; j <= pll_m_max; j++) {
				m = j*2;

				raw = xtal * m / n;
				tmp = raw / od[i];

				tmp = (tmp > speed) ? (tmp - speed) : (speed - tmp);

				if (tmp < distance) {
					distance = tmp;
					
					plcr_m_n_od = (j << CPPCR0_PLLM_LSB) 
						| (k << CPPCR0_PLLN_LSB)
						| (i << CPPCR0_PLLOD_LSB);

					if (!distance) {	/* Match. */
						return plcr_m_n_od;
					}
				}
			}
		}
	}
	return plcr_m_n_od;
}

/* PLL output clock = EXTAL * NF / (NR * NO)
 *
 * NF = FD + 2, NR = RD + 2
 * NO = 1 (if OD = 0), NO = 2 (if OD = 1 or 2), NO = 4 (if OD = 3)
 */
static void pll0_init(unsigned int freq)
{
    register unsigned int cfcr, plcr1;
    int n2FR[9] = {
        0, 0, 1, 2, 3, 0, 4, 0, 5
    };

    /** divisors,
     *  for jz4760b,I:H:H2:P:M:S.
     *  DIV should be one of [1, 2, 3, 4, 6, 8]
     */
    int div[6] = {1, 4, 4, 4, 4, 4};
    int usbdiv;

    /* set ahb **/
    REG32(HARB0_BASE) = 0x00300000;
    REG32(0xb3070048) = 0x00000000;
    REG32(HARB2_BASE) = 0x00FFFFFF;
	
    cfcr = CPCCR_PCS |
        (n2FR[div[0]] << CPCCR_CDIV_LSB) |
        (n2FR[div[1]] << CPCCR_HDIV_LSB) |
        (n2FR[div[2]] << CPCCR_H2DIV_LSB) |
        (n2FR[div[3]] << CPCCR_PDIV_LSB) |
        (n2FR[div[4]] << CPCCR_MDIV_LSB) |
        (n2FR[div[5]] << CPCCR_SDIV_LSB);

    // write REG_DDRC_CTRL 8 times to clear ddr fifo
    REG_DDRC_CTRL = 0;
    REG_DDRC_CTRL = 0;
    REG_DDRC_CTRL = 0;
    REG_DDRC_CTRL = 0;
    REG_DDRC_CTRL = 0;
    REG_DDRC_CTRL = 0;
    REG_DDRC_CTRL = 0;
    REG_DDRC_CTRL = 0;

    if (CFG_EXTAL > 16000000)
        cfcr |= CPCCR_ECS;
    else
        cfcr &= ~CPCCR_ECS;

    cfcr &= ~CPCCR_MEM; /* mddr */
    cfcr |= CPCCR_CE;

    plcr1 = pll_calc_m_n_od(freq, CFG_EXTAL);
    plcr1 |= (0x20 << CPPCR0_PLLST_LSB)  /* PLL stable time */
             | CPPCR0_PLLEN;             /* enable PLL */
	
    /*
     * Init USB Host clock, pllout2 must be n*48MHz
     * For JZ4760b UHC - River.
     */
    usbdiv = (cfcr & CPCCR_PCS) ? CPU_FREQ : (CPU_FREQ / 2);
    REG_CPM_UHCCDR = usbdiv / 48000000 - 1;

    /* init PLL */
    REG_CPM_CPCCR = cfcr;
    REG_CPM_CPPCR0 = plcr1;

    __cpm_enable_pll_change();

    /*wait for pll output stable ...*/
    while (!(REG_CPM_CPPCR0 & CPPCR0_PLLS));

    REG_CPM_CPPCR0 &= ~CPPCR0_LOCK;
}

void pll1_init(unsigned int freq)
{
    register unsigned int plcr2;

    /* set CPM_CPCCR_MEM only for ddr1 or ddr2 */
    plcr2 = pll_calc_m_n_od(freq, CFG_EXTAL)
            | CPPCR1_PLL1EN;            /* enable PLL1 */

    /* init PLL_1 , source clock is extal clock */
    REG_CPM_CPPCR1 = plcr2;

    __cpm_enable_pll_change();

    /*wait for pll_1 output stable ...*/
    while (!(REG_CPM_CPPCR1 & CPPCR1_PLL1S));

    REG_CPM_CPPCR1 &= ~CPPCR1_LOCK;
}

static void serial_setbrg(void)
{
    volatile u8 *uart_lcr = (volatile u8 *)(CFG_UART_BASE + OFF_LCR);
    volatile u8 *uart_dlhr = (volatile u8 *)(CFG_UART_BASE + OFF_DLHR);
    volatile u8 *uart_dllr = (volatile u8 *)(CFG_UART_BASE + OFF_DLLR);
    volatile u8 *uart_umr = (volatile u8 *)(CFG_UART_BASE + OFF_UMR);
    volatile u8 *uart_uacr = (volatile u8 *)(CFG_UART_BASE + OFF_UACR);
    u16 baud_div, tmp;

    *uart_umr = 16;
    *uart_uacr = 0;
    baud_div = 13; /* 57600 */

    tmp = *uart_lcr;
    tmp |= UARTLCR_DLAB;
    *uart_lcr = tmp;

    *uart_dlhr = (baud_div >> 8) & 0xff;
    *uart_dllr = baud_div & 0xff;

    tmp &= ~UARTLCR_DLAB;
    *uart_lcr = tmp;
}

int serial_preinit(void)
{
    volatile u8 *uart_fcr = (volatile u8 *)(CFG_UART_BASE + OFF_FCR);
    volatile u8 *uart_lcr = (volatile u8 *)(CFG_UART_BASE + OFF_LCR);
    volatile u8 *uart_ier = (volatile u8 *)(CFG_UART_BASE + OFF_IER);
    volatile u8 *uart_sircr = (volatile u8 *)(CFG_UART_BASE + OFF_SIRCR);

    __gpio_as_uart1();
    __cpm_start_uart1();

    /* Disable port interrupts while changing hardware */
    *uart_ier = 0;

    /* Disable UART unit function */
    *uart_fcr = ~UARTFCR_UUE;

    /* Set both receiver and transmitter in UART mode (not SIR) */
    *uart_sircr = ~(SIRCR_RSIRE | SIRCR_TSIRE);

    /* Set databits, stopbits and parity. (8-bit data, 1 stopbit, no parity) */
    *uart_lcr = UARTLCR_WLEN_8 | UARTLCR_STOP1;
	
    /* Set baud rate */
    serial_setbrg();
	
    /* Enable UART unit, enable and clear FIFO */
    *uart_fcr = UARTFCR_UUE | UARTFCR_FE | UARTFCR_TFLS | UARTFCR_RFLS;

    return 0;
}

void usb_preinit(void)
{
    /* Clear ECS bit of CPCCR, 0:clock source is EXCLK, 1:clock source is EXCLK/2 */
    REG_CPM_CPCCR &= ~CPCCR_ECS;

    /* Clear all bits of USBCDR, 0:OTG clock source is pin EXCLK, PLL0 output, divider = 1:12MHZ */
    REG_CPM_USBCDR = 0;

    /* Set CE bit of CPCCR, it means frequence is changed immediately */
    REG_CPM_CPCCR |= CPCCR_CE;

    udelay(3);

    /* Clear OTG bit of CLKGR0, 0:device can be accessed */
    REG_CPM_CLKGR0 &= ~CLKGR0_OTG;

    /* fil */
    REG_CPM_USBVBFIL = 0x80;

    /* rdt */
    REG_CPM_USBRDT = (600 * (CPU_FREQ / 1000000)) / 1000;

    /* rdt - filload_en */
    REG_CPM_USBRDT |= (1 << 25);

    /* TXRISETUNE & TXVREFTUNE. */
    REG_CPM_USBPCR &= ~0x3f;
    REG_CPM_USBPCR |= 0x35;

    /* enable tx pre-emphasis */
    REG_CPM_USBPCR |= 0x40;

    /* most DC leave of tx */
    REG_CPM_USBPCR |= 0xf;

    /* Device Mode. */
    REG_CPM_USBPCR &= ~(1 << 31);
    REG_CPM_USBPCR |= USBPCR_VBUSVLDEXT;

    /* phy reset */
    REG_CPM_USBPCR |= USBPCR_POR;
    udelay(30);
    REG_CPM_USBPCR &= ~USBPCR_POR;
    udelay(300);

    /* Enable the USB PHY */
    REG_CPM_OPCR |= OPCR_OTGPHY_ENABLE;

    /* Wait PHY Clock Stable. */
    udelay(300);
}

void dma_preinit(void)
{
    __cpm_start_mdma();
    __cpm_start_dmac();

    REG_MDMAC_DMACKES = 0x1;

    REG_DMAC_DMACR(DMA_AIC_TX_CHANNEL) = DMAC_DMACR_DMAE | DMAC_DMACR_FAIC;
    REG_DMAC_DMACR(DMA_SD_RX_CHANNEL) = DMAC_DMACR_DMAE | DMAC_DMACR_FMSC;
    REG_DMAC_DMACR(DMA_SD_TX_CHANNEL) = DMAC_DMACR_DMAE | DMAC_DMACR_FMSC;
}

/* Gets called *before* main */
void ICODE_ATTR system_main(void)
{
    int i;
       
    __dcache_writeback_all();
    __icache_invalidate_all();
    
    write_c0_status(1 << 28 | 1 << 10 ); /* Enable CP | Mask interrupt 2 */

    /* Disable all interrupts */
    for(i=0; i<IRQ_INTC_MAX; i++)
        dis_irq(i);

    mmu_init();

    pll0_init(CPU_FREQ);
    pll1_init(CPU_FREQ);

    serial_preinit();
    usb_preinit();
    dma_preinit();

    /* Enable interrupts at core level */
    enable_interrupt();
}

void system_reboot(void)
{
    REG_WDT_WCSR = WCSR_PRESCALE4 | WCSR_CLKIN_EXT;
    REG_WDT_WCNT = 0;
    REG_WDT_WDR = JZ_EXTAL/1000; /* reset after 4ms */
    REG_TCU_TSCR = TSCR_WDT;     /* enable wdt clock */
    REG_WDT_WCER = WCER_TCEN;    /* wdt start */
    while (1);
}

void system_exception_wait(void)
{
    /* check for power button without including any .h file */
    while(1)
    {
        if( (~REG_GPIO_PXPIN(0)) & (1 << 30) )
            return;
        asm volatile("nop");
    }
}

void power_off(void)
{
    REG_CPM_RSR = 0x0;
	
    /* Set minimum wakeup_n pin low-level assertion time for wakeup: 100ms */
    rtc_write_reg(RTC_HWFCR, HWFCR_WAIT_TIME(1000));

    /* Set reset pin low-level assertion time after wakeup: must  > 60ms */
    rtc_write_reg(RTC_HRCR, HRCR_WAIT_TIME(60));

    /* clear wakeup status register */
    rtc_write_reg(RTC_HWRSR, 0x0);

    /* set wake up valid level as low */
    rtc_write_reg(RTC_HWCR,0x8);

    /* Put CPU to hibernate mode */
    rtc_write_reg(RTC_HCR, HCR_PD);

    while (1);
}

void system_init(void)
{
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
void set_cpu_frequency(long frequency)
{
    serial_putsf("set_cpu_frequency: %d\n", frequency);
}
#endif
