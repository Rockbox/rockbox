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
#include "keydefs.h"
#include "pcm_output.h"
#include "libsvox/picoapi.h"
#include "libsvox/picodefs.h"

#define EV_EXIT 100

const char * picoSupportedCountryIso3[] = { "USA", "GBR", "DEU", "ESP", "FRA", "ITA" };
const char * picoInternalTaLingware[] = { "en-US_ta.bin",     "en-GB_ta.bin",     "de-DE_ta.bin",     "es-ES_ta.bin",     "fr-FR_ta.bin",     "it-IT_ta.bin" };
const char * picoInternalSgLingware[] = { "en-US_lh0_sg.bin", "en-GB_kh0_sg.bin", "de-DE_gl0_sg.bin", "es-ES_zl0_sg.bin", "fr-FR_nk0_sg.bin", "it-IT_cm0_sg.bin" };
const char * PICO_LINGWARE_PATH = "/.rockbox/lang_pico/";
const char * PICO_VOICE_NAME = "Voice";

static pico_System picoSystem = NULL;
static pico_Engine picoEngine = NULL;
static pico_Resource picoTaResource = NULL;
static pico_Resource picoSgResource = NULL;

static pico_Char picoTaResourceName[PICO_MAX_RESOURCE_NAME_SIZE];
static pico_Char picoSgResourceName[PICO_MAX_RESOURCE_NAME_SIZE];

static pico_Char picoTaFileName[PICO_MAX_DATAPATH_NAME_SIZE + PICO_MAX_FILE_NAME_SIZE];
static pico_Char picoSgFileName[PICO_MAX_DATAPATH_NAME_SIZE + PICO_MAX_FILE_NAME_SIZE];

static struct dsp_config *dsp;
static struct event_queue tts_queue SHAREDBSS_ATTR;

static size_t mem_pool_size;
static void *mem_pool;
static off_t filelen;

struct wavinfo_t
{
  int fd;
  int samplerate;
  int channels;
  int sampledepth;
  int stereomode;
  int totalsamples;
} wavinfo;

static unsigned char wav_header[44] =
{
    'R','I','F','F',     //  0 - ChunkID
     0,0,0,0,            //  4 - ChunkSize (filesize-8)
     'W','A','V','E',    //  8 - Format
     'f','m','t',' ',    // 12 - SubChunkID
     16,0,0,0,           // 16 - SubChunk1ID  // 16 for PCM
     1,0,                // 20 - AudioFormat (1=16-bit)
     0,0,                // 22 - NumChannels
     0,0,0,0,            // 24 - SampleRate in Hz
     0,0,0,0,            // 28 - Byte Rate (SampleRate*NumChannels*(BitsPerSample/8)
     0,0,                // 32 - BlockAlign (== NumChannels * BitsPerSample/8)
     16,0,               // 34 - BitsPerSample
     'd','a','t','a',    // 36 - Subchunk2ID
     0,0,0,0             // 40 - Subchunk2Size
};

static inline void int2le32(unsigned char* buf, int32_t x)
{
  buf[0] = (x & 0xff);
  buf[1] = (x & 0xff00) >> 8;
  buf[2] = (x & 0xff0000) >> 16;
  buf[3] = (x & 0xff000000) >>24;
}

static inline void int2le16(unsigned char* buf, int16_t x)
{
  buf[0] = (x & 0xff);
  buf[1] = (x & 0xff00) >> 8;
}

void init_wav(char* filename)
{
    wavinfo.samplerate = rb->dsp_configure(dsp, DSP_GET_OUT_FREQUENCY, 0);
    wavinfo.channels = 2;
    wavinfo.sampledepth = 16;
    wavinfo.stereomode = STEREO_INTERLEAVED; //STEREO_MONO;
    wavinfo.totalsamples = 0;

    wavinfo.fd = rb->creat(filename, 0666);

    if (wavinfo.fd >= 0)
    {
        /* Write WAV header - we go back and fill in the details at the end */
        rb->write(wavinfo.fd, wav_header, sizeof(wav_header));
    }
}

void close_wav(void)
{
    int filesize = rb->filesize(wavinfo.fd);
    int channels = (wavinfo.stereomode == STEREO_MONO) ? 1 : 2;
    int bps = 16; /* TODO */

    /* We assume 16-bit, Stereo */

    rb->lseek(wavinfo.fd,0,SEEK_SET);

    int2le32(wav_header+4, filesize-8); /* ChunkSize */
    int2le16(wav_header+22, channels);
    int2le32(wav_header+24, wavinfo.samplerate);
    int2le32(wav_header+28, wavinfo.samplerate * channels * (bps / 8)); /* ByteRate */
    int2le16(wav_header+32, channels * (bps / 8));
    int2le32(wav_header+40, filesize - 44);  /* Subchunk2Size */

    rb->write(wavinfo.fd, wav_header, sizeof(wav_header));
    rb->close(wavinfo.fd);
}

