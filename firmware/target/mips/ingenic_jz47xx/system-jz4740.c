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

void intr_handler(void)
{
    printf("Interrupt!");
    return;
}

void except_handler(void* stack_ptr, unsigned int cause, unsigned int epc)
{
    panicf("Exception occurred: [0x%x] at 0x%x (stack at 0x%x)", cause, epc, (unsigned int)stack_ptr);
}

static const int FR2n[] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32};
static unsigned int iclk;
 
static void detect_clock(void)
{
    unsigned int cfcr, pllout;
    cfcr = REG_CPM_CPCCR;
    pllout = (__cpm_get_pllm() + 2)* JZ_EXTAL / (__cpm_get_plln() + 2);
    iclk = pllout / FR2n[__cpm_get_cdiv()];
    /*printf("EXTAL_CLK = %dM PLL = %d iclk = %d\r\n",EXTAL_CLK / 1000 /1000,pllout,iclk);*/
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

#define cache_op(op,addr)                    \
    __asm__ __volatile__(                    \
    "    .set    noreorder        \n"        \
    "    .set    mips32\n\t       \n"        \
    "    cache    %0, %1          \n"        \
    "    .set    mips0            \n"        \
    "    .set    reorder          \n"        \
    :                                        \
    : "i" (op), "m" (*(unsigned char *)(addr)))

void __flush_dcache_line(unsigned long addr)
{
    cache_op(Hit_Writeback_Inv_D, addr);
    SYNC_WB();
}

void __icache_invalidate_all(void)
{
    unsigned int i;

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

    asm volatile (".set   noreorder  \n"
                  ".set   mips32     \n"
                  "mtc0   $0,$28     \n"
                  "mtc0   $0,$29     \n"
                  ".set   mips0      \n"
                  ".set   reorder    \n"
                  );
    for(i=KSEG0; i<KSEG0+CACHE_SIZE; i+=CACHE_LINE_SIZE)
        cache_op(Index_Store_Tag_I, i);

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

    do
    {
        unsigned long tmp;
        __asm__ __volatile__(
        ".set mips32       \n"
        "mfc0 %0, $16, 7   \n"
        "nop               \n"
        "ori  %0, 2        \n"
        "mtc0 %0, $16, 7   \n"
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
                  "mtc0   $0,$28     \n"
                  "mtc0   $0,$29     \n"
                  ".set   mips0      \n"
                  ".set   reorder    \n"
                  );
    for (i=KSEG0; i<KSEG0+CACHE_SIZE; i+=CACHE_LINE_SIZE)
        cache_op(Index_Store_Tag_D, i);
}

void __dcache_writeback_all(void)
{
    unsigned int i;
    for(i=KSEG0; i<KSEG0+CACHE_SIZE; i+=CACHE_LINE_SIZE)
        cache_op(Index_Writeback_Inv_D, i);
    
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
		while (1)
        {
			__flush_dcache_line(a);	/* Hit_Writeback_Inv_D */
			if (a == end)
				break;
			a += dc_lsize;
		}
	}
}

extern void (*tick_funcs[MAX_NUM_TICK_TASKS])(void);

#define USE_RTC_CLOCK 0
void tick_start(unsigned int interval_in_ms)
{
    unsigned int tps = interval_in_ms;
	unsigned int latch;
	__cpm_start_tcu();
	
	__tcu_disable_pwm_output(0);
	__tcu_mask_half_match_irq(0); 
	__tcu_unmask_full_match_irq(0);

#if USE_RTC_CLOCK
	__tcu_select_rtcclk(0);
	__tcu_select_clk_div1(0);
    latch = (__cpm_get_rtcclk() + (tps>>1)) / tps;
#else	
	__tcu_select_extalclk(0);
	__tcu_select_clk_div4(0);
	
	latch = (JZ_EXTAL / 4 + (tps>>1)) / tps;
#endif
	REG_TCU_TDFR(0) = latch;
	REG_TCU_TDHR(0) = latch;

	__tcu_clear_full_match_flag(0);
	__tcu_start_counter(0);
	
    //printf("TCSR = 0x%04x\r\n",*(volatile u16 *)0xb000204C);
}

extern int main(void);
extern unsigned int _loadaddress;
extern unsigned int _resetvectorsstart;
extern unsigned int _resetvectorsend;
extern unsigned int _vectorsstart;
extern unsigned int _vectorsend; /* see boot.lds/app.lds */

void system_main(void)
{
    cli();
    write_c0_status(1 << 28 | 1 << 10); /* Enable CP | Mask interrupt 2 */
    
    memcpy((void *)A_K0BASE, (void *)&_vectorsstart, 0x20);
    memcpy((void *)(A_K0BASE + 0x180), (void *)&_vectorsstart, 0x20);
    memcpy((void *)(A_K0BASE + 0x200), (void *)&_vectorsstart, 0x20);
    
    __dcache_writeback_all();
    __icache_invalidate_all();
    
    (*((unsigned int*)(0x80000200))) = 0x42;
    (*((unsigned int*)(0x80000204))) = 0x45;
    (*((unsigned int*)(0x80000208))) = 0x10020;

    set_c0_status(1 << 22); /* Enable Boot Exception Vectors */
    
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
