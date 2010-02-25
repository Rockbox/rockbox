#include "../src/m_pd.h"

/* ---------- tabread: control, non-interpolating ------------------------ */

static t_class *tabread_class;

typedef struct _tabread
{
    t_object x_obj;
    t_symbol *x_arrayname;
} t_tabread;

static void tabread_float(t_tabread *x, t_float f)
{
    t_garray *a;
    int npoints;
    t_sample *vec;

    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    	pd_error(x, "%s: no such array", x->x_arrayname->s_name);
    else if (!garray_getfloatarray(a, &npoints, &vec))
    	pd_error(x, "%s: bad template for tabread", x->x_arrayname->s_name);
    else
    {
    	int n = f;
    	if (n < 0) n = 0;
    	else if (n >= npoints) n = npoints - 1;
    	outlet_float(x->x_obj.ob_outlet, (npoints ? fixtof(vec[n]) : 0));
    }
}

static void tabread_set(t_tabread *x, t_symbol *s)
{
    x->x_arrayname = s;
}

static void *tabread_new(t_symbol *s)
{
    t_tabread *x = (t_tabread *)pd_new(tabread_class);
    x->x_arrayname = s;
    outlet_new(&x->x_obj, &s_float);
    return (x);
}

void tabread_setup(void)
{
    tabread_class = class_new(gensym("tabread"), (t_newmethod)tabread_new,
    	0, sizeof(t_tabread), 0, A_DEFSYM, 0);
    class_addfloat(tabread_class, (t_method)tabread_float);
    class_addmethod(tabread_class, (t_method)tabread_set, gensym("set"),
    	A_SYMBOL, 0);
}

