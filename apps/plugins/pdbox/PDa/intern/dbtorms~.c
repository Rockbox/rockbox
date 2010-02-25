#ifdef ROCKBOX
#include "plugin.h"
#include "../../pdbox.h"
#endif

#include "../src/m_pd.h"
#include <../src/m_fixed.h>


#define LOGTEN 2.302585092994

typedef struct dbtorms_tilde
{
    t_object x_obj;
    float x_f;
} t_dbtorms_tilde;

t_class *dbtorms_tilde_class;

static void *dbtorms_tilde_new(void)
{
    t_dbtorms_tilde *x = (t_dbtorms_tilde *)pd_new(dbtorms_tilde_class);
    outlet_new(&x->x_obj, gensym("signal"));
    x->x_f = 0;
    return (x);
}

static t_int *dbtorms_tilde_perform(t_int *w)
{
    float *in = *(t_float **)(w+1), *out = *(t_float **)(w+2);
    t_int n = *(t_int *)(w+3);
    for (; n--; in++, out++)
    {
	float f = *in;
	if (f <= 0) *out = 0;
	else
	{
    	    if (f > 485)
		f = 485;
	    *out = exp((LOGTEN * 0.05) * (f-100.));
	}
    }
    return (w + 4);
}

static void dbtorms_tilde_dsp(t_dbtorms_tilde *x, t_signal **sp)
{
#ifdef ROCKBOX
    (void) x;
#endif
    post("warning: %s not usable yet",__FUNCTION__);
    dsp_add(dbtorms_tilde_perform, 3, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

void dbtorms_tilde_setup(void)
{
    dbtorms_tilde_class = class_new(gensym("dbtorms~"), (t_newmethod)dbtorms_tilde_new, 0,
    	sizeof(t_dbtorms_tilde), 0, 0);
    CLASS_MAINSIGNALIN(dbtorms_tilde_class, t_dbtorms_tilde, x_f);
    class_addmethod(dbtorms_tilde_class, (t_method)dbtorms_tilde_dsp, gensym("dsp"), 0);
}

