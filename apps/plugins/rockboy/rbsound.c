#include "rockmacros.h"
#include "defs.h"
#include "pcm.h"

struct pcm pcm IBSS_ATTR;

#define N_BUFS 2
#define BUF_SIZE 1024

#if CONFIG_CODEC == SWCODEC && !defined(SIMULATOR)

bool doneplay=1;

static unsigned char *buf=0;
static unsigned short *gmbuf;

static bool newly_started;

void get_more(unsigned char** start, size_t* size)
{
    *start = (unsigned char*)(&gmbuf[pcm.len*doneplay]);
    *size = BUF_SIZE*sizeof(short);
}

void pcm_init(void)
{
    if(plugbuf)
        return;

    newly_started = true;
    
    pcm.hz = 11025;
    pcm.stereo = 1;

    pcm.len = BUF_SIZE;
    if(!buf)
    {
        buf = my_malloc(pcm.len * N_BUFS);
        gmbuf = my_malloc(pcm.len * N_BUFS*sizeof (short));

        pcm.buf = buf;
        pcm.pos = 0;
        memset(gmbuf, 0, pcm.len * N_BUFS *sizeof(short));
        memset(buf, 0,  pcm.len * N_BUFS);
    }

    rb->pcm_play_stop();
   
    rb->pcm_set_frequency(11025); /* 44100 22050 11025 */
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

    if (pcm.pos < pcm.len) return 1;

    doneplay=!doneplay;

    if(doneplay)
        pcm.buf = buf + pcm.len;
    else
        pcm.buf = buf;

    pcm.pos = 0;

    /* gotta convert the 8 bit buffer to 16 */
    for(i=0; i<pcm.len;i++)
        gmbuf[i+pcm.len*doneplay] = (pcm.buf[i]<<8)-0x8000;

    if(newly_started)
    {
        rb->pcm_play_data(&get_more,NULL,0);
        newly_started = false;
    }

   return 1;
}
#else
static byte buf1_unal[(BUF_SIZE / sizeof(short)) + 2]; /* 4 byte aligned */
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

