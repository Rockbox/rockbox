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

/* Various "helper functions" common to all the xxx2wav decoder plugins 

   We include them as a .h because the plugin build system links
   exactly one .c file with exactly one .rock file. 
 */


/* the main data structure of the program */
typedef struct {
    int infile;
    int outfile;
    off_t curpos;
    off_t filesize;
    int samplerate;
    int channels;
    int frames_decoded;
    unsigned long total_samples;
    unsigned long current_sample;
    unsigned long start_tick;
} file_info_struct;

file_info_struct file_info;

#define MALLOC_BUFSIZE (512*1024)

int mem_ptr;
int bufsize;
unsigned char* mp3buf;     // The actual MP3 buffer from Rockbox
unsigned char* mallocbuf;  // 512K from the start of MP3 buffer
unsigned char* filebuf;    // The rest of the MP3 buffer

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

void display_status(file_info_struct* file_info) {
  char s[32];
  unsigned long ticks_taken;
  unsigned long long speed;
  unsigned long xspeed;

  rb->snprintf(s,32,"Bytes read: %d",file_info->curpos);
  rb->lcd_putsxy(0,0,s);
  rb->snprintf(s,32,"Samples Decoded: %d",file_info->current_sample);
  rb->lcd_putsxy(0,20,s);
  rb->snprintf(s,32,"Frames Decoded: %d",file_info->frames_decoded);
  rb->lcd_putsxy(0,40,s);

  ticks_taken=*(rb->current_tick)-file_info->start_tick;

  /* e.g.:
   ticks_taken=500
   sam_fmt.rate=44,100
   samples_decoded=172,400
   (samples_decoded/sam_fmt.rate)*100=400 (time it should have taken)
   % Speed=(400/500)*100=80%
  */

  if (ticks_taken==0) { ticks_taken=1; } // Avoid fp exception.

  speed=(100*file_info->current_sample)/file_info->samplerate;
  xspeed=(speed*10000)/ticks_taken;
  rb->snprintf(s,32,"Speed %ld.%02ld %% Secs: %d",(xspeed/100),(xspeed%100),ticks_taken/100); 
  rb->lcd_putsxy(0,60,s);

  rb->lcd_update();
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


int local_init(char* infilename, char* outfilename, file_info_struct* file_info) {
  char s[32];
  int i,n,bytesleft;

  mem_ptr=0;
  mp3buf=rb->plugin_get_mp3_buffer(&bufsize);
  mallocbuf=mp3buf;
  filebuf=&mp3buf[MALLOC_BUFSIZE];

  rb->snprintf(s,32,"mp3 bufsize: %d",bufsize);
  rb->lcd_putsxy(0,100,s);
  rb->lcd_update();

  file_info->infile=rb->open(infilename,O_RDONLY);
  file_info->outfile=rb->creat(outfilename,O_WRONLY);
  rb->write(file_info->outfile,wav_header,sizeof(wav_header));
  file_info->curpos=0;
  file_info->current_sample=0;
  file_info->frames_decoded=0;
  file_info->filesize=rb->filesize(file_info->infile);

  if (file_info->filesize > (bufsize-MALLOC_BUFSIZE)) {
    rb->close(file_info->infile);
    rb->splash(HZ*2, true, "File too large");
    return(1);
  }

  rb->snprintf(s,32,"Loading file...");
  rb->lcd_putsxy(0,0,s);
  rb->lcd_update();

  bytesleft=file_info->filesize;
  i=0;
  while (bytesleft > 0) {
    n=rb->read(file_info->infile,&filebuf[i],bytesleft);
    if (n < 0) {
      rb->close(file_info->infile);
      rb->splash(HZ*2, true, "ERROR READING FILE");
      return(1);
    }      
    i+=n; bytesleft-=n;
  }
  rb->close(file_info->infile);
  return(0);
}

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
  wav_header[24]=file_info->samplerate&0xff;
  wav_header[25]=(file_info->samplerate&0xff00)>>8;
  wav_header[26]=(file_info->samplerate&0xff0000)>>16;
  wav_header[27]=(file_info->samplerate&0xff000000)>>24;

  // ByteRate
  x=file_info->samplerate*4;
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

