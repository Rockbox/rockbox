/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
 *
 * Ingenic X1600 display controller (DPU / "DC") driver.
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

/* The DPU scans a separate framebuffer using a self-linked descriptor.
 * lcd_update() copies Rockbox pixels into that buffer. Register values
 * follow the vendor configuration. Powering down the DPU SRAM resets its
 * registers. */

#include "lcd.h"
#include "system.h"
#include "kernel.h"
#include "panic.h"
#include "dpu-x1600.h"
#include "dma-ingenic.h"
#include "clk-x1600.h"
#include "x1600/dpu.h"
#include "x1600/cpm.h"
#include "x1600/dma.h"
#include "x1600/dma_chn.h"
#include <stdint.h>
#include <string.h>

/* The copy path needs whole scanlines contiguous, which is also the only layout
 * the DPU's Stride descriptor field can describe. */
#if defined(LCD_STRIDEFORMAT) && LCD_STRIDEFORMAT == VERTICAL_STRIDE
# error "dpu-x1600.c: the DPU scans out row-major; vertical stride is unsupported"
#endif

#if LCD_DEPTH != 16
/* FORMAT can also do RGB555 and RGB888; nothing else here handles them. */
# error "dpu-x1600.c: only LCD_DEPTH == 16 (RGB565) is implemented"
#endif

/* Total TFT timing, including sync pulses and porches. */
#define DPU_HTOTAL  (lcd_tgt_config.hsync_len + lcd_tgt_config.left_margin + \
                     LCD_WIDTH + lcd_tgt_config.right_margin)
#define DPU_VTOTAL  (lcd_tgt_config.vsync_len + lcd_tgt_config.upper_margin + \
                     LCD_HEIGHT + lcd_tgt_config.lower_margin)

/* A frame is ~16 ms at this panel's ~61 Hz, so 100 ms means "not responding". */
#define DPU_TIMEOUT_US  100000

/* State */

/* Separate framebuffer used for DPU scanout. */
static fb_data lcd_scanoutfb[LCD_HEIGHT][LCD_WIDTH]
    __attribute__((aligned(64)));

#define LCD_SCANOUTFB_BYTES  (sizeof(lcd_scanoutfb))

/* Align the SRD descriptor to a cache line. Update it through the uncached
 * alias because the DPU refetches it every frame. */
static uint32_t lcd_srd_desc[8] __attribute__((aligned(32)));
static volatile uint32_t* lcd_srd;

/* DPU registers are accessible only after clock and SRAM power verification. */
static bool dpu_powered = false;

static bool lcd_on = false;

/* lcd_enable(false) left the panel controller asleep, so lcd_enable(true) wakes
 * it rather than running a full init. */
static bool lcd_sleeping = false;

/* Last framebuffer address for diagnostics. Resolve FBADDR() again for
 * each copy. */
static fb_data* lcd_fbbase;

static volatile int fbcopy_done;

/* Framebuffer DMA statistics. */
static volatile uint32_t lcd_fbcopy_fails;
static volatile uint32_t lcd_fbcopy_runs;

/* Count scanout-stop requests that were not acknowledged. */
static volatile uint32_t dpu_stop_noack;

/* Zero-frequency pixel-clock requests and their last source rate. */
static volatile uint32_t lcd_clk_unset;
static volatile uint32_t lcd_clk_in_freq;

/* Panic mode: interrupts are off, so the DMA completion interrupt never arrives
 * and the copy must be done in software. */
#define lcd_panic_mode  UNLIKELY((read_c0_status() & 1) == 0)

/* Bounded busy-wait.  Iteration guard as well as an OST deadline: if the OST is
 * not counting the deadline never expires, and the loop would be unbounded. */
#define WAIT_UNTIL(cond, timeout_us)                                        \
    ({                                                                      \
        uint32_t __t0 = __ost_read32();                                     \
        uint32_t __guard = 2000000u;                                        \
        bool __ok;                                                          \
        while(!(__ok = !!(cond)) && --__guard &&                            \
              __ost_read32() - __t0 < (timeout_us) * OST_TICKS_PER_US)      \
            ;                                                               \
        __ok;                                                               \
    })

/* Clock and power */

