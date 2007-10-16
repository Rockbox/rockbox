
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

void pcm_init(void);
int  pcm_submit(void);
void pcm_close(void);

#endif


