#ifdef ROCKBOX
#include "plugin.h"
#include "../../pdbox.h"
#endif

#include "../src/m_pd.h"
#include <../src/m_fixed.h>

#define LOGTEN 2.302585092994


typedef struct powtodb_tilde
{
    t_object x_obj;
    float x_f;
} t_powtodb_tilde;

t_class *powtodb_tilde_class;

static void *powtodb_tilde_new(void)
{
    t_powtodb_tilde *x = (t_powtodb_tilde *)pd_new(powtodb_tilde_class);
    outlet_new(&x->x_obj, gensym("signal"));
    x->x_f = 0;
    return (x);
}

static t_int *powtodb_tilde_perform(t_int *w)
{
    t_sample *in = *(t_sample **)(w+1), *out = *(t_sample **)(w+2);
    t_int n = *(t_int *)(w+3);
    for (; n--; in++, out++)
    {
	float f = *in;
	if (f <= 0) *out = 0;
	else
	{
	    float g = 100 + 10./LOGTEN * log(f);
	    *out = (g < 0 ? 0 : g);
	}
    }
    return (w + 4);
}

static void powtodb_tilde_dsp(t_powtodb_tilde *x, t_signal **sp)
{
#ifdef ROCKBOX
    (void) x;
#endif
    post("warning: %s not usable yet",__FUNCTION__);
    dsp_add(powtodb_tilde_perform, 3, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

void powtodb_tilde_setup(void)
{
    powtodb_tilde_class = class_new(gensym("powtodb~"), (t_newmethod)powtodb_tilde_new, 0,
    	sizeof(t_powtodb_tilde), 0, 0);
    CLASS_MAINSIGNALIN(powtodb_tilde_class, t_powtodb_tilde, x_f);
    class_addmethod(powtodb_tilde_class, (t_method)powtodb_tilde_dsp, gensym("dsp"), 0);
}

