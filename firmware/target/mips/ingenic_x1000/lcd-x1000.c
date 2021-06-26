/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "lcd.h"
#include "system.h"
#include "kernel.h"
#include "lcd-x1000.h"
#include "dma-x1000.h"
#include "irq-x1000.h"
#include "x1000/lcd.h"
#include "x1000/cpm.h"
#include <stdint.h>
#include <string.h>

#define LCD_DMA_CMD_SOFINT  (1 << 31)
#define LCD_DMA_CMD_EOFINT  (1 << 30)
#define LCD_DMA_CMD_COMMAND (1 << 29)
#define LCD_DMA_CMD_FRM_EN  (1 << 26)

#define LCD_DMA_CNT_BPP_15BIT          ((4 << 27)|(1<<30))
#define LCD_DMA_CNT_BPP_16BIT          (4 << 27)
#define LCD_DMA_CNT_BPP_18BIT_OR_24BIT (5 << 27)

struct lcd_dma_desc {
    uint32_t da;    /* Next descriptor address */
    uint32_t sa;    /* Source buffer address */
    uint32_t fid;   /* Frame ID */
    uint32_t cmd;   /* Command bits */
    uint32_t osz;   /* OFFSIZE register */
    uint32_t pw;    /* page width */
    uint32_t cnt;   /* CNUM / CPOS, depending on LCD_DMA_CMD_COMMAND bit */
    uint32_t fsz;   /* Frame size (set to 0 for commands) */
} __attribute__((aligned(32)));

/* We need two descriptors, one for framebuffer write command and one for
 * frame data. Even if no command is needed we need a dummy command descriptor
 * with cnt=0, or the hardware will refuse to transfer the frame data.
 *
 * First descriptor always has to be a command (lcd_dma_desc[0] here) or
 * the hardware will give up.
 */
static struct lcd_dma_desc lcd_dma_desc[2];

/* Shadow copy of main framebuffer, needed to avoid tearing */
static fb_data shadowfb[LCD_HEIGHT*LCD_WIDTH] __attribute__((aligned(64)));

/* Signals DMA copy to shadow FB is done */
static volatile int fbcopy_done;

#if defined(HAVE_LCD_SLEEP) || defined(LCD_X1000_FASTSLEEP)
/* True if we're in sleep mode */
static bool lcd_sleeping = false;
#endif

/* Check if running with interrupts disabled (eg: panic screen) */
#define lcd_panic_mode \
    UNLIKELY((read_c0_status() & 1) == 0)

static void lcd_init_controller(const struct lcd_tgt_config* cfg)
{
    /* Set MCFG/MCFG_NEW according to target interface settings */
    unsigned mcfg = 0, mcfg_new = 0;

    switch(cfg->cmd_width) {
    case 8:  mcfg |= BF_LCD_MCFG_CWIDTH_V(8BIT); break;
    case 9:  mcfg |= BF_LCD_MCFG_CWIDTH_V(16BIT_OR_9BIT); break;
    case 16: mcfg |= BF_LCD_MCFG_CWIDTH_V(16BIT_OR_9BIT); break;
    case 18: mcfg |= BF_LCD_MCFG_CWIDTH_V(18BIT); break;
    case 24: mcfg |= BF_LCD_MCFG_CWIDTH_V(24BIT); break;
    default: break;
    }

    if(cfg->cmd_width == 9)
        mcfg_new |= BM_LCD_MCFG_NEW_CMD_9BIT;

    switch(cfg->bus_width) {
    case 8:  mcfg_new |= BF_LCD_MCFG_NEW_DWIDTH_V(8BIT); break;
    case 9:  mcfg_new |= BF_LCD_MCFG_NEW_DWIDTH_V(9BIT); break;
    case 16: mcfg_new |= BF_LCD_MCFG_NEW_DWIDTH_V(16BIT); break;
    case 18: mcfg_new |= BF_LCD_MCFG_NEW_DWIDTH_V(18BIT); break;
    case 24: mcfg_new |= BF_LCD_MCFG_NEW_DWIDTH_V(24BIT); break;
    default: break;
    }

    if(cfg->use_serial)
        mcfg_new |= jz_orf(LCD_MCFG_NEW, DTYPE_V(SERIAL), CTYPE_V(SERIAL));
    else
        mcfg_new |= jz_orf(LCD_MCFG_NEW, DTYPE_V(PARALLEL), CTYPE_V(PARALLEL));

    jz_vwritef(mcfg_new, LCD_MCFG_NEW,
               6800_MODE(cfg->use_6800_mode),
               CSPLY(cfg->wr_polarity ? 0 : 1),
               RSPLY(cfg->dc_polarity),
               CLKPLY(cfg->clk_polarity));

    /* Program the configuration. Note we cannot enable TE signal at
     * this stage, because the panel will need to be configured first.
     */
    jz_write(LCD_MCFG, mcfg);
    jz_write(LCD_MCFG_NEW, mcfg_new);
    jz_writef(LCD_MCTRL, NARROW_TE(0), TE_INV(0), NOT_USE_TE(1),
              DCSI_SEL(0), MIPI_SLCD(0), FAST_MODE(1), GATE_MASK(0),
              DMA_MODE(1), DMA_START(0), DMA_TX_EN(0));
    jz_writef(LCD_WTIME, DHTIME(0), DLTIME(0), CHTIME(0), CLTIME(0));
    jz_writef(LCD_TASH, TAH(0), TAS(0));
    jz_write(LCD_SMWT, 0);

    /* DMA settings */
    jz_writef(LCD_CTRL, ENABLE(0), BURST_V(64WORD),
              EOFM(1), SOFM(0), IFUM(0), QDM(0),
              BEDN(cfg->big_endian), PEDN(0));
    jz_write(LCD_DAH, LCD_WIDTH);
    jz_write(LCD_DAV, LCD_HEIGHT);
}

