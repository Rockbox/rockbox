/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
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
#include "jz4740.h"
#include "mips.h"
#include "mipsregs.h"
#include "panic.h"
#include "system-target.h"
#include <string.h>
#include "kernel.h"

#define NUM_DMA  6
#define NUM_GPIO 128
#define IRQ_MAX  (IRQ_GPIO_0 + NUM_GPIO)

static int irq;
static void UIRQ(void)
{
    panicf("Unhandled interrupt occurred: %d\n", irq);
}

#define default_interrupt(name) \
  extern __attribute__((weak,alias("UIRQ"))) void name (void)

default_interrupt(I2C);
default_interrupt(EMC);
default_interrupt(UHC);
default_interrupt(UART0);
default_interrupt(SADC);
default_interrupt(MSC);
default_interrupt(RTC);
default_interrupt(SSI);
default_interrupt(CIM);
default_interrupt(AIC);
default_interrupt(ETH);
default_interrupt(TCU2);
default_interrupt(TCU1);
default_interrupt(TCU0);
default_interrupt(UDC);
default_interrupt(IPU);
default_interrupt(LCD);

default_interrupt(DMA0);
default_interrupt(DMA1);
default_interrupt(DMA2);
default_interrupt(DMA3);
default_interrupt(DMA4);
default_interrupt(DMA5);

default_interrupt(GPIO0);
default_interrupt(GPIO1);
default_interrupt(GPIO2);
default_interrupt(GPIO3);
default_interrupt(GPIO4);
default_interrupt(GPIO5);
default_interrupt(GPIO6);
default_interrupt(GPIO7);
default_interrupt(GPIO8);
default_interrupt(GPIO9);
default_interrupt(GPIO10);
default_interrupt(GPIO11);
default_interrupt(GPIO12);
default_interrupt(GPIO13);
default_interrupt(GPIO14);
default_interrupt(GPIO15);
default_interrupt(GPIO16);
default_interrupt(GPIO17);
default_interrupt(GPIO18);
default_interrupt(GPIO19);
default_interrupt(GPIO20);
default_interrupt(GPIO21);
default_interrupt(GPIO22);
default_interrupt(GPIO23);
default_interrupt(GPIO24);
default_interrupt(GPIO25);
default_interrupt(GPIO26);
default_interrupt(GPIO27);
default_interrupt(GPIO28);
default_interrupt(GPIO29);
default_interrupt(GPIO30);
default_interrupt(GPIO31);
default_interrupt(GPIO32);
default_interrupt(GPIO33);
default_interrupt(GPIO34);
default_interrupt(GPIO35);
default_interrupt(GPIO36);
default_interrupt(GPIO37);
default_interrupt(GPIO38);
default_interrupt(GPIO39);
default_interrupt(GPIO40);
default_interrupt(GPIO41);
default_interrupt(GPIO42);
default_interrupt(GPIO43);
default_interrupt(GPIO44);
default_interrupt(GPIO45);
default_interrupt(GPIO46);
default_interrupt(GPIO47);
default_interrupt(GPIO48);
default_interrupt(GPIO49);
default_interrupt(GPIO50);
default_interrupt(GPIO51);
default_interrupt(GPIO52);
default_interrupt(GPIO53);
default_interrupt(GPIO54);
default_interrupt(GPIO55);
default_interrupt(GPIO56);
default_interrupt(GPIO57);
default_interrupt(GPIO58);
default_interrupt(GPIO59);
default_interrupt(GPIO60);
default_interrupt(GPIO61);
default_interrupt(GPIO62);
default_interrupt(GPIO63);
default_interrupt(GPIO64);
default_interrupt(GPIO65);
default_interrupt(GPIO66);
default_interrupt(GPIO67);
default_interrupt(GPIO68);
default_interrupt(GPIO69);
default_interrupt(GPIO70);
default_interrupt(GPIO71);
default_interrupt(GPIO72);
default_interrupt(GPIO73);
default_interrupt(GPIO74);
default_interrupt(GPIO75);
default_interrupt(GPIO76);
default_interrupt(GPIO77);
default_interrupt(GPIO78);
default_interrupt(GPIO79);
default_interrupt(GPIO80);
default_interrupt(GPIO81);
default_interrupt(GPIO82);
default_interrupt(GPIO83);
default_interrupt(GPIO84);
default_interrupt(GPIO85);
default_interrupt(GPIO86);
default_interrupt(GPIO87);
default_interrupt(GPIO88);
default_interrupt(GPIO89);
default_interrupt(GPIO90);
default_interrupt(GPIO91);
default_interrupt(GPIO92);
default_interrupt(GPIO93);
default_interrupt(GPIO94);
default_interrupt(GPIO95);
default_interrupt(GPIO96);
default_interrupt(GPIO97);
default_interrupt(GPIO98);
default_interrupt(GPIO99);
default_interrupt(GPIO100);
default_interrupt(GPIO101);
default_interrupt(GPIO102);
default_interrupt(GPIO103);
default_interrupt(GPIO104);
default_interrupt(GPIO105);
default_interrupt(GPIO106);
default_interrupt(GPIO107);
default_interrupt(GPIO108);
default_interrupt(GPIO109);
default_interrupt(GPIO110);
default_interrupt(GPIO111);
default_interrupt(GPIO112);
default_interrupt(GPIO113);
default_interrupt(GPIO114);
default_interrupt(GPIO115);
default_interrupt(GPIO116);
default_interrupt(GPIO117);
default_interrupt(GPIO118);
default_interrupt(GPIO119);
default_interrupt(GPIO120);
default_interrupt(GPIO121);
default_interrupt(GPIO122);
default_interrupt(GPIO123);
default_interrupt(GPIO124);
default_interrupt(GPIO125);
default_interrupt(GPIO126);
default_interrupt(GPIO127);

