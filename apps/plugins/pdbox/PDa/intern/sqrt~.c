#define DUMTAB1SIZE 256
#define DUMTAB2SIZE 1024

#ifdef ROCKBOX
#include "plugin.h"
#include "../../pdbox.h"
#endif

#include"../src/m_pd.h"
#include<../src/m_fixed.h>

/* sigsqrt -  square root good to 8 mantissa bits  */

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

typedef struct sigsqrt
{
    t_object x_obj;
    float x_f;
} t_sigsqrt;

static t_class *sigsqrt_class;

static void *sigsqrt_new(void)
{
    t_sigsqrt *x = (t_sigsqrt *)pd_new(sigsqrt_class);
    outlet_new(&x->x_obj, gensym("signal"));
    x->x_f = 0;
    return (x);
}

t_int *sigsqrt_perform(t_int *w)    /* not static; also used in d_fft.c */
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
    	    *out++ = f * (1.5 * g - 0.5 * g * g * g * f);
	}
    }
    return (w + 4);
}

static void sigsqrt_dsp(t_sigsqrt *x, t_signal **sp)
{
#ifdef ROCKBOX
    (void) x;
#endif
    dsp_add(sigsqrt_perform, 3, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

void sqrt_tilde_setup(void)
{
    init_rsqrt();
    sigsqrt_class = class_new(gensym("sqrt~"), (t_newmethod)sigsqrt_new, 0,
    	sizeof(t_sigsqrt), 0, 0);
    class_addcreator(sigsqrt_new, gensym("q8_sqrt~"), 0);   /* old name */
    CLASS_MAINSIGNALIN(sigsqrt_class, t_sigsqrt, x_f);
    class_addmethod(sigsqrt_class, (t_method)sigsqrt_dsp, gensym("dsp"), 0);
}

