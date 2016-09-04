/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) Marcin Bukat 2016
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 **************************************************************************/
#include "plugin.h"
#include "lib/configfile.h"
#include "pcm_output.h"
#include "menu.h"
#include "settings.h"
#include "bookmark.h"
#include "libsvox/picoapi.h"
#include "libsvox/picodefs.h"

#define SYNTH_CHUNK 64

#define EV_EXIT 100
#define EV_TTS_RUN 101
#define EV_TTS_PAUSE 102
#define EV_TTS_RESET 103
#define EV_TTS_FINISHED 104
#define EV_TTS_SEEK 105

/* Text Analysis files */
const char * picoInternalTaLingware[] = { "en-US_ta.bin",     "en-GB_ta.bin",     "de-DE_ta.bin",     "es-ES_ta.bin",     "fr-FR_ta.bin",     "it-IT_ta.bin" };

/* Signal Generation files */
const char * picoInternalSgLingware[] = { "en-US_lh0_sg.bin", "en-GB_kh0_sg.bin", "de-DE_gl0_sg.bin", "es-ES_zl0_sg.bin", "fr-FR_nk0_sg.bin", "it-IT_cm0_sg.bin" };

/* Directory with resources */
const char * PICO_LINGWARE_PATH = "/.rockbox/lang_pico/";

/* Arbitrary voice name */
const char * PICO_VOICE_NAME = "V";

static pico_System picoSystem = NULL;
static pico_Engine picoEngine = NULL;
static pico_Resource picoTaResource = NULL;
static pico_Resource picoSgResource = NULL;

static pico_Char picoTaResourceName[PICO_MAX_RESOURCE_NAME_SIZE];
static pico_Char picoSgResourceName[PICO_MAX_RESOURCE_NAME_SIZE];

static pico_Char picoTaFileName[PICO_MAX_DATAPATH_NAME_SIZE + PICO_MAX_FILE_NAME_SIZE];
static pico_Char picoSgFileName[PICO_MAX_DATAPATH_NAME_SIZE + PICO_MAX_FILE_NAME_SIZE];

static struct event_queue tts_queue_rx, tts_queue_tx SHAREDBSS_ATTR;

static size_t mem_pool_size;
static void *mem_pool;
static off_t textlen;

static void resample(struct dsp_config *tts_dsp, const char * inbuf, unsigned int inlen, uint32_t txtchunk)
{
    struct dsp_buffer src;
    struct dsp_buffer dst;

    /* PicoTTS output is 16kHz mono signed 16bit samples */
    src.remcount = inlen / sizeof(int16_t);
    src.pin[0] = inbuf;
    src.pin[1] = NULL;
    src.proc_mask = 0;

    static uint32_t  curtxt = 0;

    if (txtchunk != curtxt)
    {
        txtchunkmap_add(curtxt);
        curtxt = txtchunk;
    }

    while (src.remcount > 0)
    {
        /* wait for available output buffer */
        while((dst.p16out = pcm_output_get_buffer(PCMOUT_CHUNK)) == NULL)
        {
            rb->yield();
        }

        /* samples in output buffer */
        dst.remcount = 0;

        /* out buffer lenght expressed in samples */
        dst.bufcount = PCMOUT_CHUNK / (2 * sizeof(int16_t));

        /* perform DSP processing step */
        rb->dsp_process(tts_dsp, &src, &dst);

        if (dst.remcount > 0)
        {
            /* DSP processing produced some samples
             * commit this chunk to output buffer
             */
            pcm_output_commit_data(dst.remcount * 2 * sizeof(int16_t), txtchunk);
        }
    }
}