static void lcd_fbcopy_dma_cb(int evt);

static void lcd_init_descriptors(const struct lcd_tgt_config* cfg)
{
    struct lcd_dma_desc* desc = &lcd_dma_desc[0];
    int cmdsize = cfg->dma_wr_cmd_size / 4;

    /* Set up the command descriptor */
    desc[0].da  = PHYSADDR(&desc[1]);
    desc[0].sa  = PHYSADDR(cfg->dma_wr_cmd_buf);
    desc[0].fid = 0xc0;
    desc[0].cmd = LCD_DMA_CMD_COMMAND | cmdsize;
    desc[0].osz = 0;
    desc[0].pw  = 0;
    desc[0].fsz = 0;
    switch(cfg->cmd_width) {
    case 8:  desc[0].cnt = 4*cmdsize; break;
    case 9:
    case 16: desc[0].cnt = 2*cmdsize; break;
    case 18:
    case 24: desc[0].cnt = cmdsize; break;
    default: break;
    }

    /* Set up the frame descriptor */
    desc[1].da  = PHYSADDR(&desc[0]);
    desc[1].sa  = PHYSADDR(shadowfb);
    desc[1].fid = 0xf0;
    desc[1].cmd = LCD_DMA_CMD_EOFINT | LCD_DMA_CMD_FRM_EN |
                  (LCD_WIDTH * LCD_HEIGHT * sizeof(fb_data) / 4);
    desc[1].osz = 0;
    desc[1].pw  = 0;
    desc[1].fsz = (LCD_WIDTH - 1) | ((LCD_HEIGHT - 1) << 12);
#if LCD_DEPTH == 16
    desc[1].cnt = LCD_DMA_CNT_BPP_16BIT;
#elif LCD_DEPTH == 24
    desc[1].cnt = LCD_DMA_CNT_BPP_18BIT_OR_24BIT;
#else
# error "unsupported LCD bit depth"
#endif

    /* Commit LCD DMA descriptors */
    commit_dcache_range(&desc[0], 2*sizeof(struct lcd_dma_desc));

    /* Set fbcopy channel callback */
    dma_set_callback(DMA_CHANNEL_FBCOPY, lcd_fbcopy_dma_cb);
}

static void lcd_fbcopy_dma_cb(int evt)
{
    (void)evt;
    fbcopy_done = 1;
}

static void lcd_fbcopy_dma_run(dma_desc* d)
{
    if(lcd_panic_mode) {
        /* Can't use DMA if interrupts are off, so just do a memcpy().
         * Doesn't need to be efficient, since AFAIK the panic screen is
         * the only place that can update the LCD with interrupts disabled. */
        memcpy(shadowfb, FBADDR(0, 0), LCD_WIDTH*LCD_HEIGHT*sizeof(fb_data));
        commit_dcache();
        return;
    }

    commit_dcache_range(d, sizeof(struct dma_desc));

    /* Start the transfer */
    fbcopy_done = 0;
    REG_DMA_CHN_DA(DMA_CHANNEL_FBCOPY) = PHYSADDR(d);
    jz_writef(DMA_CHN_CS(DMA_CHANNEL_FBCOPY), DES8(1), NDES(0));
    jz_set(DMA_DB, 1 << DMA_CHANNEL_FBCOPY);
    jz_writef(DMA_CHN_CS(DMA_CHANNEL_FBCOPY), CTE(1));

    while(!fbcopy_done);
}