static void (* const irqvector[])(void) =
{
    I2C,EMC,UHC,UIRQ,UIRQ,UIRQ,UIRQ,UIRQ,
    UART0,UIRQ,UIRQ,SADC,UIRQ,MSC,RTC,SSI,
    CIM,AIC,ETH,UIRQ,TCU2,TCU1,TCU0,UDC,
    UIRQ,UIRQ,UIRQ,UIRQ,IPU,LCD,UIRQ,DMA0,
    DMA1,DMA2,DMA3,DMA4,DMA5,UIRQ,UIRQ,UIRQ,
    UIRQ,UIRQ,UIRQ,UIRQ,UIRQ,UIRQ,UIRQ,
    GPIO0,GPIO1,GPIO2,GPIO3,GPIO4,GPIO5,GPIO6,GPIO7,
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
    GPIO120,GPIO121,GPIO122,GPIO123,GPIO124,GPIO125,GPIO126,GPIO127
};

static unsigned int dma_irq_mask = 0;
static unsigned int gpio_irq_mask[4] = {0};

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
        dma_irq_mask |= (1 << (irq - IRQ_DMA_0));
        __intc_unmask_irq(IRQ_DMAC);
    }
    else if (irq < 32)
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
    else if ((irq >= IRQ_DMA_0) && (irq <= IRQ_DMA_0 + NUM_DMA))
    {
        __dmac_channel_disable_irq(irq - IRQ_DMA_0);
        dma_irq_mask &= ~(1 << (irq - IRQ_DMA_0));
        if (!dma_irq_mask)
            __intc_mask_irq(IRQ_DMAC);
    }
    else if (irq < 32)
        __intc_mask_irq(irq);
}

static void ack_irq(unsigned int irq)
{
    if ((irq >= IRQ_GPIO_0) && (irq <= IRQ_GPIO_0 + NUM_GPIO))
    {
        __intc_ack_irq(IRQ_GPIO0 - ((irq - IRQ_GPIO_0)>>5));
        __gpio_ack_irq(irq - IRQ_GPIO_0);
    }
    else if ((irq >= IRQ_DMA_0) && (irq <= IRQ_DMA_0 + NUM_DMA))
        __intc_ack_irq(IRQ_DMAC);
    else if (irq < 32)
        __intc_ack_irq(irq);
}

