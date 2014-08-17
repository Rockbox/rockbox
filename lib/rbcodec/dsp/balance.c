#include "balance.h"
#include "config.h"
#include "fixedpoint.h"
#include "fracmul.h"
#include "settings.h"
#include "dsp_proc_entry.h"
#include "dsp_filter.h"

static bool balance_enabled = false;
static int balance = 0;

void dsp_balance_enable(int var)
{
    struct dsp_config *dsp = dsp_get_config(CODEC_IDX_AUDIO);

    balance = var;
    balance_enabled=(var != 0)?  true:false;
    dsp_proc_enable(dsp, DSP_PROC_BALANCE, balance_enabled);
}

static void balance_process(struct dsp_proc_entry *this,
                               struct dsp_buffer **buf_p)
{
    struct dsp_buffer *buf = *buf_p;
    int count = buf->remcount;
    int32_t i;
    for (i = 0; i < count; i++)
    {
        if (balance > 0)
            buf->p32[0][i] -=  (buf->p32[0][i]/100) *  balance;
        else if (balance < 0)
            buf->p32[1][i] -=  (buf->p32[1][i]/100) *  -balance;
    }
    (void)this;
}

/* DSP message hook */
static intptr_t balance_configure(struct dsp_proc_entry *this,
                                     struct dsp_config *dsp,
                                     unsigned int setting,
                                     intptr_t value)
{
    switch (setting)
    {
    case DSP_PROC_INIT:
        if (value != 0)
            break; /* Already enabled */
        this->process = balance_process;
        dsp_proc_activate(dsp, DSP_PROC_BALANCE, true);
        break;
    case DSP_FLUSH:
        break;
    case DSP_SET_OUT_FREQUENCY:
        if (!balance_enabled)
            this->process = NULL;
        else
            this->process = balance_process;
        break;
    case DSP_PROC_CLOSE:
        break;
    }

    return 1;
    (void)dsp;
}

/* Database entry */
DSP_PROC_DB_ENTRY(
    BALANCE,
    balance_configure);
