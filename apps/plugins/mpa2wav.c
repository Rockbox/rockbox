/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Dave Chapman
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

#include <codecs/libmad/mad.h>

typedef struct ao_sample_format {
        int bits; /* bits per sample */
        int rate; /* samples per second (in a single channel) */
        int channels; /* number of audio channels */
        int byte_format; /* Byte ordering in sample, see constants below */
} ao_sample_format;

typedef struct {
    int infile;
    int outfile;
    off_t curpos;
    off_t filesize;
    ao_sample_format samfmt; /* bits, rate, channels, byte_format */
    int framesize;
    unsigned long total_samples;
    unsigned long current_sample;
} file_info_struct;


struct mad_stream  Stream;
struct mad_frame  Frame;
struct mad_synth  Synth;
mad_timer_t      Timer;
struct dither d0, d1;

file_info_struct file_info;

#define MALLOC_BUFSIZE (512*1024)

int mem_ptr;
int bufsize;
unsigned char* mp3buf;     // The actual MP3 buffer from Rockbox
unsigned char* mallocbuf;  // 512K from the start of MP3 buffer
unsigned char* filebuf;    // The rest of the MP3 buffer

/* here is a global api struct pointer. while not strictly necessary,
   it's nice not to have to pass the api pointer in all function calls
   in the plugin */
static struct plugin_api* rb;

