
/* Ripped off from Game_Music_Emu 0.5.2. http://www.slack.net/~ant/ */
/* Inflate code taken from WikiViewer plugin by Adam Gashlin */

#include <codecs/lib/codeclib.h>

#include "libgme/blargg_endian.h"
#include "libgme/vgm_emu.h" 
#include "libgme/inflate/mallocer.h"
#include "libgme/inflate/inflate.h"

CODEC_HEADER

/* Maximum number of bytes to process in one iteration */
#define CHUNK_SIZE (1024*4)
#define MAINMEMBUF 0

static int16_t samples[CHUNK_SIZE] IBSS_ATTR;
static struct Vgm_Emu vgm_emu;

static void *inflatebuf;              /* heap for gunzip */
static char *songbuf;               /* destination for uncompressed song */
static uint32_t songbuflen=0;  /* size of the song buffer */
static uint32_t songlen=0;       /* used size of the song buffer */

static void codec_vgz_update_length(void)
{
    ci->id3->length = Track_get_length( &vgm_emu );
    ci->id3->tail_trim = 4 * 1000;

    if (vgm_emu.info.loop_length <= 0)
        ci->id3->tail_trim = 0;

    ci->id3->length += ci->id3->tail_trim;
}

static void codec_update_fade(void)
{
    /* for REPEAT_ONE we disable track limits */
    Track_set_fade(&vgm_emu,
                   ci->loop_track() ? indefinite_count :
                        ci->id3->length - ci->id3->tail_trim,
                   ci->id3->tail_trim);
}

static void codec_update_elapsed(void)
{
    ci->set_elapsed(ci->loop_track() ? 0 : Track_tell(&vgm_emu));
}

/****************** rockbox interface ******************/

/* this is the codec entry point */
enum codec_status codec_main(enum codec_entry_call_reason reason)
{
    if (reason == CODEC_LOAD) {
        /* we only render 16 bits */
        ci->configure(DSP_SET_SAMPLE_DEPTH, 16);

        /* 44 Khz, Interleaved stereo */
        ci->configure(DSP_SET_FREQUENCY, 44100);
        ci->configure(DSP_SET_STEREO_MODE, STEREO_INTERLEAVED);

        Vgm_init(&vgm_emu);
        Vgm_set_sample_rate(&vgm_emu, 44100);
    }

    return CODEC_OK;
}

/* this is called for each file to process */
enum codec_status codec_run(void)
{
    blargg_err_t err;
    uint8_t *buf;
    size_t n;
    intptr_t param;

    DEBUGF("VGM: next_track\n");
    if (codec_init()) {
        return CODEC_ERROR;
    }

    codec_set_replaygain(ci->id3);

    /* Read the entire file */
    DEBUGF("VGM: request file\n");
    ci->seek_buffer(0);
    buf = ci->request_buffer(&n, ci->filesize);
    if (!buf) {
        DEBUGF("VGM: file load failed\n");
        return CODEC_ERROR;
    }
    
    /* If couldn't get the whole buffer
        will trim file and put and 'end_command'
        at the end*/
    if (n < (size_t)ci->filesize) {
        DEBUGF("VGM: file was trimmed\n");
    }
    
    /* If is gzipped decompress it */
    if ( get_le16( buf ) == 0x8b1f ) {
        wpw_init_mempool(MAINMEMBUF);
        inflatebuf=wpw_malloc(MAINMEMBUF,0x13500);
            
        /* Will use available remaining memory 
            as output buffer */
        songbuflen=wpw_available(MAINMEMBUF);
        songbuf=wpw_malloc(MAINMEMBUF,songbuflen);

        songlen=decompress(buf,n,songbuf,songbuflen,0,inflatebuf);  

        if ((err = Vgm_load_mem(&vgm_emu, songbuf, songlen, true))) {
            DEBUGF("VGM: Vgm_load_mem failed (%s)\n", err);
            return CODEC_ERROR;
        }

        /* Since metadata parser doesn't support VGZ 
             will set song length here */
        codec_vgz_update_length();
    }
    else if ((err = Vgm_load_mem(&vgm_emu, buf, n, false))) {
        DEBUGF("VGM: Vgm_load failed_mem (%s)\n", err);
        return CODEC_ERROR;
    }

    Vgm_start_track(&vgm_emu); 

    if (ci->id3->elapsed != 0)
        Track_seek(&vgm_emu, ci->id3->elapsed);

    codec_update_elapsed();
    codec_update_fade();

    /* The main decoder loop */
    while (1) {
        long action = ci->get_command(&param);

        if (action == CODEC_ACTION_HALT)
            break;

        if (action == CODEC_ACTION_SEEK_TIME) {
            Track_seek(&vgm_emu, param);
            codec_update_elapsed();
            ci->seek_complete();
            
            /* Set fade again in case we seek to start of song */
            codec_update_fade();
        }

        /* Generate audio buffer */
        err = Vgm_play(&vgm_emu, CHUNK_SIZE, samples);
        if (err || Track_ended(&vgm_emu)) break;

        ci->pcmbuf_insert(samples, NULL, CHUNK_SIZE / 2);
        codec_update_elapsed();
    }

    return CODEC_OK;
}
