/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/*  send~, receive~, throw~, catch~ */

#ifdef ROCKBOX
#include "plugin.h"
#include "../../pdbox.h"
#endif

#include "m_pd.h"

#ifndef ROCKBOX
#include <string.h>
#endif

#define DEFSENDVS 64	/* LATER get send to get this from canvas */

/* ----------------------------- send~ ----------------------------- */
static t_class *sigsend_class;

typedef struct _sigsend
{
    t_object x_obj;
    t_symbol *x_sym;
    int x_n;
    t_sample *x_vec;
    float x_f;
} t_sigsend;

static void *sigsend_new(t_symbol *s)
{
    t_sigsend *x = (t_sigsend *)pd_new(sigsend_class);
    pd_bind(&x->x_obj.ob_pd, s);
    x->x_sym = s;
    x->x_n = DEFSENDVS;
    x->x_vec = (t_sample *)getbytes(DEFSENDVS * sizeof(t_sample));
    memset((char *)(x->x_vec), 0, DEFSENDVS * sizeof(t_sample));
    x->x_f = 0;
    return (x);
}

static t_int *sigsend_perform(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    t_sample *out = (t_sample *)(w[2]);
    int n = (int)(w[3]);
    while (n--)
    {
    	*out = (PD_BIGORSMALL(*in) ? 0 : *in);
	out++;
	in++;
    }
    return (w+4);
}

static void sigsend_dsp(t_sigsend *x, t_signal **sp)
{
    if (x->x_n == sp[0]->s_n)
    	dsp_add(sigsend_perform, 3, sp[0]->s_vec, x->x_vec, sp[0]->s_n);
    else error("sigsend %s: unexpected vector size", x->x_sym->s_name);
}

static void sigsend_free(t_sigsend *x)
{
    pd_unbind(&x->x_obj.ob_pd, x->x_sym);
    freebytes(x->x_vec, x->x_n * sizeof(float));
}

static void sigsend_setup(void)
{
    sigsend_class = class_new(gensym("send~"), (t_newmethod)sigsend_new,
    	(t_method)sigsend_free, sizeof(t_sigsend), 0, A_DEFSYM, 0);
    class_addcreator((t_newmethod)sigsend_new, gensym("s~"), A_DEFSYM, 0);
    CLASS_MAINSIGNALIN(sigsend_class, t_sigsend, x_f);
    class_addmethod(sigsend_class, (t_method)sigsend_dsp, gensym("dsp"), 0);
}

/* ----------------------------- receive~ ----------------------------- */
static t_class *sigreceive_class;

typedef struct _sigreceive
{
    t_object x_obj;
    t_symbol *x_sym;
    t_sample *x_wherefrom;
    int x_n;
} t_sigreceive;

static void *sigreceive_new(t_symbol *s)
{
    t_sigreceive *x = (t_sigreceive *)pd_new(sigreceive_class);
    x->x_n = DEFSENDVS;   	    /* LATER find our vector size correctly */
    x->x_sym = s;
    x->x_wherefrom = 0;
    outlet_new(&x->x_obj, &s_signal);
    return (x);
}

static t_int *sigreceive_perform(t_int *w)
{
    t_sigreceive *x = (t_sigreceive *)(w[1]);
    t_sample *out = (t_sample *)(w[2]);
    int n = (int)(w[3]);
    t_sample *in = x->x_wherefrom;
    if (in)
    {
    	while (n--)
	    *out++ = *in++; 
    }
    else
    {
    	while (n--)
	    *out++ = 0; 
    }
    return (w+4);
}

static void sigreceive_set(t_sigreceive *x, t_symbol *s)
{
    t_sigsend *sender = (t_sigsend *)pd_findbyclass((x->x_sym = s),
    	sigsend_class);
    if (sender)
    {
    	if (sender->x_n == x->x_n)
    	    x->x_wherefrom = sender->x_vec;
	else
	{
	    pd_error(x, "receive~ %s: vector size mismatch", x->x_sym->s_name);
	    x->x_wherefrom = 0;
	}
    }
    else
    {
    	pd_error(x, "receive~ %s: no matching send", x->x_sym->s_name);
    	x->x_wherefrom = 0;
    }
}

static void sigreceive_dsp(t_sigreceive *x, t_signal **sp)
{
    if (sp[0]->s_n != x->x_n)
    {
    	pd_error(x, "receive~ %s: vector size mismatch", x->x_sym->s_name);
    }
    else
    {
    	sigreceive_set(x, x->x_sym);
    	dsp_add(sigreceive_perform, 3,
    	    x, sp[0]->s_vec, sp[0]->s_n);
    }
}

static void sigreceive_setup(void)
{
    sigreceive_class = class_new(gensym("receive~"),
    	(t_newmethod)sigreceive_new, 0,
    	sizeof(t_sigreceive), 0, A_DEFSYM, 0);
    class_addcreator((t_newmethod)sigreceive_new, gensym("r~"), A_DEFSYM, 0);
    class_addmethod(sigreceive_class, (t_method)sigreceive_set, gensym("set"),
    	A_SYMBOL, 0);
    class_addmethod(sigreceive_class, (t_method)sigreceive_dsp, gensym("dsp"),
    	0);
    class_sethelpsymbol(sigreceive_class, gensym("send~"));
}