void* malloc(size_t size) {
  void* x;
  char s[32];

  x=&mallocbuf[mem_ptr];
  mem_ptr+=size+(size%4); // Keep memory 32-bit aligned (if it was already?)

  rb->snprintf(s,30,"Memory used: %d",mem_ptr);
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


void abort(void) {
  /* Let's hope this is never called by libmad */
}

/* The "dither" code to convert the 24-bit samples produced by libmad was
   taken from the coolplayer project - coolplayer.sourceforge.net */

struct dither {
	mad_fixed_t error[3];
	mad_fixed_t random;
};

# define SAMPLE_DEPTH	16
# define scale(x, y)	dither((x), (y))

/*
 * NAME:		prng()
 * DESCRIPTION:	32-bit pseudo-random number generator
 */
static __inline
unsigned long prng(unsigned long state)
{
  return (state * 0x0019660dL + 0x3c6ef35fL) & 0xffffffffL;
}

/*
 * NAME:        dither()
 * DESCRIPTION:	dither and scale sample
 */
static __inline
signed int dither(mad_fixed_t sample, struct dither *dither)
{
  unsigned int scalebits;
  mad_fixed_t output, mask, random;

  enum {
    MIN = -MAD_F_ONE,
    MAX =  MAD_F_ONE - 1
  };

  /* noise shape */
  sample += dither->error[0] - dither->error[1] + dither->error[2];

  dither->error[2] = dither->error[1];
  dither->error[1] = dither->error[0] / 2;

  /* bias */
  output = sample + (1L << (MAD_F_FRACBITS + 1 - SAMPLE_DEPTH - 1));

  scalebits = MAD_F_FRACBITS + 1 - SAMPLE_DEPTH;
  mask = (1L << scalebits) - 1;

  /* dither */
  random  = prng(dither->random);
  output += (random & mask) - (dither->random & mask);

  dither->random = random;

  /* clip */
  if (output > MAX) {
    output = MAX;

    if (sample > MAX)
      sample = MAX;
  }
  else if (output < MIN) {
    output = MIN;

    if (sample < MIN)
      sample = MIN;
  }

  /* quantize */
  output &= ~mask;

  /* error feedback */
  dither->error[0] = sample - output;

  /* scale */
  return output >> scalebits;
}

#define SHRT_MAX 32767

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

#define INPUT_BUFFER_SIZE	(5*8192)
#define OUTPUT_BUFFER_SIZE	8192 /* Must be an integer multiple of 4. */

unsigned char InputBuffer[INPUT_BUFFER_SIZE+MAD_BUFFER_GUARD];
unsigned char OutputBuffer[OUTPUT_BUFFER_SIZE];
unsigned char *OutputPtr=OutputBuffer;
unsigned char *GuardPtr=NULL;
const unsigned char *OutputBufferEnd=OutputBuffer+OUTPUT_BUFFER_SIZE;

/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* file)
{
  int i,n,bytesleft;
  char s[32];
  unsigned long ticks_taken;
  unsigned long start_tick;
  unsigned long long speed;
  unsigned long xspeed;
  long file_ptr;

  int Status=0;
  unsigned long	FrameCount=0;

  TEST_PLUGIN_API(api);

  rb = api;

  mem_ptr=0;
  mp3buf=rb->plugin_get_mp3_buffer(&bufsize);
  mallocbuf=mp3buf;
  filebuf=&mp3buf[MALLOC_BUFSIZE];

  rb->snprintf(s,32,"mp3 bufsize: %d",bufsize);
  rb->lcd_putsxy(0,100,s);
  rb->lcd_update();

  /* Create a decoder instance */

  mad_stream_init(&Stream);
  mad_frame_init(&Frame);
  mad_synth_init(&Synth);
  mad_timer_reset(&Timer);

 //if error:   return PLUGIN_ERROR;

  file_info.infile=rb->open(file,O_RDONLY);
  file_info.outfile=rb->creat("/libmadtest.wav",O_WRONLY);
  rb->write(file_info.outfile,wav_header,sizeof(wav_header));
  file_info.curpos=0;
  file_info.filesize=rb->filesize(file_info.infile);

  if (file_info.filesize > (bufsize-512*1024)) {
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
    n+=i; bytesleft-=n;
  }
  rb->close(file_info.infile);

  mad_stream_init(&Stream);
  mad_frame_init(&Frame);
  mad_synth_init(&Synth);
  mad_timer_reset(&Timer);

  file_ptr=0;
  start_tick=*(rb->current_tick);

  rb->button_clear_queue();

  /* This is the decoding loop. */
  while (file_ptr < file_info.filesize) {
    if(Stream.buffer==NULL || Stream.error==MAD_ERROR_BUFLEN) {
      size_t ReadSize, Remaining;
      unsigned char  *ReadStart;

      if(Stream.next_frame!=NULL) {
        Remaining=Stream.bufend-Stream.next_frame;
        memmove(InputBuffer,Stream.next_frame,Remaining);
        ReadStart=InputBuffer+Remaining;
        ReadSize=INPUT_BUFFER_SIZE-Remaining;
      } else {
        ReadSize=INPUT_BUFFER_SIZE;
        ReadStart=InputBuffer;
        Remaining=0;
      }

      /* Fill-in the buffer. If an error occurs print a message
       * and leave the decoding loop. If the end of stream is
       * reached we also leave the loop but the return status is
       * left untouched.
       */

      if ((file_info.filesize-file_ptr) < (int) ReadSize) {
         ReadSize=file_info.filesize-file_ptr;
      }
      memcpy(ReadStart,&filebuf[file_ptr],ReadSize);
      file_ptr+=ReadSize;

      if (file_ptr >= file_info.filesize) 
      {
        GuardPtr=ReadStart+ReadSize;
        memset(GuardPtr,0,MAD_BUFFER_GUARD);
        ReadSize+=MAD_BUFFER_GUARD;
      }

      /* Pipe the new buffer content to libmad's stream decoder
             * facility.
       */
      mad_stream_buffer(&Stream,InputBuffer,ReadSize+Remaining);
      Stream.error=0;
    }

    if(mad_frame_decode(&Frame,&Stream))
    {
      if(MAD_RECOVERABLE(Stream.error))
      {
        if(Stream.error!=MAD_ERROR_LOSTSYNC || Stream.this_frame!=GuardPtr)
        {
          rb->splash(HZ*1, true, "Recoverable...!");
        }
        continue;
      }
      else
        if(Stream.error==MAD_ERROR_BUFLEN)
          continue;
        else
        {
          rb->splash(HZ*1, true, "Recoverable...!");
          //fprintf(stderr,"%s: unrecoverable frame level error.\n",ProgName);
          Status=1;
          break;
        }
    }

    /* We assume all frames have same samplerate as the first */
    if(FrameCount==0) {
      file_info.samfmt.rate=Frame.header.samplerate;
    }

    FrameCount++;
    /* ?? Do we need the timer module? */
    mad_timer_add(&Timer,Frame.header.duration);

/* DAVE: This can be used to attenuate the audio */
//    if(DoFilter)
//      ApplyFilter(&Frame);   

    mad_synth_frame(&Synth,&Frame);

    /* Convert MAD's numbers to an array of 16-bit LE signed integers */
    for(i=0;i<Synth.pcm.length;i++)
    {
      unsigned short  Sample;

      /* Left channel */
      Sample=scale(Synth.pcm.samples[0][i],&d0);
      *(OutputPtr++)=Sample&0xff;
      *(OutputPtr++)=Sample>>8;

      /* Right channel. If the decoded stream is monophonic then
       * the right output channel is the same as the left one.
       */
      if(MAD_NCHANNELS(&Frame.header)==2)
        Sample=scale(Synth.pcm.samples[1][i],&d1);
      *(OutputPtr++)=Sample&0xff;
      *(OutputPtr++)=Sample>>8;

      /* Flush the buffer if it is full. */
      if(OutputPtr==OutputBufferEnd)
      {
        rb->write(file_info.outfile,OutputBuffer,OUTPUT_BUFFER_SIZE);
        OutputPtr=OutputBuffer;
      }
    }

    rb->snprintf(s,32,"Bytes Read: %d   ",file_ptr);
    rb->lcd_putsxy(0,0,s);

    rb->snprintf(s,32,"Samples Decoded: %d",file_info.current_sample);
    rb->lcd_putsxy(0,20,s);
    rb->snprintf(s,32,"Frames Decoded: %d",FrameCount);
    rb->lcd_putsxy(0,40,s);

    file_info.current_sample+=Synth.pcm.length;

    ticks_taken=*(rb->current_tick)-start_tick;

    if (ticks_taken==0) { ticks_taken=1; } // Avoid fp exception.

    speed=(100*file_info.current_sample)/file_info.samfmt.rate;
    xspeed=(speed*10000)/ticks_taken;
    rb->snprintf(s,32,"Speed %ld.%02ld%% Secs: %d",(xspeed/100),(xspeed%100),ticks_taken/100); 
    rb->lcd_putsxy(0,60,s);

    rb->lcd_update();
    if (rb->button_get(false)!=BUTTON_NONE) { 
      close_wav(&file_info);
      return PLUGIN_OK;
    }
  }

  close_wav(&file_info);
  rb->splash(HZ*2, true, "FINISHED!");

  return PLUGIN_OK;
}
#endif /* CONFIG_HWCODEC == MASNONE */