static void lcd_fbcopy_dma_full(void)
{
    dma_desc d;
    d.cm = jz_orf(DMA_CHN_CM, SAI(1), DAI(1), RDIL(9),
                  SP_V(32BIT), DP_V(32BIT), TSZ_V(AUTO),
                  STDE(0), TIE(1), LINK(0));
    d.sa = PHYSADDR(FBADDR(0, 0));
    d.ta = PHYSADDR(shadowfb);
    d.tc = LCD_WIDTH * LCD_HEIGHT * sizeof(fb_data);
    d.sd = 0;
    d.rt = jz_orf(DMA_CHN_RT, TYPE_V(AUTO));
    d.pad0 = 0;
    d.pad1 = 0;
    lcd_fbcopy_dma_run(&d);
}

/* NOTE: DMA stride mode can only transfer up to 255 blocks at once.
 *
 * - for LCD_STRIDEFORMAT == VERTICAL_STRIDE, keep width <= 255
 * - for LCD_STRIDEFORMAT == HORIZONTAL_STRIDE, keep height <= 255
 */
static void lcd_fbcopy_dma_partial1(int x, int y, int width, int height)
{
    int stride = STRIDE_MAIN(LCD_WIDTH - width, LCD_HEIGHT - height);

    dma_desc d;
    d.cm = jz_orf(DMA_CHN_CM, SAI(1), DAI(1), RDIL(9),
                  SP_V(32BIT), DP_V(32BIT), TSZ_V(AUTO),
                  STDE(stride ? 1 : 0), TIE(1), LINK(0));
    d.sa = PHYSADDR(FBADDR(x, y));
    d.ta = PHYSADDR(&shadowfb[STRIDE_MAIN(y * LCD_WIDTH + x,
                                          x * LCD_HEIGHT + y)]);
    d.rt = jz_orf(DMA_CHN_RT, TYPE_V(AUTO));
    d.pad0 = 0;
    d.pad1 = 0;

    if(stride) {
        stride *= sizeof(fb_data);
        d.sd = (stride << 16) | stride;
        d.tc = (STRIDE_MAIN(height, width) << 16) |
               (STRIDE_MAIN(width, height) * sizeof(fb_data));
    } else {
        d.sd = 0;
        d.tc = width * height * sizeof(fb_data);
    }

    lcd_fbcopy_dma_run(&d);
}

#if STRIDE_MAIN(LCD_HEIGHT, LCD_WIDTH) > 255
static void lcd_fbcopy_dma_partial(int x, int y, int width, int height)
{
    do {
        int count = MIN(STRIDE_MAIN(height, width), 255);

        lcd_fbcopy_dma_partial1(x, y, STRIDE_MAIN(width, count),
                                STRIDE_MAIN(count, height));

        STRIDE_MAIN(height, width) -= count;
        STRIDE_MAIN(y, x) += count;
    } while(STRIDE_MAIN(height, width) != 0);
}
#else
# define lcd_fbcopy_dma_partial lcd_fbcopy_dma_partial1
#endif

static void lcd_dma_start(void)
{
    /* Set format conversion bit, seems necessary for DMA mode.
     * Must set DTIMES here if we use an 8-bit bus type. */
    int dtimes = lcd_tgt_config.bus_width == 8 ? (LCD_DEPTH/8 - 1) : 0;
    jz_writef(LCD_MCFG_NEW, FMT_CONV(1), DTIMES(dtimes));

    /* Program vsync configuration */
    jz_writef(LCD_MCTRL, NARROW_TE(lcd_tgt_config.te_narrow),
              TE_INV(lcd_tgt_config.te_polarity ? 0 : 1),
              NOT_USE_TE(lcd_tgt_config.te_enable ? 0 : 1));

    /* Begin DMA transfer. Need to start a dummy frame or else we will
     * not be able to pass lcd_wait_frame() at the first lcd_update(). */
    jz_write(LCD_STATE, 0);
    jz_write(LCD_DA, PHYSADDR(&lcd_dma_desc[0]));
    jz_writef(LCD_MCTRL, DMA_MODE(1), DMA_START(1), DMA_TX_EN(1));
    jz_writef(LCD_CTRL, ENABLE(1));
}

static bool lcd_wait_frame(void)
{
    /* Bail out if DMA is not enabled */
    int irq = disable_irq_save();
    int bit = jz_readf(LCD_CTRL, ENABLE);
    restore_irq(irq);
    if(!bit)
        return false;

    /* Usual case -- wait for EOF, wait for FIFO to drain, clear EOF */
    while(jz_readf(LCD_STATE, EOF) == 0);
    while(jz_readf(LCD_MSTATE, BUSY));
    jz_writef(LCD_STATE, EOF(0));
    return true;
}

static void lcd_dma_stop(void)
{
#ifdef LCD_X1000_DMA_WAIT_FOR_FRAME
    /* Wait for frame to finish to avoid misaligning the write pointer */
    lcd_wait_frame();
#endif

    /* Stop the DMA transfer */
    jz_writef(LCD_CTRL, ENABLE(0));
    jz_writef(LCD_MCTRL, DMA_TX_EN(0));

    /* Wait for disable to take effect */
    while(jz_readf(LCD_STATE, QD) == 0);
    jz_writef(LCD_STATE, QD(0));

    /* Clear format conversion bit, disable vsync */
    jz_writef(LCD_MCFG_NEW, FMT_CONV(0), DTIMES(0));
    jz_writef(LCD_MCTRL, NARROW_TE(0), TE_INV(0), NOT_USE_TE(1));
}

