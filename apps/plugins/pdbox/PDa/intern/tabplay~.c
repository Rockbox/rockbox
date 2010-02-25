
#ifndef ROCKBOX
#define FIXEDPOINT
#endif

#include "../src/m_pd.h"
#include <../src/m_fixed.h>

static t_class *tabplay_tilde_class;

typedef struct _tabplay_tilde
{
    t_object x_obj;
    t_outlet *x_bangout;
    int x_phase;
    int x_nsampsintab;
    int x_limit;
    t_sample *x_vec;
    t_symbol *x_arrayname;
    t_clock *x_clock;
} t_tabplay_tilde;

static void tabplay_tilde_tick(t_tabplay_tilde *x);

static void *tabplay_tilde_new(t_symbol *s)
{
    t_tabplay_tilde *x = (t_tabplay_tilde *)pd_new(tabplay_tilde_class);
    x->x_clock = clock_new(x, (t_method)tabplay_tilde_tick);
    x->x_phase = 0x7fffffff;
    x->x_limit = 0;
    x->x_arrayname = s;
    outlet_new(&x->x_obj, &s_signal);
    x->x_bangout = outlet_new(&x->x_obj, &s_bang);
    return (x);
}

static t_int *tabplay_tilde_perform(t_int *w)
{
    t_tabplay_tilde *x = (t_tabplay_tilde *)(w[1]);
    t_sample *out = (t_sample *)(w[2]), *fp;
    int n = (int)(w[3]), phase = x->x_phase,
    	endphase = (x->x_nsampsintab < x->x_limit ?
	    x->x_nsampsintab : x->x_limit), nxfer, n3;
    if (!x->x_vec || phase >= endphase)
    	goto zero;
    
    nxfer = endphase - phase;
    fp = x->x_vec + phase;
    if (nxfer > n)
    	nxfer = n;
    n3 = n - nxfer;
    phase += nxfer;
    while (nxfer--)
    	*out++ = *fp++;
    if (phase >= endphase)
    {
    	clock_delay(x->x_clock, 0);
    	x->x_phase = 0x7fffffff;
	while (n3--)
	    *out++ = 0;
    }
    else x->x_phase = phase;
    
    return (w+4);
zero:
    while (n--) *out++ = 0;
    return (w+4);
}

void tabplay_tilde_set(t_tabplay_tilde *x, t_symbol *s)
{
    t_garray *a;

    x->x_arrayname = s;
    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    {
    	if (*s->s_name) pd_error(x, "tabplay~: %s: no such array",
    	    x->x_arrayname->s_name);
    	x->x_vec = 0;
    }
    else if (!garray_getfloatarray(a, &x->x_nsampsintab, &x->x_vec))
    {
    	error("%s: bad template for tabplay~", x->x_arrayname->s_name);
    	x->x_vec = 0;
    }
    else garray_usedindsp(a);
}

static void tabplay_tilde_dsp(t_tabplay_tilde *x, t_signal **sp)
{
    tabplay_tilde_set(x, x->x_arrayname);
    dsp_add(tabplay_tilde_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

static void tabplay_tilde_list(t_tabplay_tilde *x, t_symbol *s,
    int argc, t_atom *argv)
{
    long start = atom_getfloatarg(0, argc, argv);
    long length = atom_getfloatarg(1, argc, argv);

#ifdef ROCKBOX
    (void) s;
#endif

    if (start < 0) start = 0;
    if (length <= 0)
    	x->x_limit = 0x7fffffff;
    else
    	x->x_limit = start + length;
    x->x_phase = start;
}

static void tabplay_tilde_stop(t_tabplay_tilde *x)
{
    x->x_phase = 0x7fffffff;
}

static void tabplay_tilde_tick(t_tabplay_tilde *x)
{
    outlet_bang(x->x_bangout);
}

static void tabplay_tilde_free(t_tabplay_tilde *x)
{
    clock_free(x->x_clock);
}

void tabplay_tilde_setup(void)
{
    tabplay_tilde_class = class_new(gensym("tabplay~"),
    	(t_newmethod)tabplay_tilde_new, (t_method)tabplay_tilde_free,
    	sizeof(t_tabplay_tilde), 0, A_DEFSYM, 0);
    class_addmethod(tabplay_tilde_class, (t_method)tabplay_tilde_dsp,
    	gensym("dsp"), 0);
    class_addmethod(tabplay_tilde_class, (t_method)tabplay_tilde_stop,
    	gensym("stop"), 0);
    class_addmethod(tabplay_tilde_class, (t_method)tabplay_tilde_set,
    	gensym("set"), A_DEFSYM, 0);
    class_addlist(tabplay_tilde_class, tabplay_tilde_list);
}

