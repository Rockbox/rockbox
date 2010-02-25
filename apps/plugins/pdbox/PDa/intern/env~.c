#ifdef ROCKBOX
#include "plugin.h"
#include "../../pdbox.h"
#else /* ROCKBOX */
#define FIXEDPOINT
#endif /* ROCKBOX */
#include "../src/m_pd.h"
#include <../src/m_fixed.h>


#define MAXOVERLAP 10
#define MAXVSTAKEN 64

typedef struct sigenv
{
    t_object x_obj; 	    	    /* header */
    void *x_outlet;		    /* a "float" outlet */
    void *x_clock;		    /* a "clock" object */
    t_sample *x_buf;		    /* a Hanning window */
    int x_phase;		    /* number of points since last output */
    int x_period;		    /* requested period of output */
    int x_realperiod;		    /* period rounded up to vecsize multiple */
    int x_npoints;		    /* analysis window size in samples */
    t_float x_result;		    /* result to output */
    t_sample x_sumbuf[MAXOVERLAP];	    /* summing buffer */
    t_float x_f;
} t_sigenv;

t_class *sigenv_class;
static void sigenv_tick(t_sigenv *x);

static void *sigenv_new(t_floatarg fnpoints, t_floatarg fperiod)
{
    int npoints = fnpoints;
    int period = fperiod;
    t_sigenv *x;
    t_sample *buf;
    int i;

    if (npoints < 1) npoints = 1024;
    if (period < 1) period = npoints/2;
    if (period < npoints / MAXOVERLAP + 1)
	period = npoints / MAXOVERLAP + 1;
    if (!(buf = getbytes(sizeof(t_sample) * (npoints + MAXVSTAKEN))))
    {
	error("env: couldn't allocate buffer");
	return (0);
    }
    x = (t_sigenv *)pd_new(sigenv_class);
    x->x_buf = buf;
    x->x_npoints = npoints;
    x->x_phase = 0;
    x->x_period = period;
    for (i = 0; i < MAXOVERLAP; i++) x->x_sumbuf[i] = 0;
    for (i = 0; i < npoints; i++)
	buf[i] = ftofix((1. - cos((2 * 3.14159 * i) / npoints))/npoints);
    for (; i < npoints+MAXVSTAKEN; i++) buf[i] = 0;
    x->x_clock = clock_new(x, (t_method)sigenv_tick);
    x->x_outlet = outlet_new(&x->x_obj, gensym("float"));
    x->x_f = 0;
    return (x);
}

static t_int *sigenv_perform(t_int *w)
{
    t_sigenv *x = (t_sigenv *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    int n = (int)(w[3]);
    int count;
    t_sample *sump; 
    in += n;
    for (count = x->x_phase, sump = x->x_sumbuf;
	count < x->x_npoints; count += x->x_realperiod, sump++)
    {
	t_sample *hp = x->x_buf + count;
	t_sample *fp = in;
	t_sample sum = *sump;
	int i;
	
	for (i = 0; i < n; i++)
	{
	    fp--;
	    sum += *hp++ * ((*fp * *fp)>>16)>>16;
	}
	*sump = sum;
    }
    sump[0] = 0;
    x->x_phase -= n;
    if (x->x_phase < 0)
    {
	x->x_result = x->x_sumbuf[0];
	for (count = x->x_realperiod, sump = x->x_sumbuf;
	    count < x->x_npoints; count += x->x_realperiod, sump++)
		sump[0] = sump[1];
	sump[0] = 0;
	x->x_phase = x->x_realperiod - n;
	clock_delay(x->x_clock, 0L);
    }
    return (w+4);
}

static void sigenv_dsp(t_sigenv *x, t_signal **sp)
{
    if (x->x_period % sp[0]->s_n) x->x_realperiod =
	x->x_period + sp[0]->s_n - (x->x_period % sp[0]->s_n);
    else x->x_realperiod = x->x_period;
    dsp_add(sigenv_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
    if (sp[0]->s_n > MAXVSTAKEN) bug("sigenv_dsp");
}

static void sigenv_tick(t_sigenv *x)	/* callback function for the clock */
{
    outlet_float(x->x_outlet, powtodb(x->x_result*3.051757e-05));
}

static void sigenv_ff(t_sigenv *x)		/* cleanup on free */
{
    clock_free(x->x_clock);
    freebytes(x->x_buf, (x->x_npoints + MAXVSTAKEN) * sizeof(float));
}


void env_tilde_setup(void )
{
    sigenv_class = class_new(gensym("env~"), (t_newmethod)sigenv_new,
    	(t_method)sigenv_ff, sizeof(t_sigenv), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(sigenv_class, t_sigenv, x_f);
    class_addmethod(sigenv_class, (t_method)sigenv_dsp, gensym("dsp"), 0);
}

