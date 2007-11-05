/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 * Tuner driver for the Sanyo LV24020LP
 *
 * Copyright (C) 2007 Ivan Zupan
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdbool.h>
#include <stdlib.h>
#include "config.h"
#include "thread.h"
#include "kernel.h"
#include "tuner.h" /* tuner abstraction interface */
#include "power.h"
#include "fmradio.h" /* physical interface driver */
#include "sound.h"
#include "pp5024.h"
#include "system.h"

#ifndef BOOTLOADER

static struct mutex tuner_mtx;

#if 0
/* define to enable tuner logging */
#define SANYO_TUNER_LOG
#endif

#ifdef SANYO_TUNER_LOG
#include "sprintf.h"
#include "file.h"

static int fd_log = -1;

#define TUNER_LOG_OPEN()    if (fd_log < 0) \
                                fd_log = creat("/tuner_dump.txt")
/* syncing required because close() is never called */
#define TUNER_LOG_SYNC()    fsync(fd_log)
#define TUNER_LOG(s...)     fdprintf(fd_log, s)
#else
#define TUNER_LOG_OPEN()
#define TUNER_LOG_SYNC()
#define TUNER_LOG(s...)
#endif /* SANYO_TUNER_LOG */

/** tuner register defines **/

/* pins on GPIOH port */
#define FM_NRW_PIN      3
#define FM_CLOCK_PIN    4
#define FM_DATA_PIN     5
#define FM_CLK_DELAY    1

/* block 1 registers */

/* R */
#define CHIP_ID     0x00

/* W */
#define BLK_SEL     0x01
    #define BLK1 0x01
    #define BLK2 0x02

/* W */
#define MSRC_SEL    0x02
    #define MSR_O   (1 << 7)
    #define AFC_LVL (1 << 6)
    #define AFC_SPD (1 << 5)
    #define MSS_SD  (1 << 2)
    #define MSS_FM  (1 << 1)
    #define MSS_IF  (1 << 0)

/* W */
#define FM_OSC      0x03

/* W */
#define SD_OSC      0x04

/* W */
#define IF_OSC      0x05

/* W */
#define CNT_CTRL    0x06
    #define CNT1_CLR  (1 << 7)
    #define CTAB(x)   ((x) & (0x7 << 4))
        #define CTAB_STOP_2     (0x0 << 4)
        #define CTAB_STOP_8     (0x1 << 4)
        #define CTAB_STOP_32    (0x2 << 4)
        #define CTAB_STOP_128   (0x3 << 4)
        #define CTAB_STOP_512   (0x4 << 4)
        #define CTAB_STOP_2048  (0x5 << 4)
        #define CTAB_STOP_8192  (0x6 << 4)
        #define CTAB_STOP_32768 (0x7 << 4)
    #define SWP_CNT_L (1 << 3)
    #define CNT_EN    (1 << 2)
    #define CNT_SEL   (1 << 1)
    #define CNT_SET   (1 << 0)

/* W */
#define IRQ_MSK 0x08
    #define IM_MS   (1 << 6)
    #define IRQ_LVL (1 << 3)
    #define IM_AFC  (1 << 2)
    #define IM_FS   (1 << 1)
    #define IM_CNT2 (1 << 0)

/* W */
#define FM_CAP      0x09

/* R */
#define CNT_L       0x0a /* Counter register low value */

/* R */
#define CNT_H       0x0b /* Counter register high value */

/* R */
#define CTRL_STAT   0x0c
    #define AFC_FLG (1 << 0)

/* R */
#define RADIO_STAT  0x0d
    #define RSS_MS        (1 << 7)
    #define RSS_FS(x)     ((x) & 0x7f)
    #define RSS_FS_GET(x) ((x) & 0x7f)
    #define RSS_FS_SET(x) (x)
/* Note: Reading this register will clear field strength and mono/stereo interrupt. */

/* R */
#define IRQ_ID      0x0e
    #define II_CNT2  (1 << 5)
    #define II_AFC   (1 << 3)
    #define II_FS_MS (1 << 0)

/* W */
#define IRQ_OUT     0x0f

/* block 2 registers - offset added in order to id and avoid manual
   switching */
