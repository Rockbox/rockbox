#include "../src/m_pd.h"
#include <../src/m_fixed.h>

static t_class *sig_tilde_class;

typedef struct _sig
{
    t_object x_obj;
    t_sample x_f;
} t_sig;

static t_int *sig_tilde_perform(t_int *w)
{
    t_sample f = *(t_sample *)(w[1]);
    t_sample *out = (t_sample *)(w[2]);
    int n = (int)(w[3]);
    while (n--)
    	*out++ = f; 
    return (w+4);
}

static t_int *sig_tilde_perf8(t_int *w)
{
    t_sample f = *(t_sample *)(w[1]);
    t_sample *out = (t_sample *)(w[2]);
    int n = (int)(w[3]);
    
    for (; n; n -= 8, out += 8)
    {
    	out[0] = f;
    	out[1] = f;
    	out[2] = f;
    	out[3] = f;
    	out[4] = f;
    	out[5] = f;
    	out[6] = f;
    	out[7] = f;
    }
    return (w+4);
}


static void sig_tilde_float(t_sig *x, t_float f)
{
    x->x_f = ftofix(f);
}

static void sig_tilde_dsp(t_sig *x, t_signal **sp)
{
    dsp_add(sig_tilde_perform, 3, &x->x_f, sp[0]->s_vec, sp[0]->s_n);
}

static void *sig_tilde_new(t_floatarg f)
{
    t_sig *x = (t_sig *)pd_new(sig_tilde_class);
    x->x_f = ftofix(f);
    outlet_new(&x->x_obj, gensym("signal"));
    return (x);
}

void sig_tilde_setup(void)
{
    sig_tilde_class = class_new(gensym("sig~"), (t_newmethod)sig_tilde_new, 0,
    	sizeof(t_sig), 0, A_DEFFLOAT, 0);
    class_addfloat(sig_tilde_class, (t_method)sig_tilde_float);
    class_addmethod(sig_tilde_class, (t_method)sig_tilde_dsp, gensym("dsp"), 0);
}

