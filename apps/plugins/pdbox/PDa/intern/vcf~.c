#include "../src/m_pd.h"
#include <../src/m_fixed.h>
#include "cos_table.h"

/* ---------------- vcf~ - 2-pole bandpass filter. ----------------- */
/* GG: complex resonator with signal frequency control 
   this time using the bigger cos_table without interpolation 
   really have to switch to a separate fixpoint format sometime 
*/

typedef struct vcfctl
{
    t_sample c_re;
    t_sample c_im;
    t_sample c_q;
    t_sample c_isr;
} t_vcfctl;

typedef struct sigvcf
{
    t_object x_obj;
    t_vcfctl x_cspace;
    t_vcfctl *x_ctl;
    float x_f;
} t_sigvcf;

t_class *sigvcf_class;

static void *sigvcf_new(t_floatarg q)
{
    t_sigvcf *x = (t_sigvcf *)pd_new(sigvcf_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("ft1"));
    outlet_new(&x->x_obj, gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
    x->x_ctl = &x->x_cspace;
    x->x_cspace.c_re = 0;
    x->x_cspace.c_im = 0;
    x->x_cspace.c_q = ftofix(q);
    x->x_cspace.c_isr = 0;
    x->x_f = 0;
    return (x);
}

static void sigvcf_ft1(t_sigvcf *x, t_floatarg f)
{
    x->x_ctl->c_q = (f > 0 ? ftofix(f) : 0);
}

static t_int *sigvcf_perform(t_int *w)
{
    t_sample *in1 = (t_sample *)(w[1]);
    t_sample *in2 = (t_sample *)(w[2]);
    t_sample *out1 = (t_sample *)(w[3]);
    t_sample *out2 = (t_sample *)(w[4]);
    t_vcfctl *c = (t_vcfctl *)(w[5]);
    int n = (t_int)(w[6]);
    int i;
    t_sample re = c->c_re, re2;
    t_sample im = c->c_im;
    t_sample q = c->c_q;
    t_sample qinv = (q > 0 ? idiv(ftofix(1.0),q) : 0);
    t_sample ampcorrect = ftofix(2.0f) - idiv(ftofix(2.0f) , (q + ftofix(2.0f)));
    t_sample isr = c->c_isr;
    t_sample coefr, coefi;
    t_sample *tab = cos_table;
    	t_sample oneminusr,cfindx,cf,r;

    for (i = 0; i < n; i++)
    {
    	cf = mult(*in2++,isr);
    	if (cf < 0) cf = 0;
        cfindx = mult(cf,ftofix(0.15915494))>>(fix1-ILOGCOSTABSIZE); /* 1/2*PI */
    	r = (qinv > 0 ? ftofix(1.01) - mult(cf,qinv) : 0);
      
    	if (r < 0) r = 0;
    	oneminusr = ftofix(1.02f) - r; /* hand adapted */

        /* r*cos(cf) */
	coefr = mult(r,tab[cfindx]);

        /* r*sin(cf) */
        cfindx-=(ICOSTABSIZE>>2);
        cfindx += cfindx < 0 ? ICOSTABSIZE:0;
	coefi = mult(r,tab[cfindx]);

    	re2 = re;
    	*out1++ = re = mult(ampcorrect,mult(oneminusr,*in1++)) 
    	    + mult(coefr,re2) - mult(coefi, im);
    	*out2++ = im = mult(coefi,re2) + mult(coefr,im);
    }
    c->c_re = re;
    c->c_im = im;
    return (w+7);
}

static void sigvcf_dsp(t_sigvcf *x, t_signal **sp)
{
    /* TODO sr is hardcoded */
    x->x_ctl->c_isr = ftofix(0.0001424758);
// idiv(ftofix(6.28318),ftofix(sp[0]->s_sr));
    post("%f",fixtof(x->x_ctl->c_isr));
   dsp_add(sigvcf_perform, 6,
	sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, 
	    x->x_ctl, sp[0]->s_n);

}

void vcf_tilde_setup(void)
{
    sigvcf_class = class_new(gensym("vcf~"), (t_newmethod)sigvcf_new, 0,
	sizeof(t_sigvcf), 0, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(sigvcf_class, t_sigvcf, x_f);
    class_addmethod(sigvcf_class, (t_method)sigvcf_dsp, gensym("dsp"), 0);
    class_addmethod(sigvcf_class, (t_method)sigvcf_ft1,
    	gensym("ft1"), A_FLOAT, 0);
    class_sethelpsymbol(sigvcf_class, gensym("lop~-help.pd"));
}

