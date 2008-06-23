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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"

PLUGIN_HEADER

/* All swcodec targets have BUTTON_SELECT apart from the H10 and M3 */

#if CONFIG_KEYPAD == IRIVER_H10_PAD
#define TESTCODEC_EXITBUTTON BUTTON_RIGHT
#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define TESTCODEC_EXITBUTTON BUTTON_RC_PLAY
#elif CONFIG_KEYPAD == COWOND2_PAD
#define TESTCODEC_EXITBUTTON BUTTON_POWER
#else
#define TESTCODEC_EXITBUTTON BUTTON_SELECT
#endif

static const struct plugin_api* rb;

CACHE_FUNCTION_WRAPPERS(rb)

/* Log functions copied from test_disk.c */
static int line = 0;
static int max_line = 0;
static int log_fd = -1;
static char logfilename[MAX_PATH];

static bool log_init(bool use_logfile)
{
    int h;

    rb->lcd_getstringsize("A", NULL, &h);
    max_line = LCD_HEIGHT / h;
    line = 0;
    rb->lcd_clear_display();
    rb->lcd_update();

    if (use_logfile) {
        rb->create_numbered_filename(logfilename, "/", "test_codec_log_", ".txt",
                                     2 IF_CNFN_NUM_(, NULL));
        log_fd = rb->open(logfilename, O_RDWR|O_CREAT|O_TRUNC);
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

static void log_close(void)
{
    if (log_fd >= 0)
        rb->close(log_fd);
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
static char str[MAX_PATH];

/* Our local implementation of the codec API */
static struct codec_api ci;

struct test_track_info {
    struct mp3entry id3;       /* TAG metadata */
    size_t filesize;           /* File total length */
};

static struct test_track_info track;
static bool taginfo_ready = true;

static volatile unsigned int elapsed;
static volatile bool codec_playing;
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

void init_wav(char* filename)
{
    wavinfo.totalsamples = 0;

    wavinfo.fd = rb->creat(filename);

    if (wavinfo.fd >= 0)
    {
        /* Write WAV header - we go back and fill in the details at the end */
        rb->write(wavinfo.fd, wav_header, sizeof(wav_header));
    }
}


void close_wav(void) {
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
static void* get_codec_memory(size_t *size)
{
   DEBUGF("get_codec_memory(%d)\n",(int)size);
   *size = 512*1024;
   return codec_mallocbuf;
}

/* Null output */
static bool pcmbuf_insert_null(const void *ch1, const void *ch2, int count)
{
    /* Always successful - just discard data */
    (void)ch1;
    (void)ch2;
    (void)count;

    /* Prevent idle poweroff */
    rb->reset_poweroff_timer();

    return true;
}

/* 64KB should be enough */
static unsigned char wavbuffer[64*1024];

static inline int32_t clip_sample(int32_t sample)
{
    if ((int16_t)sample != sample)
        sample = 0x7fff ^ (sample >> 31);

    return sample;
}


/* WAV output */
static bool pcmbuf_insert_wav(const void *ch1, const void *ch2, int count)
{
    const int16_t* data1_16;
    const int16_t* data2_16;
    const int32_t* data1_32;
    const int32_t* data2_32;
    unsigned char* p = wavbuffer;
    const int scale = wavinfo.sampledepth - 15;
    const int dc_bias = 1 << (scale - 1);

    /* Prevent idle poweroff */
    rb->reset_poweroff_timer();

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
                    int2le16(p, clip_sample((*data1_32++ + dc_bias) >> scale));
                    p += 2;
                    int2le16(p, clip_sample((*data1_32++ + dc_bias) >> scale));
                    p += 2;
                }
                break;
 
            case STEREO_NONINTERLEAVED:
                while (count--) {
                    int2le16(p, clip_sample((*data1_32++ + dc_bias) >> scale));
                    p += 2;
                    int2le16(p, clip_sample((*data2_32++ + dc_bias) >> scale));
                    p += 2;
                }

                break;
     
            case STEREO_MONO:
                while (count--) {
                    int2le16(p, clip_sample((*data1_32++ + dc_bias) >> scale));
                    p += 2;
                }
                break;
        }
    }

    wavinfo.totalsamples += count;
    rb->write(wavinfo.fd, wavbuffer, p - wavbuffer);

    return true;
}


/* Set song position in WPS (value in ms). */
static void set_elapsed(unsigned int value)
{
    elapsed = value;
}


/* Read next <size> amount bytes from file buffer to <ptr>.
   Will return number of bytes read or 0 if end of file. */
static size_t read_filebuf(void *ptr, size_t size)
{
   if (ci.curpos > (off_t)track.filesize)
   {
       return 0;
   } else {
       /* TODO: Don't read beyond end of buffer */
       rb->memcpy(ptr, audiobuf + ci.curpos, size);
       ci.curpos += size;
       return size;
   }
}


/* Request pointer to file buffer which can be used to read
   <realsize> amount of data. <reqsize> tells the buffer system
   how much data it should try to allocate. If <realsize> is 0,
   end of file is reached. */
static void* request_buffer(size_t *realsize, size_t reqsize)
{
    *realsize = MIN(track.filesize-ci.curpos,reqsize);

    return (audiobuf + ci.curpos);
}


/* Advance file buffer position by <amount> amount of bytes. */
static void advance_buffer(size_t amount)
{
    ci.curpos += amount;
}


/* Advance file buffer to a pointer location inside file buffer. */
static void advance_buffer_loc(void *ptr)
{
    ci.curpos = ptr - audiobuf;
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

/* Request file change from file buffer. Returns true is next
   track is available and changed. If return value is false,
   codec should exit immediately with PLUGIN_OK status. */
static bool request_next_track(void)
{
    /* We are only decoding a single track */
    return false;
}


/* Free the buffer area of the current codec after its loaded */
static void discard_codec(void)
{
    /* ??? */
}


static void set_offset(size_t value)
{
    /* ??? */
    (void)value;
}


/* Configure different codec buffer parameters. */
static void configure(int setting, intptr_t value)
{
    switch(setting)
    {
        case DSP_SWITCH_FREQUENCY:
        case DSP_SET_FREQUENCY:
            DEBUGF("samplerate=%d\n",(int)value);
            wavinfo.samplerate = (int)value;
            break;

        case DSP_SET_SAMPLE_DEPTH:
            DEBUGF("sampledepth = %d\n",(int)value);
            wavinfo.sampledepth=(int)value;
            break;

        case DSP_SET_STEREO_MODE:
            DEBUGF("Stereo mode = %d\n",(int)value);
            wavinfo.stereomode=(int)value;
            break;
    }

}

static void init_ci(void)
{
    /* --- Our "fake" implementations of the codec API functions. --- */

    ci.get_codec_memory = get_codec_memory;

    if (wavinfo.fd >= 0) {
        ci.pcmbuf_insert = pcmbuf_insert_wav;
    } else {
        ci.pcmbuf_insert = pcmbuf_insert_null;
    }
    ci.set_elapsed = set_elapsed;
    ci.read_filebuf = read_filebuf;
    ci.request_buffer = request_buffer;
    ci.advance_buffer = advance_buffer;
    ci.advance_buffer_loc = advance_buffer_loc;
    ci.seek_buffer = seek_buffer;
    ci.seek_complete = seek_complete;
    ci.request_next_track = request_next_track;
    ci.discard_codec = discard_codec;
    ci.set_offset = set_offset;
    ci.configure = configure;

    /* --- "Core" functions --- */

    /* kernel/ system */
    ci.PREFIX(sleep) = rb->PREFIX(sleep);
    ci.yield = rb->yield;

    /* strings and memory */
    ci.strcpy = rb->strcpy;
    ci.strncpy = rb->strncpy;
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
    ci.global_settings = rb->global_settings;

#ifdef RB_PROFILE
    ci.profile_thread = rb->profile_thread;
    ci.profstop = rb->profstop;
    ci.profile_func_enter = rb->profile_func_enter;
    ci.profile_func_exit = rb->profile_func_exit;
#endif

#ifdef CACHE_FUNCTIONS_AS_CALL
    ci.invalidate_icache = invalidate_icache;
    ci.flush_icache = flush_icache;
#endif
}

static void codec_thread(void)
{
    const char* codecname;
    int res;

    codecname = rb->get_codec_filename(track.id3.codectype);

    /* Load the codec and start decoding. */
    res = rb->codec_load_file(codecname,&ci);

    /* Signal to the main thread that we are done */
    codec_playing = false;
}

static uintptr_t* codec_stack;
static size_t codec_stack_size;

static enum plugin_status test_track(const char* filename)
{
    size_t n;
    int fd;
    enum plugin_status res = PLUGIN_ERROR;
    unsigned long starttick;
    unsigned long ticks;
    unsigned long speed;
    unsigned long duration;
    struct thread_entry* codecthread_id;
    const char* ch;

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
        log_text("File too large",true);
        goto exit;
    }

    n = rb->read(fd, audiobuf, track.filesize);

    if (n != track.filesize)
    {
        log_text("Read failed.",true);
        goto exit;
    }

    /* Initialise the function pointers in the codec API */
    init_ci();

    /* Prepare the codec struct for playing the whole file */
    ci.filesize = track.filesize;
    ci.id3 = &track.id3;
    ci.taginfo_ready = &taginfo_ready;
    ci.curpos = 0;
    ci.stop_codec = false;
    ci.new_track = 0;
    ci.seek_time = 0;

    starttick = *rb->current_tick;

    codec_playing = true;

    if ((codecthread_id = rb->create_thread(codec_thread,
            codec_stack, codec_stack_size, 0, "testcodec"
            IF_PRIO(,PRIORITY_PLAYBACK) IF_COP(, CPU))) == NULL)
    {
        log_text("Cannot create codec thread!",true);
        goto exit;
    }

    /* Wait for codec thread to die */
    while (codec_playing)
    {
        rb->sleep(HZ);
        rb->snprintf(str,sizeof(str),"%d of %d",elapsed,(int)track.id3.length);
        log_text(str,false);
    }
    /* Save the current time before we spin up the disk to access the log */
    ticks = *rb->current_tick - starttick;

    /* Be sure it is done */
    rb->thread_wait(codecthread_id);

    log_text(str,true);
    
    if (wavinfo.fd < 0) 
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
        
#ifndef SIMULATOR
        /* show effective clockrate in MHz needed for realtime decoding */
        if (speed > 0)
        {
            speed = CPUFREQ_MAX / speed;
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

/* plugin entry point */
enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter)
{
    uintptr_t* codec_stack_copy;
    int result, selection = 0;
    enum plugin_status res = PLUGIN_OK;
    int scandir;
    int i;
    struct dirent *entry;
    DIR* dir;
    char* ch;
    char dirpath[MAX_PATH];
    char filename[MAX_PATH];

    rb = api;

    if (parameter == NULL)
    {
        rb->splash(HZ*2, "No File");
        return PLUGIN_ERROR;
    }

    codec_mallocbuf = rb->plugin_get_audio_buffer(&audiosize);

#ifdef SIMULATOR
    /* The simulator thread implementation doesn't have stack buffers */
    (void)i;
    codec_stack_size = 0;
#else
    /* Borrow the codec thread's stack (in IRAM on most targets) */
    codec_stack = NULL;
    for (i = 0; i < MAXTHREADS; i++)
    {
        if (rb->strcmp(rb->threads[i].name,"codec")==0)
        {
            /* Wait to ensure the codec thread has blocked */
            while (rb->threads[i].state!=STATE_BLOCKED)
                rb->yield();

            codec_stack = rb->threads[i].stack;
            codec_stack_size = rb->threads[i].stack_size;
            break;
        }
    }

    if (codec_stack == NULL)
    {
        rb->splash(HZ*2, "No codec thread!");
        return PLUGIN_ERROR;
    }
#endif

    codec_stack_copy = codec_mallocbuf + 512*1024;
    audiobuf = SKIPBYTES(codec_stack_copy, codec_stack_size);
    audiosize -= 512*1024 + codec_stack_size;

#ifndef SIMULATOR
    /* Backup the codec thread's stack */
    rb->memcpy(codec_stack_copy,codec_stack,codec_stack_size);    
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif
    rb->lcd_clear_display();
    rb->lcd_update();

    MENUITEM_STRINGLIST(
        menu, "test_codec", NULL,
        "Speed test",
        "Speed test folder",
        "Write WAV",
    );

    rb->lcd_clear_display();

    result=rb->do_menu(&menu,&selection, NULL, false);

    scandir = 0;

    if (result==0) {
        wavinfo.fd = -1;
        log_init(false);
    } else if (result==1) {
        wavinfo.fd = -1;
        scandir = 1;

        /* Only create a log file when we are testing a folder */
        if (!log_init(true)) {
            rb->splash(HZ*2, "Cannot create logfile");
            res = PLUGIN_ERROR;
            goto exit;
        }
    } else if (result==2) {
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

        rb->strncpy(dirpath,parameter,sizeof(dirpath));
        ch = rb->strrchr(dirpath,'/');
        ch[1]=0;

        DEBUGF("Scanning directory \"%s\"\n",dirpath);
        dir = rb->opendir(dirpath);
        if (dir) {
            entry = rb->readdir(dir);
            while (entry) {
                if (!(entry->attribute & ATTR_DIRECTORY)) {
                    rb->snprintf(filename,sizeof(filename),"%s%s",dirpath,entry->d_name);
                    test_track(filename);
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

        while (rb->button_get(true) != TESTCODEC_EXITBUTTON);
    }

exit:
    log_close();

#ifndef SIMULATOR
    /* Restore the codec thread's stack */
    rb->memcpy(codec_stack, codec_stack_copy, codec_stack_size);    
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif

    return res;
}