static bool synth(struct dsp_config *tts_dsp, uint32_t txtchunk)
{
        bool data_present = false;
        char ALIGNED_ATTR(4) synthbuf[4*SYNTH_CHUNK];
        unsigned int synthbuf_used = 0;

        pico_Status ret;
        pico_Int16 bytes_recv;
        pico_Int16 out_data_type;

        void *outbuf_ptr = synthbuf;
        do
        {
            /* get samples */
            ret = pico_getData(picoEngine, (void *)outbuf_ptr, 2*SYNTH_CHUNK, &bytes_recv, &out_data_type);

            if (bytes_recv)
            {
                data_present = true;
                synthbuf_used += bytes_recv;
                outbuf_ptr += bytes_recv;

                if (synthbuf_used + SYNTH_CHUNK > sizeof(synthbuf))
                {
                    resample(tts_dsp, synthbuf, synthbuf_used, txtchunk);

                    outbuf_ptr = synthbuf;
                    synthbuf_used = 0;

                }
            }
            
            rb->yield();

        } while (PICO_STEP_BUSY == ret);

        resample(tts_dsp, synthbuf, synthbuf_used, txtchunk);

        return data_present;
}

static void tts_thread(void)
{
    bool run = true;
    struct queue_event ev;
    int text_remaining = textlen;
    pico_Char *input = (pico_Char *)mem_pool;
    pico_Int16 bytes_sent;

    static struct dsp_config *tts_dsp;

#ifdef HAVE_PRIORITY_SCHEDULING
    /* Up the priority since the core DSP over-yields internally */
    int old_priority = rb->thread_set_priority(rb->thread_self(),
                                               PRIORITY_PLAYBACK-4);
#endif

    tts_dsp = rb->dsp_get_config(CODEC_IDX_AUDIO);

    rb->dsp_configure(tts_dsp, DSP_RESET, 0);
    rb->dsp_configure(tts_dsp, DSP_FLUSH, 0);
    rb->dsp_configure(tts_dsp, DSP_SET_FREQUENCY, 16000);
    rb->dsp_configure(tts_dsp, DSP_SET_STEREO_MODE, STEREO_MONO);
    rb->dsp_configure(tts_dsp, DSP_SET_SAMPLE_DEPTH, 16);

    unsigned speed = PITCH_SPEED_PRECISION * get_speed();
    unsigned pitch = PITCH_SPEED_PRECISION * get_pitch();

    rb->sound_set_pitch(pitch);
    rb->dsp_set_timestretch(GET_STRETCH(pitch, speed));

    uint32_t txtchunk = 0;
    while (text_remaining > 0)
    {
            rb->queue_wait_w_tmo(&tts_queue_rx, &ev, 1);
            if (ev.id == EV_EXIT)
            {
                break;
            }
            else if (ev.id == EV_TTS_PAUSE)
            {
                run = false;
            }
            else if (ev.id == EV_TTS_RUN)
            {
                run = true;
            }
            else if (ev.id == EV_TTS_RESET)
            {
                /* This flushes pcmbuffer and restart
                 * synth from the current txtchunk
                 */
                txtchunk = get_txtchunk();
                input = (pico_Char *)mem_pool + txtchunk;
                text_remaining = textlen - txtchunk;

                pcm_output_flush();
            }
            else if (ev.id == EV_TTS_SEEK)
            {
                /* This flushes pcmbuffer and set
                 * txtchunk to the provided value
                 */
                if ((ev.data < textlen) && (ev.data >= 0))
                {
                    txtchunk = ev.data;
                    input = (pico_Char *)mem_pool + txtchunk;
                    text_remaining = textlen - txtchunk;
                    pcm_output_flush();
                }
            }

            if (pcm_output_bytes_used() < (ssize_t)PCMOUT_HIGH_WM)
            {
                run = true;
#ifdef HAVE_SCHEDULER_BOOSTCTRL
                rb->trigger_cpu_boost();
#endif
            }

            if (run)
            {
                if (pcm_output_bytes_used() >= pcm_limit_wm())
                {
                    run = false;
#ifdef HAVE_SCHEDULER_BOOSTCTRL
                    rb->cancel_cpu_boost();
#endif
                }
                int txtchunk_size = MIN(text_remaining, 10);

                /* put text into TTS engine input buffer */
                pico_putTextUtf8(picoEngine, input, txtchunk_size, &bytes_sent);

                text_remaining -= bytes_sent;
                input += bytes_sent;

                bool out = synth(tts_dsp, txtchunk);

                if (out)
                    txtchunk = textlen - text_remaining;
            }

            rb->yield();
    }

#ifdef HAVE_PRIORITY_SCHEDULING
    rb->thread_set_priority(rb->thread_self(), old_priority);
#endif
    rb->queue_post(&tts_queue_tx, EV_TTS_FINISHED, 0);
}

