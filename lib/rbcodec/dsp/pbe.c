#include "config.h"
#include "fixedpoint.h"
#include "fracmul.h"
#include "settings.h"
#include "dsp_proc_entry.h"
#include "dsp_misc.h"
#include "dsp_filter.h"

#define b3_dly 72    /* ~800 uS */
#define b2_dly 180   /* ~2000 uS*/
#define b0_dly 276   /* ~3050 uS */

static bool pbe_enabled = false;
static int strength = 100;
static int dsp_precut=0;
static int32_t tcoef1,tcoef2,tcoef3;
static int32_t b0[2][b0_dly],b2[2][b2_dly],b3[2][b3_dly];

static int32_t temp_buffer[2][3072 * 2];

static unsigned int fout;
static int id;

/* Data for each DSP */
static struct dsp_filter pbe_filters[DSP_COUNT] IBSS_ATTR;

static void dsp_pbe_flush(void)
{
    if (!pbe_enabled)
        return;

    memset(b0[0],0,b0_dly * sizeof(int32_t)); 
    memset(b0[1],0,b0_dly * sizeof(int32_t));
    memset(b2[0],0,b2_dly * sizeof(int32_t));
    memset(b2[1],0,b2_dly * sizeof(int32_t));
    memset(b3[0],0,b3_dly * sizeof(int32_t)); 
    memset(b3[1],0,b3_dly * sizeof(int32_t));  
}

static void pbe_update_filter(unsigned int fout)
{
    tcoef1 = fp_div(160, fout, 31);
    tcoef2 = fp_div(500, fout, 31);
    tcoef3 = fp_div(1150, fout, 31);

    memset(b0[0],0,b0_dly * sizeof(int32_t)); 
    memset(b0[1],0,b0_dly * sizeof(int32_t));
    memset(b2[0],0,b2_dly * sizeof(int32_t));
    memset(b2[1],0,b2_dly * sizeof(int32_t));
    memset(b3[0],0,b3_dly * sizeof(int32_t)); 
    memset(b3[1],0,b3_dly * sizeof(int32_t));

    filter_bishelf_coefs(fp_div(200, fout, 32),
                         fp_div(1280, fout, 32),
                         50, 30, -5+dsp_precut,
                         &pbe_filters[id]);
 
}

void dsp_pbe_precut(int var)
{
    dsp_precut = var;
    filter_flush(&pbe_filters[id]);

    filter_bishelf_coefs(fp_div(200, fout, 32),
                         fp_div(1280, fout, 32),
                         50, 30, -5+dsp_precut,
                         &pbe_filters[id]);
}

void dsp_pbe_enable(int var)
{
    struct dsp_config *dsp = dsp_get_config(CODEC_IDX_AUDIO);
    bool prev_pbe_enabled = pbe_enabled;
    int pre_strength = strength;
    strength = var;
    if (pre_strength != strength)
    {
        dsp_proc_enable(dsp, DSP_PROC_PBE, false); 
        dsp_pbe_flush();
        dsp_proc_enable(dsp, DSP_PROC_PBE, pbe_enabled);
    }
    pbe_enabled=(var > 0)?  true:false;

    if (prev_pbe_enabled == pbe_enabled)
        return; /* No change */
    dsp_proc_enable(dsp, DSP_PROC_PBE, pbe_enabled);
}



static void pbe_process(struct dsp_proc_entry *this,
                               struct dsp_buffer **buf_p)
{

    (void)this;
    int32_t i;
    struct dsp_buffer *buf = *buf_p;
    int count = buf->remcount;
    int b2_level = (b2_dly * strength) / 100;
    int b0_level = (b0_dly * strength) / 100;

    if (count < b0_dly )
    {
        dsp_pbe_flush(); 
        return;  
    }

    /* add 160hz - 500hz no delay */
    for (i = 0; i < count; i++)
    {
        temp_buffer[0][i]= FRACMUL(buf->p32[0][i], tcoef1) - FRACMUL(buf->p32[0][i], tcoef2);
        temp_buffer[1][i]= FRACMUL(buf->p32[1][i], tcoef1) - FRACMUL(buf->p32[1][i], tcoef2);
    }

