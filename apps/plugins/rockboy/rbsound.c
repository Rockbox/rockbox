#include "rockmacros.h"
#include "defs.h"
#include "pcm.h"

struct pcm pcm IBSS_ATTR;

#define N_BUFS 2
#define BUF_SIZE 2048

bool doneplay=1;
bool bufnum=0;

static unsigned short *buf=0, *hwbuf=0;

static bool newly_started;

static void get_more(const void** start, size_t* size)
{
    memcpy(hwbuf, &buf[pcm.len*doneplay], BUF_SIZE*sizeof(short));
    *start = hwbuf;
    *size = BUF_SIZE*sizeof(short);
    doneplay=1;
}

void rockboy_pcm_init(void)
{
    if(plugbuf)
        return;

    newly_started = true;

#if defined(HW_HAVE_11) && !defined(TOSHIBA_GIGABEAT_F)
    pcm.hz = SAMPR_11;
#else
    pcm.hz = SAMPR_44;
#endif

    pcm.stereo = 1;

    pcm.len = BUF_SIZE;
    if(!buf)
    {
        buf = my_malloc(pcm.len * N_BUFS *sizeof(short));
        hwbuf = my_malloc(pcm.len *sizeof(short));

        pcm.buf = buf;
        pcm.pos = 0;
        memset(buf, 0,  pcm.len * N_BUFS*sizeof(short));
    }
    rb->audio_stop();

#if INPUT_SRC_CAPS != 0
    /* Select playback */
    rb->audio_set_input_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
    rb->audio_set_output_source(AUDIO_SRC_PLAYBACK);
#endif

    rb->mixer_set_frequency(pcm.hz); /* 44100 22050 11025 */
}

void rockboy_pcm_close(void)
{
    memset(&pcm, 0, sizeof pcm);
    newly_started = true;
    rb->mixer_channel_stop(PCM_MIXER_CHAN_PLAYBACK);
    rb->mixer_set_frequency(HW_SAMPR_DEFAULT);
}

int rockboy_pcm_submit(void)
{
    if (!pcm.buf) return 0;
    if (pcm.pos < pcm.len) return 1;

    if(newly_started)
    {
        rb->mixer_channel_play_data(PCM_MIXER_CHAN_PLAYBACK, &get_more, NULL, 0);
        newly_started = false;
    }

    while (!doneplay)
    {rb->yield();}

    doneplay=0;

    pcm.pos = 0;
    return 1;
}
