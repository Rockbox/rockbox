
#include "../src/m_pd.h"
#include <../src/m_fixed.h>

static t_class *tabwrite_tilde_class;

typedef struct _tabwrite_tilde
{
    t_object x_obj;
    int x_phase;
    int x_nsampsintab;
    t_sample *x_vec;
    t_symbol *x_arrayname;
    t_clock *x_clock;
    float x_f;
} t_tabwrite_tilde;

static void tabwrite_tilde_tick(t_tabwrite_tilde *x);

static void *tabwrite_tilde_new(t_symbol *s)
{
    t_tabwrite_tilde *x = (t_tabwrite_tilde *)pd_new(tabwrite_tilde_class);
    x->x_clock = clock_new(x, (t_method)tabwrite_tilde_tick);
    x->x_phase = 0x7fffffff;
    x->x_arrayname = s;
    x->x_f = 0;
    return (x);
}

static t_int *tabwrite_tilde_perform(t_int *w)
{
    t_tabwrite_tilde *x = (t_tabwrite_tilde *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    int n = (int)(w[3]), phase = x->x_phase, endphase = x->x_nsampsintab;
    if (!x->x_vec) goto bad;
    
    if (endphase > phase)
    {
    	int nxfer = endphase - phase;
    	t_sample *fp = x->x_vec + phase;
    	if (nxfer > n) nxfer = n;
    	phase += nxfer;
    	while (nxfer--)
	{
	    t_sample f = *in++;
	    *fp++ = f;
    	}
	if (phase >= endphase)
    	{
    	    clock_delay(x->x_clock, 0);
    	    phase = 0x7fffffff;
    	}
    	x->x_phase = phase;
    }
bad:
    return (w+4);
}

void tabwrite_tilde_set(t_tabwrite_tilde *x, t_symbol *s)
{
    t_garray *a;

    x->x_arrayname = s;
    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    {
    	if (*s->s_name) pd_error(x, "tabwrite~: %s: no such array",
    	    x->x_arrayname->s_name);
    	x->x_vec = 0;
    }
    else if (!garray_getfloatarray(a, &x->x_nsampsintab, &x->x_vec))
    {
    	error("%s: bad template for tabwrite~", x->x_arrayname->s_name);
    	x->x_vec = 0;
    }
    else garray_usedindsp(a);
}

static void tabwrite_tilde_dsp(t_tabwrite_tilde *x, t_signal **sp)
{
    tabwrite_tilde_set(x, x->x_arrayname);
    dsp_add(tabwrite_tilde_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

static void tabwrite_tilde_bang(t_tabwrite_tilde *x)
{
    x->x_phase = 0;
}

static void tabwrite_tilde_float(t_tabwrite_tilde *x,t_float n)
{
    if (n < x->x_nsampsintab)
        x->x_phase = n;
    else
        x->x_phase = 0;
}

static void tabwrite_tilde_stop(t_tabwrite_tilde *x)
{
    if (x->x_phase != 0x7fffffff)
    {
    	tabwrite_tilde_tick(x);
    	x->x_phase = 0x7fffffff;
    }
}

static void tabwrite_tilde_tick(t_tabwrite_tilde *x)
{
    t_garray *a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class);
    if (!a) bug("tabwrite_tilde_tick");
    else garray_redraw(a);
}

static void tabwrite_tilde_free(t_tabwrite_tilde *x)
{
    clock_free(x->x_clock);
}

void tabwrite_tilde_setup(void)
{
    tabwrite_tilde_class = class_new(gensym("tabwrite~"),
    	(t_newmethod)tabwrite_tilde_new, (t_method)tabwrite_tilde_free,
    	sizeof(t_tabwrite_tilde), 0, A_DEFSYM, 0);
    CLASS_MAINSIGNALIN(tabwrite_tilde_class, t_tabwrite_tilde, x_f);
    class_addmethod(tabwrite_tilde_class, (t_method)tabwrite_tilde_dsp,
    	gensym("dsp"), 0);
    class_addmethod(tabwrite_tilde_class, (t_method)tabwrite_tilde_set,
    	gensym("set"), A_SYMBOL, 0);
    class_addmethod(tabwrite_tilde_class, (t_method)tabwrite_tilde_stop,
    	gensym("stop"), 0);
    class_addbang(tabwrite_tilde_class, tabwrite_tilde_bang);
    class_addfloat(tabwrite_tilde_class, tabwrite_tilde_float);
}