    /* filter below 160 and with delays */
    for (i = 0; i < b0_level; i++)
    {
         temp_buffer[0][i] +=b0[0][i];
         temp_buffer[1][i] +=b0[1][i];
    }
    for (i = 0; i < count-b0_level; i++)
    {
         temp_buffer[0][i+b0_level] += buf->p32[0][i] - FRACMUL(buf->p32[0][i], tcoef1);
         temp_buffer[1][i+b0_level] += buf->p32[1][i] - FRACMUL(buf->p32[1][i], tcoef1);
    }
    for (i = 0; i < b0_level; i++)
    {
         b0[0][i] = buf->p32[0][count-b0_level+i] - FRACMUL(buf->p32[0][count-b0_level+i], tcoef1);
         b0[1][i] = buf->p32[1][count-b0_level+i] - FRACMUL(buf->p32[1][count-b0_level+i], tcoef1);
    }

    /* add 500-1150hz with delays */
    for (i = 0; i < b2_level; i++)
    {
        temp_buffer[0][i] +=b2[0][i];
        temp_buffer[1][i] +=b2[1][i];
    }
    for (i = 0; i < count-b2_level; i++)
    {
        temp_buffer[0][i+b2_level]+= FRACMUL(buf->p32[0][i], tcoef2) - FRACMUL(buf->p32[0][i], tcoef3);
        temp_buffer[1][i+b2_level]+= FRACMUL(buf->p32[1][i], tcoef2) - FRACMUL(buf->p32[1][i], tcoef3);
    } 
    for (i = 0; i < b2_level; i++)
    {
         b2[0][i] = FRACMUL(buf->p32[0][count-b2_level+i], tcoef2) - FRACMUL(buf->p32[0][count-b2_level+i], tcoef3);
         b2[1][i] = FRACMUL(buf->p32[1][count-b2_level+i], tcoef2) - FRACMUL(buf->p32[1][count-b2_level+i], tcoef3);
    }

    /* add above 1150 with delays */
    for (i = 0; i < b3_dly; i++)
    {
        temp_buffer[0][i] +=b3[0][i];
        temp_buffer[1][i] +=b3[1][i];
    }
    for (i = 0; i < count-b3_dly; i++)
    {
        temp_buffer[0][i+b3_dly]+= FRACMUL(buf->p32[0][i], tcoef3);
        temp_buffer[1][i+b3_dly]+= FRACMUL(buf->p32[1][i], tcoef3);
    } 
    for (i = 0; i < b3_dly; i++)
    {
        b3[0][i] = FRACMUL(buf->p32[0][count-b3_dly+i], tcoef3);
        b3[1][i] = FRACMUL(buf->p32[1][count-b3_dly+i], tcoef3);
    }

    /* end of repair */
    memcpy(buf->p32[0],temp_buffer[0],count * sizeof(int32_t));
    memcpy(buf->p32[1],temp_buffer[1],count * sizeof(int32_t));

    /* bishelf boost */
    filter_process(pbe_filters, buf->p32, buf->remcount,
                   buf->format.num_channels);

}

/* DSP message hook */
static intptr_t pbe_configure(struct dsp_proc_entry *this,
                                     struct dsp_config *dsp,
                                     unsigned int setting,
                                     intptr_t value)
{
    fout = dsp_get_output_frequency(dsp);
    id = dsp_get_id(dsp);
    switch (setting)
    {
    case DSP_PROC_INIT:
        if (value != 0)
            break; /* Already enabled */
        this->process = pbe_process;
        pbe_update_filter(fout);
        dsp_proc_activate(dsp, DSP_PROC_PBE, true);
        break;
    case DSP_FLUSH:
        dsp_pbe_flush();
        break;
    case DSP_SET_OUT_FREQUENCY:
        if (!pbe_enabled)
           this->process =NULL;
        else
           this->process = pbe_process; 
        break; 
    case DSP_PROC_CLOSE:
        break;
    }

    return 1;
    (void)dsp;
}

/* Database entry */
DSP_PROC_DB_ENTRY(
    PBE,
    pbe_configure);
