
/* Ripped off from Game_Music_Emu 0.5.2. http://www.slack.net/~ant/ */

#define GME_NSF_TYPE

#include <codecs/lib/codeclib.h>
#include "libgme/nsf_emu.h" 

CODEC_HEADER

/* Maximum number of bytes to process in one iteration */
#define CHUNK_SIZE (1024*2)

static int16_t samples[CHUNK_SIZE] IBSS_ATTR;
static struct Nsf_Emu nsf_emu;

/****************** rockbox interface ******************/

static void set_codec_track(int t, int multitrack) {
    Nsf_start_track(&nsf_emu, t); 

    /* for REPEAT_ONE we disable track limits */
    if (!ci->loop_track()) {
        Track_set_fade(&nsf_emu, Track_length( &nsf_emu, t ) - 4000, 4000);
    }
    if (multitrack) ci->set_elapsed(t*1000); /* t is track no to display */
    else ci->set_elapsed(0);
}

/* this is the codec entry point */
enum codec_status codec_main(enum codec_entry_call_reason reason)
{
    if (reason == CODEC_LOAD) {
        /* we only render 16 bits */
        ci->configure(DSP_SET_SAMPLE_DEPTH, 16);

        /* 44 Khz, Interleaved stereo */
        ci->configure(DSP_SET_FREQUENCY, 44100);
        ci->configure(DSP_SET_STEREO_MODE, STEREO_INTERLEAVED);

        Nsf_init(&nsf_emu);
        Nsf_set_sample_rate(&nsf_emu, 44100);
    }

    return CODEC_OK;
}

/* this is called for each file to process */
enum codec_status codec_run(void)
{
    blargg_err_t err;
    uint8_t *buf;
    size_t n;
    int track, is_multitrack;
    uint32_t elapsed_time;
    intptr_t param;

    track = is_multitrack = 0;
    elapsed_time = 0;

    DEBUGF("NSF: next_track\n");
    if (codec_init()) {
        return CODEC_ERROR;
    }  

    codec_set_replaygain(ci->id3);
        
    /* Read the entire file */
    DEBUGF("NSF: request file\n");
    ci->seek_buffer(0);
    buf = ci->request_buffer(&n, ci->filesize);
    if (!buf || n < (size_t)ci->filesize) {
        DEBUGF("NSF: file load failed\n");
        return CODEC_ERROR;
    }
   
    if ((err = Nsf_load_mem(&nsf_emu, buf, ci->filesize))) {
        DEBUGF("NSF: Nsf_load_mem failed (%s)\n", err);
        return CODEC_ERROR;
    }

    /* Update internal track count */
    if (nsf_emu.m3u.size > 0)
        nsf_emu.track_count = nsf_emu.m3u.size;

    if (nsf_emu.track_count > 1) is_multitrack = 1;

next_track:
    set_codec_track(track, is_multitrack);

    /* The main decoder loop */
    while (1) {
        enum codec_command_action action = ci->get_command(&param);

        if (action == CODEC_ACTION_HALT)
            break;

        if (action == CODEC_ACTION_SEEK_TIME) {
            if (is_multitrack) {
                track = param/1000;
                ci->seek_complete();
                if (track >= nsf_emu.track_count) break;
                goto next_track;
            }

            ci->set_elapsed(param);
            elapsed_time = param;
            Track_seek(&nsf_emu, param);
            ci->seek_complete();
            
            /* Set fade again */
            if (!ci->loop_track()) {
                Track_set_fade(&nsf_emu, Track_length( &nsf_emu, track ), 4000);
            }
        }

        /* Generate audio buffer */
        err = Nsf_play(&nsf_emu, CHUNK_SIZE, samples);
        if (err || Track_ended(&nsf_emu)) {
            track++;
            if (track >= nsf_emu.track_count) break;
            goto next_track;
        }

        ci->pcmbuf_insert(samples, NULL, CHUNK_SIZE >> 1);

        /* Set elapsed time for one track files */
        if (is_multitrack == 0) {
            elapsed_time += (CHUNK_SIZE / 2) * 10 / 441;
            ci->set_elapsed(elapsed_time);
        }
    }

    return CODEC_OK;
}
