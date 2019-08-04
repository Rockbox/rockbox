/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Code that has been in mpeg.c before, now creating an encapsulated play
 * data module, to be used by other sources than file playback as well.
 *
 * Copyright (C) 2004 by Linus Nielsen Feltzing
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
#include <stdbool.h>
#include "config.h"
#include "debug.h"
#include "panic.h"
#include <kernel.h>
#include "mp3_playback.h"
#include "sound.h"
#include "i2c.h"
#include "system.h"
#include "audiohw.h"

/* hacking into mpeg.c, recording is still there */
#if CONFIG_CODEC == MAS3587F
enum
{
    MPEG_DECODER,
    MPEG_ENCODER
} mpeg_mode;
#endif /* #ifdef MAS3587F */

#if ((CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)) && !defined(SIMULATOR)
extern unsigned long shadow_io_control_main;
extern unsigned shadow_codec_reg0;
#endif

/**** globals ****/

/* own version, independent of mpeg.c */
static bool paused; /* playback is paused */
static bool playing; /* We are playing an MP3 stream */

/* the registered callback function to ask for more mp3 data */
static mp3_play_callback_t callback_for_more;

/* list of tracks in memory */
#define MAX_ID3_TAGS (1<<4) /* Must be power of 2 */
#define MAX_ID3_TAGS_MASK (MAX_ID3_TAGS - 1)

bool audio_is_initialized = false;

/* FIX: this code pretty much assumes a MAS */

/* dirty calls to mpeg.c */
extern void playback_tick(void);
extern void rec_tick(void);

unsigned long mas_version_code;

#if CONFIG_CODEC == MAS3507D
static void mas_poll_start(void)
{
    unsigned int count;

    count = 9 * FREQ / 10000 / 8; /* 0.9 ms */

    /* We are using timer 1 */
    
    TSTR &= ~0x02; /* Stop the timer */
    TSNC &= ~0x02; /* No synchronization */
    TMDR &= ~0x02; /* Operate normally */

    TCNT1 = 0;   /* Start counting at 0 */
    GRA1 = count;
    TCR1 = 0x23; /* Clear at GRA match, sysclock/8 */

    /* Enable interrupt on level 5 */
    IPRC = (IPRC & ~0x000f) | 0x0005;
    
    TSR1 &= ~0x02;
    TIER1 = 0xf9; /* Enable GRA match interrupt */

    TSTR |= 0x02; /* Start timer 1 */
}
#elif CONFIG_CODEC != SWCODEC
static void postpone_dma_tick(void)
{
    unsigned int count;

    count = 8 * FREQ / 10000 / 8; /* 0.8 ms */

    /* We are using timer 1 */
    
    TSTR &= ~0x02; /* Stop the timer */
    TSNC &= ~0x02; /* No synchronization */
    TMDR &= ~0x02; /* Operate normally */

    TCNT1 = 0;   /* Start counting at 0 */
    GRA1 = count;
    TCR1 = 0x23; /* Clear at GRA match, sysclock/8 */

    /* Enable interrupt on level 5 */
    IPRC = (IPRC & ~0x000f) | 0x0005;
    
    TSR1 &= ~0x02;
    TIER1 = 0xf9; /* Enable GRA match interrupt */

    TSTR |= 0x02; /* Start timer 1 */
}
#endif

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
void demand_irq_enable(bool on)
{
    int oldlevel = disable_irq_save();
    
    if(on)
    {
        IPRA = (IPRA & 0xfff0) | 0x000b;
        ICR &= ~0x0010; /* IRQ3 level sensitive */
    }
    else
        IPRA &= 0xfff0;

    restore_irq(oldlevel);
}
#endif /* #if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F) */


static void play_tick(void)
{
    if(playing && !paused)
    {
        /* Start DMA if it is disabled and the DEMAND pin is high */
        if(!(SCR0 & 0x80) && (PBDR & 0x4000))
        {
            SCR0 |= 0x80;
        }

        playback_tick(); /* dirty call to mpeg.c */
    }
}

