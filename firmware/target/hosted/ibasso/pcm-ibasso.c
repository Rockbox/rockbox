/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 by Ilia Sergachev: Initial Rockbox port to iBasso DX50
 * Copyright (C) 2014 by Mario Basister: iBasso DX90 port
 * Copyright (C) 2014 by Simon Rothen: Initial Rockbox repository submission, additional features
 * Copyright (C) 2014 by Udo Schläpfer: Code clean up, additional features
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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "config.h"
#include "debug.h"
#include "panic.h"
#include "pcm.h"
#include "pcm-internal.h"
#include "tick.h"

#include "sound/asound.h"
#include "tinyalsa/asoundlib.h"

#include "debug-ibasso.h"


/* Tiny alsa handle. */
struct pcm* _alsa_handle = NULL;


/*
    Number of frames to copy to alsa which each write.

    This is the size of each period in Hz. Not bytes, but Hz! If the period size is configured to
    for example 32, each write should contain exactly 32 frames of sound data, and each read will
    return either 32 frames of data or nothing at all.

    Tick task and threads share the same timer. Sheduler hast to wait on tick tasks. So setting
    this to high will interrupt all processing inside Rockbox.

    On the other hand, setting this to low will underrun the alsa output buffer.

    To sustain 16-bit stereo @ 44.1 KHz analog rate the system must be capable of a data transfer
    rate, in Bytes/sec:
    Bps_rate = 2 channels * 2 bytes/sample * 44100 samples/sec = 2 * 2 * 44100 = 176400 Bytes/sec

    Thus, if we set 16-bit stereo @ 44.1Khz, and the period_size to 512 frames => 2048 bytes => an
    interrupt will be generated each 2048 bytes, each ~12ms.

    So pcm_tick() must provide ALSA with data inside the time frame of 12ms.
*/
static const snd_pcm_sframes_t PERIOD_SIZE = 512;


/* Bytes left in the Rockbox PCM frame buffer. */
static size_t _pcm_buffer_size = 0;


/* Rockbox PCM frame buffer. */
static const void  *_pcm_buffer = NULL;


/* Internal PCM frame buffer. */
static char *_frame_buffer = NULL;


/* Copy PCM frames from Rockbox PCM frame buffer to internal PCM frame buffer. */
static bool fill_frames( void )
{
    /*TRACE;*/

    bool new_buffer = false;

    size_t frames_left = PERIOD_SIZE;
    while( frames_left > 0 )
    {
        if( _pcm_buffer_size <= 0 )
        {
            /* Retrive a new PCM buffer from Rockbox. */
            new_buffer = pcm_play_dma_complete_callback( PCM_DMAST_OK, &_pcm_buffer, &_pcm_buffer_size );
            if( ! new_buffer )
            {
                return false;
            }
        }

        /* Number of bytes to copy. */
        size_t copy_now = MIN( _pcm_buffer_size, pcm_frames_to_bytes( _alsa_handle, frames_left ) );

        /* Copy Rockbox PCM buffer to internal PCM frame buffer. */
        memcpy( &_frame_buffer[pcm_frames_to_bytes( _alsa_handle, PERIOD_SIZE - frames_left )], _pcm_buffer, copy_now );

        /* Adjust Rockbox PCM buffer. */
        _pcm_buffer += copy_now;

        /* Adjust Rockbox PCM buffer size. */
        _pcm_buffer_size -= copy_now;

        /* Adjust frames left for this go. */
        frames_left -= pcm_bytes_to_frames( _alsa_handle, copy_now );

        if( new_buffer )
        {
            /* Still alive and kicking. */
            new_buffer = false;
            pcm_play_dma_status_callback( PCM_DMAST_STARTED );
        }
    }

    return true;
}


static void pcm_tick( void )
{
    /*
        TODO: Make this a thread. Possibly asynchronous. Possibly mmaped.
        Or ... can we mmap the Rockbox PCM frame buffer directly to ALSA?
    */

    /*TRACE;*/

    if( fill_frames() )
    {
        if( pcm_write( _alsa_handle, _frame_buffer, pcm_frames_to_bytes( _alsa_handle, PERIOD_SIZE ) ) != 0 )
        {
            DEBUGF( "ERROR %s: pcm_write failed: %s.", __func__, pcm_get_error( _alsa_handle ) );
            panicf( "ERROR %s: pcm_write failed: %s.", __func__, pcm_get_error( _alsa_handle ) );
        }
    }
}


#ifdef DEBUG

/* https://github.com/tinyalsa/tinyalsa/blob/master/tinypcminfo.c */