static unsigned long ipl;
static int get_irq_number(void)
{
    register int irq = 0;
    
    ipl |= REG_INTC_IPR;
    
    if (ipl == 0)
        return -1;

    /* find out the real irq defined in irq_xxx.c */
    for (irq = 31; irq >= 0; irq--)
        if (ipl & (1 << irq))
            break;
    
    if (irq < 0)
        return -1;

    ipl &= ~(1 << irq);

    switch (irq)
    {
        case IRQ_GPIO0:
            irq = __gpio_group_irq(0) + IRQ_GPIO_0;
            break;
        case IRQ_GPIO1:
            irq = __gpio_group_irq(1) + IRQ_GPIO_0 + 32;
            break;
        case IRQ_GPIO2:
            irq = __gpio_group_irq(2) + IRQ_GPIO_0 + 64;
            break;
        case IRQ_GPIO3:
            irq = __gpio_group_irq(3) + IRQ_GPIO_0 + 96;
            break;
        case IRQ_DMAC:
            irq = __dmac_get_irq() + IRQ_DMA_0;
            break;
    }

    return irq;
}

void intr_handler(void)
{
    irq = get_irq_number();
    if(irq < 0)
        return;
    
    ack_irq(irq);
    if(irq > 0)
        irqvector[irq-1]();
}

#define EXC(x,y) if(_cause == (x)) return (y);
static char* parse_exception(unsigned int cause)
{
    unsigned int _cause = cause & M_CauseExcCode;
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
    return NULL;
}

void exception_handler(void* stack_ptr, unsigned int cause, unsigned int epc)
{
    panicf("Exception occurred: %s [0x%08x] at 0x%08x (stack at 0x%08x)", parse_exception(cause), cause, epc, (unsigned int)stack_ptr);
}

static const int FR2n[] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32};
static unsigned int iclk;
 
static void detect_clock(void)
{
    unsigned int cfcr, pllout;
    cfcr = REG_CPM_CPCCR;
    pllout = (__cpm_get_pllm() + 2)* JZ_EXTAL / (__cpm_get_plln() + 2);
    iclk = pllout / FR2n[__cpm_get_cdiv()];
}