static pico_Status load_voice(int langIndex)
{
    pico_Status ret;

    rb->strcpy((char *) picoTaFileName, PICO_LINGWARE_PATH);
    rb->strcat((char *) picoTaFileName, (const char *) picoInternalTaLingware[langIndex]);

    rb->strcpy((char *) picoSgFileName, PICO_LINGWARE_PATH);
    rb->strcat((char *) picoSgFileName, (const char *) picoInternalSgLingware[langIndex]);

    pico_releaseVoiceDefinition(picoSystem, (pico_Char *)PICO_VOICE_NAME);

    if (picoTaResource)
    {
        pico_unloadResource(picoSystem, &picoTaResource);
        picoTaResource = NULL;
    }

    if (picoSgResource)
    {
        pico_unloadResource(picoSystem, &picoSgResource);
        picoSgResource = NULL;
    }

    ret = pico_loadResource(picoSystem, picoTaFileName, &picoTaResource);
    if (PICO_OK != ret)
    {
        rb->splashf(HZ*3, "loading %s failed", picoTaFileName);
        return ret;
    }

    /* get unique resource name */
    ret = pico_getResourceName(picoSystem, picoTaResource, (char *)picoTaResourceName);
    if (PICO_OK != ret)
    {
        rb->splashf(3*HZ, "getResourceName for %s failed", picoTaFileName);
        return ret;
    }

    ret = pico_loadResource(picoSystem, picoSgFileName, &picoSgResource);
    if (PICO_OK != ret)
    {
        rb->splashf(HZ*3, "loading %s failed", picoSgFileName);
        return ret;
    }

    ret = pico_getResourceName(picoSystem, picoSgResource, (char *)picoSgResourceName);
    if (PICO_OK != ret)
    {
        rb->splashf(3*HZ, "getResourceName for %s failed", picoSgFileName);
        return ret;
    }

    ret = pico_createVoiceDefinition(picoSystem, (const pico_Char *)PICO_VOICE_NAME);
    if (PICO_OK != ret)
    {
        rb->splash(3*HZ, "pico_createVoiceDefinition() failed");
        return ret;
    }

    ret = pico_addResourceToVoiceDefinition(picoSystem, (const pico_Char *)PICO_VOICE_NAME, picoTaResourceName);
    if (PICO_OK != ret)
    {
        rb->splash(3*HZ, "Adding Ta to voice failed");
        return ret;
    }

    ret = pico_addResourceToVoiceDefinition( picoSystem, (const pico_Char *)PICO_VOICE_NAME, picoSgResourceName);
    if (PICO_OK != ret)
    {
        rb->splash(3*HZ, "Adding Sg to voice failed");
        return ret;
    }

    return ret;
}

static void change_volume(int delta)
{
    int minvol = rb->sound_min(SOUND_VOLUME);
    int maxvol = rb->sound_max(SOUND_VOLUME);
    int vol = rb->global_settings->volume + delta;

    if (vol > maxvol)
    {
        vol = maxvol;
    }
    else if (vol < minvol)
    {
        vol = minvol;
    }

    if (vol != rb->global_settings->volume)
    {
        rb->sound_set(SOUND_VOLUME, vol);
        rb->global_settings->volume = vol;
    }
}

