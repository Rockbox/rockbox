/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "plugin.h"

#if (CONFIG_HWCODEC == MASNONE) && !defined(SIMULATOR)
/* software codec platforms, not for simulator */

#include <inttypes.h>  /* Needed by a52.h */

#include <codecs/liba52/config.h>
#include <codecs/liba52/a52.h>

/* Currently used for WAV output */
#ifdef WORDS_BIGENDIAN
  #warning ************************************* BIG ENDIAN
  #define LE_S16(x) ( (uint16_t) ( ((uint16_t)(x) >> 8) | ((uint16_t)(x) << 8) ) )
#else
  #define LE_S16(x) (x)
#endif

typedef struct ao_sample_format {
        int bits; /* bits per sample */
        int rate; /* samples per second (in a single channel) */
        int channels; /* number of audio channels */
        int byte_format; /* Byte ordering in sample, see constants below */
} ao_sample_format;

#define AO_FMT_LITTLE 1
#define AO_FMT_BIG    2
#define AO_FMT_NATIVE 4

/* the main data structure of the program */
typedef struct {
    int infile;
    int outfile;
    off_t curpos;
    off_t filesize;
    ao_sample_format samfmt; /* bits, rate, channels, byte_format */
  //    ao_device *ao_dev;
    unsigned long total_samples;
    unsigned long current_sample;
    float total_time;        /* seconds */
    float elapsed_time;      /* seconds */
} file_info_struct;

file_info_struct file_info;

#define MALLOC_BUFSIZE (512*1024)

int mem_ptr;
int bufsize;
unsigned char* mp3buf;     // The actual MP3 buffer from Rockbox
unsigned char* mallocbuf;  // 512K from the start of MP3 buffer
unsigned char* filebuf;    // The rest of the MP3 buffer



#define BUFFER_SIZE 4096
//static uint8_t buffer[BUFFER_SIZE];
static float gain = 1;
static a52_state_t * state;

int output;

// DAVE: I'm not sure what these are for.
int disable_accel=0;
int disable_adjust=0;
int disable_dynrng=0;

/* welcome to the example rockbox plugin */

/* here is a global api struct pointer. while not strictly necessary,
   it's nice not to have to pass the api pointer in all function calls
   in the plugin */
static struct plugin_api* rb;

void* malloc(size_t size) {
  void* x;
  char s[32];

  x=&mallocbuf[mem_ptr];
  mem_ptr+=size+(size%4); // Keep memory 32-bit aligned (if it was already?)

  rb->snprintf(s,30,"Memory used: %d\r",mem_ptr);
  rb->lcd_putsxy(0,80,s);
  rb->lcd_update();
  return(x);
}

void* calloc(size_t nmemb, size_t size) {
  void* x;
  x=malloc(nmemb*size);
  rb->memset(x,0,nmemb*size);
  return(x);
}

void free(void* ptr) {
  (void)ptr;
}

void* realloc(void* ptr, size_t size) {
  void* x;
  (void)ptr;
  x=malloc(size);
  return(x);
}

void *memcpy(void *dest, const void *src, size_t n) {
  return(rb->memcpy(dest,src,n));
}

void *memset(void *s, int c, size_t n) {
  return(rb->memset(s,c,n));
}

int memcmp(const void *s1, const void *s2, size_t n) {
  return(rb->memcmp(s1,s2,n));
}

void* memmove(const void *s1, const void *s2, size_t n) {
  char* dest=(char*)s1;
  char* src=(char*)s2;
  size_t i;

  for (i=0;i<n;i++) { dest[i]=src[i]; }
  //  while(n>0) { *(dest++)=*(src++); n--; }
  return(dest);
}

void qsort(void *base, size_t nmemb, size_t size, int(*compar)(const void *, const void *)) {
  rb->qsort(base,nmemb,size,compar);
}




static unsigned char wav_header[44]={'R','I','F','F',    //  0 - ChunkID
                              0,0,0,0,            //  4 - ChunkSize (filesize-8)
                              'W','A','V','E',    //  8 - Format
                              'f','m','t',' ',    // 12 - SubChunkID
                              16,0,0,0,           // 16 - SubChunk1ID  // 16 for PCM
                              1,0,                // 20 - AudioFormat (1=16-bit)
                              2,0,                // 22 - NumChannels
                              0,0,0,0,            // 24 - SampleRate in Hz
                              0,0,0,0,            // 28 - Byte Rate (SampleRate*NumChannels*(BitsPerSample/8)
                              4,0,                // 32 - BlockAlign (== NumChannels * BitsPerSample/8)
                              16,0,               // 34 - BitsPerSample
                              'd','a','t','a',    // 36 - Subchunk2ID
                              0,0,0,0             // 40 - Subchunk2Size
                             };

