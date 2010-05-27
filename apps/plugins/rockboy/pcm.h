
#ifndef __PCM_H__
#define __PCM_H__


#include "defs.h"

struct pcm
{
    int hz, len;
    int stereo;
    short *buf;
    int pos;
};

extern struct pcm pcm;

void rockboy_pcm_init(void);
int  rockboy_pcm_submit(void);
void rockboy_pcm_close(void);

#endif


