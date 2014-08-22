#include "afr.h"
#include "config.h"
#include "fixedpoint.h"
#include "fracmul.h"
#include "settings.h"
#include "dsp_proc_entry.h"
#include "dsp_filter.h"


static bool afr_enabled = false;
static int strength;

/* Data for each DSP */
static struct dsp_filter afr_filters[4][CODEC_IDX_AUDIO+1] IBSS_ATTR;

static void dsp_afr_flush(int id)
{
    if (!afr_enabled)
        return;
    for(int i = 0 ;i < 4;i++)
        filter_flush(&afr_filters[i][id]);
}

static void strength_update(int var,int id,unsigned int fout)
{
    int hs=0,ls=0,drop3k=0;
    if (var == 1){hs=-29;ls= 0;drop3k=-13;}
    if (var == 2){hs=-58;ls=-5;drop3k=-29;}
    if (var == 3){hs=-72;ls=-5;drop3k=-48;}

    filter_bishelf_coefs(fp_div(8000, fout, 32),
                         fp_div(20000, fout, 32),
                         0, (hs/2), 0,
                         &afr_filters[0][id]);

    filter_pk_coefs(fp_div(8000, fout, 32), 28, hs,
                     &afr_filters[1][id]);

    filter_pk_coefs(fp_div(3150, fout, 32), 28, drop3k,
                     &afr_filters[2][id]);

    filter_bishelf_coefs(fp_div(200, fout, 32),
                         fp_div(1280, fout, 32),
                         ls, 0, 0,
                         &afr_filters[3][id]);
}

void dsp_afr_enable(int var)
{
    struct dsp_config *dsp = dsp_get_config(CODEC_IDX_AUDIO);
    unsigned int fout = dsp_get_output_frequency(dsp);
    int id = dsp_get_id(dsp);

    strength = var;
    afr_enabled=(var > 0)?  true:false;

    dsp_proc_enable(dsp, DSP_PROC_AFR, false);
    dsp_afr_flush(id);
    strength_update(strength,id,fout);

    dsp_proc_enable(dsp, DSP_PROC_AFR, afr_enabled);
}

static void afr_reduce_process(struct dsp_proc_entry *this,
                               struct dsp_buffer **buf_p)
{
    struct dsp_buffer *buf = *buf_p;

    for(int i = 0; i < 4; i++)
        filter_process(afr_filters[i], buf->p32, buf->remcount,
                   buf->format.num_channels);

    (void)this;
}

/* DSP message hook */
static intptr_t afr_configure(struct dsp_proc_entry *this,
                                     struct dsp_config *dsp,
                                     unsigned int setting,
                                     intptr_t value)
{
    unsigned int fout = dsp_get_output_frequency(dsp);
    int id = dsp_get_id(dsp);
    switch (setting)
    {
    case DSP_PROC_INIT:
        if (value != 0)
            break; /* Already enabled */
        strength_update(0,id,fout);
        this->process = afr_reduce_process;
        dsp_proc_activate(dsp, DSP_PROC_AFR, true);
        break;
    case DSP_FLUSH:
        dsp_afr_flush(id);
        break;
    case DSP_SET_OUT_FREQUENCY:
        if (!afr_enabled)
            this->process = NULL;
        else
           this->process = afr_reduce_process;
        break;
    case DSP_PROC_CLOSE:
        break;
    }

    return 1;
    (void)dsp;
}

/* Database entry */
DSP_PROC_DB_ENTRY(
    AFR,
    afr_configure);
