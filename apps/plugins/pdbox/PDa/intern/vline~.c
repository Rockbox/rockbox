#ifdef ROCKBOX
#include "plugin.h"
#include "../../pdbox.h"
#endif

#include "../src/m_pd.h"
#include <../src/m_fixed.h>



static t_class *vline_tilde_class;

typedef struct _vseg
{
    t_time s_targettime;
    t_time s_starttime;
    t_sample s_target;
    struct _vseg *s_next;
} t_vseg;

typedef struct _vline
{
    t_object x_obj;
    t_sample x_value;
    t_sample x_inc;
    t_time x_referencetime;
    t_time x_samppermsec;
    t_time x_msecpersamp;
    t_time x_targettime;
    t_sample x_target;
    float x_inlet1;
    float x_inlet2;
    t_vseg *x_list;
} t_vline;

static t_int *vline_tilde_perform(t_int *w)
{
    t_vline *x = (t_vline *)(w[1]);
    t_sample *out = (t_sample *)(w[2]);
    int n = (int)(w[3]), i;
    t_sample f = x->x_value;
    t_sample inc = x->x_inc;
    t_time msecpersamp = x->x_msecpersamp;
#ifndef ROCKBOX
    t_time samppermsec = x->x_samppermsec;
#endif
    t_time timenow = clock_gettimesince(x->x_referencetime) - n * msecpersamp;
    t_vseg *s = x->x_list;
    for (i = 0; i < n; i++)
    {
    	t_time timenext = timenow + msecpersamp;
    checknext:
	if (s)
	{
	    /* has starttime elapsed?  If so update value and increment */
	    if (s->s_starttime < timenext)
	    {
		if (x->x_targettime <= timenext)
		    f = x->x_target, inc = 0;
	    	    /* if zero-length segment bash output value */
	    	if (s->s_targettime <= s->s_starttime)
		{
		    f = s->s_target;
		    inc = 0;
		}
		else
		{
		    t_time incpermsec = idiv((s->s_target - f),
		    	(s->s_targettime - s->s_starttime));
		    f = mult(f + incpermsec,(timenext - s->s_starttime));
		    inc = mult(incpermsec,msecpersamp);
		}
    	    	x->x_inc = inc;
		x->x_target = s->s_target;
		x->x_targettime = s->s_targettime;
		x->x_list = s->s_next;
		t_freebytes(s, sizeof(*s));
		s = x->x_list;
		goto checknext;
	    }
	}
	if (x->x_targettime <= timenext)
	    f = x->x_target, inc = 0;
	*out++ = f;
	f = f + inc;
	timenow = timenext;
    }
    x->x_value = f;
    return (w+4);
}

static void vline_tilde_stop(t_vline *x)
{
    t_vseg *s1, *s2;
    for (s1 = x->x_list; s1; s1 = s2)
    	s2 = s1->s_next, t_freebytes(s1, sizeof(*s1));
    x->x_list = 0;
    x->x_inc = 0;
    x->x_inlet1 = x->x_inlet2 = 0;
}

static void vline_tilde_float(t_vline *x, t_float f)
{
    t_time timenow = clock_gettimesince(x->x_referencetime);
    t_sample inlet1 = (x->x_inlet1 < 0 ? 0 : (t_sample)x->x_inlet1);
    t_sample inlet2 = (t_sample) x->x_inlet2;
    t_time starttime = timenow + inlet2;
    t_vseg *s1, *s2, *deletefrom = 0,
    	*snew = (t_vseg *)t_getbytes(sizeof(*snew));
    if (PD_BADFLOAT(f))
	f = 0;

    	/* negative delay input means stop and jump immediately to new value */
    if (inlet2 < 0)
    {
    	vline_tilde_stop(x);
	x->x_value = ftofix(f);
	return;
    }
    	/* check if we supplant the first item in the list.  We supplant
	an item by having an earlier starttime, or an equal starttime unless
	the equal one was instantaneous and the new one isn't (in which case
	we'll do a jump-and-slide starting at that time.) */
    if (!x->x_list || x->x_list->s_starttime > starttime ||
    	(x->x_list->s_starttime == starttime &&
	    (x->x_list->s_targettime > x->x_list->s_starttime || inlet1 <= 0)))
    {
    	deletefrom = x->x_list;
	x->x_list = snew;
    }
    else
    {
    	for (s1 = x->x_list; (s2 = s1->s_next); s1 = s2)
	{
    	    if (s2->s_starttime > starttime ||
    		(s2->s_starttime == starttime &&
		    (s2->s_targettime > s2->s_starttime || inlet1 <= 0)))
	    {
    		deletefrom = s2;
		s1->s_next = snew;
		goto didit;
	    }
	}
	s1->s_next = snew;
	deletefrom = 0;
    didit: ;
    }
    while (deletefrom)
    {
    	s1 = deletefrom->s_next;
	t_freebytes(deletefrom, sizeof(*deletefrom));
	deletefrom = s1;
    }
    snew->s_next = 0;
    snew->s_target = f;
    snew->s_starttime = starttime;
    snew->s_targettime = starttime + inlet1;
    x->x_inlet1 = x->x_inlet2 = 0;
}

static void vline_tilde_dsp(t_vline *x, t_signal **sp)
{
    dsp_add(vline_tilde_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
    x->x_samppermsec = idiv(ftofix(sp[0]->s_sr),ftofix(1000));
    x->x_msecpersamp = idiv(ftofix(1000),ftofix(sp[0]->s_sr));
}

static void *vline_tilde_new(void)
{
    t_vline *x = (t_vline *)pd_new(vline_tilde_class);
    outlet_new(&x->x_obj, gensym("signal"));
    floatinlet_new(&x->x_obj, &x->x_inlet1);
    floatinlet_new(&x->x_obj, &x->x_inlet2);
    x->x_inlet1 = x->x_inlet2 = 0;
    x->x_value = x->x_inc = 0;
    x->x_referencetime = clock_getlogicaltime();
    x->x_list = 0;
    x->x_samppermsec = 0;
    return (x);
}

void vline_tilde_setup(void)
{
    vline_tilde_class = class_new(gensym("vline~"), vline_tilde_new, 
    	(t_method)vline_tilde_stop, sizeof(t_vline), 0, 0);
    class_addfloat(vline_tilde_class, (t_method)vline_tilde_float);
    class_addmethod(vline_tilde_class, (t_method)vline_tilde_dsp,
    	gensym("dsp"), 0);
    class_addmethod(vline_tilde_class, (t_method)vline_tilde_stop,
    	gensym("stop"), 0);
}

