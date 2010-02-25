#ifdef ROCKBOX
#include "plugin.h"
#include "../../pdbox.h"
#endif

#include "../src/m_pd.h"
#include <../src/m_fixed.h>

/* sigrsqrt - reciprocal square root good to 8 mantissa bits  */

#define DUMTAB1SIZE 256
#define DUMTAB2SIZE 1024

static float rsqrt_exptab[DUMTAB1SIZE], rsqrt_mantissatab[DUMTAB2SIZE];

static void init_rsqrt(void)
{
    int i;
    for (i = 0; i < DUMTAB1SIZE; i++)
    {
#ifdef ROCKBOX
        union f2i f2i;
        f2i.i = (i ? (i == DUMTAB1SIZE-1 ? DUMTAB1SIZE-2 : i) : 1)<< 23;
        rsqrt_exptab[i] = 1./sqrt(f2i.f);
#else /* ROCKBOX */
	float f;
	long l = (i ? (i == DUMTAB1SIZE-1 ? DUMTAB1SIZE-2 : i) : 1)<< 23;
	*(long *)(&f) = l;
	rsqrt_exptab[i] = 1./sqrt(f);	
#endif /* ROCKBOX */
    }
    for (i = 0; i < DUMTAB2SIZE; i++)
    {
	float f = 1 + (1./DUMTAB2SIZE) * i;
	rsqrt_mantissatab[i] = 1./sqrt(f);	
    }
}

    /* these are used in externs like "bonk" */

float q8_rsqrt(float f)
{
#ifdef ROCKBOX
    union f2i f2i;
    if(f < 0.0)
        return 0.0;
    else
    {
        f2i.f = f;
        return (rsqrt_exptab[(f2i.i >> 23) & 0xff] *
	    rsqrt_mantissatab[(f2i.i >> 13) & 0x3ff]);
    }
#else /* ROCKBOX */
    long l = *(long *)(&f);
    if (f < 0) return (0);
    else return (rsqrt_exptab[(l >> 23) & 0xff] *
	    rsqrt_mantissatab[(l >> 13) & 0x3ff]);
#endif /* ROCKBOX */
}

float q8_sqrt(float f)
{
#ifdef ROCKBOX
    union f2i f2i;
    if(f < 0.0)
        return 0.0;
    else
    {
        f2i.f = f;
        return (f * rsqrt_exptab[(f2i.i >> 23) & 0xff] *
	    rsqrt_mantissatab[(f2i.i >> 13) & 0x3ff]);
    }
#else /* ROCKBOX */
    long l = *(long *)(&f);
    if (f < 0) return (0);
    else return (f * rsqrt_exptab[(l >> 23) & 0xff] *
	    rsqrt_mantissatab[(l >> 13) & 0x3ff]);
#endif /* ROCKBOX */
}

    /* the old names are OK unless we're in IRIX N32 */

#ifndef N32
float qsqrt(float f) {return (q8_sqrt(f)); }
float qrsqrt(float f) {return (q8_rsqrt(f)); }
#endif



typedef struct sigrsqrt
{
    t_object x_obj;
    float x_f;
} t_sigrsqrt;

static t_class *sigrsqrt_class;

static void *sigrsqrt_new(void)
{
    t_sigrsqrt *x = (t_sigrsqrt *)pd_new(sigrsqrt_class);
    outlet_new(&x->x_obj, gensym("signal"));
    x->x_f = 0;
    return (x);
}

static t_int *sigrsqrt_perform(t_int *w)
{
    float *in = *(t_float **)(w+1), *out = *(t_float **)(w+2);
    t_int n = *(t_int *)(w+3);
    while (n--)
    {	
	float f = *in;
	long l = *(long *)(in++);
	if (f < 0) *out++ = 0;
	else
	{
	    float g = rsqrt_exptab[(l >> 23) & 0xff] *
	    	rsqrt_mantissatab[(l >> 13) & 0x3ff];
    	    *out++ = 1.5 * g - 0.5 * g * g * g * f;
	}
    }
    return (w + 4);
}

static void sigrsqrt_dsp(t_sigrsqrt *x, t_signal **sp)
{
#ifdef ROCKBOX
    (void) x;
#endif
    dsp_add(sigrsqrt_perform, 3, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

void rsqrt_tilde_setup(void)
{
    init_rsqrt();
    sigrsqrt_class = class_new(gensym("rsqrt~"), (t_newmethod)sigrsqrt_new, 0,
    	sizeof(t_sigrsqrt), 0, 0);
    	    /* an old name for it: */
    class_addcreator(sigrsqrt_new, gensym("q8_rsqrt~"), 0);
    CLASS_MAINSIGNALIN(sigrsqrt_class, t_sigrsqrt, x_f);
    class_addmethod(sigrsqrt_class, (t_method)sigrsqrt_dsp, gensym("dsp"), 0);
}

