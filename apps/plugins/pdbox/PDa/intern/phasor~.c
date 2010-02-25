
#include "../src/m_pd.h"
#include <../src/m_fixed.h>

/* -------------------------- phasor~ ------------------------------ */
static t_class *phasor_class;

typedef struct _phasor
{
    t_object x_obj;
    unsigned int x_phase;
    t_sample x_conv;
    t_sample x_f;	    /* scalar frequency */
} t_phasor;

static void *phasor_new(t_floatarg f)
{
    t_phasor *x = (t_phasor *)pd_new(phasor_class);
    x->x_f = ftofix(f);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft1"));
    x->x_phase = 0;
    x->x_conv = 0;
    outlet_new(&x->x_obj, gensym("signal"));
    return (x);
}

static t_int *phasor_perform(t_int *w)
{
    t_phasor *x = (t_phasor *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    unsigned int phase = x->x_phase;
    int conv = x->x_conv;

    while (n--) {
	 phase+= mult(conv , (*in++));
	 phase &= (itofix(1) - 1);
	 *out++ = phase;
    }
    x->x_phase = phase;

    return (w+5);
}

static void phasor_dsp(t_phasor *x, t_signal **sp)
{
    x->x_conv = ftofix(1000.)/sp[0]->s_sr;
    x->x_conv = mult(x->x_conv + 500,ftofix(0.001));
    dsp_add(phasor_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}


static void phasor_ft1(t_phasor *x, t_float f)
{
    x->x_phase = ftofix(f);
}

void phasor_tilde_setup(void)
{
    phasor_class = class_new(gensym("phasor~"), (t_newmethod)phasor_new, 0,
    	sizeof(t_phasor), 0, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(phasor_class, t_phasor, x_f);
    class_addmethod(phasor_class, (t_method)phasor_dsp, gensym("dsp"), 0);
    class_addmethod(phasor_class, (t_method)phasor_ft1,
    	gensym("ft1"), A_FLOAT, 0);
    class_sethelpsymbol(phasor_class, gensym("osc~-help.pd"));
}

