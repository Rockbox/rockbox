/***************************************************************************
 * iPod Classic 6G/7G S5L8702 composite video output
 *
 * Copyright (C) 2026 David Cormier
 *
 * The register sequence and private planar format were reconstructed from
 * Apple's iPod Classic firmware and physically qualified on a Philips
 * DCP750/37.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 ****************************************************************************/

#include "config.h"
#include "system.h"
#include "kernel.h"
#include "s5l87xx.h"
#include "clocking-s5l8702.h"
#include "lcd-s5l8702.h"
#include "adc-target.h"
#include "pmu-target.h"
#include "videoout.h"
#include <string.h>

#include <stdint.h>

/* RetailOS 35.2.0.4 format 8 selects the S5L8702 private planar layout.
 * In private plane mode 1, offsets 0x28, 0x2c, and 0x30 carry the three
 * YUV420 planes rather than the later public Samsung top/bottom-field
 * layout. */
/* The S5L8702 mixer predates the public S5P register layout.  RetailOS
 * 35.2.0.4 writes layer-0 base, position, and dimensions at 0x10, 0x14, and
 * 0x18.  Offset 0x14 is not a source stride. */
/* Qualified on the DCP750: a centered 90% NTSC viewport keeps every
 * Rockbox UI pixel inside the panel's composite overscan area. */
#define SVID_UI_DESTINATION_X      36u
#define SVID_UI_DESTINATION_Y      24u
#define SVID_UI_DESTINATION_WIDTH  648u
#define SVID_UI_DESTINATION_HEIGHT 432u
/* RetailOS's nominal 4:3 movie path supplies private format 8 with a native
 * 320x240 planar source.  The VP, not software, scales that source into the
 * 720x480 NTSC destination.  This exact source geometry matters: the DCP750
 * repeatedly rejected the otherwise equivalent 640x240 software-expanded
 * layout at logical source row 120 regardless of image/source height values.
 * One Cb and one Cr sample cover a 2x2 block of native luma samples. */
#define SVID_PLANAR_Y_WIDTH      LCD_WIDTH
#define SVID_PLANAR_Y_HEIGHT     LCD_HEIGHT
#define SVID_PLANAR_SOURCE_HEIGHT SVID_PLANAR_Y_HEIGHT
#define SVID_PLANAR_Y_SIZE       (SVID_PLANAR_Y_WIDTH * \
                                  SVID_PLANAR_Y_HEIGHT)
#define SVID_PLANAR_C_WIDTH      (SVID_PLANAR_Y_WIDTH / 2)
#define SVID_PLANAR_C_HEIGHT     (SVID_PLANAR_Y_HEIGHT / 2)
#define SVID_PLANAR_C_SIZE       (SVID_PLANAR_C_WIDTH * \
                                  SVID_PLANAR_C_HEIGHT)
#define SVID_PLANAR_FRAME_SIZE   (SVID_PLANAR_Y_SIZE + \
                                  2 * SVID_PLANAR_C_SIZE)

#define SVID_CLOCK_SOURCE    3
#define SVID_CLOCK_DIVIDER   4
#define SVID_CLOCK_POLL_MAX  1000
#define SVID_POWER_GATE_0    14
#define SVID_POWER_GATE_1    15
#define SVID_POWER_GATE_2    16
#define SVID_POWER_GATE_MASK ((1u << SVID_POWER_GATE_0) | \
                              (1u << SVID_POWER_GATE_1) | \
                              (1u << SVID_POWER_GATE_2))

#define SVID_GPIO_E4_VIDEO   0x000a040fu
#define SVID_GPIO_E4_MASK    (0xfu << 16)

struct svid_regval
{
    uint16_t offset;
    uint32_t value;
};

static bool svid_active;
static bool svid_layer_active;
static bool svid_layer_planar;
static bool svid_mirror_enabled;
static bool svid_bus_boosted;
/* 96 bytes total.  These target-owned tables replace three software
 * divisions per mirrored pixel without taking core or playback memory. */
static uint8_t svid_rgb5_to_8[32];
static uint8_t svid_rgb6_to_8[64];
static bool svid_rgb_expansion_ready;
static uint8_t svid_output_framebuffer[SVID_PLANAR_FRAME_SIZE]
                                       CACHEALIGN_ATTR;
static const void *svid_policy_framebuffer;
static int svid_policy_width;
static int svid_policy_height;
static volatile enum videoout_mode svid_mode = VIDEOOUT_OFF;
static volatile enum videoout_accessory svid_accessory =
    VIDEOOUT_ACCESSORY_NONE;
static volatile bool svid_policy_pending;
static volatile bool svid_identification_pending;
static bool svid_platform_saved;
static uint16_t svid_saved_clock;
static uint32_t svid_saved_power_gates;
static uint32_t svid_saved_gpio_e4;

static void svid_apply_policy(void);

/* Auto remains deliberately narrow: this is the physically observed
 * DCP750/37 accessory-identify resistor range. */
#define VIDEOOUT_DCP750_RID_MIN 500000
#define VIDEOOUT_DCP750_RID_MAX 575000