#define BLK2_START  0x10

/* W */
#define RADIO_CTRL1 (0x02 + BLK2_START)
    #define EN_MEAS (1 << 7)
    #define EN_AFC  (1 << 6)
    #define DIR_AFC (1 << 3)
    #define RST_AFC (1 << 2)

/* W */
#define IF_CENTER   (0x03 + BLK2_START)

/* W */
#define IF_BW       (0x05 + BLK2_START)

/* W */
#define RADIO_CTRL2 (0x06 + BLK2_START)
  #define VREF2      (1 << 7)
  #define VREF       (1 << 6)
  #define STABI_BP   (1 << 5)
  #define IF_PM_L    (1 << 4)
  #define AGCSP      (1 << 1)
  #define AM_ANT_BSW (1 << 0) /* ?? */

/* W */
#define RADIO_CTRL3 (0x07 + BLK2_START)
    #define AGC_SLVL (1 << 7)
    #define VOLSH    (1 << 6)
    #define TB_ON    (1 << 5)
    #define AMUTE_L  (1 << 4)
    #define SE_FM    (1 << 3)
    #define SE_BE    (1 << 1)
    #define SE_EXT   (1 << 0) /* For LV24000=0, LV24001/24002=Ext source enab. */

/* W */
#define STEREO_CTRL (0x08 + BLK2_START)
    #define FRCST       (1 << 7)
    #define FMCS(x)     ((x) & (0x7 << 4))
    #define FMCS_GET(x) (((x) & (0x7 << 4)) >> 4)
    #define FMCS_SET(x) ((x) << 4)
    #define AUTOSSR     (1 << 3)
    #define PILTCA      (1 << 2)
    #define SD_PM       (1 << 1)
    #define ST_M        (1 << 0)

/* W */
#define AUDIO_CTRL1 (0x09 + BLK2_START)
    #define TONE_LVL(x)     ((x) & (0xf << 4))
    #define TONE_LVL_GET(x) (((x) & (0xf << 4)) >> 4)
    #define TONE_LVL_SET(x) ((x) << 4)
    #define VOL_LVL(x)      ((x) & 0xf)
    #define VOL_LVL_GET(x)  ((x) & 0xf)
    #define VOL_LVL_SET(x)  ((x) << 4)

/* W */
#define AUDIO_CTRL2 (0x0a + BLK2_START)
    #define BASS_PP     (1 << 0)
    #define BASS_P      (1 << 1) /* BASS_P, BASS_N are mutually-exclusive */
    #define BASS_N      (1 << 2)
    #define TREB_P      (1 << 3) /* TREB_P, TREB_N are mutually-exclusive */
    #define TREB_N      (1 << 4)
    #define DEEMP       (1 << 5)
    #define BPFREQ(x)   ((x) & (0x3 << 6))
    #define BPFREQ_2_0K (0x0 << 6)
    #define BPFREQ_1_0K (0x1 << 6)
    #define BPFREQ_0_5K (0x2 << 6)
    #define BPFREQ_HIGH (0x3 << 6)

/* W */
#define PW_SCTRL    (0x0b + BLK2_START)
    #define SS_CTRL(x)     ((x) & (0x7 << 5))
    #define SS_CTRL_GET(x) (((x) & (0x7 << 5)) >> 5)
    #define SS_CTRL_SET(x) ((x) << 5)
    #define SM_CTRL(x)     ((x) & (0x7 << 2))
    #define SM_CTRL_GET(x) (((x) & (0x7 << 2)) >> 2)
    #define SM_CTRL_SET(x) ((x) << 2)
    #define PW_HPA         (1 << 1) /* LV24002 only */
    #define PW_RAD         (1 << 0)

/* shadow for writeable registers */
#define TUNER_POWERED           (1 << 0)
#define TUNER_PRESENT           (1 << 1)
#define TUNER_AWAKE             (1 << 2)
#define TUNER_PRESENCE_CHECKED  (1 << 3)
static unsigned tuner_status = 0;

static unsigned char lv24020lp_regs[0x1c];

static const int sw_osc_low  = 10;  /* 30; */
static const int sw_osc_high = 240; /* 200; */
static const int sw_cap_low  = 0;
static const int sw_cap_high = 191;

