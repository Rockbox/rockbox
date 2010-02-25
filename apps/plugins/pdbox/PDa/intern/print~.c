#include "../src/m_pd.h"
#include <../src/m_fixed.h>

static t_class *print_class;

typedef struct _print
{
    t_object x_obj;
    float x_f;
    t_symbol *x_sym;
    int x_count;
} t_print;

static t_int *print_perform(t_int *w)
{
    t_print *x = (t_print *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    int n = (int)(w[3]);
    if (x->x_count)
    {
    	post("%s:", x->x_sym->s_name);
    	if (n == 1) post("%8g", in[0]);
    	else if (n == 2) post("%8g %8g", in[0], in[1]);
    	else if (n == 4) post("%8g %8g %8g %8g",
    	    in[0], in[1], in[2], in[3]);
    	else while (n > 0)
    	{
    	    post("%-8.5g %-8.5g %-8.5g %-8.5g %-8.5g %-8.5g %-8.5g %-8.5g",
    	    	fixtof(in[0]), 
		 fixtof(in[1]), 
		 fixtof(in[2]), 
		 fixtof(in[3]), 
		 fixtof(in[4]), 
		 fixtof(in[5]), 
		 fixtof(in[6]), 
		 fixtof(in[7]));
    	    n -= 8;
    	    in += 8;
    	}
    	x->x_count--;
    }
    return (w+4);
}

static void print_dsp(t_print *x, t_signal **sp)
{
    dsp_add(print_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

static void print_float(t_print *x, t_float f)
{
    if (f < 0) f = 0;
    x->x_count = f;
}

static void print_bang(t_print *x)
{
    x->x_count = 1;
}

static void *print_new(t_symbol *s)
{
    t_print *x = (t_print *)pd_new(print_class);
    x->x_sym = (s->s_name[0]? s : gensym("print~"));
    x->x_count = 0;
    x->x_f = 0;
    return (x);
}

void print_tilde_setup(void)
{
    print_class = class_new(gensym("print~"), (t_newmethod)print_new, 0,
    	sizeof(t_print), 0, A_DEFSYM, 0);
    CLASS_MAINSIGNALIN(print_class, t_print, x_f);
    class_addmethod(print_class, (t_method)print_dsp, gensym("dsp"), 0);
    class_addbang(print_class, print_bang);
    class_addfloat(print_class, print_float);
}

