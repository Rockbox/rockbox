#ifdef ROCKBOX
#include "plugin.h"
#include "../../pdbox.h"
#endif

#include "../src/m_pd.h"
#include <../src/m_fixed.h>

#define LOGTEN 2.302585092994


typedef struct rmstodb_tilde
{
    t_object x_obj;
    float x_f;
} t_rmstodb_tilde;

t_class *rmstodb_tilde_class;

static void *rmstodb_tilde_new(void)
{
    t_rmstodb_tilde *x = (t_rmstodb_tilde *)pd_new(rmstodb_tilde_class);
    outlet_new(&x->x_obj, gensym("signal"));
    x->x_f = 0;
    return (x);
}

static t_int *rmstodb_tilde_perform(t_int *w)
{
    float *in = *(t_float **)(w+1), *out = *(t_float **)(w+2);
    t_int n = *(t_int *)(w+3);
    for (; n--; in++, out++)
    {
	float f = *in;
	if (f <= 0) *out = 0;
	else
	{
	    float g = 100 + 20./LOGTEN * log(f);
	    *out = (g < 0 ? 0 : g);
	}
    }
    return (w + 4);
}

static void rmstodb_tilde_dsp(t_rmstodb_tilde *x, t_signal **sp)
{
#ifdef ROCKBOX
    (void) x;
#endif
    post("warning: %s not usable yet",__FUNCTION__);
    dsp_add(rmstodb_tilde_perform, 3, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

void rmstodb_tilde_setup(void)
{
    rmstodb_tilde_class = class_new(gensym("rmstodb~"), (t_newmethod)rmstodb_tilde_new, 0,
    	sizeof(t_rmstodb_tilde), 0, 0);
    CLASS_MAINSIGNALIN(rmstodb_tilde_class, t_rmstodb_tilde, x_f);
    class_addmethod(rmstodb_tilde_class, (t_method)rmstodb_tilde_dsp, gensym("dsp"), 0);
}