/* linear coefficients used for tuning */
static int coef_00, coef_01, coef_10, coef_11;

/* DAC control register set values */
int if_set, sd_set;

static inline bool tuner_awake(void)
{
    return (tuner_status & TUNER_AWAKE) != 0;
}

/* send a byte to the tuner - expects write mode to be current */
static void lv24020lp_send_byte(unsigned int byte)
{
    int i;

    byte <<= FM_DATA_PIN;

    for (i = 0; i < 8; i++)
    {
        GPIOH_OUTPUT_VAL &= ~(1 << FM_CLOCK_PIN);

        GPIOH_OUTPUT_VAL = (GPIOH_OUTPUT_VAL & ~(1 << FM_DATA_PIN)) |
                            (byte & (1 << FM_DATA_PIN));

        GPIOH_OUTPUT_VAL |= (1 << FM_CLOCK_PIN);
        udelay(FM_CLK_DELAY);

        byte >>= 1;
    }
}

/* end a write cycle on the tuner */
static void lv24020lp_end_write(void)
{
    /* switch back to read mode */
    GPIOH_OUTPUT_EN &= ~(1 << FM_DATA_PIN);
    GPIOH_OUTPUT_VAL &= ~(1 << FM_NRW_PIN);
}

/* prepare a write cycle on the tuner */
static unsigned int lv24020lp_begin_write(unsigned int address)
{
    /* Get register's block, translate address */
    unsigned int blk = (address >= BLK2_START) ?
                            (address -= BLK2_START, BLK2) : BLK1;

    for (;;)
    {
        /* Prepare 3-wire bus pins for write cycle */
        GPIOH_OUTPUT_VAL |= (1 << FM_NRW_PIN);
        GPIOH_OUTPUT_EN |= (1 << FM_DATA_PIN);

        udelay(FM_CLK_DELAY);

        /* current block == register block? */
        if (blk == lv24020lp_regs[BLK_SEL])
            return address;

        /* switch block */
        lv24020lp_regs[BLK_SEL] = blk;

        /* data first */
        lv24020lp_send_byte(blk);
        /* then address */
        lv24020lp_send_byte(BLK_SEL);

        lv24020lp_end_write();

        udelay(FM_CLK_DELAY);
    }
}

/* write a byte to a tuner register */
static void lv24020lp_write(unsigned int address, unsigned int data)
{
    /* shadow logical values but do logical=>physical remappings on some
       registers' data. */
    lv24020lp_regs[address] = data;

    switch (address)
    {
    case FM_OSC:
        /* L: 000..255
         * P: 255..000 */
        data = 255 - data;
        break;
    case FM_CAP:
        /* L: 000..063, 064..191
         * P: 255..192, 127..000 */
        data = ((data < 64) ? 255 : (255 - 64)) - data;
        break;
    case RADIO_CTRL1:
        /* L: data
         * P: data | always "1" bits */
        data |= (1 << 4) | (1 << 1) | (1 << 0);
        break;
    }

    /* Check if interface is turned on */
    if (!(tuner_status & TUNER_POWERED))
        return;

    address = lv24020lp_begin_write(address);

    /* data first */
    lv24020lp_send_byte(data);
    /* then address */
    lv24020lp_send_byte(address);

    lv24020lp_end_write();
}

/* helpers to set/clear register bits */
static void lv24020lp_write_or(unsigned int address, unsigned int bits)
{
    lv24020lp_write(address, lv24020lp_regs[address] | bits);
}

static void lv24020lp_write_and(unsigned int address, unsigned int bits)
{
    lv24020lp_write(address, lv24020lp_regs[address] & bits);
}

/* read a byte from a tuner register */
static unsigned int lv24020lp_read(unsigned int address)
{
    int i;
    unsigned int toread;

    /* Check if interface is turned on */
    if (!(tuner_status & TUNER_POWERED))
        return 0;

    address = lv24020lp_begin_write(address);

    /* address */
    lv24020lp_send_byte(address);

    lv24020lp_end_write();

    /* data */
    toread = 0;
    for (i = 0; i < 8; i++)
    {
        GPIOH_OUTPUT_VAL &= ~(1 << FM_CLOCK_PIN);
        udelay(FM_CLK_DELAY);

        toread |= (GPIOH_INPUT_VAL & (1 << FM_DATA_PIN)) << i;

        GPIOH_OUTPUT_VAL |= (1 << FM_CLOCK_PIN);
    }

    return toread >> FM_DATA_PIN;
}