static const char* format_lookup[] =
{
    /*[0] =*/ "S8",
    "U8",
    "S16_LE",
    "S16_BE",
    "U16_LE",
    "U16_BE",
    "S24_LE",
    "S24_BE",
    "U24_LE",
    "U24_BE",
    "S32_LE",
    "S32_BE",
    "U32_LE",
    "U32_BE",
    "FLOAT_LE",
    "FLOAT_BE",
    "FLOAT64_LE",
    "FLOAT64_BE",
    "IEC958_SUBFRAME_LE",
    "IEC958_SUBFRAME_BE",
    "MU_LAW",
    "A_LAW",
    "IMA_ADPCM",
    "MPEG",
    /*[24] =*/ "GSM",
    [31] = "SPECIAL",
    "S24_3LE",
    "S24_3BE",
    "U24_3LE",
    "U24_3BE",
    "S20_3LE",
    "S20_3BE",
    "U20_3LE",
    "U20_3BE",
    "S18_3LE",
    "S18_3BE",
    "U18_3LE",
    /*[43] =*/ "U18_3BE"
};


const char* pcm_get_format_name( unsigned int bit_index )
{
    return( bit_index < 43 ? format_lookup[bit_index] : NULL );
}

#endif


/* ALSA card and device. */
static const unsigned int CARD   = 0;
static const unsigned int DEVICE = 0;


void pcm_play_dma_init( void )
{
    TRACE;

#ifdef DEBUG

    /*
        DEBUG pcm_play_dma_init: Access: 0x000009
        DEBUG pcm_play_dma_init: Format[0]: 0x000044
        DEBUG pcm_play_dma_init: Format[1]: 0x000010
        DEBUG pcm_play_dma_init: Format: S16_LE
        DEBUG pcm_play_dma_init: Format: S24_LE
        DEBUG pcm_play_dma_init: Format: S20_3LE
        DEBUG pcm_play_dma_init: Subformat: 0x000001
        DEBUG pcm_play_dma_init: Rate: min = 8000Hz, max = 192000Hz
        DEBUG pcm_play_dma_init: Channels: min = 2, max = 2
        DEBUG pcm_play_dma_init: Sample bits: min=16, max=32
        DEBUG pcm_play_dma_init: Period size: min=8, max=10922
        DEBUG pcm_play_dma_init: Period count: min=3, max=128
    */

    struct pcm_params* params = pcm_params_get( CARD, DEVICE, PCM_OUT );
    if( params == NULL )
    {
        DEBUGF( "ERROR %s: Card/device does not exist.", __func__ );
        panicf( "ERROR %s: Card/device does not exist.", __func__ );
        return;
    }

    struct pcm_mask* m = pcm_params_get_mask( params, PCM_PARAM_ACCESS );
    if( m )
    {
        DEBUGF( "DEBUG %s: Access: %#08x", __func__, m->bits[0] );
    }

    m = pcm_params_get_mask( params, PCM_PARAM_FORMAT );
    if( m )
    {
        DEBUGF("DEBUG %s: Format[0]: %#08x", __func__, m->bits[0] );
        DEBUGF("DEBUG %s: Format[1]: %#08x", __func__, m->bits[1] );

        unsigned int j;
        unsigned int k;
        const unsigned int bitcount = sizeof( m->bits[0] ) * 8;
        for( k = 0; k < 2; ++k )
        {
            for( j = 0; j < bitcount; ++j )
            {
                const char* name;
                if( m->bits[k] & ( 1 << j ) )
                {
                    name = pcm_get_format_name( j + ( k * bitcount ) );
                    if( name )
                    {
                        DEBUGF("DEBUG %s: Format: %s", __func__, name );
                    }
                }
            }
        }
    }

    m = pcm_params_get_mask( params, PCM_PARAM_SUBFORMAT );
    if( m )
    {
        DEBUGF( "DEBUG %s: Subformat: %#08x", __func__, m->bits[0] );
    }

    unsigned int min = pcm_params_get_min( params, PCM_PARAM_RATE );
    unsigned int max = pcm_params_get_max( params, PCM_PARAM_RATE) ;
    DEBUGF( "DEBUG %s: Rate: min = %uHz, max = %uHz", __func__, min, max );

    min = pcm_params_get_min( params, PCM_PARAM_CHANNELS );
    max = pcm_params_get_max( params, PCM_PARAM_CHANNELS );
    DEBUGF( "DEBUG %s: Channels: min = %u, max = %u", __func__, min, max );

    min = pcm_params_get_min( params, PCM_PARAM_SAMPLE_BITS );
    max = pcm_params_get_max( params, PCM_PARAM_SAMPLE_BITS );
    DEBUGF( "DEBUG %s: Sample bits: min=%u, max=%u", __func__, min, max);

    min = pcm_params_get_min( params, PCM_PARAM_PERIOD_SIZE );
    max = pcm_params_get_max( params, PCM_PARAM_PERIOD_SIZE );
    DEBUGF( "DEBUG %s: Period size: min=%u, max=%u", __func__, min, max );

    min = pcm_params_get_min( params, PCM_PARAM_PERIODS );
    max = pcm_params_get_max( params, PCM_PARAM_PERIODS );
    DEBUGF( "DEBUG %s: Period count: min=%u, max=%u", __func__, min, max );

    pcm_params_free( params );

#endif

    if( ( _alsa_handle != NULL ) || ( _frame_buffer != NULL ) )
    {
        DEBUGF( "ERROR %s: Allready initialized.", __func__ );
        panicf( "ERROR %s: Allready initialized.", __func__ );
        return;
    }

    /* Rockbox outputs 16 Bit/44.1kHz only. */
    struct pcm_config config;
    config.channels          = 2;
    config.rate              = 44100;
    config.period_size       = PERIOD_SIZE;
    config.period_count      = 4;
    config.format            = PCM_FORMAT_S16_LE;
    config.start_threshold   = 0;
    config.stop_threshold    = 0;
    config.silence_threshold = 0;

    _alsa_handle = pcm_open( CARD, DEVICE, PCM_OUT, &config );
    if( ! pcm_is_ready( _alsa_handle ) )
    {
        DEBUGF( "ERROR %s: pcm_open failed: %s.", __func__, pcm_get_error( _alsa_handle ) );
        panicf( "ERROR %s: pcm_open failed: %s.", __func__, pcm_get_error( _alsa_handle ) );
        return;
    }

    /*
        _frame_buffer size = config.period_count * config.period_size * config.channels * ( 16 \ 8 )
                           = 4 * 512 * 2 * 2
                           = 8192
    */
    _frame_buffer = malloc( pcm_frames_to_bytes( _alsa_handle, pcm_get_buffer_size( _alsa_handle ) ) );
    if( _frame_buffer == NULL )
    {
        DEBUGF( "ERROR %s: malloc for _frame_buffer failed.", __func__ );
        panicf( "ERROR %s: malloc for _frame_buffer failed.", __func__ );
        return;
    }
}


