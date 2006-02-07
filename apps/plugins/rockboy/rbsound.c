#include "rockmacros.h"
#include "defs.h"
#include "pcm.h"
#include "rc.h"

//#define ONEBUF    // Note: I think the single buffer implementation is more responsive with sound(less lag)
                  // but it creates more choppyness overall to the sound.  2 buffer's don't seem to make
                  // a difference, but 4 buffers is definately noticable

struct pcm pcm IBSS_ATTR;

bool sound = 1;
#ifdef ONEBUF
#define N_BUFS 1
#else
#define N_BUFS 4
#endif
#define BUF_SIZE 1024

rcvar_t pcm_exports[] =
{
	    RCV_END
};

#if CONFIG_CODEC == SWCODEC && !defined(SIMULATOR)

#ifndef ONEBUF
static short curbuf,gmcurbuf;
#else
bool doneplay=0;
#endif

static unsigned char *buf=0;
static unsigned short *gmbuf;

static bool newly_started;

void get_more(unsigned char** start, size_t* size)
{
#ifdef ONEBUF
   doneplay=1;
   *start = (unsigned char*)(gmbuf);
#else
   *start = (unsigned char*)(&gmbuf[pcm.len*curbuf]);
#endif
   *size = BUF_SIZE*sizeof(short);
}

void pcm_init(void)
{
	if(!sound) return;

   newly_started = true;
    
   pcm.hz = 11025;
   pcm.stereo = 1;

	pcm.len = BUF_SIZE;
	if(!buf){
      buf = my_malloc(pcm.len * N_BUFS);
	   gmbuf = my_malloc(pcm.len * N_BUFS*sizeof (short));
	   pcm.buf = buf;
	   pcm.pos = 0;
#ifndef ONEBUF
      curbuf = gmcurbuf= 0;
#endif
      memset(gmbuf, 0, pcm.len * N_BUFS *sizeof(short));
      memset(buf, 0,  pcm.len * N_BUFS);
	}

   rb->pcm_play_stop();
   
   rb->pcm_set_frequency(11025); // 44100 22050 11025
}

void pcm_close(void)
{
   memset(&pcm, 0, sizeof pcm);    
   newly_started = true;   
   rb->pcm_play_stop();	
	rb->pcm_set_frequency(44100);
}
            
int pcm_submit(void)
{
	register int i;

	if (!sound) {
		pcm.pos = 0;
		return 0;
   }

   if (pcm.pos < pcm.len) return 1;

#ifndef ONEBUF
   curbuf = (curbuf + 1) % N_BUFS;
   pcm.buf = buf + pcm.len * curbuf;
#endif
   pcm.pos = 0;

   // gotta convert the 8 bit buffer to 16
   for(i=0; i<pcm.len;i++)
#ifdef ONEBUF
      gmbuf[i] = (pcm.buf[i]<<8)-0x8000;
#else
      gmbuf[i+pcm.len*curbuf] = (pcm.buf[i]<<8)-0x8000;
#endif
    
   if(newly_started)
   {  
      rb->pcm_play_data(&get_more,NULL,0);
      newly_started = false;
   }    
   
   // this while loop and done play are in place to make sure the sound timing is correct(although it's not)
#ifdef ONEBUF
   while(doneplay==0) rb->yield();
   doneplay=0;
#endif
   return 1;
}
#else
static byte buf1_unal[(BUF_SIZE / sizeof(short)) + 2]; // to make sure 4 byte aligned
void pcm_init(void)
{
    pcm.hz = 11025;
    pcm.stereo = 1;
    pcm.buf = buf1_unal;
    pcm.len = (BUF_SIZE / sizeof(short));
    pcm.pos = 0;
}

void pcm_close(void)
{
    memset(&pcm, 0, sizeof pcm);
}
int pcm_submit(void)
{
    pcm.pos =0;
    return 0;
}
#endif