void close_wav(file_info_struct* file_info) {
  int x;
  int filesize=rb->filesize(file_info->outfile);

  /* We assume 16-bit, Stereo */

  rb->lseek(file_info->outfile,0,SEEK_SET);

  // ChunkSize
  x=filesize-8;
  wav_header[4]=(x&0xff);
  wav_header[5]=(x&0xff00)>>8;
  wav_header[6]=(x&0xff0000)>>16;
  wav_header[7]=(x&0xff000000)>>24;

  // Samplerate
  wav_header[24]=file_info->samfmt.rate&0xff;
  wav_header[25]=(file_info->samfmt.rate&0xff00)>>8;
  wav_header[26]=(file_info->samfmt.rate&0xff0000)>>16;
  wav_header[27]=(file_info->samfmt.rate&0xff000000)>>24;

  // ByteRate
  x=file_info->samfmt.rate*4;
  wav_header[28]=(x&0xff);
  wav_header[29]=(x&0xff00)>>8;
  wav_header[30]=(x&0xff0000)>>16;
  wav_header[31]=(x&0xff000000)>>24;
  
  // Subchunk2Size
  x=filesize-44;
  wav_header[40]=(x&0xff);
  wav_header[41]=(x&0xff00)>>8;
  wav_header[42]=(x&0xff0000)>>16;
  wav_header[43]=(x&0xff000000)>>24;

  rb->write(file_info->outfile,wav_header,sizeof(wav_header));
  rb->close(file_info->outfile);
}

static inline int16_t convert (int32_t i)
{
    i >>= 15;
    return (i > 32767) ? 32767 : ((i < -32768) ? -32768 : i);
}

void convert2s16_2 (sample_t * _f, int16_t * s16)
{
    int i;
    int32_t * f = (int32_t *) _f;
    for (i = 0; i < 256; i++) {
        s16[2*i] = LE_S16(convert (f[i]));
        s16[2*i+1] = LE_S16(convert (f[i+256]));
    }
}

void ao_play(file_info_struct* file_info,sample_t* samples,int flags) {
  int i;
  static int16_t int16_samples[256*2];

  flags &= A52_CHANNEL_MASK | A52_LFE;

  if (flags==A52_STEREO) {
//    convert2s16_2(samples,int16_samples,flags);
    for (i = 0; i < 256; i++) {
        int16_samples[2*i] = LE_S16(convert (samples[i]));
        int16_samples[2*i+1] = LE_S16(convert (samples[i+256]));
    }
  } else {
#ifdef SIMULATOR
    fprintf(stderr,"ERROR: unsupported format: %d\n",flags);
#endif
  }

  i=rb->write(file_info->outfile,int16_samples,256*2*2);

#ifdef SIMULATOR
  if (i!=(256*2*2)) {
    fprintf(stderr,"Attempted to write %d bytes, wrote %d bytes\n",256*2*2,i);
  }
#endif
}


void a52_decode_data (file_info_struct* file_info, uint8_t * start, uint8_t * end)
{
    static uint8_t buf[3840];
    static uint8_t * bufptr = buf;
    static uint8_t * bufpos = buf + 7;

    /*
     * sample_rate and flags are static because this routine could
     * exit between the a52_syncinfo() and the ao_setup(), and we want
     * to have the same values when we get back !
     */

    static int sample_rate;
    static int flags;
    int bit_rate;
    int len;

    while (1) {
	len = end - start;
	if (!len)
	    break;
	if (len > bufpos - bufptr)
	    len = bufpos - bufptr;
	memcpy (bufptr, start, len);
	bufptr += len;
	start += len;
	if (bufptr == bufpos) {
	    if (bufpos == buf + 7) {
		int length;

		length = a52_syncinfo (buf, &flags, &sample_rate, &bit_rate);
		if (!length) {
#ifdef SIMULATOR
		    fprintf (stderr, "skip\n");
#endif
		    for (bufptr = buf; bufptr < buf + 6; bufptr++)
			bufptr[0] = bufptr[1];
		    continue;
		}
		bufpos = buf + length;
	    } else {
                // The following two defaults are taken from audio_out_oss.c:
		level_t level;
		sample_t bias;
		int i;

                /* This is the configuration for the downmixing: */
                flags=A52_STEREO|A52_ADJUST_LEVEL|A52_LFE;
                level=(1 << 26);
                bias=0;

		level = (level_t) (level * gain);

		if (a52_frame (state, buf, &flags, &level, bias))
		    goto error;

                if (output==0) {
                  file_info->samfmt.bits=16;
                  file_info->samfmt.rate=sample_rate;
                  output=1;
//                  output=ao_open(&format);
                }

                // An A52 frame consists of 6 blocks of 256 samples
                // So we decode and output them one block at a time
		for (i = 0; i < 6; i++) {
		    if (a52_block (state)) {
			goto error;
                    }
		    ao_play (file_info, a52_samples (state),flags);
                    file_info->current_sample+=256;
		}
		bufptr = buf;
		bufpos = buf + 7;
//		print_fps (0);
		continue;
	    error:
#ifdef SIMULATOR
		fprintf (stderr, "error\n");
#endif
		bufptr = buf;
		bufpos = buf + 7;
	    }
	}
    }
}