static void svid_identify_accessory(void)
{
    int resistor;

    if (!svid_identification_pending)
        return;

    svid_identification_pending = false;
    if (!pmu_accessory_present())
    {
        svid_accessory = VIDEOOUT_ACCESSORY_NONE;
        return;
    }

    /* This function is reached only from normal LCD/settings context.  The
     * ADC implementation takes a mutex, sleeps for its 50 ms bias-settle
     * interval, and yields while converting; none of that is IRQ-safe. */
    resistor = adc_read_accessory_resistor();

    /* The connector may have been removed while the normal thread slept. */
    if (!pmu_accessory_present())
        svid_accessory = VIDEOOUT_ACCESSORY_NONE;
    else if (resistor >= VIDEOOUT_DCP750_RID_MIN &&
             resistor <= VIDEOOUT_DCP750_RID_MAX)
        svid_accessory = VIDEOOUT_ACCESSORY_VIDEO;
    else
        svid_accessory = VIDEOOUT_ACCESSORY_OTHER;
}

static void svid_set_bus_boost(bool enable)
{
    if (enable == svid_bus_boosted)
        return;

    /* The VP memory reader underruns when Rockbox lowers HClk to 54 MHz.
     * Own one normal boost reference for the lifetime of every memory-backed
     * layer; this holds HClk at 108 MHz without interfering with other boost
     * owners, and the paired release restores normal power policy. */
    cpu_boost(enable);
    svid_bus_boosted = enable;
}

static const struct svid_regval compositor_init[] = {
    {0x004, 0}, {0x008, 0}, {0x00c, 0},
    {0x028, 0x08000000}, {0x02c, 0x08000000},
    {0x030, 0x08000000}, {0x034, 0x08000000},
    {0x038, 0x08000000},
    {0x03c, 64}, {0x040, 16}, {0x044, 0}, {0x048, 0},
    {0x04c, 64}, {0x050, 16}, {0x054, 0}, {0x058, 0},
    {0x05c, 64}, {0x060, 16}, {0x064, 512}, {0x068, 512},
    {0x3c0, 1}, {0x3cc, 1},
    {0x06c, 0x00070707}, {0x070, 0x07070707},
    {0x074, 0x07070707}, {0x078, 0x07000000},
    {0x07c, 0x00020405}, {0x080, 0x06060606},
    {0x084, 0x06050504}, {0x088, 0x03020101},
    {0x08c, 0x007a7470}, {0x090, 0x6e6c6b6c},
    {0x094, 0x6c6e7073}, {0x098, 0x76787b7e},
    {0x09c, 0x7f7e7d79}, {0x0a0, 0x726b6359},
    {0x0a4, 0x4f44392e}, {0x0a8, 0x23191008},
    {0x0ec, 0x003d3a38}, {0x0f0, 0x38383839},
    {0x0f4, 0x3a3b3c3d}, {0x0f8, 0x3e3f3f00},
    {0x0fc, 0x7f7e7c76}, {0x100, 0x6f665c51},
    {0x104, 0x463b3025}, {0x108, 0x1b130b05},
    {0x10c, 0x00050b13}, {0x110, 0x1b25303b},
    {0x114, 0x46515c66}, {0x118, 0x6f767c7e},
    {0x11c, 0x00003f3f}, {0x120, 0x3e3d3c3b},
    {0x124, 0x3a393838}, {0x128, 0x38383a3d},
    {0x12c, 0x6b6b6d6f}, {0x130, 0x3336393d},
    {0x134, 0x3f010203}, {0x138, 0x03030202},
    {0x13c, 0xaaa69f95}, {0x140, 0x88786754},
    {0x144, 0x412e1e0f}, {0x148, 0x0279726d},
    {0x200, 1}, {0x20c, 0}, {0x210, 0},
    {0x218, 0x80}, {0x21c, 0x80000080},
    {0x220, 0x80}, {0x224, 0x80}, {0x228, 0x80},
    {0x22c, 0x80}, {0x230, 0x80}, {0x234, 0x80},
    {0x238, 0},
};

static const struct svid_regval router_init[] = {
    {SVID_MXR_STATUS, SVID_MXR_STATUS_IDLE | SVID_MXR_STATUS_SYNC},
    {SVID_MXR_CONFIG, 0},
    {SVID_MXR_VIDEO_CONFIG, 0},
    {SVID_MXR_GRAPHIC0_CONFIG, 0},
    {SVID_MXR_GRAPHIC0_BASE, 0}, {SVID_MXR_GRAPHIC0_POS, 0},
    {SVID_MXR_GRAPHIC0_SIZE, 0},
    {0x01c, 0}, {0x020, 0}, {0x024, 0}, {0x028, 0}, {0x02c, 0},
    {0x030, 0}, {0x034, 0}, {0x038, 0}, {0x03c, 0},
    {SVID_MXR_GRAPHIC_FORMATS, 0}, {0x044, 0},
    {SVID_MXR_BG_COLOR0, 0x00108080},
    {SVID_MXR_BG_COLOR1, 0}, {SVID_MXR_BG_COLOR2, 0},
    {0x054, 0}, {0x058, 0},
    {0x080, 0x08440832}, {0x084, 0x3b4dace1},
    {0x088, 0x0e1d13dc}, {SVID_MXR_COMMIT, 1},
};