static bool tts_engine_init(void *pico_pool, size_t pico_pool_size, int langIndex)
{
    pico_Status ret;

    ret = pico_initialize(pico_pool,
                          pico_pool_size,
                          &picoSystem);
    if (PICO_OK != ret)
    {
        rb->splash(HZ*3, "pico_initilize() failed");
        return false;
    }

    /* voice index */
    ret = load_voice(langIndex);
    if (PICO_OK != ret)
    {
        return false;
    }

    /* create new TTS engine instance */
    ret = pico_newEngine(picoSystem,
                         (const pico_Char *)PICO_VOICE_NAME,
                         &picoEngine);
    if (PICO_OK != ret)
    {
        rb->splash(3*HZ, "pico_newEngine() failed");
        return false;
    }

    return true;
}

static void tts_engine_deinit(void)
{
    if (picoEngine)
    {
        pico_disposeEngine(picoSystem, &picoEngine);
        pico_releaseVoiceDefinition(picoSystem, (pico_Char *)PICO_VOICE_NAME);
        picoEngine = NULL;
    }

    if (picoTaResource)
    {
        pico_unloadResource(picoSystem, &picoTaResource);
        picoTaResource = NULL;
    }

    if (picoSgResource)
    {
        pico_unloadResource(picoSystem, &picoSgResource);
        picoSgResource = NULL;
    }

    if (picoSystem)
    {
        pico_terminate(&picoSystem);
        picoSystem = NULL;
    }
}

/* memory layout:
 * 1) text
 * 2) pico buffer 3M
 * 3) pcm buffer
 */
enum plugin_status plugin_start(const void* file)
{
    bool is_playing = true;

    /* grab audio buffer */
    mem_pool = rb->plugin_get_audio_buffer(&mem_pool_size);

    int fd = rb->open(file, O_RDONLY);
    if (fd < 0)
    {
        rb->splashf(HZ*3, "Can't open %s", (char *)file);
        return PLUGIN_ERROR;
    }

    textlen = rb->filesize(fd);
    rb->read(fd, mem_pool, textlen);
    rb->close(fd);

    /* Add null byte at the end as a marker for TTS engine
     * to flush all output at the very end
     */
    ((char *)mem_pool)[textlen] = '\0';
    textlen += 1;

    void *pico_pool = ALIGN_UP(((char *)mem_pool + textlen), 4);
    size_t pico_pool_size = 3 * 1024 * 1024; /* 3MB */

    void *pcmbuf_pool = ALIGN_UP(((char *)pico_pool + pico_pool_size), 4);
    size_t pcmbuf_size = (char *)mem_pool + mem_pool_size - (char *)pcmbuf_pool;

    pcm_output_init(pcmbuf_pool, &pcmbuf_size);

    /* load settings */
    load_settings();

    /* initialize TTS engine */
    tts_engine_init(pico_pool, pico_pool_size, get_lang());

    /* initialize queues for communicating with TTS engine */
    rb->queue_init(&tts_queue_rx, false);
    rb->queue_init(&tts_queue_tx, false);

    /* steal codec thread for tts synthesis */
    unsigned int thread_id;
    rb->codec_thread_do_callback(tts_thread, &thread_id);

    /* calculate crc of file requested to be processed */
    uint32_t crc = rb->crc_32(mem_pool, textlen, 0);
    bookmark_init(crc);

    /* set start position from bookmark if any */
    rb->queue_post(&tts_queue_rx, EV_TTS_SEEK, load_bookmark(file));

    rb->lcd_clear_display();

    /* start playback */
    pcm_output_play_pause(true);

    struct font *pf = rb->font_get(FONT_UI);
    int font_h = pf->height;