void udelay(unsigned int usec)
{
    unsigned int i = usec * (iclk / 2000000);
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

/* Core-level interrupt masking */
void cli(void)
{
    register unsigned int t;
    t = read_c0_status();
    t &= ~1;
    write_c0_status(t);
}

unsigned int mips_get_sr(void)
{
    return read_c0_status();
}

void sti(void)
{
    register unsigned int t;
    t = read_c0_status();
    t |= 1;
    t &= ~2;
    write_c0_status(t);
}

#define Index_Invalidate_I      0x00
#define Index_Writeback_Inv_D   0x01
#define Index_Load_Tag_I        0x04
#define Index_Load_Tag_D        0x05
#define Index_Store_Tag_I       0x08
#define Index_Store_Tag_D       0x09
#define Hit_Invalidate_I        0x10
#define Hit_Invalidate_D        0x11
#define Hit_Writeback_Inv_D     0x15
#define Hit_Writeback_I         0x18
#define Hit_Writeback_D         0x19

#define CACHE_SIZE              16*1024
#define CACHE_LINE_SIZE         32
#define KSEG0                   0x80000000

#define SYNC_WB() __asm__ __volatile__ ("sync")

#define __CACHE_OP(op, addr)                   \
    __asm__ __volatile__(                    \
    "    .set    noreorder        \n"        \
    "    .set    mips32\n\t       \n"        \
    "    cache   %0, %1           \n"        \
    "    .set    mips0            \n"        \
    "    .set    reorder          \n"        \
    :                                        \
    : "i" (op), "m" (*(unsigned char *)(addr)))

void __flush_dcache_line(unsigned long addr)
{
    __CACHE_OP(Hit_Writeback_Inv_D, addr);
    SYNC_WB();
}

void __icache_invalidate_all(void)
{
    unsigned int i;

/*
    do 
    {
        unsigned long __k0_addr;
        
        __asm__ __volatile__(
                             "la    %0, 1f      \n"
                             "or    %0, %0, %1  \n"
                             "jr    %0          \n"
                             "nop               \n"
                             "1: nop            \n"
                             : "=&r"(__k0_addr)
                             : "r" (0x20000000)
                             );
    } while(0);
*/

    asm volatile (".set   noreorder  \n"
                  ".set   mips32     \n"
                  "mtc0   $0, $28    \n" /* TagLo */
                  "mtc0   $0, $29    \n" /* TagHi */
                  ".set   mips0      \n"
                  ".set   reorder    \n"
                  );
    for(i=KSEG0; i<KSEG0+CACHE_SIZE; i+=CACHE_LINE_SIZE)
        __CACHE_OP(Index_Store_Tag_I, i);

/*
    do
    {
        unsigned long __k0_addr;
        __asm__ __volatile__(
                             "nop;nop;nop;nop;nop;nop;nop  \n"
                             "la    %0, 1f                 \n"
                             "jr    %0                     \n"
                             "nop                          \n"
                             "1:    nop                    \n"
                             : "=&r" (__k0_addr)
                             );
    } while(0);
*/

    do
    {
        unsigned long tmp;
        __asm__ __volatile__(
        ".set mips32       \n"
        "mfc0 %0, $16, 7   \n" /* Config */
        "nop               \n"
        "ori  %0, 2        \n"
        "mtc0 %0, $16, 7   \n" /* Config */
        "nop               \n"
        ".set mips0        \n"
        : "=&r" (tmp));
    } while(0);
}

void __dcache_invalidate_all(void)
{
    unsigned int i;

    asm volatile (".set   noreorder  \n"
                  ".set   mips32     \n"
                  "mtc0   $0, $28    \n"
                  "mtc0   $0, $29    \n"
                  ".set   mips0      \n"
                  ".set   reorder    \n"
                  );
    for (i=KSEG0; i<KSEG0+CACHE_SIZE; i+=CACHE_LINE_SIZE)
        __CACHE_OP(Index_Store_Tag_D, i);
}

void __dcache_writeback_all(void)
{
    unsigned int i;
    for(i=KSEG0; i<KSEG0+CACHE_SIZE; i+=CACHE_LINE_SIZE)
        __CACHE_OP(Index_Writeback_Inv_D, i);
    
    SYNC_WB();
}

void dma_cache_wback_inv(unsigned long addr, unsigned long size)
{
    unsigned long end, a;

    if (size >= CACHE_SIZE)
        __dcache_writeback_all();
    else
    {
        unsigned long dc_lsize = CACHE_LINE_SIZE;
        
        a = addr & ~(dc_lsize - 1);
        end = (addr + size - 1) & ~(dc_lsize - 1);
        for(; a < end; a += dc_lsize)
            __flush_dcache_line(a);    /* Hit_Writeback_Inv_D */
    }
}

#define BARRIER                            \
    __asm__ __volatile__(                  \
    "    .set    noreorder          \n"    \
    "    nop                        \n"    \
    "    nop                        \n"    \
    "    nop                        \n"    \
    "    nop                        \n"    \
    "    nop                        \n"    \
    "    nop                        \n"    \
    "    .set    reorder            \n");

#define DEFAULT_PAGE_SHIFT       PL_4K
#define DEFAULT_PAGE_MASK        PM_4K
#define UNIQUE_ENTRYHI(idx, ps)  (A_K0BASE + ((idx) << (ps + 1)))
#define ASID_MASK                M_EntryHiASID
#define VPN2_SHIFT               S_EntryHiVPN2
#define PFN_SHIFT                S_EntryLoPFN
#define PFN_MASK                 0xffffff
static void local_flush_tlb_all(void)
{
    unsigned long old_ctx;
    int entry;
    unsigned int old_irq = disable_irq_save();
    
    /* Save old context and create impossible VPN2 value */
    old_ctx = read_c0_entryhi();
    write_c0_entrylo0(0);
    write_c0_entrylo1(0);
    BARRIER;

    /* Blast 'em all away. */
    for(entry = 0; entry < 32; entry++)
    {
        /* Make sure all entries differ. */
        write_c0_entryhi(UNIQUE_ENTRYHI(entry, DEFAULT_PAGE_SHIFT));
        write_c0_index(entry);
        BARRIER;
        tlb_write_indexed();
    }
    BARRIER;
    write_c0_entryhi(old_ctx);
    
    restore_irq(old_irq);
}

static void add_wired_entry(unsigned long entrylo0, unsigned long entrylo1,
                            unsigned long entryhi,  unsigned long pagemask)
{
    unsigned long wired;
    unsigned long old_pagemask;
    unsigned long old_ctx;
    unsigned int  old_irq = disable_irq_save();
    
    old_ctx = read_c0_entryhi() & ASID_MASK;
    old_pagemask = read_c0_pagemask();
    wired = read_c0_wired();
    write_c0_wired(wired + 1);
    write_c0_index(wired);
    BARRIER;
    write_c0_pagemask(pagemask);
    write_c0_entryhi(entryhi);
    write_c0_entrylo0(entrylo0);
    write_c0_entrylo1(entrylo1);
    BARRIER;
    tlb_write_indexed();
    BARRIER;

    write_c0_entryhi(old_ctx);
    BARRIER;
    write_c0_pagemask(old_pagemask);
    local_flush_tlb_all();
    restore_irq(old_irq);
}

static void map_address(unsigned long virtual, unsigned long physical, unsigned long length)
{
    unsigned long entry0  = (physical & PFN_MASK) << PFN_SHIFT;
    unsigned long entry1  = ((physical+length) & PFN_MASK) << PFN_SHIFT;
    unsigned long entryhi = virtual & ~VPN2_SHIFT;
    
    entry0 |= (M_EntryLoG | M_EntryLoV | (K_CacheAttrC << S_EntryLoC) );
    entry1 |= (M_EntryLoG | M_EntryLoV | (K_CacheAttrC << S_EntryLoC) );
    
    add_wired_entry(entry0, entry1, entryhi, DEFAULT_PAGE_MASK);
}


static void tlb_init(void)
{
    write_c0_pagemask(DEFAULT_PAGE_MASK);
    write_c0_wired(0);
    write_c0_framemask(0);
    
    local_flush_tlb_all();
/*
    map_address(0x80000000, 0x80000000, 0x4000);
    map_address(0x80004000, 0x80004000, MEM * 0x100000);
*/
}

void tlb_refill_handler(void)
{
    panicf("TLB refill handler! [0x%x] [0x%lx]", read_c0_badvaddr(), read_c0_epc());
}

static void tlb_call_refill(void)
{
    asm("la $8, tlb_refill_handler \n"
        "jr $8                     \n");
}

extern int main(void);
extern void except_common_entry(void);

void system_main(void)
{
    int i;
    
    /*
     * 0x0   - Simple TLB refill handler
     * 0x100 - Cache error handler
     * 0x180 - Exception/Interrupt handler
     * 0x200 - Special Exception Interrupt handler (when IV is set in CP0_CAUSE)
     */
    memcpy((void *)A_K0BASE, (void *)&tlb_call_refill, 0x20);
    memcpy((void *)(A_K0BASE + 0x100), (void *)&except_common_entry, 0x20);
    memcpy((void *)(A_K0BASE + 0x180), (void *)&except_common_entry, 0x20);
    memcpy((void *)(A_K0BASE + 0x200), (void *)&except_common_entry, 0x20);
    
    __dcache_writeback_all();
    __icache_invalidate_all();
    
    write_c0_status(1 << 28 | 1 << 10 | 1 << 3); /* Enable CP | Mask interrupt 2 | Supervisor mode */
    
    /* Disable all interrupts */
    for(i=0; i<IRQ_MAX; i++)
        dis_irq(i);
    
    //tlb_init();
    
    sti();
    
    detect_clock();
    
    main();
    
    while(1);
}

void system_reboot(void)
{
    REG_WDT_TCSR = WDT_TCSR_PRESCALE4 | WDT_TCSR_EXT_EN;
    REG_WDT_TCNT = 0;
    REG_WDT_TDR = JZ_EXTAL/1000;   /* reset after 4ms */
    REG_TCU_TSCR = TCU_TSSR_WDTSC; /* enable wdt clock */
    REG_WDT_TCER = WDT_TCER_TCEN;  /* wdt start */
    
    while (1);
}

void power_off(void)
{
    /* Put system into hibernate mode */
    __rtc_clear_alarm_flag();
    __rtc_clear_hib_stat_all();
    //__rtc_set_scratch_pattern(0x12345678);
    __rtc_enable_alarm_wakeup();
    __rtc_set_hrcr_val(0xfe0);
    __rtc_set_hwfcr_val((0xFFFF << 4));
    __rtc_power_down();
    
    while(1);
}
