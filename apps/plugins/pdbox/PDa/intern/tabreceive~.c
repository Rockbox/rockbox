#include "../src/m_pd.h"
#include <../src/m_fixed.h>

static t_class *tabreceive_class;

typedef struct _tabreceive
{
    t_object x_obj;
    t_sample *x_vec;
    t_symbol *x_arrayname;
} t_tabreceive;

static t_int *tabreceive_perform(t_int *w)
{
    t_tabreceive *x = (t_tabreceive *)(w[1]);
    t_sample *out = (t_sample *)(w[2]);
    int n = w[3];
    t_sample *from = x->x_vec;
    if (from) while (n--) *out++ = *from++;
    else while (n--) *out++ = 0;
    return (w+4);
}

static void tabreceive_dsp(t_tabreceive *x, t_signal **sp)
{
    t_garray *a;
    int vecsize;
    
    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    {
    	if (*x->x_arrayname->s_name)
    	    error("tabsend~: %s: no such array", x->x_arrayname->s_name);
    }
    else if (!garray_getfloatarray(a, &vecsize, &x->x_vec))
    	error("%s: bad template for tabreceive~", x->x_arrayname->s_name);
    else 
    {
    	int n = sp[0]->s_n;
    	if (n < vecsize) vecsize = n;
    	garray_usedindsp(a);
    	dsp_add(tabreceive_perform, 3, x, sp[0]->s_vec, vecsize);
    }
}

static void *tabreceive_new(t_symbol *s)
{
    t_tabreceive *x = (t_tabreceive *)pd_new(tabreceive_class);
    x->x_arrayname = s;
    outlet_new(&x->x_obj, &s_signal);
    return (x);
}

void tabreceive_tilde_setup(void)
{
    tabreceive_class = class_new(gensym("tabreceive~"),
    	(t_newmethod)tabreceive_new, 0,
    	sizeof(t_tabreceive), 0, A_DEFSYM, 0);
    class_addmethod(tabreceive_class, (t_method)tabreceive_dsp,
    	gensym("dsp"), 0);
}

