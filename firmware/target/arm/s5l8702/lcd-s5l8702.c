/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: lcd-nano2g.c 28868 2010-12-21 06:59:17Z Buschel $
 *
 * Copyright (C) 2009 by Dave Chapman
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
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "config.h"

#include "kernel.h"
#include "system.h"
#include "cpu.h"
#include "lcd.h"
#ifdef IPOD_6G
#include "pmu-target.h"
#endif
#include "backlight-target.h"

#include "s5l87xx.h"
#include "clocking-s5l8702.h"
#include "dma-s5l8702.h"
#include "lcd-s5l8702.h"
#include "lcd-target.h"


// TODO TODO TODO: HAVE_LCD_ENABLE

/* Switch command/frame mode:
 *
 * XXX: WIP, TBC
 *
 * Configures MPU interface and pixel mode.
 *
 * Frame mode:
 *   6g:
 *       16-bit 8080-series parallel MPU interface
 *       pixel mode: RBG565/1-transfer
 *       APB[15:0] -> D[17:10,8:1]
 *   nano3g:
 *       9-bit 8080-series parallel PMU interface (TBC)
 *       pixel mode: RGB565/2-transfers (TBC)
 *       APB[15:0] -> D[8:1] + D[8:1] (TBC)
 *
 * Command mode:
 *   LCD HW that uses 8-bit command set (6g, nano3g):
 *       8-bit 8080-series parallel MPU interface
 *       APB[7:0] -> D[8:1]
 *   LCD HW that uses 16-bit command set (6g):
 *       18-bit 8080-series parallel MPU interface (TBC)
 *       APB[15:0] -> D[17:10,8:1]
 *
 * See ILI9320, ILI9326, ILI9340 DS.
 */
/* XXX: see ili9340, pg.27 for MCU interface selection,
 *      bit31 could be the one that selects interface D[7:0] or D[8:1] for 8-bit), Type-I uses D[7:0] and Type-II uses D[17:10] (ILI9340)
 *                                                                           9-bit               D[8:0]                D[17:9]
 *                                                                          16-bit               D[15:0]               D[17:10],D[8:1]
 *                                                                          18-bit               D[17:0]               D[17:0]
 *      bit24 could be the one that selects interface D[8:1] or D[17:10]
 *                                          MCU_interface Mode          Pins            Write                           Read
 * #define LCD_MODE_8      0x80000c20       8080 MCU 8-bit bus iface    D[8:1]          APB[7:0] -> D[8:1]              D[8:1] -> LCD_DBUFF[8:1]
 * #define LCD_MODE_9      0x81100db8                9-bit
 * #define LCD_MODE_16     0x80100db0               16-bit              D[17:10,8:1]    APB[15:0] -> D[17:10,8:1]
 * #define LCD_MODE_18     0x80000da8               18-bit              D[17:0]         APB[17:0] -> D[17:0] (TBC)
 *
 * #define LCD_MODE_8      0x80000c20          // LCD_MODE_P8T1        // paralled 8-bit, 1 transfer, TBC: DB[17:10] or DB[8:1] ???
 * #define LCD_MODE_9      0x81100db8          // LCD_MODE_P9T2        // TBC: DB[8:1] or DB[17:10] ???, 2-transfers or 1.5-transfers, RGB565 or RBG666 ???
 * #define LCD_MODE_16     0x80100db0          // LCD_MODE_P16T1       // TBC: DB[17:10,8:1]
 * #define LCD_MODE_18     0x80000da8          // LCD_MODE_P18T1       // TBC: DB[17:10,8:1]
 */

#define LCD_MODE_P8     0x80000c20          // LCD_MODE_P8T1        // paralled 8-bit, 1 transfer, TBC: DB[17:10] or DB[8:1] ???
#define LCD_MODE_P8b    0x80100c20          // TODO: see if it influences, so far we are using it in nano3g with 0x80000c20 and it seems to be working fine
                                            // It may be a "pixel format" setting for 8 bit, which would only affect DATA and not CMD ???
#define LCD_MODE_P9     0x81100db8          // LCD_MODE_P9T2        // TBC: DB[8:1] or DB[17:10] ???, 2-transfers or 1.5-transfers, RGB565 or RBG666 ???
#define LCD_MODE_P16    0x80100db0          // LCD_MODE_P16T1       // TBC: DB[17:10,8:1]
#define LCD_MODE_P18    0x80000da8          // LCD_MODE_P18T1       // TBC: DB[17:10,8:1]