static void lcd_send(uint32_t d)
{
    while(jz_readf(LCD_MSTATE, BUSY));
    REG_LCD_MDATA = d;
}

void lcd_set_clock(x1000_clk_t clk, uint32_t freq)
{
    uint32_t in_freq = clk_get(clk);
    uint32_t div = clk_calc_div(in_freq, freq);

    jz_writef(CPM_LPCDR, CE(1), CLKDIV(div - 1),
              CLKSRC(clk == X1000_CLK_MPLL ? 1 : 0));
    while(jz_readf(CPM_LPCDR, BUSY));
    jz_writef(CPM_LPCDR, CE(0));
}

void lcd_exec_commands(const uint32_t* cmdseq)
{
    while(*cmdseq != LCD_INSTR_END) {
        uint32_t instr = *cmdseq++;
        uint32_t d = 0;
        switch(instr) {
        case LCD_INSTR_CMD:
            d = jz_orf(LCD_MDATA, TYPE_V(CMD));
            /* fallthrough */

        case LCD_INSTR_DAT:
            d |= *cmdseq++;
            lcd_send(d);
            break;

        case LCD_INSTR_UDELAY:
            udelay(*cmdseq++);
            break;

        default:
            break;
        }
    }
}

void lcd_init_device(void)
{
    jz_writef(CPM_CLKGR, LCD(0));

    lcd_init_controller(&lcd_tgt_config);
    lcd_init_descriptors(&lcd_tgt_config);

    lcd_tgt_enable(true);

    lcd_dma_start();
}

#ifdef HAVE_LCD_SHUTDOWN
void lcd_shutdown(void)
{
    if(lcd_sleeping)
        lcd_tgt_sleep(false);
    else if(jz_readf(LCD_CTRL, ENABLE))
        lcd_dma_stop();

    lcd_tgt_enable(false);
    jz_writef(CPM_CLKGR, LCD(1));
}
#endif

#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
bool lcd_active(void)
{
    return jz_readf(LCD_CTRL, ENABLE);
}

void lcd_enable(bool en)
{
    /* Must disable IRQs to turn off the running LCD */
    int irq = disable_irq_save();
    int bit = jz_readf(LCD_CTRL, ENABLE);
    if(bit && !en)
        lcd_dma_stop();
    restore_irq(irq);

    /* Deal with sleep mode */
#if defined(HAVE_LCD_SLEEP) || defined(LCD_X1000_FASTSLEEP)
#if defined(LCD_X1000_FASTSLEEP)
    if(bit && !en) {
        lcd_tgt_sleep(true);
        lcd_sleeping = true;
    } else
#endif
    if(!bit && en && lcd_sleeping) {
        lcd_tgt_sleep(false);
        lcd_sleeping = false;
    }
#endif

    /* Handle turning the LCD back on */
    if(!bit && en)
        lcd_dma_start();
}
#endif

#if defined(HAVE_LCD_SLEEP)
#if defined(LCD_X1000_FASTSLEEP)
# error "Do not define HAVE_LCD_SLEEP if target has LCD_X1000_FASTSLEEP"
#endif

void lcd_sleep(void)
{
    if(!lcd_sleeping) {
        lcd_enable(false);
        lcd_tgt_sleep(true);
        lcd_sleeping = true;
    }
}
#endif

void lcd_update(void)
{
    if(!lcd_wait_frame())
        return;

    commit_dcache();
    lcd_fbcopy_dma_full();
    jz_writef(LCD_MCTRL, DMA_START(1), DMA_MODE(1));
}

/* We can do partial updates even though the DMA doesn't seem to handle it well,
 * due to the fact that this is actually putting it into a buffer, and then
 * it gets transferred via DMA to a secondary buffer, which gets transferred in
 * its entirety to the LCD through a different DMA process.                      */
void lcd_update_rect(int x, int y, int width, int height)
{
    /* Clamp the coordinates */
    if(x < 0) {
        width += x;
        x = 0;
    }

    if(y < 0) {
        height += y;
        y = 0;
    }

    if(width > LCD_WIDTH - x)
        width = LCD_WIDTH - x;

    if(height > LCD_HEIGHT - y)
        height = LCD_HEIGHT - y;

    if(width < 0 || height < 0)
        return;

    if(!lcd_wait_frame())
        return;

    commit_dcache();
    lcd_fbcopy_dma_partial(x, y, width, height);
    jz_writef(LCD_MCTRL, DMA_START(1), DMA_MODE(1));
}