/* The R1 uses MPLL/50 for a 28 MHz pixel clock. */
void lcd_set_clock(x1600_clk_t clksrc, uint32_t freq)
{
    /* LPCDR's mux is its own three-way encoding (PM 11.1.2.7), not CPCCR's. */
    uint32_t lpcs;
    switch(clksrc) {
    case X1600_CLK_SCLK_A: lpcs = BV_CPM_LPCDR_LPCS__SCLK_A; break;
    case X1600_CLK_MPLL:   lpcs = BV_CPM_LPCDR_LPCS__MPLL;   break;
    case X1600_CLK_EPLL:   lpcs = BV_CPM_LPCDR_LPCS__EPLL;   break;
    default: return;    /* not reachable through this mux */
    }

    uint32_t in_freq = clk_get(clksrc);
    if(in_freq == 0 || freq == 0) {
        lcd_clk_unset = 1;
        lcd_clk_in_freq = in_freq;
        return;
    }

    /* Ratio is LPCDR + 1, and LPCDR is 8 bits, so 1..256. */
    uint32_t div = clk_calc_div(in_freq, freq);
    if(div < 1 || div > 256)
        return;

    /* Stop the pixel clock before changing LPCS; the mux is not glitch-free (PM 11.1.2.7). */
    jz_writef(CPM_LPCDR, CE_LCD(1), LCD_STOP(1));
    WAIT_UNTIL(!jz_readf(CPM_LPCDR, LCD_BUSY), DPU_TIMEOUT_US);

    jz_writef(CPM_LPCDR, CE_LCD(1), LCD_STOP(0), LPCS(lpcs),
              LCD_IO_INV(0), LPCDR(div - 1));
    WAIT_UNTIL(!jz_readf(CPM_LPCDR, LCD_BUSY), DPU_TIMEOUT_US);

    jz_writef(CPM_LPCDR, CE_LCD(0));
}

/* PM 11.2.2.10: the module clock must not be gated while its SRAM is powered
 * down, so the orderings mirror -- clock on then RAM on, RAM off then clock. */
static bool dpu_power(bool on)
{
    if(on == dpu_powered)
        return true;

    if(on) {
        jz_writef(CPM_CLKGR, LCD(0));
        jz_writef(CPM_MEMPD1, DPU(0));

        /* Verify both clock and SRAM power before accessing DPU registers. */
        if(jz_readf(CPM_CLKGR, LCD) != 0 || jz_readf(CPM_MEMPD1, DPU) != 0) {
            dpu_powered = false;
            return false;
        }
        /* TODO: verify the SRAM power-up settling time; the PM gives no
         * value. */
        udelay(100);
        dpu_powered = true;
    } else {
        dpu_powered = false;
        jz_writef(CPM_MEMPD1, DPU(1));
        jz_writef(CPM_CLKGR, LCD(1));
    }

    return true;
}

/* Controller programming */

/* Poll DC_ST until every bit in 'mask' is set.  Returns false on timeout, or
 * immediately if the block is down (reading it then would hang the bus). */
static bool dpu_wait_status(uint32_t mask)
{
    if(!dpu_powered)
        return false;

    return WAIT_UNTIL((REG_DPU_ST & mask) == mask, DPU_TIMEOUT_US);
}

/* Wait for the end of the frame being displayed.  DC_ST's bits are set whether
 * or not the matching interrupt is unmasked, so this works with interrupts off.
 * Returns false on timeout, which normally means scanout is not running. */
static bool dpu_wait_frame(void)
{
    if(!dpu_powered)
        return false;

    jz_writef(DPU_CLR_ST, CLR_DISP_END(1));
    return dpu_wait_status(BM_DPU_ST_DISP_END);
}

/* Set TFT timing start/end pairs in pixel clocks and lines (PM 10.5.4.2).
 * All four fields must be nonzero. */
