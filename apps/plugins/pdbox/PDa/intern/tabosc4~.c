
#include "../src/m_pd.h"
#include <../src/m_fixed.h>

static t_class *tabosc4_tilde_class;

typedef struct _tabosc4_tilde
{
    t_object x_obj;
    int x_fnpoints;
    int x_lognpoints;
    t_sample *x_vec;
    t_symbol *x_arrayname;
    t_sample x_f;
    unsigned int x_phase;
    t_sample x_conv;
} t_tabosc4_tilde;

static void *tabosc4_tilde_new(t_symbol *s)
{
    t_tabosc4_tilde *x = (t_tabosc4_tilde *)pd_new(tabosc4_tilde_class);
    x->x_arrayname = s;
    x->x_vec = 0;
    x->x_fnpoints = 512;
    outlet_new(&x->x_obj, gensym("signal"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft1"));
    x->x_f = 0;
    post("new done");
    return (x);
}

static t_int *tabosc4_tilde_perform(t_int *w)
{
    t_tabosc4_tilde *x = (t_tabosc4_tilde *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    t_sample *tab = x->x_vec;
    unsigned int phase = x->x_phase;
    int conv = x->x_conv;
    int off;
    int frac;
    int logp = x->x_lognpoints;

    if (!tab) goto zero;

    while (n--) {
        t_sample f2;
	 phase+= mult(conv ,(*in++));
	 phase &= (itofix(1) -1);
	 off =  fixtoi((long long)phase<<logp);

#ifdef NO_INTERPOLATION
	 *out = *(tab+off);
#else
	 frac = phase & ((1<<logp)-1);
         frac <<= (fix1-logp);
         
         f2 = (off == x->x_fnpoints) ? 
             mult(*(tab),frac) :
             mult(*(tab + off + 1),frac);

	 *out = mult(*(tab + off),(itofix(1) - frac)) + f2;
#endif
	 out++;
    }
    x->x_phase = phase;

    return (w+5);

zero:
    while (n--) *out++ = 0;

    return (w+5);
}

void tabosc4_tilde_set(t_tabosc4_tilde *x, t_symbol *s)
{
    t_garray *a;
#ifdef ROCKBOX
    int pointsinarray;
#else
    int npoints, pointsinarray;
#endif
    x->x_arrayname = s;

    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    {
    	if (*s->s_name)
    	    pd_error(x, "tabosc4~: %s: no such array", x->x_arrayname->s_name);
    	x->x_vec = 0;
    }
    else if (!garray_getfloatarray(a, &pointsinarray, &x->x_vec))
    {
    	pd_error(x, "%s: bad template for tabosc4~", x->x_arrayname->s_name);
    	x->x_vec = 0;
    }
    else
    {
#ifndef ROCKBOX
        int i;
#endif
    	x->x_fnpoints = pointsinarray;
	x->x_lognpoints = ilog2(pointsinarray);
	post("tabosc~: using %d (log %d) points of array",x->x_fnpoints,x->x_lognpoints);

	garray_usedindsp(a);
    }
}

static void tabosc4_tilde_ft1(t_tabosc4_tilde *x, t_float f)
{
    x->x_phase = f;
}

static void tabosc4_tilde_dsp(t_tabosc4_tilde *x, t_signal **sp)
{
    x->x_conv = ftofix(1000.)/sp[0]->s_sr;
    x->x_conv = mult(x->x_conv + 500,ftofix(0.001));

    tabosc4_tilde_set(x, x->x_arrayname);
    dsp_add(tabosc4_tilde_perform, 4, x,
    	sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

void tabosc4_tilde_setup(void)
{
    tabosc4_tilde_class = class_new(gensym("tabosc4~"),
    	(t_newmethod)tabosc4_tilde_new, 0,
    	sizeof(t_tabosc4_tilde), 0, A_DEFSYM, 0);
    CLASS_MAINSIGNALIN(tabosc4_tilde_class, t_tabosc4_tilde, x_f);
    class_addmethod(tabosc4_tilde_class, (t_method)tabosc4_tilde_dsp,
    	gensym("dsp"), 0);
    class_addmethod(tabosc4_tilde_class, (t_method)tabosc4_tilde_set,
    	gensym("set"), A_SYMBOL, 0);
    class_addmethod(tabosc4_tilde_class, (t_method)tabosc4_tilde_ft1,
    	gensym("ft1"), A_FLOAT, 0);
}

