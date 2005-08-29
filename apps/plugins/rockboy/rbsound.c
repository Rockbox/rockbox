#include "rockmacros.h"
#include "defs.h"
#include "pcm.h"
#include "rc.h"

struct pcm pcm;

#define BUF_SIZE (8192)
#define DMA_PORTION (1024)

static short buf1_unal[(BUF_SIZE / sizeof(short)) + 2]; // to make sure 4 byte aligned

rcvar_t pcm_exports[] =
{
	    RCV_END
};

/*#if CONFIG_CODEC == SWCODEC && !defined(SIMULATOR)
 * disabled cause it crashes with current audio implementation.. no sound.
 */
#if 0
static short* buf1;

static short front_buf[512];

static short* last_back_pos;

static bool newly_started;
static int turns;

void pcm_init(void)
{
    buf1 = (signed short*)((((unsigned int)buf1_unal) >> 2) << 2); /* here i just make sure that buffer is aligned to 4 bytes*/
    newly_started = true;
    last_back_pos = buf1;
    turns = 0;
    
    pcm.hz = 11025;
    pcm.stereo = 1;
    pcm.buf = front_buf;
    pcm.len = (sizeof(front_buf)) / sizeof(short); /* length in shorts, not bytes */
    pcm.pos = 0;

    
    rb->pcm_play_stop();
    rb->pcm_set_frequency(11025);
    rb->pcm_set_volume(200);
}

void pcm_close(void)
{
    memset(&pcm, 0, sizeof pcm);    
    newly_started = true;   
    last_back_pos = buf1;
    rb->pcm_play_stop();	
}

void get_more(unsigned char** start, long* size)
{	
    int length;
    unsigned int sar = (unsigned int)SAR0;    
    length = ((unsigned int)buf1) + BUF_SIZE - sar;
    
    if(turns > 0)
    {       
        newly_started = true;       
        last_back_pos = buf1;
        turns = 0;
        return;
    } /* sound will stop if no one feeds data*/

    if(length <= 0)
    {
        *start = (unsigned char*)buf1;
        *size = DMA_PORTION;
        turns++;
    }
    else
    {
        *start = (unsigned char*)sar;
        if(length > DMA_PORTION)
            *size = DMA_PORTION;
        else
            *size = length;
    }
    
}
            
int pcm_submit(void)
{
    while( (turns < 0) && ((((unsigned int)last_back_pos) + pcm.pos * sizeof(short)) > ((unsigned int)SAR0)) && !newly_started) rb->yield(); /* wait until data is passed through DAC or until exit*/
    int shorts_left = ((((unsigned int)buf1) + BUF_SIZE) - ((unsigned int)last_back_pos)) / sizeof(short);
    if( shorts_left >= pcm.pos )
    {
        memcpy(last_back_pos,pcm.buf,pcm.pos * sizeof(short));
        last_back_pos = &last_back_pos[pcm.pos];
    }
    else
    {
        int last_pos = shorts_left;
        memcpy(last_back_pos,pcm.buf,shorts_left * sizeof(short));
        last_back_pos = buf1;                
        shorts_left = pcm.pos - shorts_left;
        memcpy(last_back_pos,&pcm.buf[last_pos],shorts_left * sizeof(short));
        last_back_pos = &buf1[shorts_left];
        turns--;	
    }
    
    if(newly_started)
    {  
        rb->pcm_play_data((unsigned char*)buf1,pcm.pos * sizeof(short),&get_more);
        newly_started = false;
    }    
    
    pcm.pos = 0;
    return 1;
}
#else
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