static const struct svid_regval encoder_ntsc[] = {
    {0x00c, 6}, {0x010, 1}, {0x014, 0x0000440c},
    {0x01c, 0x800}, {0x020, 0x800}, {0x024, 0x800},
    {0x028, 0x800}, {0x02c, 0x800}, {0x030, 0x800},
    {0x038, 0}, {0x03c, 0x01000700},
    {0x044, 0}, {0x048, 0}, {0x04c, 0}, {0x050, 0},
    {0x054, 0}, {0x058, 0}, {0x05c, 0}, {0x060, 0},
    {0x064, 0}, {0x068, 0}, {0x06c, 0},
    {0x070, 0x0000025d},
    {0x080, 0}, {0x084, 0}, {0x088, 0}, {0x08c, 0},
    {0x090, 0}, {0x094, 1}, {0x098, 7}, {0x09c, 20},
    {0x0a0, 40}, {0x0a4, 63}, {0x0a8, 82}, {0x0ac, 90},
    {0x0c0, 0}, {0x0c4, 0}, {0x0c8, 0}, {0x0cc, 0},
    {0x0d0, 0}, {0x0d4, 1}, {0x0d8, 9}, {0x0dc, 28},
    {0x0e0, 57}, {0x0e4, 90}, {0x0e8, 116}, {0x0ec, 126},
    {0x0f0, 0},
    {0x100, 0}, {0x104, 0}, {0x108, 0}, {0x10c, 0},
    {0x110, 0}, {0x114, 0}, {0x118, 0}, {0x11c, 0},
    {0x120, 0}, {0x124, 0}, {0x128, 0}, {0x12c, 0},
    {0x130, 0}, {0x134, 0}, {0x138, 0}, {0x13c, 0},
    {0x140, 0}, {0x144, 0}, {0x148, 0}, {0x14c, 0},
    {0x150, 0}, {0x154, 0}, {0x158, 0}, {0x15c, 0},
    {0x184, 0x00800000}, {0x188, 0x00800000},
    {0x18c, 0x80}, {0x190, 0}, {0x194, 0x0000eb10},
    {0x198, 0x02000000}, {0x19c, 0x03ff0200},
    {0x1a0, 0x1ff}, {0x1a4, 0x03ff0000},
    {0x1a8, 0x1ff}, {0x1c0, 17},
    {0x200, 0x00fd00fe}, {0x204, 0}, {0x208, 0x00050004},
    {0x20c, 0xff}, {0x210, 0x00f700fa}, {0x214, 1},
    {0x218, 0x000e000a}, {0x21c, 0x1ff},
    {0x220, 0x01ec01f2}, {0x224, 1},
    {0x228, 0x001d0014}, {0x22c, 0x000001fe},
    {0x230, 0x03d803e4}, {0x234, 2}, {0x238, 0x00380028},
    {0x23c, 0x3fd}, {0x240, 0x03b003c7}, {0x244, 5},
    {0x248, 0x00790056}, {0x24c, 0x000003f6},
    {0x250, 0x072c0766}, {0x254, 27},
    {0x258, 0x028b0265}, {0x25c, 0x04000ecc},
    {0x260, 0}, {0x264, 0}, {0x268, 0}, {0x26c, 0x00011a00},
    {0x280, 0}, {0x3c0, 0}, {0x3c4, 0x00010000},
    {0x3c8, 8}, {0x3cc, 0x00010000},
    {0x3d0, 1}, {0x3d4, 8},
};

static void svid_write_table(uintptr_t base,
                             const struct svid_regval *table,
                             unsigned count)
{
    for (unsigned i = 0; i < count; i++)
        SVID_REG(base, table[i].offset) = table[i].value;
}

static bool svid_write_clock(uint16_t value)
{
    volatile uint32_t *reg32 = (volatile uint32_t *)
        ((uintptr_t)&CG16_SVID & ~(uintptr_t)3);
    unsigned shift = ((uintptr_t)&CG16_SVID & 2u) << 3;
    uint32_t preserve = 0xffff0000u >> shift;

    *reg32 = (*reg32 & preserve) | ((uint32_t)value << shift);
    for (unsigned i = 0; i < SVID_CLOCK_POLL_MAX; i++)
    {
        if (CG16_SVID == value)
            return true;
        udelay(1);
    }
    return false;
}

static void svid_platform_restore(void)
{
    if (!svid_platform_saved)
        return;

    PCON(14) = (PCON(14) & ~SVID_GPIO_E4_MASK) | svid_saved_gpio_e4;
    PWRCON(0) = (PWRCON(0) & ~SVID_POWER_GATE_MASK) |
                svid_saved_power_gates;
    svid_write_clock(svid_saved_clock);
    svid_platform_saved = false;
}

static bool svid_platform_enable(void)
{
    uint16_t clock_value =
        ((SVID_CLOCK_SOURCE & CG16_SEL_MSK) << CG16_SEL_POS) |
        (((SVID_CLOCK_DIVIDER - 1) & CG16_DIV1_MSK) << CG16_DIV1_POS);

    svid_saved_clock = CG16_SVID;
    svid_saved_power_gates = PWRCON(0) & SVID_POWER_GATE_MASK;
    svid_saved_gpio_e4 = PCON(14) & SVID_GPIO_E4_MASK;
    svid_platform_saved = true;

    /* Stock firmware selects PLL2 with a divide-by-four SVID clock. */
    if (!svid_write_clock(clock_value))
    {
        svid_platform_restore();
        return false;
    }

    clockgate_enable(SVID_POWER_GATE_0, true);
    clockgate_enable(SVID_POWER_GATE_1, true);
    clockgate_enable(SVID_POWER_GATE_2, true);
    lcd_videoout_clock_acquire();
    GPIOCMD = SVID_GPIO_E4_VIDEO;
    return true;
}

static void svid_begin_reset(void)
{
    SVID_REG(SVID_COMPOSITOR_BASE, 0) &= 2;
    svid_write_table(SVID_COMPOSITOR_BASE, compositor_init,
                     ARRAYLEN(compositor_init));

    /* RetailOS initializes compositor, encoder, then output mixer. */
    SVID_REG(SVID_ENCODER_BASE, SVID_SDO_CLOCK) =
        SVID_SDO_SOFTWARE_RESET;
}

