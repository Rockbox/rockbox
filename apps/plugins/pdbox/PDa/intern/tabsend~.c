
#include "../src/m_pd.h"
#include <../src/m_fixed.h>
static t_class *tabsend_class;

typedef struct _tabsend
{
    t_object x_obj;
    t_sample *x_vec;
    int x_graphperiod;
    int x_graphcount;
    t_symbol *x_arrayname;
    t_clock *x_clock;
    float x_f;
} t_tabsend;

static void tabsend_tick(t_tabsend *x);

static void *tabsend_new(t_symbol *s)
{
    t_tabsend *x = (t_tabsend *)pd_new(tabsend_class);
    x->x_graphcount = 0;
    x->x_arrayname = s;
    x->x_clock = clock_new(x, (t_method)tabsend_tick);
    x->x_f = 0;
    return (x);
}

static t_int *tabsend_perform(t_int *w)
{
    t_tabsend *x = (t_tabsend *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    int n = w[3];
    t_sample *dest = x->x_vec;
    int i = x->x_graphcount;
    if (!x->x_vec) goto bad;

    while (n--)
    {	
    	t_sample f = *in++;
    	if (PD_BADFLOAT(f))
	    f = 0;
	 *dest++ = f;
    }
    if (!i--)
    {
    	clock_delay(x->x_clock, 0);
    	i = x->x_graphperiod;
    }
    x->x_graphcount = i;
bad:
    return (w+4);
}

static void tabsend_dsp(t_tabsend *x, t_signal **sp)
{
#ifdef ROCKBOX
    int vecsize;
#else
    int i, vecsize;
#endif
    t_garray *a;

    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    {
    	if (*x->x_arrayname->s_name)
	    error("tabsend~: %s: no such array", x->x_arrayname->s_name);
    }
    else if (!garray_getfloatarray(a, &vecsize, &x->x_vec))
    	error("%s: bad template for tabsend~", x->x_arrayname->s_name);
    else
    {
    	int n = sp[0]->s_n;
    	int ticksper = sp[0]->s_sr/n;
    	if (ticksper < 1) ticksper = 1;
    	x->x_graphperiod = ticksper;
    	if (x->x_graphcount > ticksper) x->x_graphcount = ticksper;
    	if (n < vecsize) vecsize = n;
    	garray_usedindsp(a);
    	dsp_add(tabsend_perform, 3, x, sp[0]->s_vec, vecsize);
    }
}

static void tabsend_tick(t_tabsend *x)
{
    t_garray *a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class);
    if (!a) bug("tabsend_tick");
    else garray_redraw(a);
}

static void tabsend_free(t_tabsend *x)
{
    clock_free(x->x_clock);
}

void tabsend_tilde_setup(void)
{
    tabsend_class = class_new(gensym("tabsend~"), (t_newmethod)tabsend_new,
    	(t_method)tabsend_free, sizeof(t_tabsend), 0, A_DEFSYM, 0);
    CLASS_MAINSIGNALIN(tabsend_class, t_tabsend, x_f);
    class_addmethod(tabsend_class, (t_method)tabsend_dsp, gensym("dsp"), 0);
}

