

#include "../src/m_pd.h"
#include <../src/m_fixed.h>
#include "cos_table.h"

/* ------------------------ osc~ ----------------------------- */

#ifdef ROCKBOX
static t_class *osc_class;
#else
static t_class *osc_class, *scalarosc_class;
#endif

typedef struct _osc
{
    t_object x_obj;
    unsigned int x_phase;
    t_sample x_conv;
    t_sample x_f;	    /* frequency if scalar */
} t_osc;

static void *osc_new(t_floatarg f)
{
    t_osc *x = (t_osc *)pd_new(osc_class);
    x->x_f = ftofix(f);
    outlet_new(&x->x_obj, gensym("signal"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft1"));
    x->x_phase = 0;
    x->x_conv = 0;
    return (x);
}


static t_int *osc_perform(t_int *w)
{
    t_osc *x = (t_osc *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    t_sample *tab = cos_table;
    unsigned int phase = x->x_phase;
    int conv = x->x_conv;
    int off;
    int frac;
    while (n--) {
	 phase+= mult(conv ,(*in++));
	 phase &= (itofix(1) -1);
	 off =  fixtoi((long long)phase<<ILOGCOSTABSIZE);

#ifdef NO_INTERPOLATION
	 *out = *(tab+off);
#else
//	 frac = phase & (itofix(1)-1);
	 frac = phase & ((1<<ILOGCOSTABSIZE)-1);
         frac <<= (fix1-ILOGCOSTABSIZE);
	 *out = mult(*(tab + off),(itofix(1) - frac)) + 
	      mult(*(tab + off + 1),frac);	 
#endif
	 out++;
    }
    x->x_phase = phase;

    return (w+5);
}

static void osc_dsp(t_osc *x, t_signal **sp)
{
	post("samplerate %f",sp[0]->s_sr);
    x->x_conv = ftofix(1000.)/sp[0]->s_sr;
     post("conf %d",x->x_conv);
    x->x_conv = mult(x->x_conv + 500,ftofix(0.001));
     post("conf %d",x->x_conv);
    dsp_add(osc_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

static void osc_ft1(t_osc *x, t_float f)
{
     x->x_phase = ftofix(f); /* *2 ??? */
}

void osc_tilde_setup(void)
{    
    osc_class = class_new(gensym("osc~"), (t_newmethod)osc_new, 0,
    	sizeof(t_osc), 0, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(osc_class, t_osc, x_f);
    class_addmethod(osc_class, (t_method)osc_dsp, gensym("dsp"), 0);
    class_addmethod(osc_class, (t_method)osc_ft1, gensym("ft1"), A_FLOAT, 0);
}