static void svid_finish_reset(void)
{
    SVID_REG(SVID_ENCODER_BASE, SVID_SDO_CLOCK) = 0;
    SVID_REG(SVID_ENCODER_BASE, 0x180) &= ~0x1fu;
    SVID_REG(SVID_ENCODER_BASE, 0x180) |= 0x10;
    svid_write_table(SVID_ENCODER_BASE, encoder_ntsc,
                     ARRAYLEN(encoder_ntsc));
    svid_write_table(SVID_ROUTER_BASE, router_init, ARRAYLEN(router_init));
}

static void svid_select_composite(void)
{
    uint32_t timing = 0x792;
    uint32_t reg = SVID_REG(SVID_ENCODER_BASE, SVID_SDO_CONFIG);

    /* RetailOS 35.2.0.4 does these as two adjacent operations in its output
     * setup: clear mixer bit 2 for NTSC, then set mixer bit 1 before loading
     * the interlaced SDO timing table.  Every qualified failing build reached
     * the mixer as C=0x10 instead of stock C=0x12 and presented only the
     * first 120-line field before repeating its final row. */
    SVID_REG(SVID_ROUTER_BASE, SVID_MXR_CONFIG) =
        (SVID_REG(SVID_ROUTER_BASE, SVID_MXR_CONFIG) & ~4u) |
        SVID_MXR_SD_SCAN;
    SVID_REG(SVID_ENCODER_BASE, 0x34) = 0;
    /* RetailOS selects the dock's composite path with DAC mux values 1, 0,
     * and 2.  Clearing these bits produced sync with a green no-input field. */
    reg &= ~(SVID_SDO_DAC_MUX_MASK | SVID_SDO_COMPONENT |
             SVID_SDO_PROGRESSIVE | SVID_SDO_STANDARD_MASK);
    SVID_REG(SVID_ENCODER_BASE, SVID_SDO_CONFIG) = reg | 0x1200u;

    if (SVID_REG(0x3f000000u, 4) & 0x100u)
    {
        SVID_REG(SVID_ENCODER_BASE, 0x28) = timing + 0x23;
        SVID_REG(SVID_ENCODER_BASE, 0x2c) = timing + 0x6d;
        SVID_REG(SVID_ENCODER_BASE, 0x30) = timing;
    }
    else
    {
        SVID_REG(SVID_ENCODER_BASE, 0x28) = timing | (timing >> 7);
        SVID_REG(SVID_ENCODER_BASE, 0x2c) = timing + 0x27;
        SVID_REG(SVID_ENCODER_BASE, 0x30) = timing + 8;
    }

}

static void svid_start_pipeline(void)
{
    SVID_REG(SVID_ENCODER_BASE, SVID_SDO_DAC) |= SVID_SDO_DAC_ALL_ON;
    SVID_REG(SVID_ENCODER_BASE, SVID_SDO_CLOCK) |= SVID_SDO_CLOCK_ON;
    SVID_REG(SVID_ROUTER_BASE, SVID_MXR_STATUS) |= SVID_MXR_STATUS_RUN;
}

static void svid_mixer_commit(void)
{
    SVID_REG(SVID_ROUTER_BASE, SVID_MXR_COMMIT) = 1;
}

static void svid_init_rgb_expansion(void)
{
    if (svid_rgb_expansion_ready)
        return;

    for (unsigned value = 0; value < ARRAYLEN(svid_rgb5_to_8); value++)
        svid_rgb5_to_8[value] = value * 255 / 31;

    for (unsigned value = 0; value < ARRAYLEN(svid_rgb6_to_8); value++)
        svid_rgb6_to_8[value] = value * 255 / 63;

    svid_rgb_expansion_ready = true;
}

static uint32_t svid_rgb565_to_ycbcr(uint16_t pixel)
{
    int red = svid_rgb5_to_8[(pixel >> 11) & 0x1f];
    int green = svid_rgb6_to_8[(pixel >> 5) & 0x3f];
    int blue = svid_rgb5_to_8[pixel & 0x1f];
    int y;
    int cb;
    int cr;

    /* ITU-R BT.601 limited-range values, matching the SD mixer. */
    /* The exhaustive color gate proves these limited-range expressions stay
     * inside byte range for all 65,536 RGB565 inputs. */
    y = ((66 * red + 129 * green + 25 * blue + 128) >> 8) + 16;
    cb = ((-38 * red - 74 * green + 112 * blue + 128) >> 8) + 128;
    cr = ((112 * red - 94 * green - 18 * blue + 128) >> 8) + 128;
    return (uint32_t)y | ((uint32_t)cb << 8) | ((uint32_t)cr << 16);
}