void pcm_play_dma_postinit( void )
{
    TRACE;
}


/* Prevent excessive tick usage. */
static bool _play_lock = true;


void pcm_play_dma_start( const void *addr, size_t size )
{
    TRACE;

    /*
        TODO: Check this.
        Headphone output relay: if this is done at startup already, a loud click is audible on
        headphones. Here, some time later, the output caps are charged a bit and the click is much
        softer.
    */
    if( system( NULL ) )
    {
        /*
            DX50
            Mute:   echo 'A' > /sys/class/codec/mute 
            Unmute: echo 'B' > /sys/class/codec/mute

            DX90?
        */
        system( "/system/bin/muteopen" );
    }
    else
    {
        DEBUGF( "ERROR %s: No command processor available.", __func__ );
    }

    _pcm_buffer = addr;
    _pcm_buffer_size = size;

    _play_lock = false;
}


/* TODO: Why is this in the API if it gets never called? */
void pcm_play_dma_pause( bool pause )
{
    TRACE;

    (void)pause;
}


/* Track nested play locks/unlocks. */
int _play_lock_recursion_lock = 0;


void pcm_play_dma_stop( void )
{
    TRACE;

    _play_lock = true;
    tick_remove_task( pcm_tick );
    _play_lock_recursion_lock = 0;

    pcm_stop( _alsa_handle );
}


void pcm_play_lock( void )
{
    ++_play_lock_recursion_lock;

    tick_remove_task( pcm_tick );

    DEBUGF( "DEBUG %s: _play_lock_recursion_lock: %d.", __func__, _play_lock_recursion_lock );
}


void pcm_play_unlock( void )
{
    --_play_lock_recursion_lock;

    if( ( ! _play_lock ) && ( _play_lock_recursion_lock == 0 ) )
    {
        tick_add_task( pcm_tick );
    }

    DEBUGF( "DEBUG %s: _play_lock: %s, _play_lock_recursion_lock: %d.", __func__, _play_lock ? "true" : "false", _play_lock_recursion_lock );
}


void pcm_dma_apply_settings( void )
{
    DEBUGF( "DEBUG %s: Last set sample rate: %u.", __func__, pcm_get_frequency() );
}


size_t pcm_get_bytes_waiting( void )
{
    TRACE;

    return _pcm_buffer_size;
}


/* TODO: WTF */
const void* pcm_play_dma_get_peak_buffer( int* count )
{
    TRACE;

    uintptr_t addr = (uintptr_t) _pcm_buffer;
    *count = _pcm_buffer_size / 4 /* config.period_count? */;
    return (void*) ( ( addr + 3 ) & ~3 );
}


void pcm_close_device( void )
{
    TRACE;

    _play_lock = true;
    tick_remove_task( pcm_tick );

    pcm_close( _alsa_handle );
    _alsa_handle = NULL;

    free( _frame_buffer );
    _frame_buffer = NULL;
}