void DEI3(void) __attribute__((interrupt_handler));
void DEI3(void)
{
    const void* start;
    size_t size = 0;

    if (callback_for_more != NULL)
    {
        callback_for_more(&start, &size);
    }
    
    if (size > 0)
    {
        DTCR3 = size & 0xffff;
        SAR3 = (unsigned int) start;
    }
    else
    {
        CHCR3 &= ~0x0001; /* Disable the DMA interrupt */
    }

    CHCR3 &= ~0x0002; /* Clear DMA interrupt */
}

void IMIA1(void) __attribute__((interrupt_handler));
void IMIA1(void) /* Timer 1 interrupt */
{
    if(playing)
        play_tick();
    TSR1 &= ~0x01;
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    /* Disable interrupt */
    IPRC &= ~0x000f;
#endif
}

void IRQ6(void) __attribute__((interrupt_handler));
void IRQ6(void) /* PB14: MAS stop demand IRQ */
{
    SCR0 &= ~0x80;
}

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
void IRQ3(void) __attribute__((interrupt_handler));
void IRQ3(void) /* PA15: MAS demand IRQ */
{
    /* Begin with setting the IRQ to edge sensitive */
    ICR |= 0x0010;

#if CONFIG_CODEC == MAS3587F
    if(mpeg_mode == MPEG_ENCODER)
        rec_tick();
    else
#endif
        postpone_dma_tick();

    /* Workaround for sh-elf-gcc 3.3.x bug with -O2 or -Os and ISRs
     * (invalid cross-jump optimisation) */
    asm volatile ("");
}
#endif /* #if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F) */

static void setup_sci0(void)
{
    /* PB15 is I/O, PB14 is IRQ6, PB12 is SCK0, PB9 is TxD0 */
    PBCR1 = (PBCR1 & 0x0cff) | 0x1208;
    
    /* Set PB12 to output */
    or_b(0x10, &PBIORH);

    /* Disable serial port */
    SCR0 = 0x00;

    /* Synchronous, no prescale */
    SMR0 = 0x80;

    /* Set baudrate 1Mbit/s */
    BRR0 = 0x02;

    /* use SCK as serial clock output */
    SCR0 = 0x01;

    /* Clear FER and PER */
    SSR0 &= 0xe7;

    /* Set interrupt ITU2 and SCI0 priority to 0 */
    IPRD &= 0x0ff0;

    /* set PB15 and PB14 to inputs */
     and_b(~0x80, &PBIORH);
     and_b(~0x40, &PBIORH);

    /* Enable End of DMA interrupt at prio 8 */
    IPRC = (IPRC & 0xf0ff) | 0x0800;
    
    /* Enable Tx (only!) */
    SCR0 |= 0x20;
}

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
static void init_playback(void)
{
    unsigned long val;
    int rc;

    mp3_play_pause(false);
    
    mas_reset();

    /* Enable the audio CODEC and the DSP core, max analog voltage range */
    rc = mas_direct_config_write(MAS_CONTROL, 0x8c00);
    if(rc < 0)
        panicf("mas_ctrl_w: %d", rc);

    /* Stop the current application */
    val = 0;
    mas_writemem(MAS_BANK_D0, MAS_D0_APP_SELECT, &val, 1);
    do
    {
        mas_readmem(MAS_BANK_D0, MAS_D0_APP_RUNNING, &val, 1);
    } while(val);
    
    /* Enable the D/A Converter */
    shadow_codec_reg0 = 0x0001;
    mas_codec_writereg(0x0, shadow_codec_reg0);

    /* ADC scale 0%, DSP scale 100% */
    mas_codec_writereg(6, 0x0000);
    mas_codec_writereg(7, 0x4000);

#ifdef HAVE_SPDIF_OUT
    val = 0x09; /* Disable SDO and SDI, low impedance S/PDIF outputs */
#else
    val = 0x2d; /* Disable SDO and SDI, disable S/PDIF output */
#endif
    mas_writemem(MAS_BANK_D0, MAS_D0_INTERFACE_CONTROL, &val, 1);

    /* Set Demand mode and validate all settings */
    shadow_io_control_main = 0x25;
    mas_writemem(MAS_BANK_D0, MAS_D0_IO_CONTROL_MAIN, &shadow_io_control_main, 1);

    /* Start the Layer2/3 decoder applications */
    val = 0x0c;
    mas_writemem(MAS_BANK_D0, MAS_D0_APP_SELECT, &val, 1);
    do
    {
        mas_readmem(MAS_BANK_D0, MAS_D0_APP_RUNNING, &val, 1);
    } while((val & 0x0c) != 0x0c);

#if CONFIG_CODEC == MAS3587F
    mpeg_mode = MPEG_DECODER;
#endif

    /* set IRQ6 to edge detect */
    ICR |= 0x02;

    /* set IRQ6 prio 8 */
    IPRB = ( IPRB & 0xff0f ) | 0x0080;

    DEBUGF("MAS Decoding application started\n");
}
#endif /* #if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F) */

