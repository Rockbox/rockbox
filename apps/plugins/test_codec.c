/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Dave Chapman
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
#include "plugin.h"
#include "lib/pluginlib_touchscreen.h"
#include "lib/pluginlib_exit.h"
#include "lib/pluginlib_actions.h"

/* this set the context to use with PLA */
static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };

#define TESTCODEC_EXITBUTTON  PLA_EXIT
#define TESTCODEC_EXITBUTTON2 PLA_CANCEL


#ifdef HAVE_ADJUSTABLE_CPU_FREQ
static unsigned int boost =1;

static const struct opt_items boost_settings[2] = {
    { "No",    -1 },
    { "Yes",   -1 },
};
#endif

/* Log functions copied from test_disk.c */
static int line = 0;
static int max_line = 0;
static int log_fd = -1;

static void log_close(void)
{
    if (log_fd >= 0)
        rb->close(log_fd);
}

static bool log_init(bool use_logfile)
{
    int h;
    char logfilename[MAX_PATH];

    rb->lcd_getstringsize("A", NULL, &h);
    max_line = LCD_HEIGHT / h;
    line = 0;
    rb->lcd_clear_display();
    rb->lcd_update();

    if (use_logfile) {
        log_close();
        rb->create_numbered_filename(logfilename, HOME_DIR, "test_codec_log_", ".txt",
                                     2 IF_CNFN_NUM_(, NULL));
        log_fd = rb->open(logfilename, O_RDWR|O_CREAT|O_TRUNC, 0666);
        return log_fd >= 0;
    }

    return true;
}

static void log_text(char *text, bool advance)
{
    rb->lcd_puts(0, line, text);
    rb->lcd_update();
    if (advance)
    {
        if (++line >= max_line)
            line = 0;
        if (log_fd >= 0)
            rb->fdprintf(log_fd, "%s\n", text);
    }
}

struct wavinfo_t
{
  int fd;
  int samplerate;
  int channels;
  int sampledepth;
  int stereomode;
  int totalsamples;
};

static void* audiobuf;
static void* codec_mallocbuf;
static size_t audiosize;
static size_t audiobufsize;
static int offset;
static int fd;

/* Our local implementation of the codec API */
static struct codec_api ci;

struct test_track_info {
    struct mp3entry id3;       /* TAG metadata */
    size_t filesize;           /* File total length */
};

static struct test_track_info track;

static bool use_dsp;

static bool checksum;
static uint32_t crc32;

static volatile unsigned int elapsed;
static volatile bool codec_playing;
static volatile enum codec_command_action codec_action;
static volatile long endtick;
static volatile long rebuffertick;
struct wavinfo_t wavinfo;

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

static inline void int2le24(unsigned char* buf, int32_t x)
{
  buf[0] = (x & 0xff);
  buf[1] = (x & 0xff00) >> 8;
  buf[2] = (x & 0xff0000) >> 16;
}

static inline void int2le16(unsigned char* buf, int16_t x)
{
  buf[0] = (x & 0xff);
  buf[1] = (x & 0xff00) >> 8;
}

static unsigned char *wavbuffer;
static unsigned char *dspbuffer;
static int dspbuffer_count;

