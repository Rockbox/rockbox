


#include "rockmacros.h"
#include "defs.h"
#include "pcm.h"
#include "rc.h"


struct pcm pcm;

static byte buf[4096];


rcvar_t pcm_exports[] =
{
	RCV_END
};


void pcm_init(void)
{
	pcm.hz = 44100;
	pcm.stereo = 1;
	pcm.buf = buf;
	pcm.len = sizeof buf;
	pcm.pos = 0;
}

void pcm_close(void)
{
	memset(&pcm, 0, sizeof pcm);
}

int pcm_submit(void)
{
#ifdef RBSOUND
	rb->pcm_play_data(pcm.buf,pcm.pos,NULL);
	while(rb->pcm_is_playing()); /* spinlock */
	pcm.pos = 0;
	return 1;
#else
	pcm.pos = 0;
	return 0;
#endif
}