static void svid_copy_framebuffer_planar(const uint16_t *source,
                                         int width, int height)
{
    uint8_t *luma = (uint8_t *)svid_output_framebuffer;
    uint8_t *cb = luma + SVID_PLANAR_Y_SIZE;
    uint8_t *cr = cb + SVID_PLANAR_C_SIZE;

    memset(luma, 16, SVID_PLANAR_Y_SIZE);
    memset(cb, 128, SVID_PLANAR_C_SIZE);
    memset(cr, 128, SVID_PLANAR_C_SIZE);

    for (int y = 0; y < height; y += 2)
    {
        uint8_t *luma0 = luma + y * SVID_PLANAR_Y_WIDTH;
        uint8_t *cb0 = cb + (y / 2) * SVID_PLANAR_C_WIDTH;
        uint8_t *cr0 = cr + (y / 2) * SVID_PLANAR_C_WIDTH;
        int y1 = MIN(y + 1, height - 1);
        uint8_t *luma1 = luma + y1 * SVID_PLANAR_Y_WIDTH;
        const uint16_t *source0 = source + y * width;
        const uint16_t *source1 = source + y1 * width;

        for (int x = 0; x < width; x += 2)
        {
            int x1 = MIN(x + 1, width - 1);
            uint32_t pixel00 = svid_rgb565_to_ycbcr(source0[x]);
            uint32_t pixel01 = svid_rgb565_to_ycbcr(source0[x1]);
            uint32_t pixel10 = svid_rgb565_to_ycbcr(source1[x]);
            uint32_t pixel11 = svid_rgb565_to_ycbcr(source1[x1]);

            luma0[x] = pixel00;
            luma0[x1] = pixel01;
            luma1[x] = pixel10;
            luma1[x1] = pixel11;
            cb0[x / 2] = (((pixel00 >> 8) & 0xff) +
                           ((pixel01 >> 8) & 0xff) +
                           ((pixel10 >> 8) & 0xff) +
                           ((pixel11 >> 8) & 0xff) + 2) >> 2;
            cr0[x / 2] = (((pixel00 >> 16) & 0xff) +
                           ((pixel01 >> 16) & 0xff) +
                           ((pixel10 >> 16) & 0xff) +
                           ((pixel11 >> 16) & 0xff) + 2) >> 2;
        }
    }

    commit_dcache_range(luma, SVID_PLANAR_FRAME_SIZE);
}

static bool svid_enable_sync(void)
{
    svid_init_rgb_expansion();

    if (svid_active)
        return true;

    int oldlevel = disable_irq_save();
    if (!svid_platform_enable())
    {
        restore_irq(oldlevel);
        return false;
    }
    svid_begin_reset();

    /* The reset wait is almost the 44.1 kHz PCM DMA emergency-buffer
     * duration. Service DMA during the wait, but keep every SVID register
     * write in the original atomic initialization sequence. */
    restore_irq(oldlevel);
    udelay(10000);

    oldlevel = disable_irq_save();
    svid_finish_reset();
    svid_select_composite();
    svid_start_pipeline();
    restore_irq(oldlevel);

    svid_active = true;
    svid_layer_active = false;
    svid_layer_planar = false;
    svid_mirror_enabled = false;
    return true;
}

static bool svid_show_framebuffer(const void *framebuffer,
                                  int width, int height)
{
    if (framebuffer == NULL || width != LCD_WIDTH || height != LCD_HEIGHT)
        return false;

    if (!svid_enable_sync())
        return false;

    svid_set_bus_boost(true);

    int oldlevel = disable_irq_save();
    SVID_REG(SVID_ROUTER_BASE, SVID_MXR_CONFIG) &=
        ~SVID_MXR_ENABLE_MASK;
    SVID_REG(SVID_COMPOSITOR_BASE, SVID_VP_ENABLE) &=
        ~SVID_VP_ENABLE_ON;
    svid_mixer_commit();
    restore_irq(oldlevel);

    svid_copy_framebuffer_planar(framebuffer, width, height);

    oldlevel = disable_irq_save();

    uintptr_t luma = (uintptr_t)svid_output_framebuffer;
    uintptr_t cb = luma + SVID_PLANAR_Y_SIZE;
    uintptr_t cr = cb + SVID_PLANAR_C_SIZE;

    /* Exact RetailOS private format-8 setup: descriptor 0 -> 0x28,
     * descriptor 2 -> 0x2c, descriptor 1 -> 0x30, zero -> 0x34, spans width
     * and width/2, and plane mode 1.  Physical DCP750 color qualification
     * fixes this private ordering as Y, Cb, and Cr for our generated frame. */
    SVID_REG(SVID_COMPOSITOR_BASE, SVID_VP_MODE) =
        SVID_VP_MODE_RETAIL_FMT8;
    SVID_REG(SVID_COMPOSITOR_BASE, SVID_VP_PLANE0_PTR) = luma;
    SVID_REG(SVID_COMPOSITOR_BASE, SVID_VP_PLANE2_PTR) = cb;
    SVID_REG(SVID_COMPOSITOR_BASE, SVID_VP_UNUSED_PTR) = 0;
    SVID_REG(SVID_COMPOSITOR_BASE, SVID_VP_PLANE1_PTR) = cr;
    SVID_REG(SVID_COMPOSITOR_BASE, SVID_VP_IMG_WIDTH) =
        SVID_PLANAR_Y_WIDTH;
    SVID_REG(SVID_COMPOSITOR_BASE, SVID_VP_IMG_HEIGHT) =
        SVID_PLANAR_Y_HEIGHT;
    SVID_REG(SVID_COMPOSITOR_BASE, SVID_VP_SRC_H_POS) = 0;
    SVID_REG(SVID_COMPOSITOR_BASE, SVID_VP_SRC_V_POS) = 0;
    SVID_REG(SVID_COMPOSITOR_BASE, SVID_VP_SRC_WIDTH) =
        SVID_PLANAR_Y_WIDTH;
    SVID_REG(SVID_COMPOSITOR_BASE, SVID_VP_SRC_HEIGHT) =
        SVID_PLANAR_SOURCE_HEIGHT;
    SVID_REG(SVID_COMPOSITOR_BASE, SVID_VP_DST_H_POS) =
        SVID_UI_DESTINATION_X;
    /* RetailOS's direct geometry path writes caller-provided destination Y
     * and height verbatim on this exact SoC.
     * Applying the later S5P driver's field-line /2 conversion produces the
     * physically observed upper-half-only picture. */
    SVID_REG(SVID_COMPOSITOR_BASE, SVID_VP_DST_V_POS) =
        SVID_UI_DESTINATION_Y;
    SVID_REG(SVID_COMPOSITOR_BASE, SVID_VP_DST_WIDTH) =
        SVID_UI_DESTINATION_WIDTH;
    SVID_REG(SVID_COMPOSITOR_BASE, SVID_VP_DST_HEIGHT) =
        SVID_UI_DESTINATION_HEIGHT;
    SVID_REG(SVID_COMPOSITOR_BASE, SVID_VP_H_RATIO) =
        ((SVID_PLANAR_Y_WIDTH << 12) / SVID_UI_DESTINATION_WIDTH) >> 3;
    /* Destination geometry remains in RetailOS full-frame coordinates. */
    SVID_REG(SVID_COMPOSITOR_BASE, SVID_VP_V_RATIO) =
        ((SVID_PLANAR_Y_HEIGHT << 12) / SVID_UI_DESTINATION_HEIGHT) >> 4;
    SVID_REG(SVID_COMPOSITOR_BASE, SVID_VP_PLANE_MODE) =
        SVID_VP_PLANE_PLANAR;
    SVID_REG(SVID_COMPOSITOR_BASE, SVID_VP_LUMA_SPAN) =
        SVID_PLANAR_Y_WIDTH;
    SVID_REG(SVID_COMPOSITOR_BASE, SVID_VP_CHROMA_SPAN) =
        SVID_PLANAR_C_WIDTH;
    SVID_REG(SVID_COMPOSITOR_BASE, SVID_VP_ENDIAN_MODE) =
        SVID_VP_ENDIAN_LITTLE;

    SVID_REG(SVID_ROUTER_BASE, SVID_MXR_VIDEO_CONFIG) = 0;
    SVID_REG(SVID_ROUTER_BASE, SVID_MXR_BG_COLOR0) = 0x00108080u;
    SVID_REG(SVID_ROUTER_BASE, SVID_MXR_BG_COLOR1) = 0x00108080u;
    SVID_REG(SVID_ROUTER_BASE, SVID_MXR_BG_COLOR2) = 0x00108080u;
    SVID_REG(SVID_COMPOSITOR_BASE, SVID_VP_ENABLE) |=
        SVID_VP_ENABLE_ON;
    SVID_REG(SVID_ROUTER_BASE, SVID_MXR_CONFIG) =
        (SVID_REG(SVID_ROUTER_BASE, SVID_MXR_CONFIG) &
         ~SVID_MXR_ENABLE_MASK) | SVID_MXR_VIDEO_ENABLE;
    SVID_REG(SVID_ROUTER_BASE, SVID_MXR_STATUS) =
        SVID_MXR_STATUS_IDLE | SVID_MXR_STATUS_SYNC | SVID_MXR_STATUS_RUN;
    svid_mixer_commit();

    restore_irq(oldlevel);
    svid_layer_planar = true;
    svid_mirror_enabled = true;
    svid_layer_active = true;
    return true;
}