void init_wav(char* filename)
{
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

/* Returns buffer to malloc array. Only codeclib should need this. */
static void* codec_get_buffer(size_t *size)
{
   *size = CODEC_SIZE;
   return codec_mallocbuf;
}

static int process_dsp(const void *ch1, const void *ch2, int count)
{
    struct dsp_buffer src;
    src.remcount = count;
    src.pin[0] = ch1;
    src.pin[1] = ch2;
    src.proc_mask = 0;

    struct dsp_buffer dst;
    dst.remcount = 0;
    dst.p16out = (int16_t *)dspbuffer;
    dst.bufcount = dspbuffer_count;

    while (1)
    {
        int old_remcount = dst.remcount;
        rb->dsp_process(ci.dsp, &src, &dst);
        
        if (dst.bufcount <= 0 ||
            (src.remcount <= 0 && dst.remcount <= old_remcount))
        {
            /* Dest is full or no input left and DSP purged */
            break;
        }
    }
    
    return dst.remcount;
}

/* Null output */
static void pcmbuf_insert_null(const void *ch1, const void *ch2, int count)
{
    if (use_dsp)
        process_dsp(ch1, ch2, count);

    /* Prevent idle poweroff */
    rb->reset_poweroff_timer();
}

/*
 *  Helper function used when the file is larger then the available memory. 
 *  Rebuffers the file by setting the start of the audio buffer to be 
 *  new_offset and filling from there.
 */
static int fill_buffer(int new_offset){
    size_t n, bytestoread;
    long temp = *rb->current_tick;
    rb->lseek(fd,new_offset,SEEK_SET);
    
    if(new_offset + audiobufsize <= track.filesize)
        bytestoread = audiobufsize;
    else
        bytestoread = track.filesize-new_offset;
    
    n = rb->read(fd, audiobuf,bytestoread);

    if (n != bytestoread)
    {
        log_text("Read failed.",true);
        DEBUGF("read fail:  got %d bytes, expected %d\n", (int)n, (int)audiobufsize);
        rb->backlight_on();

        if (fd >= 0)
        {
            rb->close(fd);
        }

        return -1;
    }
    offset = new_offset;
    
    /*keep track of how much time we spent buffering*/
    rebuffertick += *rb->current_tick-temp;
    
    return 0;
}

/* WAV output or calculate crc32 of output*/
static void pcmbuf_insert_wav_checksum(const void *ch1, const void *ch2, int count)
{
    /* Prevent idle poweroff */
    rb->reset_poweroff_timer();

    if (use_dsp) {
        count = process_dsp(ch1, ch2, count);
        wavinfo.totalsamples += count;

#ifdef ROCKBOX_BIG_ENDIAN
        unsigned char* p = dspbuffer;
        int i;
        for (i = 0; i < count; i++) {
            int2le16(p,*(int16_t *)p);
            p += 2;
            int2le16(p,*(int16_t *)p);
            p += 2;
        }
#endif
        if (checksum) {
            crc32 = rb->crc_32(dspbuffer, count * 2 * sizeof (int16_t), crc32);
        } else {
            rb->write(wavinfo.fd, dspbuffer, count * 2 * sizeof (int16_t));
        }
    }
    else
    { 
        const int16_t* data1_16;
        const int16_t* data2_16;
        const int32_t* data1_32;
        const int32_t* data2_32;
        unsigned char* p = wavbuffer;
        const int scale = wavinfo.sampledepth - 15;
        const int dc_bias = 1 << (scale - 1);

        if (wavinfo.sampledepth <= 16) {
            data1_16 = ch1;
            data2_16 = ch2;

            switch(wavinfo.stereomode)
            {
                case STEREO_INTERLEAVED:
                    while (count--) {
                        int2le16(p,*data1_16++);
                        p += 2;
                        int2le16(p,*data1_16++);
                        p += 2;
                    }
                    break;
 
                case STEREO_NONINTERLEAVED:
                    while (count--) {
                        int2le16(p,*data1_16++);
                        p += 2;
                        int2le16(p,*data2_16++);
                        p += 2;
                    }

                    break;

                case STEREO_MONO:
                    while (count--) {
                        int2le16(p,*data1_16++);
                        p += 2;
                    }
                    break;
            }
        } else {
            data1_32 = ch1;
            data2_32 = ch2;

            switch(wavinfo.stereomode)
            {
                case STEREO_INTERLEAVED:
                    while (count--) {
                        int2le16(p, clip_sample_16((*data1_32++ + dc_bias) >> scale));
                        p += 2;
                        int2le16(p, clip_sample_16((*data1_32++ + dc_bias) >> scale));
                        p += 2;
                    }
                    break;
 
                case STEREO_NONINTERLEAVED:
                    while (count--) {
                        int2le16(p, clip_sample_16((*data1_32++ + dc_bias) >> scale));
                        p += 2;
                        int2le16(p, clip_sample_16((*data2_32++ + dc_bias) >> scale));
                        p += 2;
                    }

                    break;

                case STEREO_MONO:
                    while (count--) {
                        int2le16(p, clip_sample_16((*data1_32++ + dc_bias) >> scale));
                        p += 2;
                    }
                    break;
            }
        }

        wavinfo.totalsamples += count;
        if (checksum)
            crc32 = rb->crc_32(wavbuffer, p - wavbuffer, crc32);
        else
            rb->write(wavinfo.fd, wavbuffer, p - wavbuffer);
    } /* else */
}

/* Set song position in WPS (value in ms). */
static void set_elapsed(unsigned long value)
{
    elapsed = value;
    ci.id3->elapsed = value;
}


/* Read next <size> amount bytes from file buffer to <ptr>.
   Will return number of bytes read or 0 if end of file. */
static size_t read_filebuf(void *ptr, size_t size)
{
   if (ci.curpos > (off_t)track.filesize)
   {
       return 0;
   } else {
        size_t realsize = MIN(track.filesize-ci.curpos,size);
        
       /* check if we have enough bytes ready*/
       if(realsize >(audiobufsize - (ci.curpos-offset)))
       {
           /*rebuffer so that we start at ci.curpos*/
           fill_buffer(ci.curpos);
        }
       
       rb->memcpy(ptr, audiobuf + (ci.curpos-offset), realsize);
       ci.curpos += realsize;
       return realsize;
   }
}


/* Request pointer to file buffer which can be used to read
   <realsize> amount of data. <reqsize> tells the buffer system
   how much data it should try to allocate. If <realsize> is 0,
   end of file is reached. */
static void* request_buffer(size_t *realsize, size_t reqsize)
{
    *realsize = MIN(track.filesize-ci.curpos,reqsize);

    /*check if we have enough bytes ready - requested > bufsize-currentbufpos*/
    if(*realsize>(audiobufsize - (ci.curpos-offset)))
    {
        /*rebuffer so that we start at ci.curpos*/
       fill_buffer(ci.curpos);
    }
    
    return (audiobuf + (ci.curpos-offset));
}

/* Advance file buffer position by <amount> amount of bytes. */
static void advance_buffer(size_t amount)
{
    ci.curpos += amount;
    ci.id3->offset = ci.curpos;
}


/* Seek file buffer to position <newpos> beginning of file. */
static bool seek_buffer(size_t newpos)
{
    ci.curpos = newpos;
    return true;
}


/* Codec should call this function when it has done the seeking. */
static void seek_complete(void)
{
    /* Do nothing */
}

/* Codec calls this to know what it should do next. */
static enum codec_command_action get_command(intptr_t *param)
{
    rb->yield();
    return codec_action;
    (void)param;
}

/* Some codecs call this to determine whether they should loop. */
static bool loop_track(void)
{
    return false;
}

static void set_offset(size_t value)
{
    ci.id3->offset = value;
}


/* Configure different codec buffer parameters. */
static void configure(int setting, intptr_t value)
{
    if (use_dsp)
        rb->dsp_configure(ci.dsp, setting, value);
    switch(setting)
    {
        case DSP_SET_FREQUENCY:
            DEBUGF("samplerate=%d\n",(int)value);
            wavinfo.samplerate = use_dsp ? NATIVE_FREQUENCY : (int)value;
            break;

        case DSP_SET_SAMPLE_DEPTH:
            DEBUGF("sampledepth = %d\n",(int)value);
            wavinfo.sampledepth = use_dsp ? 16 : (int)value;
            break;

        case DSP_SET_STEREO_MODE:
            DEBUGF("Stereo mode = %d\n",(int)value);
            wavinfo.stereomode = use_dsp ? STEREO_INTERLEAVED : (int)value;
            break;
    }

}

static void init_ci(void)
{
    /* --- Our "fake" implementations of the codec API functions. --- */

    ci.dsp = rb->dsp_get_config(CODEC_IDX_AUDIO);
    ci.codec_get_buffer = codec_get_buffer;

    if (wavinfo.fd >= 0 || checksum) {
        ci.pcmbuf_insert = pcmbuf_insert_wav_checksum;
    } else {
        ci.pcmbuf_insert = pcmbuf_insert_null;
    }

    ci.set_elapsed = set_elapsed;
    ci.read_filebuf = read_filebuf;
    ci.request_buffer = request_buffer;
    ci.advance_buffer = advance_buffer;
    ci.seek_buffer = seek_buffer;
    ci.seek_complete = seek_complete;
    ci.set_offset = set_offset;
    ci.configure = configure;
    ci.get_command = get_command;
    ci.loop_track = loop_track;

    /* --- "Core" functions --- */

    /* kernel/ system */
    ci.sleep = rb->sleep;
    ci.yield = rb->yield;

    /* strings and memory */
    ci.strcpy = rb->strcpy;
    ci.strlen = rb->strlen;
    ci.strcmp = rb->strcmp;
    ci.strcat = rb->strcat;
    ci.memset = rb->memset;
    ci.memcpy = rb->memcpy;
    ci.memmove = rb->memmove;
    ci.memcmp = rb->memcmp;
    ci.memchr = rb->memchr;
#if defined(DEBUG) || defined(SIMULATOR)
    ci.debugf = rb->debugf;
#endif
#ifdef ROCKBOX_HAS_LOGF
    ci.logf = rb->logf;
#endif

    ci.qsort = rb->qsort;

#ifdef RB_PROFILE
    ci.profile_thread = rb->profile_thread;
    ci.profstop = rb->profstop;
    ci.profile_func_enter = rb->profile_func_enter;
    ci.profile_func_exit = rb->profile_func_exit;
#endif

    ci.commit_dcache = rb->commit_dcache;
    ci.commit_discard_dcache = rb->commit_discard_dcache;
    ci.commit_discard_idcache = rb->commit_discard_idcache;

#if NUM_CORES > 1
    ci.create_thread = rb->create_thread;
    ci.thread_thaw = rb->thread_thaw;
    ci.thread_wait = rb->thread_wait;
    ci.semaphore_init = rb->semaphore_init;
    ci.semaphore_wait = rb->semaphore_wait;
    ci.semaphore_release = rb->semaphore_release;
#endif

#if defined(CPU_ARM) && (CONFIG_PLATFORM & PLATFORM_NATIVE)
    ci.__div0 = rb->__div0;
#endif
}

static void codec_thread(void)
{
    const char* codecname;
    int res;

    codecname = rb->get_codec_filename(track.id3.codectype);

    /* Load the codec */
    res = rb->codec_load_file(codecname, &ci);

    if (res >= 0)
    {
        /* Decode the file */
        res = rb->codec_run_proc();
    }

    /* Clean up */
    rb->codec_close();

    /* Signal to the main thread that we are done */
    endtick = *rb->current_tick - rebuffertick;
    codec_playing = false;
}

static enum plugin_status test_track(const char* filename)
{
    size_t n;
    enum plugin_status res = PLUGIN_ERROR;
    long starttick;
    long ticks;
    unsigned long speed;
    unsigned long duration;
    const char* ch;
    char str[MAX_PATH];
    offset=0;

    /* Display filename (excluding any path)*/
    ch = rb->strrchr(filename, '/');
    if (ch==NULL)
       ch = filename; 
    else
       ch++;

    rb->snprintf(str,sizeof(str),"%s",ch);
    log_text(str,true);

    log_text("Loading...",false);

    fd = rb->open(filename,O_RDONLY);
    if (fd < 0)
    {
        log_text("Cannot open file",true);
        goto exit;
    }

    track.filesize = rb->filesize(fd);

    /* Clear the id3 struct */
    rb->memset(&track.id3, 0, sizeof(struct mp3entry));

    if (!rb->get_metadata(&(track.id3), fd, filename))
    {
        log_text("Cannot read metadata",true);
        goto exit;
    }

    if (track.filesize > audiosize)
    {
        audiobufsize=audiosize;
        
    } else 
    {
        audiobufsize=track.filesize;
    }

    n = rb->read(fd, audiobuf, audiobufsize);

    if (n != audiobufsize)
    {
        log_text("Read failed.",true);
        goto exit;
    }
    

    /* Initialise the function pointers in the codec API */
    init_ci();

    /* Prepare the codec struct for playing the whole file */
    ci.filesize = track.filesize;
    ci.id3 = &track.id3;
    ci.curpos = 0;

    if (use_dsp) {
        rb->dsp_configure(ci.dsp, DSP_RESET, 0);
        rb->dsp_configure(ci.dsp, DSP_FLUSH, 0);
    }

    if (checksum)
        crc32 = 0xffffffff;

    rebuffertick=0;
    starttick = *rb->current_tick;

    codec_playing = true;
    codec_action = CODEC_ACTION_NULL;

    rb->codec_thread_do_callback(codec_thread, NULL);

    /* Wait for codec thread to die */
    while (codec_playing)
    {
        int button = pluginlib_getaction(HZ, plugin_contexts,
                          ARRAYLEN(plugin_contexts));
        if ((button == TESTCODEC_EXITBUTTON) || (button == TESTCODEC_EXITBUTTON2))
        {
            codec_action = CODEC_ACTION_HALT;
            break;
        }

        rb->snprintf(str,sizeof(str),"%d of %d",elapsed,(int)track.id3.length);
        log_text(str,false);
    }
    ticks = endtick - starttick;

    /* Be sure it is done */
    rb->codec_thread_do_callback(NULL, NULL);
    rb->backlight_on();
    log_text(str,true);

    if (codec_action == CODEC_ACTION_HALT)
    {
        /* User aborted test */
    }    
    else if (checksum)
    {
        rb->snprintf(str, sizeof(str), "CRC32 - %08x", (unsigned)crc32);
        log_text(str,true);
    }
    else if (wavinfo.fd < 0) 
    {
        /* Display benchmark information */
        rb->snprintf(str,sizeof(str),"Decode time - %d.%02ds",(int)ticks/100,(int)ticks%100);
        log_text(str,true);

        duration = track.id3.length / 10;
        rb->snprintf(str,sizeof(str),"File duration - %d.%02ds",(int)duration/100,(int)duration%100);
        log_text(str,true);

        if (ticks > 0)
            speed = duration * 10000 / ticks;
        else
            speed = 0;

        rb->snprintf(str,sizeof(str),"%d.%02d%% realtime",(int)speed/100,(int)speed%100);
        log_text(str,true);
        
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
        /* show effective clockrate in MHz needed for realtime decoding */
        if (speed > 0)
        {
            int freq;
            freq = *rb->cpu_frequency;
            
            speed = freq / speed;
            rb->snprintf(str,sizeof(str),"%d.%02dMHz needed for realtime",
            (int)speed/100,(int)speed%100);
            log_text(str,true);
        }   
#endif
    }

    res = PLUGIN_OK;

exit:
    rb->backlight_on();

    if (fd >= 0)
    {
        rb->close(fd);
    }

    return res;
}

#ifdef HAVE_TOUCHSCREEN
void cleanup(void)
{
    rb->screens[0]->set_viewport(NULL);
}
#endif

void plugin_quit(void)
{
    int btn;
#ifdef HAVE_TOUCHSCREEN
    static struct touchbutton button[] = {{
        .action = ACTION_STD_OK,
        .title = "OK",
        /* viewport runtime initialized, rest false/NULL */
    }};
    struct viewport *vp = &button[0].vp;
    struct screen *lcd = rb->screens[SCREEN_MAIN];
    rb->viewport_set_defaults(vp, SCREEN_MAIN);
    const int border = 10;
    const int height = 50;

    lcd->set_viewport(vp);
    /* button matches the bottom center in the grid */
    vp->x = lcd->lcdwidth/3;
    vp->width = lcd->lcdwidth/3;
    vp->height = height;
    vp->y = lcd->lcdheight - height - border;

    touchbutton_draw(button, ARRAYLEN(button));
    lcd->update_viewport();
    if (rb->touchscreen_get_mode() == TOUCHSCREEN_POINT)
    {
        while (codec_action != CODEC_ACTION_HALT &&
               touchbutton_get(button, ARRAYLEN(button)) != ACTION_STD_OK);
    }
    else
#endif
        do {
            btn = pluginlib_getaction(TIMEOUT_BLOCK, plugin_contexts,
                          ARRAYLEN(plugin_contexts));
            exit_on_usb(btn);
        } while ((codec_action != CODEC_ACTION_HALT)
                       && (btn != TESTCODEC_EXITBUTTON)
                       && (btn != TESTCODEC_EXITBUTTON2));
}

/* plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    int result, selection = 0;
    enum plugin_status res = PLUGIN_OK;
    int scandir;
    struct dirent *entry;
    DIR* dir;
    char* ch;
    char dirpath[MAX_PATH];
    char filename[MAX_PATH];
    size_t buffer_size;

    if (parameter == NULL)
    {
        rb->splash(HZ*2, "No File");
        return PLUGIN_ERROR;
    }

    wavbuffer = rb->plugin_get_buffer(&buffer_size);
    dspbuffer = wavbuffer + buffer_size / 2;
    dspbuffer_count = (buffer_size - (dspbuffer - wavbuffer)) /
                        (2 * sizeof (int16_t));

    codec_mallocbuf = rb->plugin_get_audio_buffer(&audiosize);
    /* Align codec_mallocbuf to pointer size, tlsf wants that */
    codec_mallocbuf = (void*)(((intptr_t)codec_mallocbuf +
                       sizeof(intptr_t)-1) & ~(sizeof(intptr_t)-1));
    audiobuf = SKIPBYTES(codec_mallocbuf, CODEC_SIZE);
    audiosize -= CODEC_SIZE;

    rb->lcd_clear_display();
    rb->lcd_update();

#ifdef HAVE_TOUCHSCREEN
    rb->touchscreen_set_mode(rb->global_settings->touch_mode);
#endif

    enum
    {
        SPEED_TEST = 0,
        SPEED_TEST_DIR,
        WRITE_WAV,
        SPEED_TEST_WITH_DSP,
        SPEED_TEST_DIR_WITH_DSP,
        WRITE_WAV_WITH_DSP,
        CHECKSUM,
        CHECKSUM_DIR,
        QUIT,
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        BOOST,
#endif
    };

    MENUITEM_STRINGLIST(
        menu, "test_codec", NULL,
        "Speed test",
        "Speed test folder",
        "Write WAV",
        "Speed test with DSP",
        "Speed test folder with DSP",
        "Write WAV with DSP",
        "Checksum",
        "Checksum folder",
        "Quit",
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        "Boosting",
#endif
    );
    

show_menu:
    rb->lcd_clear_display();

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
menu:
#endif 

    result = rb->do_menu(&menu, &selection, NULL, false);

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    if (result == BOOST)
    {
        rb->set_option("Boosting", &boost, INT,
                        boost_settings, 2, NULL);
        goto menu;
    }
    if(boost)
        rb->cpu_boost(true);
#endif

    if (result == QUIT)
    {
        res = PLUGIN_OK;
        goto exit;
    }

    scandir = 0;

    /* Map test runs with checksum calcualtion to standard runs 
     * SPEED_TEST and SPEED_TEST_DIR and set the 'checksum' flag. */
    if ((checksum = (result == CHECKSUM || 
                     result == CHECKSUM_DIR)))
        result -= 6;

    /* Map test runs with DSP to standard runs SPEED_TEST, 
     * SPEED_TEST_DIR and WRITE_WAV and set the 'use_dsp' flag. */
    if ((use_dsp = (result >= SPEED_TEST_WITH_DSP &&
                    result <= WRITE_WAV_WITH_DSP)))
        result -= 3;

    if (result == SPEED_TEST) {
        wavinfo.fd = -1;
        log_init(false);
    } else if (result == SPEED_TEST_DIR) {
        wavinfo.fd = -1;
        scandir = 1;

        /* Only create a log file when we are testing a folder */
        if (!log_init(true)) {
            rb->splash(HZ*2, "Cannot create logfile");
            res = PLUGIN_ERROR;
            goto exit;
        }
    } else if (result == WRITE_WAV) {
        log_init(false);
        init_wav("/test.wav");
        if (wavinfo.fd < 0) {
            rb->splash(HZ*2, "Cannot create /test.wav");
            res = PLUGIN_ERROR;
            goto exit;
        }
    } else if (result == MENU_ATTACHED_USB) {
        res = PLUGIN_USB_CONNECTED;
        goto exit;
    } else if (result < 0) {
        res = PLUGIN_OK;
        goto exit;
    }

    if (scandir) {
        /* Test all files in the same directory as the file selected by the
           user */

        rb->strlcpy(dirpath,parameter,sizeof(dirpath));
        ch = rb->strrchr(dirpath,'/');
        ch[1]=0;

        DEBUGF("Scanning directory \"%s\"\n",dirpath);
        dir = rb->opendir(dirpath);
        if (dir) {
            entry = rb->readdir(dir);
            while (entry) {
                struct dirinfo info = rb->dir_get_info(dir, entry);
                if (!(info.attribute & ATTR_DIRECTORY)) {
                    rb->snprintf(filename,sizeof(filename),"%s%s",dirpath,entry->d_name);
                    test_track(filename);

                    if (codec_action == CODEC_ACTION_HALT)
                        break;

                    log_text("", true);
                }

                /* Read next entry */
                entry = rb->readdir(dir);
            }
            
            rb->closedir(dir);
        }
    } else {
        /* Just test the file */
        res = test_track(parameter);

        /* Close WAV file (if there was one) */
        if (wavinfo.fd >= 0) {
            close_wav();
            log_text("Wrote /test.wav",true);
        }
    }
    plugin_quit();

    #ifdef HAVE_ADJUSTABLE_CPU_FREQ
        if(boost)
          rb->cpu_boost(false);
    #endif

    rb->button_clear_queue();
    goto show_menu;

exit:
    log_close();

    return res;
}