/* enables auto frequency centering */
static void enable_afc(bool enabled)
{
    unsigned int radio_ctrl1 = lv24020lp_regs[RADIO_CTRL1];

    if (enabled)
    {
        radio_ctrl1 &= ~RST_AFC;
        radio_ctrl1 |= EN_AFC;
    }
    else
    {
        radio_ctrl1 |= RST_AFC;
        radio_ctrl1 &= ~EN_AFC;
    }

    lv24020lp_write(RADIO_CTRL1, radio_ctrl1);
}

static int calculate_coef(unsigned fkhz)
{
    /* Overflow below 66000kHz --
       My tuner tunes down to a min of ~72600kHz but datasheet mentions
       66000kHz as the minimum. ?? Perhaps 76000kHz was intended? */
    return fkhz < 66000 ?
        0x7fffffff : 0x81d1a47efc5cb700ull / ((uint64_t)fkhz*fkhz);
}

static int interpolate_x(int expected_y, int x1, int x2, int y1, int y2)
{
    return y1 == y2 ?
        0 : (int64_t)(expected_y - y1)*(x2 - x1) / (y2 - y1) + x1;
}

static int interpolate_y(int expected_x, int x1, int x2, int y1, int y2)
{
    return x1 == x2 ?
        0 : (int64_t)(expected_x - x1)*(y2 - y1) / (x2 - x1) + y1;
}

/* this performs measurements of IF, FM and Stereo frequencies
 * Input can be: MSS_FM, MSS_IF, MSS_SD */
static int tuner_measure(unsigned char type, int scale, int duration)
{
    int64_t finval;

    /* enable measuring */
    lv24020lp_write_or(MSRC_SEL, type);
    lv24020lp_write_and(CNT_CTRL, ~CNT_SEL);
    lv24020lp_write_or(RADIO_CTRL1, EN_MEAS);

    /* reset counter */
    lv24020lp_write_or(CNT_CTRL, CNT1_CLR);
    lv24020lp_write_and(CNT_CTRL, ~CNT1_CLR);

    /* start counter, delay for specified time and stop it */
    lv24020lp_write_or(CNT_CTRL, CNT_EN);
    udelay(duration*1000 - 16);
    lv24020lp_write_and(CNT_CTRL, ~CNT_EN);

    /* read tick count */
    finval = (lv24020lp_read(CNT_H) << 8) | lv24020lp_read(CNT_L);

    /* restore measure mode */
    lv24020lp_write_and(RADIO_CTRL1, ~EN_MEAS);
    lv24020lp_write_and(MSRC_SEL, ~type);

    /* convert value */
    if (type == MSS_FM)
        finval = scale*finval*256 / duration;
    else
        finval = scale*finval / duration;

    /* This function takes a loooong time and other stuff needs
       running by now */
    yield();

    return (int)finval;
}

