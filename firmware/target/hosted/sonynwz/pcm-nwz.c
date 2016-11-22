/* stub, will be replaced by pcm-alsa.c later */
#include "system.h"
#include "pcm.h"

#error err

void pcm_play_dma_init(void)
{
    audiohw_preinit();
}


void pcm_play_lock(void)
{
}

void pcm_play_unlock(void)
{
}

void pcm_dma_apply_settings(void)
{
}


void pcm_play_dma_pause(bool pause)
{
    (void) pause;
}


void pcm_play_dma_stop(void)
{
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    (void) addr;
    (void) size;
}

size_t pcm_get_bytes_waiting(void)
{
    return 0;
}

const void * pcm_play_dma_get_peak_buffer(int *count)
{
    (void) count;
    return NULL;
}

void pcm_play_dma_postinit(void)
{
    audiohw_postinit();
}

