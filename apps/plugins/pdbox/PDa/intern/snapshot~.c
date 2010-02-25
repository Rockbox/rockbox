#include "../src/m_pd.h"
#include <../src/m_fixed.h>


static t_class *snapshot_tilde_class;

typedef struct _snapshot
{
    t_object x_obj;
    t_sample x_value;
    float x_f;
} t_snapshot;

static void *snapshot_tilde_new(void)
{
    t_snapshot *x = (t_snapshot *)pd_new(snapshot_tilde_class);
    x->x_value = 0;
    outlet_new(&x->x_obj, &s_float);
    x->x_f = 0;
    return (x);
}

static t_int *snapshot_tilde_perform(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    t_sample *out = (t_sample *)(w[2]);
    *out = *in;
    return (w+3);
}

static void snapshot_tilde_dsp(t_snapshot *x, t_signal **sp)
{
    dsp_add(snapshot_tilde_perform, 2, sp[0]->s_vec + (sp[0]->s_n-1),
    	&x->x_value);
}

static void snapshot_tilde_bang(t_snapshot *x)
{
    outlet_float(x->x_obj.ob_outlet, fixtof(x->x_value));
}

static void snapshot_tilde_set(t_snapshot *x, t_floatarg f)
{
    x->x_value = ftofix(f);
}

void snapshot_tilde_setup(void)
{
    snapshot_tilde_class = class_new(gensym("snapshot~"), snapshot_tilde_new, 0,
    	sizeof(t_snapshot), 0, 0);
    CLASS_MAINSIGNALIN(snapshot_tilde_class, t_snapshot, x_f);
    class_addmethod(snapshot_tilde_class, (t_method)snapshot_tilde_dsp,
    	gensym("dsp"), 0);
    class_addmethod(snapshot_tilde_class, (t_method)snapshot_tilde_set,
    	gensym("set"), A_DEFFLOAT, 0);
    class_addbang(snapshot_tilde_class, snapshot_tilde_bang);
}

