#include "../src/m_pd.h"
#include <../src/m_fixed.h>

typedef struct biquadctl
{
    t_sample c_x1;
    t_sample c_x2;
    t_sample c_fb1;
    t_sample c_fb2;
    t_sample c_ff1;
    t_sample c_ff2;
    t_sample c_ff3;
} t_biquadctl;

typedef struct sigbiquad
{
    t_object x_obj;
    float x_f;
    t_biquadctl x_cspace;
    t_biquadctl *x_ctl;
} t_sigbiquad;

t_class *sigbiquad_class;

static void sigbiquad_list(t_sigbiquad *x, t_symbol *s, int argc, t_atom *argv);

static void *sigbiquad_new(t_symbol *s, int argc, t_atom *argv)
{
    t_sigbiquad *x = (t_sigbiquad *)pd_new(sigbiquad_class);
    outlet_new(&x->x_obj, gensym("signal"));
    x->x_ctl = &x->x_cspace;
    x->x_cspace.c_x1 = x->x_cspace.c_x2 = 0;
    sigbiquad_list(x, s, argc, argv);
    x->x_f = 0;
    return (x);
}

static t_int *sigbiquad_perform(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    t_sample *out = (t_sample *)(w[2]);
    t_biquadctl *c = (t_biquadctl *)(w[3]);
    int n = (t_int)(w[4]);
    int i;
    t_sample last = c->c_x1;
    t_sample prev = c->c_x2;
    t_sample fb1 = c->c_fb1;
    t_sample fb2 = c->c_fb2;
    t_sample ff1 = c->c_ff1;
    t_sample ff2 = c->c_ff2;
    t_sample ff3 = c->c_ff3;
    for (i = 0; i < n; i++)
    {
    	t_sample output =  *in++ + mult(fb1,last) + mult(fb2,prev);
	if (PD_BADFLOAT(output))
	    output = 0; 
    	*out++ = mult(ff1,output) + mult(ff2,last) + mult(ff3,prev);
	prev = last;
	last = output;
    }
    c->c_x1 = last;
    c->c_x2 = prev;
    return (w+5);
}

static void sigbiquad_list(t_sigbiquad *x, t_symbol *s, int argc, t_atom *argv)
{
#ifdef ROCKBOX
    (void) s;
#endif

    float fb1 = atom_getfloatarg(0, argc, argv);
    float fb2 = atom_getfloatarg(1, argc, argv);
    float ff1 = atom_getfloatarg(2, argc, argv);
    float ff2 = atom_getfloatarg(3, argc, argv);
    float ff3 = atom_getfloatarg(4, argc, argv);
    float discriminant = fb1 * fb1 + 4 * fb2;

    t_biquadctl *c = x->x_ctl;
    if (discriminant < 0) /* imaginary roots -- resonant filter */
    {
    	    /* they're conjugates so we just check that the product
    	    is less than one */
    	if (fb2 >= -1.0f) goto stable;
    }
    else    /* real roots */
    {
    	    /* check that the parabola 1 - fb1 x - fb2 x^2 has a
    	    	vertex between -1 and 1, and that it's nonnegative
    	    	at both ends, which implies both roots are in [1-,1]. */
    	if (fb1 <= 2.0f && fb1 >= -2.0f &&
    	    1.0f - fb1 -fb2 >= 0 && 1.0f + fb1 - fb2 >= 0)
    	    	goto stable;
    }
    	/* if unstable, just bash to zero */
    fb1 = fb2 = ff1 = ff2 = ff3 = 0;
stable:
    c->c_fb1 = ftofix(fb1);
    c->c_fb2 = ftofix(fb2);
    c->c_ff1 = ftofix(ff1);
    c->c_ff2 = ftofix(ff2);
    c->c_ff3 = ftofix(ff3);
}

static void sigbiquad_set(t_sigbiquad *x, t_symbol *s, int argc, t_atom *argv)
{
#ifdef ROCKBOX
    (void) s;
#endif
    t_biquadctl *c = x->x_ctl;
    c->c_x1 = atom_getfloatarg(0, argc, argv);
    c->c_x2 = atom_getfloatarg(1, argc, argv);
}

static void sigbiquad_dsp(t_sigbiquad *x, t_signal **sp)
{
    dsp_add(sigbiquad_perform, 4,
	sp[0]->s_vec, sp[1]->s_vec, 
	    x->x_ctl, sp[0]->s_n);

}

void biquad_tilde_setup(void)
{
    sigbiquad_class = class_new(gensym("biquad~"), (t_newmethod)sigbiquad_new,
    	0, sizeof(t_sigbiquad), 0, A_GIMME, 0);
    CLASS_MAINSIGNALIN(sigbiquad_class, t_sigbiquad, x_f);
    class_addmethod(sigbiquad_class, (t_method)sigbiquad_dsp, gensym("dsp"), 0);
    class_addlist(sigbiquad_class, sigbiquad_list);
    class_addmethod(sigbiquad_class, (t_method)sigbiquad_set, gensym("set"),
    	A_GIMME, 0);
    class_addmethod(sigbiquad_class, (t_method)sigbiquad_set, gensym("clear"),
    	A_GIMME, 0);
}