static bool svid_stop_block(uintptr_t base)
{
    SVID_REG(base, 0) &= ~1u;
    for (unsigned i = 0; i < 10; i++)
    {
        sleep(1);
        if (SVID_REG(base, 0) & 2)
            return true;
    }
    return false;
}

bool videoout_active(void)
{
    if (svid_policy_pending)
        svid_apply_policy();

    return svid_active && svid_layer_active;
}

bool videoout_lcd_clock_required(void)
{
    return svid_platform_saved;
}

static void svid_commit_planar_rect(int x, int y, int width, int height)
{
    uint8_t *luma = (uint8_t *)svid_output_framebuffer;
    uint8_t *cb = luma + SVID_PLANAR_Y_SIZE;
    uint8_t *cr = cb + SVID_PLANAR_C_SIZE;
    int first_chroma_column = x / 2;
    int last_chroma_column = (x + width - 1) / 2;
    int first_chroma_row = y / 2;
    int last_chroma_row = (y + height - 1) / 2;
    unsigned luma_span = (height - 1) * SVID_PLANAR_Y_WIDTH + width;
    unsigned chroma_width = last_chroma_column - first_chroma_column + 1;
    unsigned chroma_span =
        (last_chroma_row - first_chroma_row) * SVID_PLANAR_C_WIDTH +
        chroma_width;

    /* One clean per touched plane replaces a clean/barrier for every row.
     * The spans include untouched bytes between partial rows but never leave
     * the fixed output buffer. */
    commit_dcache_range(luma + y * SVID_PLANAR_Y_WIDTH + x, luma_span);
    commit_dcache_range(cb + first_chroma_row * SVID_PLANAR_C_WIDTH +
                        first_chroma_column, chroma_span);
    commit_dcache_range(cr + first_chroma_row * SVID_PLANAR_C_WIDTH +
                        first_chroma_column, chroma_span);
}

static void svid_copy_yuv_plane(uint8_t *destination,
                                unsigned destination_stride,
                                const uint8_t *source,
                                unsigned source_stride,
                                unsigned width, unsigned height)
{
    if (width == destination_stride && width == source_stride)
    {
        memcpy(destination, source, width * height);
        return;
    }

    while (height-- > 0)
    {
        memcpy(destination, source, width);
        destination += destination_stride;
        source += source_stride;
    }
}

