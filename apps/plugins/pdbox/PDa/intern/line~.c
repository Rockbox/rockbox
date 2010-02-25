#include "../src/m_pd.h"
#include <../src/m_fixed.h>

static t_class *line_class;

typedef struct _line
{
    t_object x_obj;
    t_sample x_target;
    t_sample x_value;
    t_sample x_biginc;
    t_sample x_inc;
    t_sample x_1overn;
    t_sample x_msectodsptick;
    t_floatarg x_inletvalue;
    t_floatarg x_inletwas;
    int x_ticksleft;
    int x_retarget;
} t_line;

static t_int *line_perform(t_int *w)
{
    t_line *x = (t_line *)(w[1]);
    t_sample *out = (t_sample *)(w[2]);
    int n = (int)(w[3]);
#ifndef ROCKBOX
    t_sample f = x->x_value;
#endif

    if (x->x_retarget)
    {
    	int nticks = mult(ftofix(x->x_inletwas),x->x_msectodsptick);
    	if (!nticks) nticks = itofix(1);
    	x->x_ticksleft = fixtoi(nticks);
    	x->x_biginc = (x->x_target - x->x_value);
	x->x_biginc = idiv(x->x_biginc,nticks);
    	x->x_inc = mult(x->x_1overn, x->x_biginc);
    	x->x_retarget = 0;
    }
    if (x->x_ticksleft)
    {
    	t_sample f = x->x_value;
    	while (n--) *out++ = f, f += x->x_inc;
    	x->x_value += x->x_biginc;
    	x->x_ticksleft--;
    }
    else
    {
    	x->x_value = x->x_target;
    	while (n--) *out++ = x->x_value;
    }
    return (w+4);
}

static void line_float(t_line *x, t_float f)
{
    if (x->x_inletvalue <= 0)
    {
    	x->x_target = x->x_value = ftofix(f);
    	x->x_ticksleft = x->x_retarget = 0;
    }
    else
    {
    	x->x_target = ftofix(f);
    	x->x_retarget = 1;
    	x->x_inletwas = x->x_inletvalue;
    	x->x_inletvalue = 0;
    }
}

static void line_stop(t_line *x)
{
    x->x_target = x->x_value;
    x->x_ticksleft = x->x_retarget = 0;
}

static void line_dsp(t_line *x, t_signal **sp)
{
    dsp_add(line_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
    x->x_1overn = ftofix(1)/sp[0]->s_n;
    x->x_msectodsptick = idiv(sp[0]->s_sr, (1000 * sp[0]->s_n));
}

static void *line_new(void)
{
    t_line *x = (t_line *)pd_new(line_class);
    outlet_new(&x->x_obj, gensym("signal"));
    floatinlet_new(&x->x_obj, &x->x_inletvalue);
    x->x_ticksleft = x->x_retarget = 0;
    x->x_value = x->x_target = x->x_inletvalue = x->x_inletwas = 0;
    return (x);
}

void line_tilde_setup(void)
{
    line_class = class_new(gensym("line~"), line_new, 0,
    	sizeof(t_line), 0, 0);
    class_addfloat(line_class, (t_method)line_float);
    class_addmethod(line_class, (t_method)line_dsp, gensym("dsp"), 0);
    class_addmethod(line_class, (t_method)line_stop, gensym("stop"), 0);
}