void mp3_init(int volume, int bass, int treble, int balance, int loudness,
              int avc, int channel_config, int stereo_width, bool swap_channels,
              int mdb_strength, int mdb_harmonics,
              int mdb_center, int mdb_shape, bool mdb_enable,
              bool superbass)
{
#if CONFIG_CODEC == MAS3507D
    unsigned long val;
    (void)loudness;
    (void)avc;
    (void)mdb_strength;
    (void)mdb_harmonics;
    (void)mdb_center;
    (void)mdb_shape;
    (void)mdb_enable;
    (void)superbass;
#endif

    setup_sci0();

#ifdef HAVE_MAS_SIBI_CONTROL
    and_b(~0x01, &PBDRH); /* drive SIBI low */
    or_b(0x01, &PBIORH); /* output for PB8 */
#endif

#if CONFIG_CODEC == MAS3507D
    mas_reset();
#elif CONFIG_CODEC == MAS3587F
    or_b(0x08, &PAIORH); /* output for /PR */
    init_playback();
    
    mas_version_code = mas_readver();
    DEBUGF("MAS3587 derivate %d, version %c%d\n",
           (mas_version_code & 0xf000) >> 12,
           'A' + ((mas_version_code & 0x0f00) >> 8), mas_version_code & 0xff);
#elif CONFIG_CODEC == MAS3539F
    or_b(0x08, &PAIORH); /* output for /PR */
    init_playback();
    
    mas_version_code = mas_readver();
    DEBUGF("MAS3539 derivate %d, version %c%d\n",
           (mas_version_code & 0xf000) >> 12,
           'A' + ((mas_version_code & 0x0f00) >> 8), mas_version_code & 0xff);
#endif

#ifdef HAVE_DAC3550A
    dac_init();
#endif
    
#if CONFIG_CODEC == MAS3507D
    /* set IRQ6 to edge detect */
    ICR |= 0x02;

    /* set IRQ6 prio 8 */
    IPRB = ( IPRB & 0xff0f ) | 0x0080;

    mas_readmem(MAS_BANK_D1, 0xff7, &mas_version_code, 1);
    
    mas_writereg(0x3b, 0x20); /* Don't ask why. The data sheet doesn't say */
    mas_run(1);
    sleep(HZ);

    /* Clear the upper 12 bits of the 32-bit samples */
    mas_writereg(0xc5, 0);
    mas_writereg(0xc6, 0);
    
    /* We need to set the PLL for a 14.31818MHz crystal */
    if(mas_version_code == 0x0601) /* Version F10? */
    {
        val = 0x5d9d0;
        mas_writemem(MAS_BANK_D0, 0x32d, &val, 1);
        val = 0xfffceceb;
        mas_writemem(MAS_BANK_D0, 0x32e, &val, 1);
        val = 0x0;
        mas_writemem(MAS_BANK_D0, 0x32f, &val, 1);
        mas_run(0x475);
    }
    else
    {
        val = 0x5d9d0;
        mas_writemem(MAS_BANK_D0, 0x36d, &val, 1);
        val = 0xfffceceb;
        mas_writemem(MAS_BANK_D0, 0x36e, &val, 1);
        val = 0x0;
        mas_writemem(MAS_BANK_D0, 0x36f, &val, 1);
        mas_run(0xfcb);
    }
    
#endif

#if CONFIG_CODEC == MAS3507D
    mas_poll_start();

    mas_writereg(MAS_REG_KPRESCALE, 0xe9400);
    dac_enable(true);
#endif

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    ICR &= ~0x0010; /* IRQ3 level sensitive */
    PACR1 = (PACR1 & 0x3fff) | 0x4000; /* PA15 is IRQ3 */
#endif

    /* Must be done before calling sound_set() */
    audio_is_initialized = true;

    sound_set(SOUND_BASS, bass);
    sound_set(SOUND_TREBLE, treble);
    sound_set(SOUND_BALANCE, balance);
    sound_set(SOUND_VOLUME, volume);
    sound_set(SOUND_CHANNELS, channel_config);
    sound_set(SOUND_STEREO_WIDTH, stereo_width);
    sound_set(SOUND_SWAP_CHANNELS, swap_channels);
    
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    sound_set(SOUND_LOUDNESS, loudness);
    sound_set(SOUND_AVC, avc);
    sound_set(SOUND_MDB_STRENGTH, mdb_strength);
    sound_set(SOUND_MDB_HARMONICS, mdb_harmonics);
    sound_set(SOUND_MDB_CENTER, mdb_center);
    sound_set(SOUND_MDB_SHAPE, mdb_shape);
    sound_set(SOUND_MDB_ENABLE, mdb_enable);
    sound_set(SOUND_SUPERBASS, superbass);
#endif

    playing = false;
    paused = true;
}

