#include "../src/m_pd.h"
#include <../src/m_fixed.h>

extern int ugen_getsortno(void);

#define DEFDELVS 64	    	/* LATER get this from canvas at DSP time */
#ifndef ROCKBOX
static int delread_zero = 0;	/* four bytes of zero for delread~, vd~ */
#endif

#include "delay.h"

t_class *sigdelwrite_class;

static void *sigdelwrite_new(t_symbol *s, t_floatarg msec)
{
    int nsamps;
    t_sigdelwrite *x = (t_sigdelwrite *)pd_new(sigdelwrite_class);
    if (!*s->s_name) s = gensym("delwrite~");
    pd_bind(&x->x_obj.ob_pd, s);
    x->x_sym = s;
    nsamps = msec * sys_getsr() * (float)(0.001f);
    if (nsamps < 1) nsamps = 1;
    nsamps += ((- nsamps) & (SAMPBLK - 1));
    nsamps += DEFDELVS;
    x->x_cspace.c_n = nsamps;
    x->x_cspace.c_vec =
    	(t_sample *)getbytes((nsamps + XTRASAMPS) * sizeof(float));
    x->x_cspace.c_phase = XTRASAMPS;
    x->x_sortno = 0;
    x->x_vecsize = 0;
    x->x_f = 0;
    return (x);
}

static t_int *sigdelwrite_perform(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    t_delwritectl *c = (t_delwritectl *)(w[2]);
    int n = (int)(w[3]);
    int phase = c->c_phase, nsamps = c->c_n;
    t_sample *vp = c->c_vec, *bp = vp + phase, *ep = vp + (c->c_n + XTRASAMPS);
    phase += n;
    while (n--)
    {
    	t_sample f = *in++;
    	if (PD_BADFLOAT(f))
	    f = 0;
    	*bp++ = f;
    	if (bp == ep)
    	{
    	    vp[0] = ep[-4];
    	    vp[1] = ep[-3];
    	    vp[2] = ep[-2];
    	    vp[3] = ep[-1];
    	    bp = vp + XTRASAMPS;
    	    phase -= nsamps;
    	}
    }
    c->c_phase = phase; 
    return (w+4);
}

static void sigdelwrite_dsp(t_sigdelwrite *x, t_signal **sp)
{
    dsp_add(sigdelwrite_perform, 3, sp[0]->s_vec, &x->x_cspace, sp[0]->s_n);
    x->x_sortno = ugen_getsortno();
    sigdelwrite_checkvecsize(x, sp[0]->s_n);
}

static void sigdelwrite_free(t_sigdelwrite *x)
{
    pd_unbind(&x->x_obj.ob_pd, x->x_sym);
    freebytes(x->x_cspace.c_vec,
    	(x->x_cspace.c_n + XTRASAMPS) * sizeof(float));
}

void delwrite_tilde_setup(void)
{
    sigdelwrite_class = class_new(gensym("delwrite~"), 
    	(t_newmethod)sigdelwrite_new, (t_method)sigdelwrite_free,
    	sizeof(t_sigdelwrite), 0, A_DEFSYM, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(sigdelwrite_class, t_sigdelwrite, x_f);
    class_addmethod(sigdelwrite_class, (t_method)sigdelwrite_dsp,
    	gensym("dsp"), 0);
}