/* set the FM oscillator frequency */
static void set_frequency(int freq)
{
    int coef, cap_value, osc_value;
    int f1, f2, x1, x2;
    int count;

    TUNER_LOG_OPEN();

    TUNER_LOG("set_frequency(%d)\n", freq);

    enable_afc(false);

    /* MHz -> kHz */
    freq /= 1000;

    TUNER_LOG("Select cap:\n");

    coef = calculate_coef(freq);
    cap_value = interpolate_x(coef, sw_cap_low, sw_cap_high,
                    coef_00, coef_01);

    osc_value = sw_osc_low;
    lv24020lp_write(FM_OSC, osc_value); 

    /* Just in case - don't go into infinite loop */
    for (count = 0; count < 30; count++)
    {
        int y0 = interpolate_y(cap_value, sw_cap_low, sw_cap_high,
                                coef_00, coef_01);
        int y1 = interpolate_y(cap_value, sw_cap_low, sw_cap_high,
                                coef_10, coef_11);
        int coef_fcur, cap_new, coef_cor, range;
         
        lv24020lp_write(FM_CAP, cap_value);

        range     = y1 - y0;
        f1        = tuner_measure(MSS_FM, 1, 16);
        coef_fcur = calculate_coef(f1);
        coef_cor  = calculate_coef((f1*1000 + 32*256) / 1000);
        y0        = coef_cor;
        y1        = y0 + range;

        TUNER_LOG("%d %d %d %d %d %d %d %d\n",
                  f1, cap_value, coef, coef_fcur, coef_cor, y0, y1, range);

        if (coef >= y0 && coef <= y1) 
        {
            osc_value = interpolate_x(coef, sw_osc_low, sw_osc_high,
                                        y0, y1);

            if (osc_value >= sw_osc_low && osc_value <= sw_osc_high)
                break;
        }

        cap_new = interpolate_x(coef, cap_value, sw_cap_high,
                                coef_fcur, coef_01);

        if (cap_new == cap_value)
        {
            if (coef < coef_fcur)
                cap_value++;
            else
                cap_value--;
        }
        else
        {
            cap_value = cap_new;
        }
    }

    TUNER_LOG("osc_value: %d\n", osc_value);

    TUNER_LOG("Tune:\n");

    x1 = sw_osc_low, x2 = sw_osc_high;
    /* FM_OSC already at SW_OSC low and f1 is already the measured
       frequency */

    do
    {
        int x2_new;

        lv24020lp_write(FM_OSC, x2);
        f2 = tuner_measure(MSS_FM, 1, 16);

        if (abs(f2 - freq) <= 16)
        {
            TUNER_LOG("%d %d %d %d\n", f1, f2, x1, x2);
            break;
        }

        x2_new = interpolate_x(freq, x1, x2, f1, f2);

        x1 = x2, f1 = f2, x2 = x2_new;
        TUNER_LOG("%d %d %d %d\n", f1, f2, x1, x2);
    }
    while (x2 != 0);

    if (x2 == 0)
    {
        /* May still be close enough */
        TUNER_LOG("tuning failed - diff: %d\n", f2 - freq);
    }

    enable_afc(true);

    TUNER_LOG("\n");

    TUNER_LOG_SYNC();
}

static void fine_step_tune(int (*setcmp)(int regval), int regval, int step)
{
    /* Registers are not always stable, timeout if best fit not found soon
       enough */
    unsigned long abort = current_tick + HZ*2;
    int flags = 0;

    while (TIME_BEFORE(current_tick, abort))
    {
        int cmp;

        regval = regval + step;

        cmp = setcmp(regval);

        if (cmp == 0)
            break;

        step = abs(step);

        if (cmp < 0)
        {
            flags |= 1;
            if (step == 1)
                flags |= 4;
        }
        else
        {
            step = -step;
            flags |= 2;
            if (step == -1)
                step |= 8;
        }

        if ((flags & 0xc) == 0xc)
            break;

        if ((flags & 0x3) == 0x3)
        {
            step /= 2;
            if (step == 0)
                step = 1;
            flags &= ~3;
        }
    }
}

static int if_setcmp(int regval)
{
    lv24020lp_write(IF_OSC, regval);
    lv24020lp_write(IF_CENTER, regval);
    lv24020lp_write(IF_BW, 65*regval/100);

    if_set = tuner_measure(MSS_IF, 1000, 32);

    /* This register is bounces around by a few hundred Hz and doesn't seem
       to be precisely tuneable. Just do 110000 +/- 500 since it's not very
       critical it seems. */
    if (abs(if_set - 110000) <= 500)
        return 0;

    return if_set < 110000 ? -1 : 1;
}

static int sd_setcmp(int regval)
{
    lv24020lp_write(SD_OSC, regval);

    sd_set = tuner_measure(MSS_SD, 1000, 32);

    if (abs(sd_set - 38300) <= 31)
        return 0;

    return sd_set < 38300 ? -1 : 1;
}