void mp3_shutdown(void)
{
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    unsigned long val = 1;
    mas_writemem(MAS_BANK_D0, MAS_D0_SOFT_MUTE, &val, 1); /* Mute */
#endif

#if CONFIG_CODEC == MAS3507D
    dac_volume(0, 0, false);
#endif
}

/* new functions, to be exported to plugin API */

#if CONFIG_CODEC == MAS3587F
void mp3_play_init(void)
{
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    init_playback();
#endif
    playing = false;
    paused = true;
    callback_for_more = NULL;
}
#endif

void mp3_play_data(const void* start, size_t size,
                   mp3_play_callback_t get_more)
{
    /* init DMA */
    DAR3 = 0x5FFFEC3;
    CHCR3 &= ~0x0002; /* Clear interrupt */
    CHCR3 = 0x1504; /* Single address destination, TXI0, IE=1 */
    DMAOR = 0x0001; /* Enable DMA */
    
    callback_for_more = get_more;

    SAR3 = (unsigned int)start;
    DTCR3 = size & 0xffff;

    playing = true;
    paused = true;

    CHCR3 |= 0x0001; /* Enable DMA IRQ */

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    demand_irq_enable(true);
#endif
}

void mp3_play_pause(bool play)
{
    if (paused && play)
    {   /* resume playback */
        SCR0 |= 0x80;
        paused = false;
    }
    else if (!paused && !play)
    {   /* stop playback */
        SCR0 &= 0x7f;
        paused = true;
    }
}     

bool mp3_pause_done(void)
{
    unsigned long frame_count;

    if (!paused)
        return false;
    
    mas_readmem(MAS_BANK_D0, MAS_D0_MPEG_FRAME_COUNT, &frame_count, 1);
    /* This works because the frame counter never wraps,
     * i.e. zero always means lost sync. */
    return frame_count == 0;
}

void mp3_play_stop(void)
{
    playing = false;
    mp3_play_pause(false);
    CHCR3 &= ~0x0001; /* Disable the DMA interrupt */
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    demand_irq_enable(false);
#endif
}

bool mp3_is_playing(void)
{
    return playing;
}


/* returns the next byte position which would be transferred */
unsigned char* mp3_get_pos(void)
{
    return (unsigned char*)SAR3;
}