#define LCD_MODE_S8     0x41000c20          // nano4g
#define LCD_MODE_S9     0x41100db8          // nano4g



#ifdef S5L_LCD_WITH_CMDSET16
/* LCD type 16-bit register defines */
#define R_HORIZ_GRAM_ADDR_SET     0x200
#define R_VERT_GRAM_ADDR_SET      0x201
#define R_WRITE_DATA_TO_GRAM      0x202
#define R_HORIZ_ADDR_START_POS    0x210
#define R_HORIZ_ADDR_END_POS      0x211
#define R_VERT_ADDR_START_POS     0x212
#define R_VERT_ADDR_END_POS       0x213
#endif

/* LCD type 8-bit register defines */
#define R_COLUMN_ADDR_SET         0x2a
#define R_ROW_ADDR_SET            0x2b
#define R_MEMORY_WRITE            0x2c


/** globals **/

int lcd_type; /* also needed in debug-s5l8702.c */
static struct mutex lcd_mutex;
static uint16_t lcd_dblbuf[LCD_HEIGHT][LCD_WIDTH] CACHEALIGN_ATTR;
static bool lcd_ispowered;

/* TODO: there is no info about these specific drivers,
   some useful info on similar HW such as ILI9320, ILI9326, ILI9340 and */

static struct lcd_info_rec *lcd_info;
static void (*lcd_run_seq)(void*);

/* LCD_CONFIG values for command/frame modes */
static uint32_t lcd_cmd_mode IDATA_ATTR;
static uint32_t lcd_frame_mode IDATA_ATTR;


// TODO
/* Each target must define a list containing all supported LCD types */
// extern struct lcd_info_rec lcd_info_list[];
// static struct lcd_info_rec lcd_info_list[];


/* DMA configuration */

/* One single transfer at once, needed LLIs:
 *   screen_size / (DMAC_LLI_MAX_COUNT << swidth) =
 *   (320*240*2) / (4095*2) = 19
 */
#define LCD_DMA_TSKBUF_SZ   1   /* N tasks, MUST be pow2 */
#define LCD_DMA_LLIBUF_SZ   32  /* N LLIs, MUST be pow2 */

static struct dmac_tsk lcd_dma_tskbuf[LCD_DMA_TSKBUF_SZ];
static struct dmac_lli volatile \
            lcd_dma_llibuf[LCD_DMA_LLIBUF_SZ] CACHEALIGN_ATTR;

static struct dmac_ch lcd_dma_ch =
{
    .dmac = &s5l8702_dmac0,
    .prio = DMAC_CH_PRIO(4),
    .cb_fn = NULL,

    .tskbuf = lcd_dma_tskbuf,
    .tskbuf_mask = LCD_DMA_TSKBUF_SZ - 1,
    .queue_mode = QUEUE_NORMAL,

    .llibuf = lcd_dma_llibuf,
    .llibuf_mask = LCD_DMA_LLIBUF_SZ - 1,
    .llibuf_bus = DMAC_MASTER_AHB1,
};

static struct dmac_ch_cfg lcd_dma_ch_cfg =
{
    .srcperi = S5L8702_DMAC0_PERI_MEM,
    .dstperi = S5L8702_DMAC0_PERI_LCD_WR,
    .sbsize  = DMACCxCONTROL_BSIZE_1,
    .dbsize  = DMACCxCONTROL_BSIZE_1,
    .swidth  = DMACCxCONTROL_WIDTH_16,
    .dwidth  = DMACCxCONTROL_WIDTH_16,
    .sbus    = DMAC_MASTER_AHB1,
    .dbus    = DMAC_MASTER_AHB1,
    .sinc    = DMACCxCONTROL_INC_ENABLE,
    .dinc    = DMACCxCONTROL_INC_DISABLE,
    .prot    = DMAC_PROT_CACH | DMAC_PROT_BUFF | DMAC_PROT_PRIV,
    .lli_xfer_max_count = DMAC_LLI_MAX_COUNT,
};


/*** clocks ***/

// TODO: In mks5lboot --mkraw put a command to specify the address of the binary,
// for example --address 0x6000, it must be greater than 0x310 which would be the default address,
// pass this address (0x310..128Kb) in the dfu_options flag