static void set_sleep(bool sleep)
{
    if (sleep || tuner_awake())
        return;

    if ((tuner_status & (TUNER_PRESENT | TUNER_POWERED)) !=
        (TUNER_PRESENT | TUNER_POWERED))
        return;

    enable_afc(false);

    /* 2. Calibrate the IF frequency at 110 kHz: */
    lv24020lp_write_and(RADIO_CTRL2, ~IF_PM_L);
    fine_step_tune(if_setcmp, 0x80, 8);
    lv24020lp_write_or(RADIO_CTRL2, IF_PM_L);

    /* 3. Calibrate the stereo decoder clock at 38.3 kHz: */
    lv24020lp_write_or(STEREO_CTRL, SD_PM);
    fine_step_tune(sd_setcmp, 0x80, 8);
    lv24020lp_write_and(STEREO_CTRL, ~SD_PM);

    /* calculate FM tuning coefficients */
    lv24020lp_write(FM_CAP, sw_cap_low);
    lv24020lp_write(FM_OSC, sw_osc_low);
    coef_00 = calculate_coef(tuner_measure(MSS_FM, 1, 64));

    lv24020lp_write(FM_CAP, sw_cap_high);
    coef_01 = calculate_coef(tuner_measure(MSS_FM, 1, 64));

    lv24020lp_write(FM_CAP, sw_cap_low);
    lv24020lp_write(FM_OSC, sw_osc_high);
    coef_10 = calculate_coef(tuner_measure(MSS_FM, 1, 64));

    lv24020lp_write(FM_CAP, sw_cap_high);
    coef_11 = calculate_coef(tuner_measure(MSS_FM, 1, 64));

    /* set various audio level settings */
    lv24020lp_write(AUDIO_CTRL1, TONE_LVL_SET(0) | VOL_LVL_SET(0));
    lv24020lp_write_or(RADIO_CTRL2, AGCSP);
    lv24020lp_write_or(RADIO_CTRL3, VOLSH);
    lv24020lp_write(STEREO_CTRL, FMCS_SET(7) | AUTOSSR);
    lv24020lp_write(PW_SCTRL, SS_CTRL_SET(3) | SM_CTRL_SET(1) |
                    PW_RAD);

    tuner_status |= TUNER_AWAKE;
}

static int lp24020lp_tuned(void)
{
    return RSS_FS(lv24020lp_read(RADIO_STAT)) < 0x1f;
}

static int lv24020lp_debug_info(int setting)
{
    int val = -1;

    if (setting >= LV24020LP_DEBUG_FIRST && setting <= LV24020LP_DEBUG_LAST)
    {
        val = 0;
    
        if (tuner_awake())
        {
            switch (setting)
            {
            /* tuner-specific debug info */
            case LV24020LP_CTRL_STAT:
                val = lv24020lp_read(CTRL_STAT);
                break;

            case LV24020LP_REG_STAT:
                val = lv24020lp_read(RADIO_STAT);
                break;

            case LV24020LP_MSS_FM:
                val = tuner_measure(MSS_FM, 1, 16);
                break;

            case LV24020LP_MSS_IF:
                val = tuner_measure(MSS_IF, 1000, 16);
                break;

            case LV24020LP_MSS_SD:
                val = tuner_measure(MSS_SD, 1000, 16);
                break;

            case LV24020LP_IF_SET:
                val = if_set;
                break;

            case LV24020LP_SD_SET:
                val = sd_set;
                break;
            }
        }
    }

    return val;
}

/** Public interfaces **/
void lv24020lp_init(void)
{
    mutex_init(&tuner_mtx);
}

void lv24020lp_lock(void)
{
    mutex_lock(&tuner_mtx);
}

void lv24020lp_unlock(void)
{
    mutex_unlock(&tuner_mtx);
}