static void dpu_set_timings(void)
{
    jz_overwritef(DPU_TFT_TIMING_HSYNC,
                  HPS(lcd_tgt_config.hsync_len),
                  HPE(DPU_HTOTAL));
    jz_overwritef(DPU_TFT_TIMING_VSYNC,
                  VPS(lcd_tgt_config.vsync_len),
                  VPE(DPU_VTOTAL));
    jz_overwritef(DPU_TFT_TIMING_HDE,
                  HDS(lcd_tgt_config.hsync_len + lcd_tgt_config.left_margin),
                  HDE(lcd_tgt_config.hsync_len + lcd_tgt_config.left_margin +
                      LCD_WIDTH));
    jz_overwritef(DPU_TFT_TIMING_VDE,
                  VDS(lcd_tgt_config.vsync_len + lcd_tgt_config.upper_margin),
                  VDE(lcd_tgt_config.vsync_len + lcd_tgt_config.upper_margin +
                      LCD_HEIGHT));

    /* R1 lane order and sync polarities from the vendor register
     * configuration. */
    jz_overwritef(DPU_TFT_TRAN_CFG,
                  COLOR_EVEN_V(BGR), COLOR_ODD_V(BGR),
                  PIX_CLK_INV(0), DE_DL(0), HSYNC_DL(0), VSYNC_DL(0),
                  MODE_V(PARALLEL_888));

    /* Retain reset values for DC_COM_CONFIG and QoS registers, including
     * undocumented bits. */
}

/* Build the self-looping SRD descriptor.  Reproduces the vendor's descriptor
 * word for word apart from the framebuffer address. */
static void dpu_init_descriptor(void)
{
    /* Write the descriptor through its uncached alias. */
    lcd_srd = (volatile uint32_t*)UNCACHEDADDR(lcd_srd_desc);

    /* Self-link for continuous scanout. The frame must contain at least
     * 500 pixels (PM 10.7). */
    lcd_srd[DPU_SRD_NEXT]   = PHYSADDR(lcd_srd_desc);
    lcd_srd[DPU_SRD_FBADDR] = PHYSADDR(lcd_scanoutfb);
    lcd_srd[DPU_SRD_STRIDE] = BF_DPU_SRD_STRIDE_STRIDE(LCD_WIDTH);
    lcd_srd[DPU_SRD_FORMAT] = BF_DPU_SRD_FORMAT_FORMAT(BV_DPU_SRD_FORMAT_FORMAT__RGB565) |
                              BF_DPU_SRD_FORMAT_COLOR(BV_DPU_SRD_FORMAT_COLOR__RGB) |
                              BF_DPU_SRD_FORMAT_CHAIN_END(0);
    /* Keep the vendor EOD mask setting. DC_INTC remains masked, so no
     * interrupt is delivered. */
    lcd_srd[DPU_SRD_INTMASK] = BM_DPU_SRD_INTMASK_EOD_MSK;
}

/* Stop scanout at the end of the current frame (PM 10.5.3.2). */
static void dpu_stop(void)
{
    if(!dpu_powered)
        return;

    if(REG_DPU_ST & jz_orm(DPU_ST, WORKING, SRD_WORKING)) {
        jz_writef(DPU_CLR_ST, CLR_STP_SRD_ACK(1));
        jz_writef(DPU_CTRL, GEN_STP_SRD(1));

        /* Non-zero means we reconfigured a DPU that was still scanning. */
        if(!dpu_wait_status(BM_DPU_ST_STOP_SRD_ACK))
            dpu_stop_noack++;
        jz_writef(DPU_CLR_ST, CLR_STP_SRD_ACK(1));
    }

    /* Out of TFT mode, so the panel stops seeing a dot clock.  RMW: DISP_COM's
     * three CLKGATE_EN bits (7:5) must survive. */
    jz_writef(DPU_DISP_COM, DP_IF_SEL_V(NONE));
}

/* Reinitialise after power-up: cycling the DPU SRAM power resets its
 * registers. */
static void dpu_start(void)
{
    /* Chain-loaded from the vendor OS, the DPU may still be scanning out of the
     * vendor kernel's framebuffer. */
    dpu_stop();

    /* Clear pending status before starting scanout. */
    jz_writef(DPU_CLR_ST, CLR_DISP_END(1), CLR_TFT_UNDR(1),
              CLR_STP_SRD_ACK(1), CLR_SRD_START(1), CLR_SRD_END(1));

    /* Scanout is continuous; no DPU interrupts are needed. */
    jz_write(DPU_INTC, 0);

    dpu_set_timings();
    dpu_init_descriptor();

    /* Set the descriptor address and interface before starting scanout.
     * Preserve reserved DISP_COM bits and disable dithering for RGB666. */
    REG_DPU_SRD_CHAIN_ADDR = PHYSADDR(lcd_srd_desc);
    jz_writef(DPU_DISP_COM, DP_DITHER_EN(0), DP_IF_SEL_V(TFT));
    jz_writef(DPU_SRD_CHAIN_CTRL, SRD_CHAIN_START(1));
}

