#include "../src/m_pd.h"
#include <../src/m_fixed.h>

typedef struct bpctl
{
    t_sample c_x1;
    t_sample c_x2;
    t_sample c_coef1;
    t_sample c_coef2;
    t_sample c_gain;
} t_bpctl;

typedef struct sigbp
{
    t_object x_obj;
    float x_sr;
    float x_freq;
    float x_q;
    t_bpctl x_cspace;
    t_bpctl *x_ctl;
    float x_f;
} t_sigbp;

t_class *sigbp_class;

static void sigbp_docoef(t_sigbp *x, t_floatarg f, t_floatarg q);

static void *sigbp_new(t_floatarg f, t_floatarg q)
{
    t_sigbp *x = (t_sigbp *)pd_new(sigbp_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("ft1"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("ft2"));
    outlet_new(&x->x_obj, gensym("signal"));
    x->x_sr = 44100;
    x->x_ctl = &x->x_cspace;
    x->x_cspace.c_x1 = 0;
    x->x_cspace.c_x2 = 0;
    sigbp_docoef(x, f, q);
    x->x_f = 0;
    return (x);
}

static float sigbp_qcos(float f)
{
    if (f >= -(0.5f*3.14159f) && f <= 0.5f*3.14159f)
    {
    	float g = f*f;
    	return (((g*g*g * (-1.0f/720.0f) + g*g*(1.0f/24.0f)) - g*0.5) + 1);
    }
    else return (0);
}

static void sigbp_docoef(t_sigbp *x, t_floatarg f, t_floatarg q)
{
    float r, oneminusr, omega;
    if (f < 0.001) f = 10;
    if (q < 0) q = 0;
    x->x_freq = f;
    x->x_q = q;
    omega = f * (2.0f * 3.14159f) / x->x_sr;
    if (q < 0.001) oneminusr = 1.0f;
    else oneminusr = omega/q;
    if (oneminusr > 1.0f) oneminusr = 1.0f;
    r = 1.0f - oneminusr;
    x->x_ctl->c_coef1 = ftofix(2.0f * sigbp_qcos(omega) * r);
    x->x_ctl->c_coef2 = ftofix(- r * r);
    x->x_ctl->c_gain = ftofix(2 * oneminusr * (oneminusr + r * omega));
    /* post("r %f, omega %f, coef1 %f, coef2 %f",
    	r, omega, x->x_ctl->c_coef1, x->x_ctl->c_coef2); */
}

static void sigbp_ft1(t_sigbp *x, t_floatarg f)
{
    sigbp_docoef(x, f, x->x_q);
}

static void sigbp_ft2(t_sigbp *x, t_floatarg q)
{
    sigbp_docoef(x, x->x_freq, q);
}

static void sigbp_clear(t_sigbp *x, t_floatarg q)
{
#ifdef ROCKBOX
    (void) q;
#endif
    x->x_ctl->c_x1 = x->x_ctl->c_x2 = 0;
}

static t_int *sigbp_perform(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    t_sample *out = (t_sample *)(w[2]);
    t_bpctl *c = (t_bpctl *)(w[3]);
    int n = (t_int)(w[4]);
    int i;
    t_sample last = c->c_x1;
    t_sample prev = c->c_x2;
    t_sample coef1 = c->c_coef1;
    t_sample coef2 = c->c_coef2;
    t_sample gain = c->c_gain;
    for (i = 0; i < n; i++)
    {
    	t_sample output =  *in++ + mult(coef1,last) + mult(coef2,prev);
    	*out++ = mult(gain,output);
	prev = last;
	last = output;
    }
    if (PD_BADFLOAT(last))
    	last = 0;
    if (PD_BADFLOAT(prev))
    	prev = 0;
    c->c_x1 = last;
    c->c_x2 = prev;
    return (w+5);
}

static void sigbp_dsp(t_sigbp *x, t_signal **sp)
{
    x->x_sr = sp[0]->s_sr;
    sigbp_docoef(x, x->x_freq, x->x_q);
    dsp_add(sigbp_perform, 4,
	sp[0]->s_vec, sp[1]->s_vec, 
	    x->x_ctl, sp[0]->s_n);

}

void bp_tilde_setup(void)
{
    sigbp_class = class_new(gensym("bp~"), (t_newmethod)sigbp_new, 0,
	sizeof(t_sigbp), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(sigbp_class, t_sigbp, x_f);
    class_addmethod(sigbp_class, (t_method)sigbp_dsp, gensym("dsp"), 0);
    class_addmethod(sigbp_class, (t_method)sigbp_ft1,
    	gensym("ft1"), A_FLOAT, 0);
    class_addmethod(sigbp_class, (t_method)sigbp_ft2,
    	gensym("ft2"), A_FLOAT, 0);
    class_addmethod(sigbp_class, (t_method)sigbp_clear, gensym("clear"), 0);
    class_sethelpsymbol(sigbp_class, gensym("lop~-help.pd"));
}

