#include "../src/m_pd.h"
#include <../src/m_fixed.h>

typedef struct lopctl
{
    t_sample c_x;
    t_sample c_coef;
} t_lopctl;

typedef struct siglop
{
    t_object x_obj;
    float x_sr;
    float x_hz;
    t_lopctl x_cspace;
    t_lopctl *x_ctl;
    float x_f;
} t_siglop;

t_class *siglop_class;

static void siglop_ft1(t_siglop *x, t_floatarg f);

static void *siglop_new(t_floatarg f)
{
    t_siglop *x = (t_siglop *)pd_new(siglop_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("ft1"));
    outlet_new(&x->x_obj, gensym("signal"));
    x->x_sr = 44100;
    x->x_ctl = &x->x_cspace;
    x->x_cspace.c_x = 0;
    siglop_ft1(x, f);
    x->x_f = 0;
    return (x);
}

static void siglop_ft1(t_siglop *x, t_floatarg f)
{
     t_float coeff;
    if (f < 0.001) f = 10;
    x->x_hz = f;
    coeff = f * (2 * 3.14159) / x->x_sr;
    if (coeff > 1) coeff = 1;
    x->x_ctl->c_coef = ftofix(coeff);
}

static void siglop_clear(t_siglop *x, t_floatarg q)
{
#ifdef ROCKBOX
    (void) q;
#endif
    x->x_cspace.c_x = 0;
}


static t_int *siglop_perform(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    t_sample *out = (t_sample *)(w[2]);
    t_lopctl *c = (t_lopctl *)(w[3]);
    int n = (t_int)(w[4]);
    int i;
    t_sample last = c->c_x;
    t_sample coef = c->c_coef;
    t_sample feedback = ftofix(1) - coef;
    for (i = 0; i < n; i++)
	last = *out++ = mult(coef, *in++) + mult(feedback,last);
    if (PD_BADFLOAT(last))
    	last = 0;
    c->c_x = last;
    return (w+5);
}

static void siglop_dsp(t_siglop *x, t_signal **sp)
{
    x->x_sr = sp[0]->s_sr;
    siglop_ft1(x,  x->x_hz);
    dsp_add(siglop_perform, 4,
	sp[0]->s_vec, sp[1]->s_vec, 
	    x->x_ctl, sp[0]->s_n);

}

void lop_tilde_setup(void)
{
    siglop_class = class_new(gensym("lop~"), (t_newmethod)siglop_new, 0,
	sizeof(t_siglop), 0, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(siglop_class, t_siglop, x_f);
    class_addmethod(siglop_class, (t_method)siglop_dsp, gensym("dsp"), 0);
    class_addmethod(siglop_class, (t_method)siglop_ft1,
    	gensym("ft1"), A_FLOAT, 0);
    class_addmethod(siglop_class, (t_method)siglop_clear, gensym("clear"), 0);
}