// TODO: to lcd-target.c
static void lcd_target_enable_clocks(bool enable)
{
    clockgate_enable(CLOCKGATE_LCD, enable);
#ifdef IPOD_NANO4G
    clockgate_enable(CLOCKGATE_LCD_2, enable);
#endif
}


/*** LCD controller - low level functions ***/

static void s5l_lcd_write_config(uint32_t config) ICODE_ATTR;
static void s5l_lcd_write_config(uint32_t config)
{
    while (!(LCD_STATUS & 0x2));
    udelay(1);
    LCD_CON = config;
}

static void s5l_lcd_write_cmd(uint16_t cmd) ICODE_ATTR;
static void s5l_lcd_write_cmd(uint16_t cmd)
{
    while (LCD_STATUS & 0x10);
    LCD_WCMD = cmd;
}

static void s5l_lcd_write_data(uint16_t data) ICODE_ATTR;
static void s5l_lcd_write_data(uint16_t data)
{
    while (LCD_STATUS & 0x10);
    LCD_WDATA = data;
}

static void s5l_lcd_send_cmd8(uint8_t cmd, int len, uint8_t *data) ICODE_ATTR;
static void s5l_lcd_send_cmd8(uint8_t cmd, int len, uint8_t *data)
{
    s5l_lcd_write_cmd(cmd);
    while (len--)
        s5l_lcd_write_data(*data++);
}

#ifdef S5L_LCD_WITH_READID
static void s5l_lcd_recv_cmd8(uint8_t cmd, int len, uint8_t *buf)
{
    s5l_lcd_write_cmd(cmd);
    while (len--) {
        while (!(LCD_STATUS & 0x2));
        LCD_RDATA = 0;
        while (!(LCD_STATUS & 1));
        *buf++ = LCD_DBUFF >> 1;
    }
}
#endif

#define s5l_lcd_set_command_mode()  s5l_lcd_write_config(lcd_cmd_mode)
#define s5l_lcd_set_frame_mode()    s5l_lcd_write_config(lcd_frame_mode)

static void s5l_lcd_run_seq8(void *seq8)
{
    uint8_t *seq = seq8;

    while (1) switch (*seq++)
    {
        case CMD:
        {
            uint8_t cmd = *seq++;
            int len = *seq++;
            s5l_lcd_send_cmd8(cmd, len, seq);
            seq += len;
            break;
        }
        case SLEEP:
            sleep(*seq++);
            break;
        case END:
        default:
            /* bye */
            return;
    }
}

#ifdef S5L_LCD_WITH_CMDSET16
static void s5l_lcd_write_reg(uint16_t cmd, uint16_t data) ICODE_ATTR;
static void s5l_lcd_write_reg(uint16_t cmd, uint16_t data)
{
    s5l_lcd_write_cmd(cmd);
    s5l_lcd_write_data(data);
}

static void s5l_lcd_run_seq16(void *seq16)
{
    uint16_t *seq = seq16;
    int action, param;
    uint16_t reg;

    while (1)
    {
        action = *seq & 0xff;
        param = *seq++ >> 8;

        switch (action)
        {
            case CMD:
                s5l_lcd_write_cmd(*seq++);
                break;
            case MREG:
                reg = *seq++;
                while (param--)
                    s5l_lcd_write_reg(reg++, *seq++);
                break;
            case SLEEP:
                sleep(param);
                break;
            case DELAY:
                udelay(param<<6);
                break;
            case END:
            default:
                /* bye */
                return;
        }
    }
}
#endif /* S5L_LCD_WITH_CMDSET16 */


/*** Update functions ***/

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void) ICODE_ATTR;
void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

static void displaylcd_setup(int x, int y, int width, int height) ICODE_ATTR;
static void displaylcd_setup(int x, int y, int width, int height)
{
    int xe = (x + width) - 1;           /* max horiz */
    int ye = (y + height) - 1;          /* max vert */

    s5l_lcd_set_command_mode();

#ifdef S5L_LCD_WITH_CMDSET16
    if (lcd_info->cmdset == LCD_CMDSET_16BIT)
    {
        s5l_lcd_write_reg(R_HORIZ_ADDR_START_POS, x);
        s5l_lcd_write_reg(R_HORIZ_ADDR_END_POS,   xe);
        s5l_lcd_write_reg(R_VERT_ADDR_START_POS,  y);
        s5l_lcd_write_reg(R_VERT_ADDR_END_POS,    ye);

        s5l_lcd_write_reg(R_HORIZ_GRAM_ADDR_SET,  x);
        s5l_lcd_write_reg(R_VERT_GRAM_ADDR_SET,   y);

        s5l_lcd_write_cmd(R_WRITE_DATA_TO_GRAM);
    }
    else
#endif
    {
        uint8_t col[] = { x >> 8, x & 0xff, xe >> 8, xe & 0xff };
        uint8_t row[] = { y >> 8, y & 0xff, ye >> 8, ye & 0xff };
        s5l_lcd_send_cmd8(R_COLUMN_ADDR_SET, 4, col);
        s5l_lcd_send_cmd8(R_ROW_ADDR_SET, 4, row);

        s5l_lcd_write_cmd(R_MEMORY_WRITE);
    }
}

