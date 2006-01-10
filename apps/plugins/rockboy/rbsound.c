#include "rockmacros.h"
#include "defs.h"
#include "pcm.h"
#include "rc.h"

struct pcm pcm;

bool sound = 1;
#define N_BUFS 4
#define BUF_SIZE 1024

rcvar_t pcm_exports[] =
{
	    RCV_END
};

#if CONFIG_CODEC == SWCODEC && !defined(SIMULATOR)

static int curbuf,gmcurbuf;

static byte *buf=0;
static short *gmbuf;

static bool newly_started;

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
    		curbuf = gmcurbuf= 0;
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

void get_more(unsigned char** start, long* size)
{	
    *start = (unsigned char*)(&gmbuf[pcm.len*curbuf]);
    *size = BUF_SIZE*sizeof(short);
}
            
int pcm_submit(void)
{
	register int i;

	if (!sound) {
		pcm.pos = 0;
		return 0;
    }

    if (pcm.pos >= pcm.len) {
		curbuf = (curbuf + 1) % N_BUFS;
		pcm.buf = buf + pcm.len * curbuf;
		pcm.pos = 0;

		// gotta convert the 8 bit buffer to 16
		for(i=0; i<pcm.len;i++)
			gmbuf[i+pcm.len*curbuf] = (pcm.buf[i]<<8)-0x8000;
    }
    
    if(newly_started)
    {  
        rb->pcm_play_data(&get_more);
        newly_started = false;
    }    
    
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