/* Framebuffer copy */

static void lcd_fbcopy_dma_cb(int evt)
{
    if(evt == DMA_EVENT_COMPLETE)
        fbcopy_done = 1;
    else if(evt == DMA_EVENT_ERROR)
        fbcopy_done = -1;
}

static bool lcd_fbcopy_wait(void)
{
    if(!WAIT_UNTIL(fbcopy_done, DPU_TIMEOUT_US))
        return false;

    return fbcopy_done > 0;
}

/* Run a single framebuffer-copy DMA descriptor and wait for completion. */
static void lcd_fbcopy_dma_run(dma_desc* d);
static void lcd_fbcopy_setup(dma_desc* d, int stride_enable);

static void lcd_fbcopy_dma_run(dma_desc* d)
{
    /* Write the stack descriptor through its uncached alias before
     * starting DMA. */
    {
        volatile dma_desc* ud = (volatile dma_desc*)UNCACHEDADDR(d);
        ud->cm = d->cm;
        ud->sa = d->sa;
        ud->ta = d->ta;
        ud->tc = d->tc;
        ud->sd = d->sd;
        ud->rt = d->rt;
        ud->pad0 = d->pad0;
        ud->pad1 = d->pad1;
    }

    fbcopy_done = 0;
    REG_DMA_CHN_DA(DMA_CHANNEL_FBCOPY) = PHYSADDR(d);
    jz_writef(DMA_CHN_CS(DMA_CHANNEL_FBCOPY), DES8(1), NDES(0));
    jz_set(DMA_DB, 1 << DMA_CHANNEL_FBCOPY);
    jz_writef(DMA_CHN_CS(DMA_CHANNEL_FBCOPY), CTE(1));

    lcd_fbcopy_runs++;
    if(!lcd_fbcopy_wait()) {
        lcd_fbcopy_fails++;
        jz_writef(DMA_CHN_CS(DMA_CHANNEL_FBCOPY), CTE(0));
        /* Do not return stack storage to the caller while DMA may own it.
         * The panic display uses the CPU copy path. */
        panicf("LCD DMA failed");
    }
}

/* Common part of the two copy paths.  PM 18.5.6: DDR-to-DDR requires SP and DP
 * both 32-bit.  PM Table 18-6: RDIL 9 is the largest recommended unit.  PM
 * Table 18-5: request type 001000 is auto-request, external to external. */
static void lcd_fbcopy_setup(dma_desc* d, int stride_enable)
{
    d->cm = jz_orf(DMA_CHN_CM, SAI(1), DAI(1), RDIL(9),
                   SP_V(32BIT), DP_V(32BIT), TSZ_V(AUTO),
                   STDE(stride_enable), TIE(1), LINK(0));
    d->rt = jz_orf(DMA_CHN_RT, TYPE_V(AUTO));
    d->sd = 0;
    d->pad0 = 0;
    d->pad1 = 0;
}

/* Copy a run of pixels contiguous in both buffers, i.e. whole scanlines.
 * 'first' and 'count' are in pixels. */
static void lcd_fbcopy_linear(unsigned first, unsigned count)
{
    const fb_data* src = (const fb_data*)FBADDR(0, 0) + first;
    fb_data* dst = &lcd_scanoutfb[0][0] + first;
    unsigned bytes = count * sizeof(fb_data);

    if(lcd_panic_mode) {
        memcpy(dst, src, bytes);
        commit_dcache_range(dst, bytes);
        return;
    }

    /* Commit the SOURCE before handing it to the engine: drawing wrote it
     * through the cache, the engine reads DRAM. */
    commit_dcache_range(src, bytes);
    dma_desc d;
    lcd_fbcopy_setup(&d, 0);
    d.sa = PHYSADDR(src);
    d.ta = PHYSADDR(dst);
    /* With TSZ = AUTO the transfer count is in bytes (PM 18.5.3 note 1), and
     * the field is 24 bits, so up to 16 MiB -- a whole frame is 768000. */
    d.tc = bytes;
    lcd_fbcopy_dma_run(&d);
}

/* Copy a rectangle narrower than the full screen.  'height' must be <= 255,
 * see lcd_fbcopy_rect(). */
