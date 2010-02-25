#ifndef ROCKBOX
#define FIXEDPOINT
#endif

#include "../src/m_pd.h"
#include <../src/m_fixed.h>

static t_class *tabread_tilde_class;

typedef struct _tabread_tilde
{
    t_object x_obj;
    int x_npoints;
    t_sample *x_vec;
    t_symbol *x_arrayname;
    float x_f;
} t_tabread_tilde;

static void *tabread_tilde_new(t_symbol *s)
{
    t_tabread_tilde *x = (t_tabread_tilde *)pd_new(tabread_tilde_class);
    x->x_arrayname = s;
    x->x_vec = 0;
    outlet_new(&x->x_obj, gensym("signal"));
    x->x_f = 0;
    return (x);
}

static t_int *tabread_tilde_perform(t_int *w)
{
    t_tabread_tilde *x = (t_tabread_tilde *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);    
    int maxindex;
#ifdef ROCKBOX
    t_sample *buf = x->x_vec;
#else
    t_sample *buf = x->x_vec, *fp;
#endif
    int i;
    
    maxindex = x->x_npoints - 1;
    if (!buf) goto zero;

    for (i = 0; i < n; i++)
    {
	int index = ((long long) mult((*in++),ftofix(44.1)) >> fix1);
	if (index < 0)
	    index = 0;
	else if (index > maxindex)
	    index = maxindex;
	*out++ = buf[index];
    }
    return (w+5);
 zero:
    while (n--) *out++ = 0;

    return (w+5);
}

void tabread_tilde_set(t_tabread_tilde *x, t_symbol *s)
{
    t_garray *a;
    
    x->x_arrayname = s;
    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    {
    	if (*s->s_name)
    	    error("tabread~: %s: no such array", x->x_arrayname->s_name);
    	x->x_vec = 0;
    }
    else if (!garray_getfloatarray(a, &x->x_npoints, &x->x_vec))
    {
    	error("%s: bad template for tabread~", x->x_arrayname->s_name);
    	x->x_vec = 0;
    }
    else garray_usedindsp(a);
}

static void tabread_tilde_dsp(t_tabread_tilde *x, t_signal **sp)
{
    tabread_tilde_set(x, x->x_arrayname);

    dsp_add(tabread_tilde_perform, 4, x,
    	sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);

}

static void tabread_tilde_free(t_tabread_tilde *x)
{
#ifdef ROCKBOX
    (void) x;
#endif
}

void tabread_tilde_setup(void)
{
    tabread_tilde_class = class_new(gensym("tabread~"),
    	(t_newmethod)tabread_tilde_new, (t_method)tabread_tilde_free,
    	sizeof(t_tabread_tilde), 0, A_DEFSYM, 0);
    CLASS_MAINSIGNALIN(tabread_tilde_class, t_tabread_tilde, x_f);
    class_addmethod(tabread_tilde_class, (t_method)tabread_tilde_dsp,
    	gensym("dsp"), 0);
    class_addmethod(tabread_tilde_class, (t_method)tabread_tilde_set,
    	gensym("set"), A_SYMBOL, 0);
}