static void displaylcd_dma(int pixels) ICODE_ATTR;
static void displaylcd_dma(int pixels)
{
    s5l_lcd_set_frame_mode();
    commit_dcache();
    dmac_ch_queue(&lcd_dma_ch, lcd_dblbuf,
            (void*)S5L8702_DADDR_PERI_LCD_WR, pixels*2, NULL);
}

// TODO: wait if there is a DMA transfer in progress
static void displaylcd_wait_dma(void) ICODE_ATTR;
static void displaylcd_wait_dma(void)
{
    while (dmac_ch_running(&lcd_dma_ch))
        yield();
}

/* Update a fraction of the display. */
void lcd_update_rect(int, int, int, int) ICODE_ATTR;
void lcd_update_rect(int x, int y, int width, int height)
{
    int pixels = width * height;
    fb_data* p = FBADDR(x,y);
    uint16_t* out = lcd_dblbuf[0];

    /* FIXME: ISR()->panicf()->lcd_update() blocks forever */
    mutex_lock(&lcd_mutex);
    if (lcd_ispowered)
    {
        // XXX: We contemplate the case where the LCD has entered SLEEP,
        //      the s5l_lcd_set_command_mode() writes LCD_CONFIG
        // XXX: It is also possible that if the LCD clockgate has been disabled,
        //      the LCD_CONFIG = xxx may still be blocked by the while(status == ok),
        //      since it does not check the same status bit as when writing command/data

        displaylcd_wait_dma();

        displaylcd_setup(x, y, width, height);

        /* Copy display bitmap to hardware */
        if (LCD_WIDTH == width) {
            /* Write all lines at once */
            memcpy(out, p, pixels * 2);
        } else {
            do {
                /* Write a single line */
                memcpy(out, p, width * 2);
                p += LCD_WIDTH;
                out += width;
            } while (--height);
        }

        displaylcd_dma(pixels);
    }
    mutex_unlock(&lcd_mutex);
}

/* Line write helper function for lcd_yuv_blit. Writes two lines of yuv420. */
extern void lcd_write_yuv420_lines(unsigned char const * const src[3],
                                   uint16_t* outbuf,
                                   int width,
                                   int stride);

/* Blit a YUV bitmap directly to the LCD */
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height) ICODE_ATTR;
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    unsigned int z;
    unsigned char const * yuv_src[3];

    width = (width + 1) & ~1;       /* ensure width is even */

    int pixels = width * height;
    uint16_t* out = lcd_dblbuf[0];

    z = stride * src_y;
    yuv_src[0] = src[0] + z + src_x;
    yuv_src[1] = src[1] + (z >> 2) + (src_x >> 1);
    yuv_src[2] = src[2] + (yuv_src[1] - src[1]);

    /* TODO: ISR()->panicf()->lcd_update() blocks forever */
    mutex_lock(&lcd_mutex);
    if (lcd_ispowered)
    {
        displaylcd_wait_dma();

        displaylcd_setup(x, y, width, height);

        height >>= 1;

        do {
            lcd_write_yuv420_lines(yuv_src, out, width, stride);
            yuv_src[0] += stride << 1;
            yuv_src[1] += stride >> 1; /* Skip down one chroma line */
            yuv_src[2] += stride >> 1;
            out += width << 1;
        } while (--height);

        displaylcd_dma(pixels);
    }
    mutex_unlock(&lcd_mutex);
}


/*** hardware configuration ***/

int lcd_default_contrast(void)
{
    return 0x1f;
}

void lcd_set_contrast(int val)
{
    (void)val;
}

void lcd_set_invert_display(bool yesno)
{
    (void)yesno;
}

void lcd_set_flip(bool yesno)
{
    (void)yesno;
}