static pico_Status synth(void)
{

    struct dsp_buffer src;
    struct dsp_buffer dst;
        static char ALIGNED_ATTR(4) synthbuf[2*1024];
        unsigned int synthbuf_used = 0;

        pico_Status ret;
        pico_Int16 bytes_recv;
        pico_Int16 out_data_type;

DEBUGF("synth() src 0x%lx dst 0x%lx\n", (unsigned int)&src, (unsigned int)&dst);
        void *outbuf_ptr = synthbuf;
        do
        {
            /* get samples */
            ret = pico_getData(picoEngine, (void *)outbuf_ptr, 128, &bytes_recv, &out_data_type);
DEBUGF("pico_getData()\n");
            if (bytes_recv)
            {
                synthbuf_used += bytes_recv;
DEBUGF("synthbuf_used: %d\n", synthbuf_used);
                outbuf_ptr += bytes_recv;
                if (synthbuf_used + 128 > sizeof(synthbuf))
                {
DEBUGF("pico out buffer ready\n");

                    ssize_t sz = synthbuf_used*3*2;
                    void *buf = NULL;
                    while (true)
                    {
                        buf = pcm_output_get_buffer(&sz);
                        if (buf)
                        {
DEBUGF("pcm_output_get_buffer: 0x%lx 0x%lx\n", (unsigned int)buf, sz);
                            break;
                        }
                        rb->yield();
                        DEBUGF("pcm_output_get_buffer() == NULL\n");
                    }
                    /* picotts output is 16bit mono PCM 16kHz samplerate */
                    src.remcount = synthbuf_used >> 1; /* Samples in buffer */
                    src.pin[0] = synthbuf;             /* Data pointer(s) */
                    src.pin[1] = NULL;                 /* Input is MONO */
                    src.proc_mask = 0;                 /* Set to zero on new
                                                        * buffer before passing
                                                        * to DSP. */

                    dst.remcount = 0;                  /* Samples in buffer */
                    dst.p16out = buf;          /* DSP output buffer */
                    dst.bufcount = sz >> 2;   /* Buffer length/dest
                                                        * buffer remaining
                                                        * (samples)
                                                        */

                    int old_remcount;
                    do
                    {
                        old_remcount = dst.remcount;
DEBUGF("dsp_process(%p, %p, %p) enter\n", (void *)dsp, (void *)&src, (void *)&dst);
                        rb->dsp_process(dsp, &src, &dst);
DEBUGF("dsp_process() quit\n");
                    } while (dst.bufcount <= 0 ||
                             (src.remcount <= 0 &&
                                 dst.remcount <= old_remcount));

                    /* number of bytes ready in output buffer
                     * which is passed to DMA.
                     * DSP engine returns 16bit stereo interleaved
                     */
                     pcm_output_commit_data(dst.remcount << 2);
DEBUGF("pcm_output_commit_data(%d)\n", synthbuf_used);
//DEBUGF("%d: synth() %d\n", *(rb->current_tick), buf->bufused);
//rb->write(wavinfo.fd, buf->buffer, buf->bufused);
                    DEBUGF("synth() quit BUSY\n");
                    return PICO_STEP_BUSY;
                }
            }

        } while (PICO_STEP_BUSY == ret);

        DEBUGF("synth() quit IDLE\n");
        return PICO_STEP_IDLE;
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


static void tts_thread(void)
{

    struct queue_event ev;
    int text_remaining = filelen;
    pico_Char *input = (pico_Char *)mem_pool;
    pico_Int16 bytes_sent;
    pico_Status ret;

    while (text_remaining)
    {
        /* put text into TTS engine input buffer */
        pico_putTextUtf8(picoEngine, input, text_remaining, &bytes_sent);
        text_remaining -= bytes_sent;
        input += bytes_sent;

        while(true)
        {
            rb->queue_wait_w_tmo(&tts_queue, &ev, TIMEOUT_NOBLOCK);
            if (ev.id == EV_EXIT)
                return;

            if (pcm_output_bytes_used() >= PCMOUT_HIGH_WM)
            {
#ifdef HAVE_SCHEDULER_BOOSTCTRL
                rb->cancel_cpu_boost();
#endif
                rb->yield();
            }
            else
            {
#ifdef HAVE_SCHEDULER_BOOSTCTRL
                if (pcm_output_bytes_used() <= PCMOUT_LOW_WM)
                {
                    rb->trigger_cpu_boost();
                }
#endif
                ret = synth();
                if (ret != PICO_STEP_BUSY)
                {
DEBUGF("synth() ret=%d\n", ret);
                    break;
                }
            }

        }
    }
}

enum plugin_status plugin_start(const void* file)
{
    pico_Status ret;
    bool is_playing = true;

    /* grab audio buffer */
    mem_pool = rb->plugin_get_audio_buffer(&mem_pool_size);
DEBUGF("plugin_get_audio_buffer: %p 0x%0x\n", mem_pool, mem_pool_size);

    int fd = rb->open(file, O_RDONLY);
    if (fd < 0)
    {
        rb->splashf(HZ*3, "Can't open %s", (char *)file);
        return PLUGIN_ERROR;
    }

    filelen = rb->filesize(fd);
    rb->read(fd, mem_pool, filelen);
    rb->close(fd);

DEBUGF("File %s len=0x%x (%d)\n", file, filelen, filelen);

    void *pcmbuf_pool = ALIGN_UP(((char *)mem_pool + filelen), 4);
    size_t pcmbuf_size = PCMOUT_BUFSIZE;
    pcm_output_init(pcmbuf_pool, &pcmbuf_size);

DEBUGF("PCM out buffer %p 0x%x\n", pcmbuf_pool, pcmbuf_size);

    void *pico_pool = ALIGN_UP(((char *)pcmbuf_pool + pcmbuf_size), 4);
    size_t pico_pool_size = (char *)mem_pool + mem_pool_size - (char *)pico_pool;

DEBUGF("pico_initialize %p 0x%x\n", pico_pool, pico_pool_size);
    /* initialize TTS engine */
    ret = pico_initialize(pico_pool,
                          pico_pool_size,
                          &picoSystem);
    if (PICO_OK != ret)
    {
        rb->splash(HZ*3, "pico_initilize() failed");
        return PLUGIN_ERROR;
    }

    /* voice index */
    ret = load_voice(1);
    if (PICO_OK != ret)
    {
        return PLUGIN_ERROR;
    }

    /* create new TTS engine instance */
    ret = pico_newEngine(picoSystem,
                         (const pico_Char *)PICO_VOICE_NAME,
                         &picoEngine);
    if (PICO_OK != ret)
    {
        rb->splash(3*HZ, "pico_newEngine() failed");
        return PLUGIN_ERROR;
    }

    dsp = rb->dsp_get_config(CODEC_IDX_AUDIO);
    rb->dsp_configure(dsp, DSP_RESET, 0);
    rb->dsp_configure(dsp, DSP_FLUSH, 0);
    rb->dsp_configure(dsp, DSP_SET_FREQUENCY, 16000);
    rb->dsp_configure(dsp, DSP_SET_STEREO_MODE, STEREO_MONO);
    rb->dsp_configure(dsp, DSP_SET_SAMPLE_DEPTH, 16);

    unsigned int thread_id;

    rb->queue_init(&tts_queue, false);

    /* steal codec thread for tts synthesis */
    rb->codec_thread_do_callback(tts_thread, &thread_id);

    /* start playback */
    pcm_output_play_pause(true);

//    init_wav("/tts.wav");

    while(pcm_output_bytes_used() > PCMOUT_EMPTY_WM)
    {
        int button = rb->button_get_w_tmo(HZ/20);
        switch(button)
        {
            case BTN_STOP:
                /* instruct TTS thread to finish */
                rb->queue_post(&tts_queue, EV_EXIT, 0);
                /* remove callback from codec thread */
                rb->codec_thread_do_callback(NULL, NULL);

                /* drain pcm buffer */
                while(pcm_output_bytes_used())
                    ;
                break;

            case BTN_VOLDOWN:
                change_volume(-1);
                break;

            case BTN_VOLUP:
                change_volume(1);
                break;

            case BTN_PAUSE:
                pcm_output_play_pause(!is_playing);
                is_playing = !is_playing;
                break;

            case BTN_RW:
                /* TODO */
                break;

            case BTN_FF:
                /* TODO */
                break;

            case BTN_MENU:
                /* TODO */
                break;
        }
    }

    pcm_output_stop();
//    close_wav();

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

    return PLUGIN_OK;
}