    bool quit = false;
    while(!quit)
    {
        uint32_t txtchunk = get_txtchunk();
        rb->lcd_clear_display();
        rb->lcd_putsxyf(0, 0, "%s, %d/%d (%d%%)", (char *)file, (int)txtchunk, (int)textlen, (int)(100*txtchunk/textlen));
        rb->lcd_putsxyf(0, 2*font_h, "pcm_buf_used: %d", (int)pcm_output_bytes_used());
        rb->lcd_putsxyf(0, 3*font_h, "speed: %d%%", get_speed());
        rb->lcd_putsxyf(0, 4*font_h, "pitch: %d%%", get_pitch());
        rb->lcd_update();

        /* prevent poweroff timer from kicking in */
        rb->reset_poweroff_timer();

        int button = rb->get_action(CONTEXT_WPS, HZ/10);
        switch(button)
        {
            case ACTION_WPS_VOLDOWN:
            {
                change_volume(-1);
                break;
            }

            case ACTION_WPS_VOLUP:
            {
                change_volume(1);
                break;
            }

            case ACTION_WPS_PLAY:
            {
                pcm_output_play_pause(!is_playing);
                is_playing = !is_playing;
                break;
            }

            case ACTION_WPS_SKIPPREV:
            {
                uint32_t prev = txtchunk_prev(txtchunk);
                if (prev < txtchunk)
                {
                    rb->queue_post(&tts_queue_rx, EV_TTS_SEEK, prev);
                }
                break;
            }

            case ACTION_WPS_SKIPNEXT:
            {
                uint32_t next = txtchunk_next(txtchunk);
                if (next > txtchunk)
                {
                    rb->queue_post(&tts_queue_rx, EV_TTS_SEEK, next);
                }
                break;
            }

            case ACTION_WPS_STOP:
            {
                /* instruct TTS engine to finish */
                rb->queue_post(&tts_queue_rx, EV_EXIT, 0);
                break;
            }

            case ACTION_WPS_MENU:
            {
                int res = display_menu();

                switch(res)
                {
                    case PICOTTS_EXIT_PLUGIN:
                    {
                        rb->queue_post(&tts_queue_rx, EV_EXIT, 0);
                        break;
                    }

                    case PICOTTS_TTS_SETTING_CHANGE:
                    {
                        rb->queue_post(&tts_queue_rx, EV_TTS_PAUSE, 0);
                        rb->sleep(HZ/10);
                        tts_engine_deinit();
                        tts_engine_init(pico_pool, pico_pool_size, get_lang());
                        rb->queue_post(&tts_queue_rx, EV_TTS_RESET, 0);
                        rb->queue_post(&tts_queue_rx, EV_TTS_RUN, 0);
                        break;
                    }
                    case PICOTTS_SPEED_SETTING_CHANGE:
                    {
                        unsigned speed = PITCH_SPEED_PRECISION * get_speed();
                        unsigned pitch = PITCH_SPEED_PRECISION * get_pitch();

                        rb->dsp_set_timestretch(GET_STRETCH(pitch, speed));
                        rb->queue_post(&tts_queue_rx, EV_TTS_RESET, 0);
                        break;
                    }
                    case PICOTTS_PITCH_SETTING_CHANGE:
                    {
                        unsigned pitch = PITCH_SPEED_PRECISION * get_pitch();

                        rb->sound_set_pitch(pitch);
                        rb->queue_post(&tts_queue_rx, EV_TTS_RESET, 0);
                        break;
                    }
                }
                break;
            }

            case ACTION_WPS_BROWSE:
            {
                dump_txtchunkmap();
                save_bookmark(file, txtchunk);
            }
        }

        struct queue_event ev;
        rb->queue_wait_w_tmo(&tts_queue_tx, &ev, TIMEOUT_NOBLOCK);
        if (ev.id == EV_TTS_FINISHED)
            quit = true;
    }

    pcm_output_stop();

    /* remove callback from codec thread */
    rb->codec_thread_do_callback(NULL, NULL);

    tts_engine_deinit();
    pcm_output_exit();
    save_settings();

    return PLUGIN_OK;
}
