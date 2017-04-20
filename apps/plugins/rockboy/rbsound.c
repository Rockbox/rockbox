#include "rockmacros.h"
#include "defs.h"
#include "pcm.h"

struct pcm pcm IBSS_ATTR;

#define N_BUFS 2
#define BUF_COUNT 1024
#define SAMPLE_SIZE (2*sizeof(int16_t))

#if CONFIG_CODEC == SWCODEC
static pcm_handle_t pcm_handle;

static volatile int doneplay=1;

static int16_t (*buf)[BUF_COUNT*2];
static int16_t *hwbuf;

static bool newly_started;

static int get_more(int status, const void** start, unsigned long* frames)
{
    if (status < 0)
        return status;

    memcpy(hwbuf, buf[doneplay], BUF_COUNT*SAMPLE_SIZE);
    *start = hwbuf;
    *frames = BUF_COUNT;
    doneplay=1;

    return 0;
}

void rockboy_pcm_init(void)
{
    if(plugbuf)
        return;

    if (!pcm_handle)
        rb->pcm_open(&pcm_handle, PCM_STREAM_PLAYBACK);

    newly_started = true;

#if defined(HW_HAVE_11) && !defined(TOSHIBA_GIGABEAT_F)
    pcm.hz = SAMPR_11;
#else
    pcm.hz = SAMPR_44;
#endif

    pcm.stereo = 1;

    pcm.len = BUF_COUNT;
    if(!buf)
    {
        buf = my_malloc(pcm.len * N_BUFS * SAMPLE_SIZE);
        hwbuf = my_malloc(pcm.len * SAMPLE_SIZE);

        pcm.buf = buf[0];
        pcm.pos = 0;
        memset(buf, 0,  pcm.len * N_BUFS * SAMPLE_SIZE);
    }

#if INPUT_SRC_CAPS != 0
    /* Select playback */
    rb->audio_set_input_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
    rb->audio_set_output_source(AUDIO_SRC_PLAYBACK);
#endif
   
    rb->pcm_set_frequency(pcm_handle, pcm.hz); /* 44100 22050 11025 */
}

void rockboy_pcm_close(void)
{
    memset(&pcm, 0, sizeof pcm);    
    newly_started = true;
    rb->pcm_set_frequency(pcm_handle, HW_SAMPR_RESET);
    rb->pcm_close(pcm_handle);
    pcm_handle = 0;
}

int rockboy_pcm_submit(void)
{
    if (!pcm.buf) return 0;
    if (pcm.pos < pcm.len) return 1;

    if(newly_started)
    {
        rb->pcm_play_data(pcm_handle, get_more, NULL, 0,
                          PCM_FORMAT(PCM_FORMAT_S16_2, 2));
        newly_started = false;
    }

    while (!doneplay)
    {rb->yield();}

    doneplay=0;

    pcm.pos = 0;
    return 1;
}

#else

void rockboy_pcm_init(void)
{
    pcm.hz = 44100;
    pcm.stereo = 1;
    pcm.buf = NULL;
    pcm.len = 0;
    pcm.pos = 0;
}

void rockboy_pcm_close(void)
{
    memset(&pcm, 0, sizeof pcm);
}

int rockboy_pcm_submit(void)
{
    pcm.pos =0;
    return 0;
}

#endif
