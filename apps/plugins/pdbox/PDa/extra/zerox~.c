#include "../src/m_pd.h"

static t_class *zerox_class;

typedef struct _zerox
{
    t_object x_obj; 
    t_sample x_f;
    t_int   x_zeros;
} t_zerox;


static t_int *zerox_perform(t_int *w)
{
    t_zerox* x = (t_zerox*)w[1];
    t_sample *in = (t_sample *)(w[2]);
    int n = (int)(w[3]) ;

    if (*in * x->x_f < 0) x->x_zeros++;
    n--;
    while (n--)
    {
    	float f = *(in++);
	x->x_zeros += f * *in < 0;
    }
    return (w+4);
}

static void zerox_dsp(t_zerox *x, t_signal **sp)
{
    dsp_add(zerox_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}


static void zerox_bang(t_zerox* x)
{
    outlet_float(x->x_obj.ob_outlet,x->x_zeros);
    x->x_zeros=0;
}

static void *zerox_new(void)
{
    t_zerox *x = (t_zerox *)pd_new(zerox_class);
    outlet_new(&x->x_obj, gensym("float"));
    x->x_f = 0;
    x->x_zeros=0;
    return (x);
}

void zerox_tilde_setup(void)
{
    zerox_class = class_new(gensym("zerox~"), (t_newmethod)zerox_new, 0,
    	sizeof(t_zerox), 0, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(zerox_class, t_zerox, x_f);
    class_addmethod(zerox_class, (t_method)zerox_dsp, gensym("dsp"), 0);
    class_addbang(zerox_class, (t_method)zerox_bang);
}

