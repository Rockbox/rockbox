
#include "../src/m_pd.h"
#include <../src/m_fixed.h>


static t_class *tabread4_tilde_class;

typedef struct _tabread4_tilde
{
    t_object x_obj;
    int x_npoints;
    t_sample *x_vec;
    t_symbol *x_arrayname;
    float x_f;
} t_tabread4_tilde;

static void *tabread4_tilde_new(t_symbol *s)
{
    t_tabread4_tilde *x = (t_tabread4_tilde *)pd_new(tabread4_tilde_class);
    x->x_arrayname = s;
    x->x_vec = 0;
    outlet_new(&x->x_obj, gensym("signal"));
    x->x_f = 0;
    return (x);
}

static t_int *tabread4_tilde_perform(t_int *w)
{
    t_tabread4_tilde *x = (t_tabread4_tilde *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);    
    int maxindex;
    t_sample *buf = x->x_vec, *fp;
    int i;
    
    maxindex = x->x_npoints - 3;

    if (!buf) goto zero;

    for (i = 0; i < n; i++)
    {
	t_time findex =  ((long long) mult((*in++),ftofix(44.1)));
	int index = fixtoi(findex);
	t_sample frac;
	//	post("%d: index %d f %lld",index,findex,*in);

	if (index < 1)
	    index = 1, frac = 0;
	else if (index > maxindex)
	    index = maxindex, frac = 1;
	else frac = findex - itofix(index);
	fp = buf + index;
	*out++ = fp[0] + mult(frac,fp[1]-fp[0]);
    }
    return (w+5);
 zero:
    while (n--) *out++ = 0;

    return (w+5);
}

void tabread4_tilde_set(t_tabread4_tilde *x, t_symbol *s)
{
    t_garray *a;
    
    x->x_arrayname = s;
    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    {
    	if (*s->s_name)
    	    error("tabread4~: %s: no such array", x->x_arrayname->s_name);
    	x->x_vec = 0;
    }
    else if (!garray_getfloatarray(a, &x->x_npoints, &x->x_vec))
    {
    	error("%s: bad template for tabread4~", x->x_arrayname->s_name);
    	x->x_vec = 0;
    }
    else garray_usedindsp(a);
}

static void tabread4_tilde_dsp(t_tabread4_tilde *x, t_signal **sp)
{
    tabread4_tilde_set(x, x->x_arrayname);

    dsp_add(tabread4_tilde_perform, 4, x,
    	sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);

}

static void tabread4_tilde_free(t_tabread4_tilde *x)
{
#ifdef ROCKBOX
    (void) x;
#endif
}

void tabread4_tilde_setup(void)
{
    tabread4_tilde_class = class_new(gensym("tabread4~"),
    	(t_newmethod)tabread4_tilde_new, (t_method)tabread4_tilde_free,
    	sizeof(t_tabread4_tilde), 0, A_DEFSYM, 0);
    CLASS_MAINSIGNALIN(tabread4_tilde_class, t_tabread4_tilde, x_f);
    class_addmethod(tabread4_tilde_class, (t_method)tabread4_tilde_dsp,
    	gensym("dsp"), 0);
    class_addmethod(tabread4_tilde_class, (t_method)tabread4_tilde_set,
    	gensym("set"), A_SYMBOL, 0);
}