bool videoout_mirror_yuv420(const unsigned char *source_luma,
                            const unsigned char *source_cb,
                            const unsigned char *source_cr,
                            int source_x, int source_y,
                            int source_stride,
                            int x, int y, int width, int height)
{
    uint8_t *luma = (uint8_t *)svid_output_framebuffer;
    uint8_t *cb = luma + SVID_PLANAR_Y_SIZE;
    uint8_t *cr = cb + SVID_PLANAR_C_SIZE;

    if (!videoout_active() || !svid_mirror_enabled ||
        !svid_layer_planar || source_luma == NULL || source_cb == NULL ||
        source_cr == NULL || source_x < 0 || source_y < 0 ||
        source_stride < width || x < 0 || y < 0 || width <= 0 ||
        height <= 0 || x + width > LCD_WIDTH || y + height > LCD_HEIGHT ||
        source_x + width > source_stride ||
        ((source_x | source_y | source_stride | x | y | width | height) & 1))
        return false;

    source_luma += source_y * source_stride + source_x;
    source_cb += (source_y / 2) * (source_stride / 2) + source_x / 2;
    source_cr += (source_y / 2) * (source_stride / 2) + source_x / 2;

    svid_copy_yuv_plane(luma + y * SVID_PLANAR_Y_WIDTH + x,
                        SVID_PLANAR_Y_WIDTH, source_luma, source_stride,
                        width, height);
    svid_copy_yuv_plane(cb + (y / 2) * SVID_PLANAR_C_WIDTH + x / 2,
                        SVID_PLANAR_C_WIDTH, source_cb, source_stride / 2,
                        width / 2, height / 2);
    svid_copy_yuv_plane(cr + (y / 2) * SVID_PLANAR_C_WIDTH + x / 2,
                        SVID_PLANAR_C_WIDTH, source_cr, source_stride / 2,
                        width / 2, height / 2);
    svid_commit_planar_rect(x, y, width, height);
    return true;
}

static void svid_mirror_rgb565_planar_even(const uint16_t *source,
                                           int x, int y, int width,
                                           int height, int stride)
{
    uint8_t *luma = (uint8_t *)svid_output_framebuffer;
    uint8_t *cb = luma + SVID_PLANAR_Y_SIZE;
    uint8_t *cr = cb + SVID_PLANAR_C_SIZE;

    for (int row = 0; row < height; row += 2)
    {
        const uint16_t *source0 = source + row * stride;
        const uint16_t *source1 = source0 + stride;
        uint8_t *luma0 = luma + (y + row) * SVID_PLANAR_Y_WIDTH + x;
        uint8_t *luma1 = luma0 + SVID_PLANAR_Y_WIDTH;
        uint8_t *cb0 = cb + ((y + row) / 2) * SVID_PLANAR_C_WIDTH + x / 2;
        uint8_t *cr0 = cr + ((y + row) / 2) * SVID_PLANAR_C_WIDTH + x / 2;

        for (int column = 0; column < width; column += 2)
        {
            uint32_t pixel00 = svid_rgb565_to_ycbcr(source0[column]);
            uint32_t pixel01 = svid_rgb565_to_ycbcr(source0[column + 1]);
            uint32_t pixel10 = svid_rgb565_to_ycbcr(source1[column]);
            uint32_t pixel11 = svid_rgb565_to_ycbcr(source1[column + 1]);

            luma0[column] = pixel00;
            luma0[column + 1] = pixel01;
            luma1[column] = pixel10;
            luma1[column + 1] = pixel11;
            cb0[column / 2] = (((pixel00 >> 8) & 0xff) +
                               ((pixel01 >> 8) & 0xff) +
                               ((pixel10 >> 8) & 0xff) +
                               ((pixel11 >> 8) & 0xff) + 2) >> 2;
            cr0[column / 2] = (((pixel00 >> 16) & 0xff) +
                               ((pixel01 >> 16) & 0xff) +
                               ((pixel10 >> 16) & 0xff) +
                               ((pixel11 >> 16) & 0xff) + 2) >> 2;
        }
    }
}