/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* file)
{
  int i,n,bytesleft;
  char s[32];
  unsigned long ticks_taken;
  unsigned long start_tick;
  unsigned long long speed;
  unsigned long xspeed;
  int accel=0;  // ??? This is the parameter to a52_init().

  /* this macro should be called as the first thing you do in the plugin.
     it test that the api version and model the plugin was compiled for
     matches the machine it is running on */
  TEST_PLUGIN_API(api);

  /* if you are using a global api pointer, don't forget to copy it!
     otherwise you will get lovely "I04: IllInstr" errors... :-) */
  rb = api;

  /* now go ahead and have fun! */
  //    rb->splash(HZ*2, true, "Hello world!");

  mem_ptr=0;
  mp3buf=rb->plugin_get_mp3_buffer(&bufsize);
  mallocbuf=mp3buf;
  filebuf=&mp3buf[MALLOC_BUFSIZE];

  rb->snprintf(s,32,"mp3 bufsize: %d\r",bufsize);
  rb->lcd_putsxy(0,100,s);
  rb->lcd_update();

  file_info.infile=rb->open(file,O_RDONLY);
  file_info.outfile=rb->creat("/ac3test.wav",O_WRONLY);
  rb->write(file_info.outfile,wav_header,sizeof(wav_header));
  file_info.curpos=0;
  file_info.filesize=rb->filesize(file_info.infile);

  if (file_info.filesize > (bufsize-MALLOC_BUFSIZE)) {
    rb->close(file_info.infile);
    rb->splash(HZ*2, true, "File too large");
    return PLUGIN_ERROR;
  }

  rb->snprintf(s,32,"Loading file...");
  rb->lcd_putsxy(0,0,s);
  rb->lcd_update();

  bytesleft=file_info.filesize;
  i=0;
  while (bytesleft > 0) {
    n=rb->read(file_info.infile,&filebuf[i],bytesleft);
    if (n < 0) {
      rb->close(file_info.infile);
      rb->splash(HZ*2, true, "ERROR READING FILE");
      return PLUGIN_ERROR;
    }      
    i+=n; bytesleft-=n;
  }
  rb->close(file_info.infile);

  state = a52_init (accel);
  if (state == NULL) {
    //fprintf (stderr, "A52 init failed\n");
    return PLUGIN_ERROR;
  }

  i=0;
  start_tick=*(rb->current_tick);
  while (file_info.curpos < file_info.filesize) {
    i++;
    if ((file_info.curpos+BUFFER_SIZE) < file_info.filesize) {   
      a52_decode_data (&file_info,&filebuf[file_info.curpos],&filebuf[file_info.curpos+BUFFER_SIZE]);
      file_info.curpos+=BUFFER_SIZE;
    } else {
      a52_decode_data (&file_info,&filebuf[file_info.curpos],&filebuf[file_info.filesize-1]);
      file_info.curpos=file_info.filesize;
    }

    rb->snprintf(s,32,"Bytes read: %d\r",file_info.curpos);
    rb->lcd_putsxy(0,0,s);
    rb->snprintf(s,32,"Samples Decoded: %d\r",file_info.current_sample);
    rb->lcd_putsxy(0,20,s);
    rb->snprintf(s,32,"Frames Decoded: %d\r",i);
    rb->lcd_putsxy(0,40,s);

    ticks_taken=*(rb->current_tick)-start_tick;

    /* e.g.:
    ticks_taken=500
    sam_fmt.rate=44,100
    samples_decoded=172,400
    (samples_decoded/sam_fmt.rate)*100=400 (time it should have taken)
    % Speed=(400/500)*100=80%

    */

    if (ticks_taken==0) { ticks_taken=1; } // Avoid fp exception.

    speed=(100*file_info.current_sample)/file_info.samfmt.rate;
    xspeed=(speed*10000)/ticks_taken;
    rb->snprintf(s,32,"Speed %ld.%02ld %% Secs: %d",(xspeed/100),(xspeed%100),ticks_taken/100); 
    rb->lcd_putsxy(0,60,s);

    rb->lcd_update();
    if (rb->button_get(false)!=BUTTON_NONE) {
      close_wav(&file_info);
      return PLUGIN_OK;
    }
  }
  close_wav(&file_info);

//NO NEED:  a52_free (state);
  rb->splash(HZ*2, true, "FINISHED!");
  return PLUGIN_OK;
}
#endif /* CONFIG_HWCODEC == MASNONE */