/* ----------------------------- catch~ ----------------------------- */
static t_class *sigcatch_class;

typedef struct _sigcatch
{
    t_object x_obj;
    t_symbol *x_sym;
    int x_n;
    t_sample *x_vec;
} t_sigcatch;

static void *sigcatch_new(t_symbol *s)
{
    t_sigcatch *x = (t_sigcatch *)pd_new(sigcatch_class);
    pd_bind(&x->x_obj.ob_pd, s);
    x->x_sym = s;
    x->x_n = DEFSENDVS;
    x->x_vec = (t_sample *)getbytes(DEFSENDVS * sizeof(t_sample));
    memset((char *)(x->x_vec), 0, DEFSENDVS * sizeof(t_sample));
    outlet_new(&x->x_obj, &s_signal);
    return (x);
}

static t_int *sigcatch_perform(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    t_sample *out = (t_sample *)(w[2]);
    int n = (int)(w[3]);
    while (n--) *out++ = *in, *in++ = 0; 
    return (w+4);
}

static void sigcatch_dsp(t_sigcatch *x, t_signal **sp)
{
    if (x->x_n == sp[0]->s_n)
    	dsp_add(sigcatch_perform, 3, x->x_vec, sp[0]->s_vec, sp[0]->s_n);
    else error("sigcatch %s: unexpected vector size", x->x_sym->s_name);
}

static void sigcatch_free(t_sigcatch *x)
{
    pd_unbind(&x->x_obj.ob_pd, x->x_sym);
    freebytes(x->x_vec, x->x_n * sizeof(float));
}

static void sigcatch_setup(void)
{
    sigcatch_class = class_new(gensym("catch~"), (t_newmethod)sigcatch_new,
    	(t_method)sigcatch_free, sizeof(t_sigcatch), CLASS_NOINLET, A_DEFSYM, 0);
    class_addmethod(sigcatch_class, (t_method)sigcatch_dsp, gensym("dsp"), 0);
    class_sethelpsymbol(sigcatch_class, gensym("throw~"));
}

/* ----------------------------- throw~ ----------------------------- */
static t_class *sigthrow_class;

typedef struct _sigthrow
{
    t_object x_obj;
    t_symbol *x_sym;
    t_sample *x_whereto;
    int x_n;
    t_float x_f;
} t_sigthrow;

static void *sigthrow_new(t_symbol *s)
{
    t_sigthrow *x = (t_sigthrow *)pd_new(sigthrow_class);
    x->x_sym = s;
    x->x_whereto  = 0;
    x->x_n = DEFSENDVS;
    x->x_f = 0;
    return (x);
}

static t_int *sigthrow_perform(t_int *w)
{
    t_sigthrow *x = (t_sigthrow *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    int n = (int)(w[3]);
    t_sample *out = x->x_whereto;
    if (out)
    {
    	while (n--)
	{
    	    *out += (PD_BIGORSMALL(*in) ? 0 : *in);
	    out++;
	    in++;
	}
    }
    return (w+4);
}

static void sigthrow_set(t_sigthrow *x, t_symbol *s)
{
    t_sigcatch *catcher = (t_sigcatch *)pd_findbyclass((x->x_sym = s),
    	sigcatch_class);
    if (catcher)
    {
    	if (catcher->x_n == x->x_n)
    	    x->x_whereto = catcher->x_vec;
	else
	{
	    pd_error(x, "throw~ %s: vector size mismatch", x->x_sym->s_name);
	    x->x_whereto = 0;
	}
    }
    else
    {
    	pd_error(x, "throw~ %s: no matching catch", x->x_sym->s_name);
    	x->x_whereto = 0;
    }
}

static void sigthrow_dsp(t_sigthrow *x, t_signal **sp)
{
    if (sp[0]->s_n != x->x_n)
    {
    	pd_error(x, "throw~ %s: vector size mismatch", x->x_sym->s_name);
    }
    else
    {
    	sigthrow_set(x, x->x_sym);
    	dsp_add(sigthrow_perform, 3,
    	    x, sp[0]->s_vec, sp[0]->s_n);
    }
}

static void sigthrow_setup(void)
{
    sigthrow_class = class_new(gensym("throw~"), (t_newmethod)sigthrow_new, 0,
    	sizeof(t_sigthrow), 0, A_DEFSYM, 0);
    class_addcreator((t_newmethod)sigthrow_new, gensym("r~"), A_DEFSYM, 0);
    class_addmethod(sigthrow_class, (t_method)sigthrow_set, gensym("set"),
    	A_SYMBOL, 0);
    CLASS_MAINSIGNALIN(sigthrow_class, t_sigthrow, x_f);
    class_addmethod(sigthrow_class, (t_method)sigthrow_dsp, gensym("dsp"), 0);
}

/* ----------------------- global setup routine ---------------- */

void d_global_setup(void)
{
    sigsend_setup();
    sigreceive_setup();
    sigcatch_setup();
    sigthrow_setup();
}

