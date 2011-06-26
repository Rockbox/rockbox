
/* Ripped off from Game_Music_Emu 0.5.2. http://www.slack.net/~ant/ */

#include <codecs/lib/codeclib.h>
#include "libgme/sgc_emu.h" 

CODEC_HEADER

/* Maximum number of bytes to process in one iteration */
#define CHUNK_SIZE (1024*2)

static int16_t samples[CHUNK_SIZE] IBSS_ATTR;
static struct Sgc_Emu sgc_emu;

/* Coleco Bios */
/* Colecovision not supported yet
static char coleco_bios[0x2000];
*/

/****************** rockbox interface ******************/

static void set_codec_track(int t) {
    Sgc_start_track(&sgc_emu, t); 

    /* for REPEAT_ONE we disable track limits */
    if (!ci->loop_track()) {
        Track_set_fade(&sgc_emu, Track_get_length( &sgc_emu, t ), 4000);
    }
    ci->set_elapsed(t*1000); /* t is track no to display */
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

        Sgc_init(&sgc_emu);
        Sgc_set_sample_rate(&sgc_emu, 44100);

        /* set coleco bios, should be named coleco_bios.rom */
        /* Colecovision not supported yet
        int fd = ci->open("/coleco_bios.rom", O_RDONLY);
        if ( fd >= 0 ) {
            ci->read(fd, coleco_bios, 0x2000);
            ci->close(fd);
            set_coleco_bios( &sgc_emu, coleco_bios );
        }
        */
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
    int track = 0;

    DEBUGF("SGC: next_track\n");
    if (codec_init()) {
        return CODEC_ERROR;
    }

    codec_set_replaygain(ci->id3);

    /* Read the entire file */
    DEBUGF("SGC: request file\n");
    ci->seek_buffer(0);
    buf = ci->request_buffer(&n, ci->filesize);
    if (!buf || n < (size_t)ci->filesize) {
        DEBUGF("SGC: file load failed\n");
        return CODEC_ERROR;
    }
   
    if ((err = Sgc_load_mem(&sgc_emu, buf, ci->filesize))) {
        DEBUGF("SGC: Sgc_load_mem failed (%s)\n", err);
        return CODEC_ERROR;
    }

    /* Update internal track count */
    if (sgc_emu.m3u.size > 0)
        sgc_emu.track_count = sgc_emu.m3u.size;

next_track:
    set_codec_track(track);

    /* The main decoder loop */
    while (1) {
        enum codec_command_action action = ci->get_command(&param);

        if (action == CODEC_ACTION_HALT)
            break;

        if (action == CODEC_ACTION_SEEK_TIME) {
            track = param/1000;
            ci->seek_complete();
            if (track >= sgc_emu.track_count) break;
            goto next_track;
        }

        /* Generate audio buffer */
        err = Sgc_play(&sgc_emu, CHUNK_SIZE, samples);
        if (err || Track_ended(&sgc_emu)) {
            track++;
            if (track >= sgc_emu.track_count) break;
            goto next_track;
        }

        ci->pcmbuf_insert(samples, NULL, CHUNK_SIZE >> 1);
    }

    return CODEC_OK;
}
