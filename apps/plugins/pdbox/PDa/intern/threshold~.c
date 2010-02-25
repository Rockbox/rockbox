#include "../src/m_pd.h"
#include <../src/m_fixed.h>

static t_class *threshold_tilde_class;

typedef struct _threshold_tilde
{
    t_object x_obj;
    t_outlet *x_outlet1;    	/* bang out for high thresh */
    t_outlet *x_outlet2;    	/* bang out for low thresh */
    t_clock *x_clock;	    	/* wakeup for message output */
    float x_f;	    	    	/* scalar inlet */
    int x_state;    		/* 1 = high, 0 = low */
    t_sample x_hithresh;	    	/* value of high threshold */
    t_sample x_lothresh;	    	/* value of low threshold */
    float x_deadwait;	    	/* msec remaining in dead period */
    float x_msecpertick;	/* msec per DSP tick */
    float x_hideadtime;	    	/* hi dead time in msec */
    float x_lodeadtime;	    	/* lo dead time in msec */
} t_threshold_tilde;

static void threshold_tilde_tick(t_threshold_tilde *x);
static void threshold_tilde_set(t_threshold_tilde *x,
    t_floatarg hithresh, t_floatarg hideadtime,
    t_floatarg lothresh, t_floatarg lodeadtime);

static t_threshold_tilde *threshold_tilde_new(t_floatarg hithresh,
    t_floatarg hideadtime, t_floatarg lothresh, t_floatarg lodeadtime)
{
    t_threshold_tilde *x = (t_threshold_tilde *)
    	pd_new(threshold_tilde_class);
    x->x_state = 0;		/* low state */
    x->x_deadwait = 0;		/* no dead time */
    x->x_clock = clock_new(x, (t_method)threshold_tilde_tick);
    x->x_outlet1 = outlet_new(&x->x_obj, &s_bang);
    x->x_outlet2 = outlet_new(&x->x_obj, &s_bang);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft1"));
    x->x_msecpertick = 0.;
    x->x_f = 0;
    threshold_tilde_set(x, hithresh, hideadtime, lothresh, lodeadtime);
    return (x);
}

    /* "set" message to specify thresholds and dead times */
static void threshold_tilde_set(t_threshold_tilde *x,
    t_floatarg hithresh, t_floatarg hideadtime,
    t_floatarg lothresh, t_floatarg lodeadtime)
{
    if (lothresh > hithresh)
    	lothresh = hithresh;
    x->x_hithresh = ftofix(hithresh);
    x->x_hideadtime = hideadtime;
    x->x_lothresh = ftofix(lothresh);
    x->x_lodeadtime = lodeadtime;
}

    /* number in inlet sets state -- note incompatible with JMAX which used
    "int" message for this, impossible here because of auto signal conversion */
static void threshold_tilde_ft1(t_threshold_tilde *x, t_floatarg f)
{
    x->x_state = (f != 0);
    x->x_deadwait = 0;
}

static void threshold_tilde_tick(t_threshold_tilde *x)	
{
    if (x->x_state)
    	outlet_bang(x->x_outlet1);
    else outlet_bang(x->x_outlet2);
}

static t_int *threshold_tilde_perform(t_int *w)
{
    t_sample *in1 = (t_sample *)(w[1]);
    t_threshold_tilde *x = (t_threshold_tilde *)(w[2]);
    int n = (t_int)(w[3]);
    if (x->x_deadwait > 0)
    	x->x_deadwait -= x->x_msecpertick;
    else if (x->x_state)
    {
    	    /* we're high; look for low sample */
    	for (; n--; in1++)
	{
	    if (*in1 < x->x_lothresh)
	    {
		clock_delay(x->x_clock, 0L);
		x->x_state = 0;
		x->x_deadwait = x->x_lodeadtime;
		goto done;
	    }
    	}
    }
    else
    {
    	    /* we're low; look for high sample */
    	for (; n--; in1++)
	{
	    if (*in1 >= x->x_hithresh)
	    {
		clock_delay(x->x_clock, 0L);
		x->x_state = 1;
		x->x_deadwait = x->x_hideadtime;
		goto done;
	    }
    	}
    }
done:
    return (w+4);
}

void threshold_tilde_dsp(t_threshold_tilde *x, t_signal **sp)
{
    x->x_msecpertick = 1000. * sp[0]->s_n / sp[0]->s_sr;
    dsp_add(threshold_tilde_perform, 3, sp[0]->s_vec, x, sp[0]->s_n);
}

static void threshold_tilde_ff(t_threshold_tilde *x)
{
    clock_free(x->x_clock);
}

void threshold_tilde_setup( void)
{
    threshold_tilde_class = class_new(gensym("threshold~"),
    	(t_newmethod)threshold_tilde_new, (t_method)threshold_tilde_ff,
	sizeof(t_threshold_tilde), 0,
	    A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(threshold_tilde_class, t_threshold_tilde, x_f);
    class_addmethod(threshold_tilde_class, (t_method)threshold_tilde_set,
    	gensym("set"), A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(threshold_tilde_class, (t_method)threshold_tilde_ft1,
    	gensym("ft1"), A_FLOAT, 0);
    class_addmethod(threshold_tilde_class, (t_method)threshold_tilde_dsp,
    	gensym("dsp"), 0);
}

