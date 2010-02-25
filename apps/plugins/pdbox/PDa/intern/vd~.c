#include "../src/m_pd.h"
#include <../src/m_fixed.h>

extern int ugen_getsortno(void);

#include "delay.h"

#define DEFDELVS 64	    	/* LATER get this from canvas at DSP time */
#ifndef ROCKBOX
static int delread_zero = 0;	/* four bytes of zero for delread~, vd~ */
#endif

static t_class *sigvd_class;

typedef struct _sigvd
{
    t_object x_obj;
    t_symbol *x_sym;
    t_sample x_sr;   	/* samples per msec */
    int x_zerodel;  	/* 0 or vecsize depending on read/write order */
    float x_f;
} t_sigvd;

static void *sigvd_new(t_symbol *s)
{
    t_sigvd *x = (t_sigvd *)pd_new(sigvd_class);
    if (!*s->s_name) s = gensym("vd~");
    x->x_sym = s;
    x->x_sr = 1;
    x->x_zerodel = 0;
    outlet_new(&x->x_obj, &s_signal);
    x->x_f = 0;
    return (x);
}


static t_int *sigvd_perform(t_int *w)
{
     t_sample *in = (t_sample *)(w[1]);
     t_sample *out = (t_sample *)(w[2]);
     t_delwritectl *ctl = (t_delwritectl *)(w[3]);
#ifndef ROCKBOX
     t_sigvd *x = (t_sigvd *)(w[4]);
#endif
     int n = (int)(w[5]);
 
     int nsamps = ctl->c_n;
     int fn = n;
     t_sample limit = nsamps - n - 1;
     t_sample *vp = ctl->c_vec, *bp, *wp = vp + ctl->c_phase;
#ifndef ROCKBOX
     t_sample zerodel = x->x_zerodel;
#endif
     while (n--)
     {
       t_time delsamps =  ((long long) mult((*in++),ftofix(44.1)));//- itofix(zerodel);
	int index = fixtoi(delsamps);
	t_sample frac;
	//	post("%d: index %d f %lld",index,findex,*in);

	frac = delsamps - itofix(index);
        index+=fn;
     	if (index < 1 ) index += nsamps ;
     	if (index > limit) index-= nsamps;
     	bp = wp - index;
     	if (bp < vp + 2) bp += nsamps;
     	*out++ = bp[-1] + mult(frac,bp[-1]-bp[0]);
        wp++;
     }
     return (w+6);
}



static void sigvd_dsp(t_sigvd *x, t_signal **sp)
{
    t_sigdelwrite *delwriter =
    	(t_sigdelwrite *)pd_findbyclass(x->x_sym, sigdelwrite_class);
    x->x_sr = sp[0]->s_sr * 0.001;
    if (delwriter)
    {
    	sigdelwrite_checkvecsize(delwriter, sp[0]->s_n);
    	x->x_zerodel = (delwriter->x_sortno == ugen_getsortno() ?
    	    0 : delwriter->x_vecsize);
    	dsp_add(sigvd_perform, 5,
    	    sp[0]->s_vec, sp[1]->s_vec,
    	    	&delwriter->x_cspace, x, sp[0]->s_n);
    }
    else error("vd~: %s: no such delwrite~",x->x_sym->s_name);
}

void vd_tilde_setup(void)
{
    sigvd_class = class_new(gensym("vd~"), (t_newmethod)sigvd_new, 0,
    	sizeof(t_sigvd), 0, A_DEFSYM, 0);
    class_addmethod(sigvd_class, (t_method)sigvd_dsp, gensym("dsp"), 0);
    CLASS_MAINSIGNALIN(sigvd_class, t_sigvd, x_f);
}