#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
bool lcd_active(void)
{
    return lcd_ispowered;
}
#endif

#if defined(HAVE_LCD_SHUTDOWN) || defined(HAVE_LCD_SLEEP)
static void lcd_powersave(void)
{
    mutex_lock(&lcd_mutex);

    displaylcd_wait_dma();
                                   // XXX: Do not change modes while data is being sent
    s5l_lcd_set_command_mode();
    lcd_run_seq(lcd_info->seq_sleep);

    lcd_target_enable_clocks(false);

    lcd_ispowered = false;

    mutex_unlock(&lcd_mutex);
}
#endif /* HAVE_LCD_SHUTDOWN || HAVE_LCD_SLEEP */

#ifdef HAVE_LCD_SHUTDOWN
void lcd_shutdown(void)
{
    backlight_hw_kill();  /* Kill the backlight, instantly. */
    lcd_powersave();
}
#endif

#ifdef HAVE_LCD_SLEEP
void lcd_sleep(void)
{
    lcd_powersave();
}

void lcd_awake(void)
{
    mutex_lock(&lcd_mutex);

    lcd_target_enable_clocks(true);
                                 // XXX: Do not change modes while data is being sent
    s5l_lcd_set_command_mode();
    lcd_run_seq(lcd_info->seq_awake);
    lcd_ispowered = true;       // XXX: we have to put the lcd_ispowered before the lcd_update()

    lcd_update();               // XXX: update the display and wait for the update to finish before returning,

    displaylcd_wait_dma();      //      we should really do sleep_out + lcd_update + display_on,
                                //      we can put a command in the sequence to do it

    mutex_unlock(&lcd_mutex);
    send_event(LCD_EVENT_ACTIVATION, NULL);
}
#endif

#ifdef S5L_LCD_WITH_READID
// TODO: protect it with mutex and while(dma) if you want to call it from other places (e.g. DEBUG)
void lcd_read_display_id(int mpuiface, uint8_t *lcd_id)
{
    s5l_lcd_write_config(
            (mpuiface == LCD_MPUIFACE_SERIAL) ? LCD_MODE_S8 : LCD_MODE_P8);
    s5l_lcd_recv_cmd8(4, 4, lcd_id);
}
#endif

/* LCD init */
void lcd_init_device(void)
{
    mutex_init(&lcd_mutex);

    lcd_target_enable_clocks(true);
#if defined(IPOD_6G) || defined(IPOD_NANO3G)
    LCD_PHTIME = 0x33;
#elif defined(IPOD_NANO4G)
    cg16_config(&CG16_LCD, true, CG16_SEL_PLL0, 16, 1, 0x0);
    s5l_lcd_write_config(LCD_MODE_S9);
    cg16_config(&CG16_LCD, false, CG16_SEL_PLL0, 16, 1, 0x0);
    LCD_PHTIME = 0x11;
#endif

    lcd_info = lcd_target_get_info();
    lcd_type = lcd_info->lcd_type;

    /* select mode to send commands */
#ifdef S5L_LCD_WITH_CMDSET16
    if (lcd_info->cmdset == LCD_CMDSET_16BIT)
    {
        lcd_cmd_mode = LCD_MODE_P18;
        lcd_run_seq = s5l_lcd_run_seq16;
    }
    else /* LCD_CMDSET_8BIT */
#endif
    {
        if (lcd_info->mpuiface == LCD_MPUIFACE_SERIAL)
            lcd_cmd_mode = LCD_MODE_S8;
        else
            lcd_cmd_mode = LCD_MODE_P8;
        lcd_run_seq = s5l_lcd_run_seq8;
    }

    /* select mode to send RGB565 data */
    if (lcd_info->mpuiface == LCD_MPUIFACE_PAR9)
        lcd_frame_mode = LCD_MODE_P9;
    else if (lcd_info->mpuiface == LCD_MPUIFACE_PAR18)
        lcd_frame_mode = LCD_MODE_P16;
    else /* LCD_MPUIFACE_SERIAL */
        lcd_frame_mode = LCD_MODE_S9;

    s5l_lcd_set_command_mode();

    /* Configure DMA channel */                             // TODO: this right after mutex_init()
    dmac_ch_init(&lcd_dma_ch, &lcd_dma_ch_cfg);

#ifdef BOOTLOADER
    lcd_run_seq(lcd_info->seq_init);
#endif

    lcd_ispowered = true;
}