static void lcd_fbcopy_rows(int x, int y, int width, int height)
{
    const fb_data* src = (const fb_data*)FBADDR(0, 0) + (y * LCD_WIDTH + x);
    fb_data* dst = &lcd_scanoutfb[y][x];
    unsigned rowbytes = width * sizeof(fb_data);
    unsigned gap = (LCD_WIDTH - width) * sizeof(fb_data);

    if(lcd_panic_mode) {
        for(int i = 0; i < height; ++i) {
            memcpy(dst, src, rowbytes);
            /* Commit every copied row so scanout sees the updated pixels. */
            commit_dcache_range(dst, rowbytes);
            src += LCD_WIDTH;
            dst += LCD_WIDTH;
        }

        return;
    }

    /* Commit the full source span, including the final partial row, before
     * DMA. */
    commit_dcache_range(src, (height - 1) * LCD_WIDTH * sizeof(fb_data) + rowbytes);
    dma_desc d;
    lcd_fbcopy_setup(&d, 1);
    d.sa = PHYSADDR(src);
    d.ta = PHYSADDR(dst);

    /* Stride mode.  PM 18.5.8: the stride difference works out as exactly the
     * gap in bytes, and source and destination share geometry so SSD == TSD.
     * PM 18.5.3: DTC's low 16 bits are one sub-block's size and the high 8 are
     * the sub-block count -- hence the 255-row cap. */
    d.sd = (gap << 16) | gap;
    d.tc = ((unsigned)height << 16) | rowbytes;

    lcd_fbcopy_dma_run(&d);
}

static void lcd_fbcopy_rect(int x, int y, int width, int height)
{
    /* A full-width rectangle is contiguous in both buffers, so it goes as one
     * linear transfer with no sub-block limit. */
    if(width == LCD_WIDTH) {
        lcd_fbcopy_linear(y * LCD_WIDTH, (unsigned)width * height);
        return;
    }

    do {
        int count = MIN(height, 255);
        lcd_fbcopy_rows(x, y, width, count);
        height -= count;
        y += count;
    } while(height > 0);
}

static void lcd_fbcopy_full(void)
{
    lcd_fbcopy_linear(0, LCD_WIDTH * LCD_HEIGHT);
}

/* Rockbox LCD driver interface */

void lcd_init_device(void)
{
    lcd_fbbase = FBADDR(0, 0);

    /* Clear and commit the scanout buffer before the DPU can read it. */
    memset(lcd_scanoutfb, 0, LCD_SCANOUTFB_BYTES);
    commit_dcache_range(lcd_scanoutfb, LCD_SCANOUTFB_BYTES);

    dma_set_callback(DMA_CHANNEL_FBCOPY, lcd_fbcopy_dma_cb);

    if(!dpu_power(true))
        return;

    /* The ST7701S accepts its initialisation sequence before the pixel
     * clock starts. */
    lcd_tgt_enable(true);

    /* Make the first displayed frame the framebuffer Rockbox has already
     * cleared, rather than whatever memset left behind. */
    commit_dcache();
    lcd_fbcopy_full();

    dpu_start();
    lcd_on = true;
}

void lcd_update(void)
{
    if(!lcd_on)
        return;

    /* The DPU reads DRAM directly, so the CPU's writes have to be out of the
     * D-cache before the copy engine sees them. */
    commit_dcache();

    lcd_fbcopy_full();
}

void lcd_update_rect(int x, int y, int width, int height)
{
    if(!lcd_on)
        return;

    /* Clamp, exactly as the other Rockbox drivers do. */
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

    if(width <= 0 || height <= 0)
        return;

    commit_dcache();

    lcd_fbcopy_rect(x, y, width, height);
}

#ifdef HAVE_LCD_ENABLE
bool lcd_active(void)
{
    return lcd_on;
}

void lcd_enable(bool en)
{
    if(en == lcd_on)
        return;

    if(!en) {
        /* Let the frame in flight finish so the panel is not cut off
         * mid-scanline, then shut the controller down. */
        dpu_wait_frame();
        dpu_stop();

        lcd_tgt_sleep(true);
        lcd_sleeping = true;
        lcd_on = false;

        /* Both halves of what the vendor kernel does when the panel sleeps,
         * and what makes every DPU register read back 0x80 afterwards. */
        dpu_power(false);
        return;
    }

    if(!dpu_power(true))
        return;

    if(lcd_sleeping) {
        lcd_tgt_sleep(false);
        lcd_sleeping = false;
    } else {
        lcd_tgt_enable(true);
    }

    /* Anyone who wants to repaint on wake hooks this. */
    send_event(LCD_EVENT_ACTIVATION, NULL);

    /* The scanout buffer lives in DRAM and survived the power-down, so the
     * screen comes back showing what it showed before. */
    dpu_start();
    lcd_on = true;
}
#endif /* HAVE_LCD_ENABLE */