static void svid_mirror_rgb565_planar(const uint16_t *source, int x, int y,
                                      int width, int height, int stride)
{
    uint8_t *luma = (uint8_t *)svid_output_framebuffer;
    uint8_t *cb = luma + SVID_PLANAR_Y_SIZE;
    uint8_t *cr = cb + SVID_PLANAR_C_SIZE;

    /* LCD and decoded-video frames use even 4:2:0 rectangles.  Convert each
     * source pixel exactly once, writing its luma and contributing its chroma
     * in the same pass. */
    if (((x | y | width | height) & 1) == 0)
    {
        svid_mirror_rgb565_planar_even(source, x, y, width, height, stride);
        svid_commit_planar_rect(x, y, width, height);
        return;
    }

    for (int row = 0; row < height; row++)
    {
        uint8_t *luma0 = luma + (y + row) * SVID_PLANAR_Y_WIDTH + x;
        const uint16_t *source_row = source + row * stride;

        for (int column = 0; column < width; column++)
        {
            luma0[column] = svid_rgb565_to_ycbcr(source_row[column]);
        }
    }

    int first_chroma_column = x / 2;
    int last_chroma_column = (x + width - 1) / 2;
    int first_chroma_row = y / 2;
    int last_chroma_row = (y + height - 1) / 2;
    for (int chroma_row = first_chroma_row;
         chroma_row <= last_chroma_row; chroma_row++)
    {
        int source_y0 = MAX(chroma_row * 2, y) - y;
        int source_y1 = MIN(chroma_row * 2 + 1, y + height - 1) - y;
        const uint16_t *source_row0 = source + source_y0 * stride;
        const uint16_t *source_row1 = source + source_y1 * stride;
        uint8_t *cb0 = cb + chroma_row * SVID_PLANAR_C_WIDTH +
                       first_chroma_column;
        uint8_t *cr0 = cr + chroma_row * SVID_PLANAR_C_WIDTH +
                       first_chroma_column;

        for (int chroma_column = first_chroma_column;
             chroma_column <= last_chroma_column; chroma_column++)
        {
            int source_x0 = MAX(chroma_column * 2, x) - x;
            int source_x1 = MIN(chroma_column * 2 + 1,
                                x + width - 1) - x;
            int output_column = chroma_column - first_chroma_column;
            uint32_t pixel00 =
                svid_rgb565_to_ycbcr(source_row0[source_x0]);
            uint32_t pixel01 =
                svid_rgb565_to_ycbcr(source_row0[source_x1]);
            uint32_t pixel10 =
                svid_rgb565_to_ycbcr(source_row1[source_x0]);
            uint32_t pixel11 =
                svid_rgb565_to_ycbcr(source_row1[source_x1]);

            cb0[output_column] = (((pixel00 >> 8) & 0xff) +
                                  ((pixel01 >> 8) & 0xff) +
                                  ((pixel10 >> 8) & 0xff) +
                                  ((pixel11 >> 8) & 0xff) + 2) >> 2;
            cr0[output_column] = (((pixel00 >> 16) & 0xff) +
                                  ((pixel01 >> 16) & 0xff) +
                                  ((pixel10 >> 16) & 0xff) +
                                  ((pixel11 >> 16) & 0xff) + 2) >> 2;
        }
    }

    svid_commit_planar_rect(x, y, width, height);
}

void videoout_mirror_rgb565(const void *source, int x, int y,
                            int width, int height, int stride)
{
    const uint16_t *src = source;

    if (!videoout_active() || !svid_mirror_enabled || source == NULL ||
        !svid_layer_planar || x < 0 || y < 0 || width <= 0 || height <= 0 ||
        stride < width || x + width > LCD_WIDTH ||
        y + height > LCD_HEIGHT)
        return;

    svid_mirror_rgb565_planar(src, x, y, width, height, stride);
}

bool videoout_disable(void)
{
    if (!svid_active)
    {
        svid_set_bus_boost(false);
        return true;
    }

    SVID_REG(SVID_ROUTER_BASE, SVID_MXR_CONFIG) &=
        ~SVID_MXR_ENABLE_MASK;
    SVID_REG(SVID_ENCODER_BASE, 0x03c) &= ~0xfu;
    bool clean = svid_stop_block(SVID_ENCODER_BASE);
    clean = svid_stop_block(SVID_ROUTER_BASE) && clean;
    clean = svid_stop_block(SVID_COMPOSITOR_BASE) && clean;

    int oldlevel = disable_irq_save();
    svid_platform_restore();
    restore_irq(oldlevel);

    svid_active = false;
    svid_layer_active = false;
    svid_layer_planar = false;
    svid_mirror_enabled = false;
    lcd_videoout_clock_release();
    svid_set_bus_boost(false);
    return clean;
}

static void svid_apply_policy(void)
{
    /* Off is fully dormant: do not even perform the one-time accessory ADC
     * transaction.  A deferred request remains available if the user later
     * selects Auto or On while the same dock is still connected. */
    if (svid_mode != VIDEOOUT_OFF)
        svid_identify_accessory();

    /* Auto requires the qualified DCP750 signature. On accepts a dock only
     * after accessory identification has completed; PENDING must not bypass
     * the startup window and start SVID while the connector/iAP state is still
     * settling. NONE must never retain CPU/LCD clocks while undocked. */
    bool enable =
        (svid_mode == VIDEOOUT_AUTO &&
         svid_accessory == VIDEOOUT_ACCESSORY_VIDEO) ||
        (svid_mode == VIDEOOUT_ON &&
         (svid_accessory == VIDEOOUT_ACCESSORY_VIDEO ||
          svid_accessory == VIDEOOUT_ACCESSORY_OTHER));

    svid_policy_pending = false;

    if (enable)
    {
        if ((!svid_active || !svid_layer_active) &&
            svid_policy_framebuffer != NULL)
            svid_show_framebuffer(svid_policy_framebuffer,
                                  svid_policy_width,
                                  svid_policy_height);
    }
    else if (svid_active)
    {
        videoout_disable();
    }
}

void videoout_request_accessory_identification(void)
{
    /* Called by the serial tick task: stores only, no ADC/I2C or SVID work. */
    svid_identification_pending = true;
    if (svid_mode != VIDEOOUT_OFF)
        svid_policy_pending = true;
}

void videoout_set_mode(enum videoout_mode mode,
                       const void *framebuffer,
                       int width, int height)
{
    if (mode > VIDEOOUT_ON)
        mode = VIDEOOUT_OFF;

    svid_mode = mode;
    svid_policy_framebuffer = framebuffer;
    svid_policy_width = width;
    svid_policy_height = height;
    svid_policy_pending = true;
    svid_apply_policy();
}

void videoout_accessory_state(enum videoout_accessory accessory)
{
    if (accessory > VIDEOOUT_ACCESSORY_OTHER)
        accessory = VIDEOOUT_ACCESSORY_OTHER;

    if (accessory != VIDEOOUT_ACCESSORY_PENDING)
        svid_identification_pending = false;

    if (svid_accessory != accessory)
    {
        svid_accessory = accessory;
        svid_policy_pending = true;
    }
}
