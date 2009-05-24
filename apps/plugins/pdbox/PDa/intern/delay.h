
#ifndef __DELAY_H__
#define __DELAY_H__


extern t_class *sigdelwrite_class;


typedef struct delwritectl
{
    int c_n;
    t_sample *c_vec;
    int c_phase;
} t_delwritectl;

typedef struct _sigdelwrite
{
    t_object x_obj;
    t_symbol *x_sym;
    t_delwritectl x_cspace;
    int x_sortno;   /* DSP sort number at which this was last put on chain */
    int x_rsortno;  /* DSP sort # for first delread or write in chain */
    int x_vecsize;  /* vector size for delread~ to use */
    float x_f;
} t_sigdelwrite;

#define XTRASAMPS 4
#define SAMPBLK 4

    /* routine to check that all delwrites/delreads/vds have same vecsize */
static void sigdelwrite_checkvecsize(t_sigdelwrite *x, int vecsize)
{
    if (x->x_rsortno != ugen_getsortno())
    {
    	x->x_vecsize = vecsize;
	x->x_rsortno = ugen_getsortno();
    }
    else if (vecsize != x->x_vecsize)
    	pd_error(x, "delread/delwrite/vd vector size mismatch");
}

#endif