#ifdef HAVE_LCD_SLEEP
/* Panel sleep support; not enabled in the target configuration. */
void lcd_sleep(void)
{
    if(lcd_on)
        lcd_enable(false);

    if(lcd_sleeping) {
        lcd_tgt_enable(false);
        lcd_sleeping = false;
    }
}
#endif /* HAVE_LCD_SLEEP */

#ifdef HAVE_LCD_SHUTDOWN
void lcd_shutdown(void)
{
    if(lcd_on) {
        dpu_wait_frame();
        dpu_stop();
        lcd_on = false;
    }

    lcd_tgt_enable(false);
    lcd_sleeping = false;
    dpu_power(false);
}
#endif /* HAVE_LCD_SHUTDOWN */

/* Debug accessors */

uint32_t dpu_get_scanout_addr(void)
{
    return lcd_srd ? lcd_srd[DPU_SRD_FBADDR] : 0;
}

uint32_t dpu_get_status(void)
{
    /* Reading the block while it is gated or powered down hangs the bus. */
    return dpu_powered ? REG_DPU_ST : 0;
}

void* lcd_fbbase_debug(void) { return lcd_fbbase; }

uint32_t lcd_dma_debug(int which)
{
    switch(which) {
    case 0: return lcd_fbcopy_runs;
    case 1: return lcd_fbcopy_fails;
    case 2: return REG_DMA_CHN_CS(DMA_CHANNEL_FBCOPY);
    /* PDMA clock gated? A gated controller accepts the writes and does
     * nothing, which is exactly what this looks like. */
    case 3: return jz_readf(CPM_CLKGR, PDMA);
    default: return 0;
    }
}

void lcd_update_debug(uint16_t* after_commit, uint16_t* after_copy)
{
    volatile uint16_t* fb = (volatile uint16_t*)FBADDR(0, 0);

    commit_dcache();
    *after_commit = *fb;

    lcd_fbcopy_full();
    *after_copy = *fb;
}

uint32_t dpu_debug_snapshot(int which)
{
    if(!dpu_powered)
        return 0xDEAD0000u | (uint32_t)which;

    switch(which) {
    case 0:  return REG_DPU_ST;
    case 1:  return REG_DPU_DISP_COM;
    case 2:  return REG_DPU_SRD_CHAIN_ADDR;
    case 3:  return REG_DPU_TFT_TIMING_HSYNC;
    case 4:  return REG_DPU_TFT_TIMING_VSYNC;
    case 5:  return REG_DPU_TFT_TIMING_HDE;
    case 6:  return REG_DPU_TFT_TIMING_VDE;
    case 7:  return REG_DPU_TFT_TRAN_CFG;
    case 8:  return lcd_fbcopy_runs;
    case 9:  return lcd_fbcopy_fails;
    case 10: return dpu_stop_noack;
    /* Did the panel ever get a dot clock?  11 == 1 IS the fault. */
    case 11: return lcd_clk_unset;
    case 12: return lcd_clk_in_freq;

    /* Source framebuffer address and first pixel; guard reads before
     * initialisation. */
    case 13: return (uint32_t)lcd_fbbase;
    case 14: return lcd_fbbase ? (uint32_t)*(volatile uint16_t*)lcd_fbbase
                               : 0xFBADFBADu;
    /* Read the DMA destination uncached to avoid reporting a stale CPU
     * cache line. */
    case 15: return (uint32_t)*(volatile uint16_t*)
                    UNCACHEDADDR(&lcd_scanoutfb[0][0]);
    /* Compare the live descriptor address with the expected scanout buffer. */
    case 16: return lcd_srd ? lcd_srd[DPU_SRD_FBADDR] : 0xFFFFFFFFu;
    case 17: return (uint32_t)PHYSADDR(&lcd_scanoutfb[0][0]);
    default: return 0;
    }
}
