#include "../src/m_pd.h"
#include <../src/m_fixed.h>

typedef struct hipctl
{
    t_sample c_x;
    t_sample c_coef;
} t_hipctl;

typedef struct sighip
{
    t_object x_obj;
    float x_sr;
    float x_hz;
    t_hipctl x_cspace;
    t_hipctl *x_ctl;
    float x_f;
} t_sighip;

t_class *sighip_class;
static void sighip_ft1(t_sighip *x, t_floatarg f);

static void *sighip_new(t_floatarg f)
{
    t_sighip *x = (t_sighip *)pd_new(sighip_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("ft1"));
    outlet_new(&x->x_obj, gensym("signal"));
    x->x_sr = 44100;
    x->x_ctl = &x->x_cspace;
    x->x_cspace.c_x = 0;
    sighip_ft1(x, f);
    x->x_f = 0;
    return (x);
}

static void sighip_ft1(t_sighip *x, t_floatarg f)
{
    t_float coeff;
    if (f < 0.001) f = 10;
    x->x_hz = f;
    coeff = 1 - f * (2 * 3.14159) / x->x_sr;
    if (coeff < 0) coeff = 0;
    x->x_ctl->c_coef = ftofix(coeff);
}

static t_int *sighip_perform(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    t_sample *out = (t_sample *)(w[2]);
    t_hipctl *c = (t_hipctl *)(w[3]);
    int n = (t_int)(w[4]);
    int i;
    t_sample last = c->c_x;
    t_sample coef = c->c_coef;
    for (i = 0; i < n; i++)
    {
	t_sample new = *in++ + mult(coef,last);
	*out++ = new - last;
	last = new;
    }
    if (PD_BADFLOAT(last))
	last = 0; 
    c->c_x = last;
    return (w+5);
}

static void sighip_dsp(t_sighip *x, t_signal **sp)
{
    x->x_sr = sp[0]->s_sr;
    sighip_ft1(x,  x->x_hz);
    dsp_add(sighip_perform, 4,
	sp[0]->s_vec, sp[1]->s_vec, 
	    x->x_ctl, sp[0]->s_n);

}

static void sighip_clear(t_sighip *x, t_floatarg q)
{
#ifdef ROCKBOX
    (void) q;
#endif
    x->x_cspace.c_x = 0;
}

void hip_tilde_setup(void)
{
    sighip_class = class_new(gensym("hip~"), (t_newmethod)sighip_new, 0,
	sizeof(t_sighip), 0, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(sighip_class, t_sighip, x_f);
    class_addmethod(sighip_class, (t_method)sighip_dsp, gensym("dsp"), 0);
    class_addmethod(sighip_class, (t_method)sighip_ft1,
    	gensym("ft1"), A_FLOAT, 0);
    class_addmethod(sighip_class, (t_method)sighip_clear, gensym("clear"), 0);
    class_sethelpsymbol(sighip_class, gensym("lop~-help.pd"));
}

