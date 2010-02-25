
#include "../src/m_pd.h"
#include <../src/m_fixed.h>
#include "cos_table.h"

/* ------------------------ cos~ ----------------------------- */
#define FRAC ((1<<(fix1-ILOGCOSTABSIZE))-1)

static t_class *cos_class;

typedef struct _cos
{
    t_object x_obj;
    float x_f;
} t_cos;

static void *cos_new(void)
{
    t_cos *x = (t_cos *)pd_new(cos_class);
    outlet_new(&x->x_obj, gensym("signal"));
    x->x_f = 0;
    return (x);
}

static t_int *cos_perform(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    t_sample *out = (t_sample *)(w[2]);
    int n = (int)(w[3]);
    t_sample *tab = cos_table;
    int off;
    int frac;
    unsigned int phase;

    while (n--) {
	 phase = *in++;
	 phase &= ((1<<fix1)-1);
	 off =  fixtoi((long long)phase<<ILOGCOSTABSIZE);

	 frac = phase&(itofix(1)-1);
	 *out = mult(*(tab + off),itofix(1) - frac) + 
	      mult(*(tab + off + 1),frac);
	 out++;
    }
    return (w+4);
}

static void cos_dsp(t_cos *x, t_signal **sp)
{
#ifdef ROCKBOX
    (void) x;
#endif
    dsp_add(cos_perform, 3, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}


void cos_tilde_setup(void)
{
    cos_class = class_new(gensym("cos~"), (t_newmethod)cos_new, 0,
    	sizeof(t_cos), 0, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(cos_class, t_cos, x_f);
    class_addmethod(cos_class, (t_method)cos_dsp, gensym("dsp"), 0);
    class_sethelpsymbol(cos_class, gensym("osc~-help.pd"));
}

