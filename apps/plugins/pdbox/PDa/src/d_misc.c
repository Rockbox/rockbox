/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/*  miscellaneous: print~; more to come.
*/

#ifdef ROCKBOX
#include "plugin.h"
#include "../../pdbox.h"
#endif

#include "m_pd.h"

#ifndef ROCKBOX
#include <stdio.h>
#include <string.h>
#endif

/* ------------------------- print~ -------------------------- */
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
    t_float *in = (t_float *)(w[2]);
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
    	    	in[0], in[1], in[2], in[3], in[4], in[5], in[6], in[7]);
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

#ifndef ROCKBOX
static
#endif
void print_setup(void)
{
    print_class = class_new(gensym("print~"), (t_newmethod)print_new, 0,
    	sizeof(t_print), 0, A_DEFSYM, 0);
    CLASS_MAINSIGNALIN(print_class, t_print, x_f);
    class_addmethod(print_class, (t_method)print_dsp, gensym("dsp"), 0);
    class_addbang(print_class, print_bang);
    class_addfloat(print_class, print_float);
}

/* ------------------------- scope~ -------------------------- */
/* this has been replaced by arrays; to be deleted later */
 
#include "g_canvas.h"

static t_class *scope_class;

#define SCOPESIZE 256

typedef struct _scope
{
    t_object x_obj;
    t_sample x_samps[SCOPESIZE];
    int x_phase;
    int x_drawn;
    void *x_canvas;
} t_scope;

static t_int *scope_perform(t_int *w)
{
    t_scope *x = (t_scope *)(w[1]);
    t_float *in = (t_float *)(w[2]);
    int n = (int)(w[3]), phase = x->x_phase;
    while (n--)
    {
    	x->x_samps[phase] = *in++;
    	phase = (phase + 1) & (SCOPESIZE-1);
    }
    x->x_phase = phase;
    return (w+4);
}

static void scope_dsp(t_scope *x, t_signal **sp)
{
    dsp_add(scope_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

static void scope_erase(t_scope *x)
{
#ifdef ROCKBOX
    (void) x;
#else
    if (x->x_drawn) sys_vgui(".x%x.c delete gumbo\n", x->x_canvas);
#endif
}

#define X1 10.
#define X2 20.
#define YC 5.
static void scope_bang(t_scope *x)
{
#ifndef ROCKBOX
    int n, phase;
    char hugebuf[10000], *s = hugebuf;
    scope_erase(x);
    sys_vgui(".x%x.c create line 10c 5c 20c 5c -tags gumbo\n", x->x_canvas);
    sprintf(s, ".x%x.c create line ", (t_int)x->x_canvas);
    s += strlen(s);
    for (n = 0, phase = x->x_phase;
    	n < SCOPESIZE; phase = ((phase+1) & (SCOPESIZE-1)), n++)
    {
    	sprintf(s, "%fc %fc ", X1 + (X2 - X1) * (float)n * (1./SCOPESIZE),
    	    YC - 5 * x->x_samps[phase]);
    	s += strlen(s);
    	/* post("phase %d", phase); */
    }
    sprintf(s, "-tags gumbo\n");
    sys_gui(hugebuf);
#endif
    x->x_drawn = 1;
}

static void scope_free(t_scope *x)
{
    scope_erase(x);
}

static void *scope_new(t_symbol *s)
{
#ifdef ROCKBOX
    (void) s;
#endif
    t_scope *x = (t_scope *)pd_new(scope_class);
    error("scope: this is now obsolete; use arrays and tabwrite~ instead");
    x->x_phase = 0;
    x->x_drawn = 0;
    x->x_canvas = canvas_getcurrent();
    return (x);
}

static void scope_setup(void)
{
    scope_class = class_new(gensym("scope~"), (t_newmethod)scope_new, 
    	(t_method)scope_free, sizeof(t_scope), 0, A_DEFSYM, 0);
    class_addmethod(scope_class, nullfn, gensym("signal"), 0);
    class_addmethod(scope_class, (t_method)scope_dsp, gensym("dsp"), 0);
    class_addbang(scope_class, scope_bang);
}

/* ------------------------ bang~ -------------------------- */

static t_class *bang_tilde_class;

typedef struct _bang
{
    t_object x_obj;
    t_clock *x_clock;
} t_bang;

static t_int *bang_tilde_perform(t_int *w)
{
    t_bang *x = (t_bang *)(w[1]);
    clock_delay(x->x_clock, 0);
    return (w+2);
}

static void bang_tilde_dsp(t_bang *x, t_signal **sp)
{
#ifdef ROCKBOX
    (void) sp;
#endif
    dsp_add(bang_tilde_perform, 1, x);
}

static void bang_tilde_tick(t_bang *x)
{
    outlet_bang(x->x_obj.ob_outlet);
}

static void bang_tilde_free(t_bang *x)
{
    clock_free(x->x_clock);
}

static void *bang_tilde_new(t_symbol *s)
{
#ifdef ROCKBOX
    (void) s;
#endif
    t_bang *x = (t_bang *)pd_new(bang_tilde_class);
    x->x_clock = clock_new(x, (t_method)bang_tilde_tick);
    outlet_new(&x->x_obj, &s_bang);
    return (x);
}

static void bang_tilde_setup(void)
{
    bang_tilde_class = class_new(gensym("bang~"), (t_newmethod)bang_tilde_new,
    	(t_method)bang_tilde_free, sizeof(t_bang), 0, 0);
    class_addmethod(bang_tilde_class, (t_method)bang_tilde_dsp,
    	gensym("dsp"), 0);
}

/* ------------------------ samplerate~~ -------------------------- */

static t_class *samplerate_tilde_class;

typedef struct _samplerate
{
    t_object x_obj;
} t_samplerate;

static void samplerate_tilde_bang(t_samplerate *x)
{
    outlet_float(x->x_obj.ob_outlet, sys_getsr());
}

static void *samplerate_tilde_new(t_symbol *s)
{
#ifdef ROCKBOX
    (void) s;
#endif
    t_samplerate *x = (t_samplerate *)pd_new(samplerate_tilde_class);
    outlet_new(&x->x_obj, &s_float);
    return (x);
}

static void samplerate_tilde_setup(void)
{
    samplerate_tilde_class = class_new(gensym("samplerate~"),
    	(t_newmethod)samplerate_tilde_new, 0, sizeof(t_samplerate), 0, 0);
    class_addbang(samplerate_tilde_class, samplerate_tilde_bang);
}

/* ------------------------ global setup routine ------------------------- */

void d_misc_setup(void)
{
#ifndef FIXEDPOINT
    print_setup();
#endif
    scope_setup();
    bang_tilde_setup();
    samplerate_tilde_setup();
}

