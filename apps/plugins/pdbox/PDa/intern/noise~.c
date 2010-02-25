#include "../src/m_pd.h"
#include <../src/m_fixed.h>


static t_class *noise_class;

typedef struct _noise
{
    t_object x_obj;
    int x_val;
} t_noise;

static void *noise_new(void)
{
    t_noise *x = (t_noise *)pd_new(noise_class);
    static int init = 307;
    x->x_val = (init *= 1319); 
    outlet_new(&x->x_obj, gensym("signal"));
    return (x);
}

static t_int *noise_perform(t_int *w)
{
    t_sample *out = (t_sample *)(w[1]);
    int *vp = (int *)(w[2]);
    int n = (int)(w[3]);
    int val = *vp;
    while (n--)
    {
#ifndef FIXEDPOINT
	 *out++ = ((t_sample)((val & 0x7fffffff) - 0x40000000)) *
	     (t_sample)(1.0 / 0x40000000);
#else
	 *out++=val>>(32-fix1);
#endif
	 val = val * 435898247 + 382842987;

    }
    *vp = val;
    return (w+4);
}

static void noise_dsp(t_noise *x, t_signal **sp)
{
    dsp_add(noise_perform, 3, sp[0]->s_vec, &x->x_val, sp[0]->s_n);
}

void noise_tilde_setup(void)
{
    noise_class = class_new(gensym("noise~"), (t_newmethod)noise_new, 0,
    	sizeof(t_noise), 0, 0);
    class_addmethod(noise_class, (t_method)noise_dsp, gensym("dsp"), 0);
}