/* This function expects the driver to be locked externally */
void lv24020lp_power(bool status)
{
    static const unsigned char tuner_defaults[][2] =
    {
        /* Block 1 writeable  registers */
        { MSRC_SEL,    AFC_LVL },
        { FM_OSC,      0x80 },
        { SD_OSC,      0x80 },
        { IF_OSC,      0x80 },
        { CNT_CTRL,    CNT1_CLR | SWP_CNT_L },
        { IRQ_MSK,     0x00 }, /* IRQ_LVL -> Low to High */
        { FM_CAP,      0x80 },
     /* { IRQ_OUT,     0x00 }, No action on this register (skip) */
        /* Block 2 writeable registers */
        { RADIO_CTRL1, EN_AFC },
        { IF_CENTER,   0x80 },
        { IF_BW,       65*0x80 / 100 }, /* 65% of IF_OSC */
        { RADIO_CTRL2, IF_PM_L },
        { RADIO_CTRL3, AGC_SLVL | SE_FM },
        { STEREO_CTRL, FMCS_SET(4) | AUTOSSR },
        { AUDIO_CTRL1, TONE_LVL_SET(7) | VOL_LVL_SET(7) },
        { AUDIO_CTRL2, BPFREQ_HIGH },   /* deemphasis 50us */
        { PW_SCTRL,    SS_CTRL_SET(3) | SM_CTRL_SET(3) | PW_RAD },
    };

    unsigned i;

    if (status)
    {
        tuner_status |= (TUNER_PRESENCE_CHECKED | TUNER_POWERED);

        /* if tuner is present, CHIP ID is 0x09 */
        if (lv24020lp_read(CHIP_ID) == 0x09)
        {
            tuner_status |= TUNER_PRESENT;

            /* After power-up, the LV2400x needs to be initialized as
               follows: */

            /* 1. Write default values to the registers: */
            lv24020lp_regs[BLK_SEL] = 0; /* Force a switch on the first */
            for (i = 0; i < ARRAYLEN(tuner_defaults); i++)
                lv24020lp_write(tuner_defaults[i][0], tuner_defaults[i][1]);

            /* Complete the startup calibration if the tuner is woken */
            sleep(HZ/10);
        }
    }
    else
    {
        tuner_status &= ~(TUNER_POWERED | TUNER_AWAKE);

        /* Power off */
        if (tuner_status & TUNER_PRESENT)
            lv24020lp_write_and(PW_SCTRL, ~PW_RAD);
    }
}

int lv24020lp_set(int setting, int value)
{
    int val = 1;

    mutex_lock(&tuner_mtx);

    switch(setting)
    {
    case RADIO_SLEEP:
        set_sleep(value);
        break;

    case RADIO_FREQUENCY:
        set_frequency(value);
        break;

    case RADIO_SCAN_FREQUENCY:
        /* TODO: really implement this */
        set_frequency(value);
        val = lp24020lp_tuned();
        break;

    case RADIO_MUTE:
        if (value)
            lv24020lp_write_and(RADIO_CTRL3, ~AMUTE_L);
        else
            lv24020lp_write_or(RADIO_CTRL3, AMUTE_L);
        break;

    case RADIO_REGION:
        if (lv24020lp_region_data[value])
            lv24020lp_write_or(AUDIO_CTRL2, DEEMP);
        else
            lv24020lp_write_and(AUDIO_CTRL2, ~DEEMP);
        break;

    case RADIO_FORCE_MONO:
        if (value)
            lv24020lp_write_or(STEREO_CTRL, ST_M);
        else
            lv24020lp_write_and(STEREO_CTRL, ~ST_M);
        break;

    default:
        value = -1;
    }

    mutex_unlock(&tuner_mtx);

    return val;
}

int lv24020lp_get(int setting)
{
    int val = -1;

    mutex_lock(&tuner_mtx);

    switch(setting)
    {
    case RADIO_TUNED:
        /* TODO: really implement this */
        val = lp24020lp_tuned();
        break;

    case RADIO_STEREO:
        val = (lv24020lp_read(RADIO_STAT) & RSS_MS) != 0;
        break;

    case RADIO_PRESENT:
    {
        bool fmstatus = true;

        if (!(tuner_status & TUNER_PRESENCE_CHECKED))
            fmstatus = tuner_power(true);

        val = (tuner_status & TUNER_PRESENT) != 0;

        if (!fmstatus)
            tuner_power(false);
        break;
        }

    default:
        val = lv24020lp_debug_info(setting);
    }

    mutex_unlock(&tuner_mtx);

    return val;
}
#endif /* BOOTLOADER */
