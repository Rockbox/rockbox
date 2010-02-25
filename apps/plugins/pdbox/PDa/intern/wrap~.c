
#include "../src/m_pd.h"
#include <../src/m_fixed.h>

typedef struct wrap
{
    t_object x_obj;
    float x_f;
} t_sigwrap;

t_class *sigwrap_class;

static void *sigwrap_new(void)
{
    t_sigwrap *x = (t_sigwrap *)pd_new(sigwrap_class);
    outlet_new(&x->x_obj, gensym("signal"));
    x->x_f = 0;
    return (x);
}

static t_int *sigwrap_perform(t_int *w)
{
    t_sample *in = *(t_sample **)(w+1), *out = *(t_sample **)(w+2);
    t_int n = *(t_int *)(w+3);
    while (n--)
    {	
	t_sample f = *in++;

#ifndef FIXEDPOINT
	int k = f;
	if (f > 0) *out++ = f-k;
	else *out++ = f - (k-1);
#else
	int k = ftofix(1.) - 1;
	*out = f&k;
#endif
    }
    return (w + 4);
}

static void sigwrap_dsp(t_sigwrap *x, t_signal **sp)
{
#ifdef ROCKBOX
    (void) x;
#endif
    dsp_add(sigwrap_perform, 3, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

void wrap_tilde_setup(void)
{
    sigwrap_class = class_new(gensym("wrap~"), (t_newmethod)sigwrap_new, 0,
    	sizeof(t_sigwrap), 0, 0);
    CLASS_MAINSIGNALIN(sigwrap_class, t_sigwrap, x_f);
    class_addmethod(sigwrap_class, (t_method)sigwrap_dsp, gensym("dsp"), 0);
}

