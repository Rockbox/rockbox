#include "../src/m_pd.h"
/* ------------------ tabwrite: control ------------------------ */

static t_class *tabwrite_class;

typedef struct _tabwrite
{
    t_object x_obj;
    t_symbol *x_arrayname;
    t_clock *x_clock;
    float x_ft1;
    double x_updtime;
    int x_set;
} t_tabwrite;

static void tabwrite_tick(t_tabwrite *x)
{
    t_garray *a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class);
    if (!a) bug("tabwrite_tick");
    else garray_redraw(a);
    x->x_set = 0;
    x->x_updtime = clock_getsystime();
}

static void tabwrite_float(t_tabwrite *x, t_float f)
{
#ifdef ROCKBOX
    int vecsize;
#else
    int i, vecsize;
#endif
    t_garray *a;
    t_sample *vec;

    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    	pd_error(x, "%s: no such array", x->x_arrayname->s_name);
    else if (!garray_getfloatarray(a, &vecsize, &vec))
    	pd_error(x, "%s: bad template for tabwrite", x->x_arrayname->s_name);
    else
    {
    	int n = x->x_ft1;
    	double timesince = clock_gettimesince(x->x_updtime);
    	if (n < 0) n = 0;
    	else if (n >= vecsize) n = vecsize-1;
    	vec[n] = ftofix(f);
    	if (timesince > 1000)
    	{
    	    tabwrite_tick(x);
    	}
    	else
    	{
    	    if (x->x_set == 0)
    	    {
    	    	clock_delay(x->x_clock, 1000 - timesince);
    	    	x->x_set = 1;
    	    }
    	}
    }
}

static void tabwrite_set(t_tabwrite *x, t_symbol *s)
{
    x->x_arrayname = s;
}

static void tabwrite_free(t_tabwrite *x)
{
    clock_free(x->x_clock);
}

static void *tabwrite_new(t_symbol *s)
{
    t_tabwrite *x = (t_tabwrite *)pd_new(tabwrite_class);
    x->x_ft1 = 0;
    x->x_arrayname = s;
    x->x_updtime = clock_getsystime();
    x->x_clock = clock_new(x, (t_method)tabwrite_tick);
    floatinlet_new(&x->x_obj, &x->x_ft1);
    return (x);
}

void tabwrite_setup(void)
{
    tabwrite_class = class_new(gensym("tabwrite"), (t_newmethod)tabwrite_new,
    	(t_method)tabwrite_free, sizeof(t_tabwrite), 0, A_DEFSYM, 0);
    class_addfloat(tabwrite_class, (t_method)tabwrite_float);
    class_addmethod(tabwrite_class, (t_method)tabwrite_set, gensym("set"), A_SYMBOL, 0);
}

