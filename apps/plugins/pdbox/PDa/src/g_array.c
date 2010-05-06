/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#ifdef ROCKBOX
#include "plugin.h"
#include "../../pdbox.h"
#include "m_pd.h"
#include "g_canvas.h"
#ifdef SIMULATOR
int printf(const char *fmt, ...);
#endif /* SIMULATOR */
#else /* ROCKBOX */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>  	/* for read/write to files */
#include "m_pd.h"
#include "g_canvas.h"
#include <math.h>
#endif /* ROCKBOX */

/* see also the "plot" object in g_scalar.c which deals with graphing
arrays which are fields in scalars.  Someday we should unify the
two, but how? */

    /* aux routine to bash leading '#' to '$' for dialogs in u_main.tk
    which can't send symbols starting with '$' (because the Pd message
    interpreter would change them!) */

static t_symbol *sharptodollar(t_symbol *s)
{
    if (*s->s_name == '#')
    {
	char buf[MAXPDSTRING];
	strncpy(buf, s->s_name, MAXPDSTRING);
	buf[MAXPDSTRING-1] = 0;
	buf[0] = '$';
	return (gensym(buf));
    }
    else return (s);
}

/* --------- "pure" arrays with scalars for elements. --------------- */

/* Pure arrays have no a priori graphical capabilities.
They are instantiated by "garrays" below or can be elements of other
scalars (g_scalar.c); their graphical behavior is defined accordingly. */

t_array *array_new(t_symbol *templatesym, t_gpointer *parent)
{
    t_array *x = (t_array *)getbytes(sizeof (*x));
    t_template *template;
#ifndef ROCKBOX
    t_gpointer *gp;
#endif
    template = template_findbyname(templatesym);
    x->a_templatesym = templatesym;
    x->a_n = 1;
    x->a_elemsize = sizeof(t_word) * template->t_n;
    x->a_vec = (char *)getbytes(x->a_elemsize);
    	/* note here we blithely copy a gpointer instead of "setting" a
	new one; this gpointer isn't accounted for and needn't be since
	we'll be deleted before the thing pointed to gets deleted anyway;
	see array_free. */
    x->a_gp = *parent;
    x->a_stub = gstub_new(0, x);
    word_init((t_word *)(x->a_vec), template, parent);
    return (x);
}

void array_resize(t_array *x, t_template *template, int n)
{
    int elemsize, oldn;
#ifndef ROCKBOX
    t_gpointer *gp;
#endif

    if (n < 1)
    	n = 1;
    oldn = x->a_n;
    elemsize = sizeof(t_word) * template->t_n;
    
    x->a_vec = (char *)resizebytes(x->a_vec, oldn * elemsize,
    	n * elemsize);
    x->a_n = n;
    if (n > oldn)
    {
    	char *cp = x->a_vec + elemsize * oldn;
    	int i = n - oldn;
    	for (; i--; cp += elemsize)
    	{
    	    t_word *wp = (t_word *)cp;
    	    word_init(wp, template, &x->a_gp);
    	}
    }
}

void word_free(t_word *wp, t_template *template);

void array_free(t_array *x)
{
    int i;
    t_template *scalartemplate = template_findbyname(x->a_templatesym);
	/* we don't unset our gpointer here since it was never "set." */
    /* gpointer_unset(&x->a_gp); */
    gstub_cutoff(x->a_stub);
    for (i = 0; i < x->a_n; i++)
    {
    	t_word *wp = (t_word *)(x->a_vec + x->a_elemsize * i);
	word_free(wp, scalartemplate);
    }
    freebytes(x->a_vec, x->a_elemsize * x->a_n);
    freebytes(x, sizeof *x);
}

/* --------------------- graphical arrays (garrays) ------------------- */

t_class *garray_class;
static int gcount = 0;

struct _garray
{
    t_gobj x_gobj;
    t_glist *x_glist;
    t_array x_array;	    /* actual array; note only 4 fields used as below */
    t_symbol *x_name;
    t_symbol *x_realname;   /* name with "$" expanded */
    t_float x_firstx;	    /* X value of first item */
    t_float x_xinc; 	    /* X increment */
    char x_usedindsp;	    /* true if some DSP routine is using this */
    char x_saveit;   	    /* true if we should save this with parent */
};

    /* macros to get into the "array" structure */
#define x_n x_array.a_n
#define x_elemsize x_array.a_elemsize
#define x_vec x_array.a_vec
#define x_templatesym x_array.a_templatesym

t_garray *graph_array(t_glist *gl, t_symbol *s, t_symbol *templatesym,
    t_floatarg f, t_floatarg saveit)
{
    int n = f, i;
    int zz, nwords;
    t_garray *x;
    t_pd *x2;
    t_template *template;
    char *str;
    if (s == &s_)
    {
    	char buf[40];
#ifdef ROCKBOX
        snprintf(buf, sizeof(buf), "array%d", ++gcount);
#else
    	sprintf(buf, "array%d", ++gcount);
#endif
    	s = gensym(buf);    	
    	templatesym = &s_float;
    	n = 100;
    }
    else if (!strncmp((str = s->s_name), "array", 5)
    	&& (zz = atoi(str + 5)) > gcount) gcount = zz;
    template = template_findbyname(templatesym);
    if (!template)
    {
    	error("array: couldn't find template %s", templatesym->s_name);
    	return (0);
    }
    nwords =  template->t_n;
    for (i = 0; i < nwords; i++)
    {
    	    /* we can't have array or list elements yet because what scalar
	    can act as their "parent"??? */
    	if (template->t_vec[i].ds_type == DT_ARRAY
	    || template->t_vec[i].ds_type == DT_LIST)
	{
	    error("array: template %s can't have sublists or arrays",
	    	templatesym->s_name);
    	    return (0);
    	}
    }
    x = (t_garray *)pd_new(garray_class);

    if (n <= 0) n = 100;  
    x->x_n = n;
    x->x_elemsize = nwords * sizeof(t_word);
    x->x_vec = getbytes(x->x_n * x->x_elemsize);
    memset(x->x_vec, 0, x->x_n * x->x_elemsize);
    	/* LATER should check that malloc */
    x->x_name = s;
    x->x_realname = canvas_realizedollar(gl, s);
    pd_bind(&x->x_gobj.g_pd, x->x_realname);
    x->x_templatesym = templatesym;
    x->x_firstx = 0;
    x->x_xinc = 1;	    	/* LATER make methods to set this... */
    glist_add(gl, &x->x_gobj);
    x->x_glist = gl;
    x->x_usedindsp = 0;
    x->x_saveit = (saveit != 0);
    if((x2 = pd_findbyclass(gensym("#A"), garray_class)))
    	pd_unbind(x2, gensym("#A"));

    pd_bind(&x->x_gobj.g_pd, gensym("#A"));
    
    return (x);
}

    /* called from array menu item to create a new one */
void canvas_menuarray(t_glist *canvas)
{
#ifdef ROCKBOX
    (void) canvas;
#else /* ROCKBOX */
    t_glist *x = (t_glist *)canvas;
    char cmdbuf[200];
    sprintf(cmdbuf, "pdtk_array_dialog %%s array%d 100 1 1\n",
	++gcount);
    gfxstub_new(&x->gl_pd, x, cmdbuf);
#endif /* ROCKBOX */
}

    /* called from graph_dialog to set properties */
void garray_properties(t_garray *x)
{
#ifdef ROCKBOX
    (void) x;
#else /* ROCKBOX */
    char cmdbuf[200];
    gfxstub_deleteforkey(x);
    	/* create dialog window.  LATER fix this to escape '$'
	properly; right now we just detect a leading '$' and escape
	it.  There should be a systematic way of doing this. */
    if (x->x_name->s_name[0] == '$')
    	sprintf(cmdbuf, "pdtk_array_dialog %%s \\%s %d %d 0\n",
	    x->x_name->s_name, x->x_n, x->x_saveit);
    else sprintf(cmdbuf, "pdtk_array_dialog %%s %s %d %d 0\n",
	x->x_name->s_name, x->x_n, x->x_saveit);
    gfxstub_new(&x->x_gobj.g_pd, x, cmdbuf);
#endif /* ROCKBOX */
}

    /* this is called back from the dialog window to create a garray. 
    The otherflag requests that we find an existing graph to put it in. */
void glist_arraydialog(t_glist *parent, t_symbol *name, t_floatarg size,
    t_floatarg saveit, t_floatarg otherflag)
{
    t_glist *gl;
    t_garray *a;
    if (size < 1)
    	size = 1;
    if (otherflag == 0 || (!(gl = glist_findgraph(parent))))
    	gl = glist_addglist(parent, &s_, 0, 1,
	    (size > 1 ? size-1 : size), -1, 0, 0, 0, 0);
    a = graph_array(gl, sharptodollar(name), &s_float, size, saveit);
}

    /* this is called from the properties dialog window for an existing array */
void garray_arraydialog(t_garray *x, t_symbol *name, t_floatarg fsize,
    t_floatarg saveit, t_floatarg deleteit)
{
    if (deleteit != 0)
    {
    	glist_delete(x->x_glist, &x->x_gobj);
    }
    else
    {
    	int size;
	t_symbol *argname = sharptodollar(name);
	if (argname != x->x_name)
	{
	    x->x_name = argname;
    	    pd_unbind(&x->x_gobj.g_pd, x->x_realname);
	    x->x_realname = canvas_realizedollar(x->x_glist, argname);
    	    pd_bind(&x->x_gobj.g_pd, x->x_realname);
	}
	size = fsize;
	if (size < 1)
    	    size = 1;
	if (size != x->x_n)
    	    garray_resize(x, size);
	garray_setsaveit(x, (saveit != 0));
	garray_redraw(x);
    }
}

static void garray_free(t_garray *x)
{
    t_pd *x2;
#ifndef ROCKBOX
    gfxstub_deleteforkey(x);
#endif
    pd_unbind(&x->x_gobj.g_pd, x->x_realname);
    	/* LATER find a way to get #A unbound earlier (at end of load?) */
    while((x2 = pd_findbyclass(gensym("#A"), garray_class)))
    	pd_unbind(x2, gensym("#A"));
    freebytes(x->x_vec, x->x_n * x->x_elemsize);
}

/* ------------- code used by both array and plot widget functions ---- */

    /* routine to get screen coordinates of a point in an array */
void array_getcoordinate(t_glist *glist,
    char *elem, int xonset, int yonset, int wonset, int indx,
    float basex, float basey, float xinc,
    float *xp, float *yp, float *wp)
{
    float xval, yval, ypix, wpix;
    if (xonset >= 0)
    	xval = fixtof(*(t_sample *)(elem + xonset));
    else xval = indx * xinc;
    if (yonset >= 0)
    	yval = fixtof(*(t_sample *)(elem + yonset));
    else yval = 0;
    ypix = glist_ytopixels(glist, basey + yval);
    if (wonset >= 0)
    {
    	    /* found "w" field which controls linewidth. */
    	float wval = *(float *)(elem + wonset);
	wpix = glist_ytopixels(glist, basey + yval + wval) - ypix;
	if (wpix < 0)
	    wpix = -wpix;
    }
    else wpix = 1;
    *xp = glist_xtopixels(glist, basex + xval);
    *yp = ypix;
    *wp = wpix;
}

static float array_motion_xcumulative;
static float array_motion_ycumulative;
static t_symbol *array_motion_xfield;
static t_symbol *array_motion_yfield;
static t_glist *array_motion_glist;
static t_gobj *array_motion_gobj;
static t_word *array_motion_wp;
static t_template *array_motion_template;
static int array_motion_npoints;
static int array_motion_elemsize;
#ifndef ROCKBOX
static int array_motion_altkey;
#endif
static float array_motion_initx;
static float array_motion_xperpix;
static float array_motion_yperpix;
static int array_motion_lastx;
static int array_motion_fatten;

    /* LATER protect against the template changing or the scalar disappearing
    probably by attaching a gpointer here ... */

static void array_motion(void *z, t_floatarg dx, t_floatarg dy)
{
#ifdef ROCKBOX
    (void) z;
#endif
    array_motion_xcumulative += dx * array_motion_xperpix;
    array_motion_ycumulative += dy * array_motion_yperpix;
    if (*array_motion_xfield->s_name)
    {
    	    /* it's an x, y plot; can drag many points at once */
    	int i;
	char *charword = (char *)array_motion_wp;
	for (i = 0; i < array_motion_npoints; i++)
	{
	    t_word *thisword = (t_word *)(charword + i * array_motion_elemsize);
	    if (*array_motion_xfield->s_name)
	    {
	    	float xwas = template_getfloat(array_motion_template,
    		    array_motion_xfield, thisword, 1);
		template_setfloat(array_motion_template,
    		    array_motion_xfield, thisword, xwas + dx, 1);
	    }
	    if (*array_motion_yfield->s_name)
	    {
	    	float ywas = template_getfloat(array_motion_template,
    		    array_motion_yfield, thisword, 1);
		if (array_motion_fatten)
		{
		    if (i == 0)
		    {
    			float newy = ywas + dy * array_motion_yperpix;
			if (newy < 0)
    			    newy = 0;
			template_setfloat(array_motion_template,
    			    array_motion_yfield, thisword, newy, 1);
		    }
		}
		else
		{
		    template_setfloat(array_motion_template,
    		    	array_motion_yfield, thisword,
			    ywas + dy * array_motion_yperpix, 1);
	    	}
	    }
	}
    }
    else
    {
    	    /* a y-only plot. */
    	int thisx = array_motion_initx +
	    array_motion_xcumulative, x2;
	int increment, i, nchange;
	char *charword = (char *)array_motion_wp;
	float newy = array_motion_ycumulative,
	    oldy = template_getfloat(
	    array_motion_template, array_motion_yfield,
    	    (t_word *)(charword + array_motion_elemsize * array_motion_lastx), 1);
	float ydiff = newy - oldy;
	if (thisx < 0) thisx = 0;
	else if (thisx >= array_motion_npoints)
	    thisx = array_motion_npoints - 1;
	increment = (thisx > array_motion_lastx ? -1 : 1);
	nchange = 1 + increment * (array_motion_lastx - thisx);

    	for (i = 0, x2 = thisx; i < nchange; i++, x2 += increment)
	{
	    template_setfloat(array_motion_template,
    		array_motion_yfield,
	    	    (t_word *)(charword + array_motion_elemsize * x2),
	    		newy, 1);
	    if (nchange > 1)
	    	newy -= ydiff * (1./(nchange - 1));
    	 }
	 array_motion_lastx = thisx;
    }
    glist_redrawitem(array_motion_glist, array_motion_gobj);
}

int array_doclick(t_array *array, t_glist *glist, t_gobj *gobj,
    t_symbol *elemtemplatesym,
    float linewidth, float xloc, float xinc, float yloc,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    t_canvas *elemtemplatecanvas;
    t_template *elemtemplate;
    int elemsize, yonset, wonset, xonset, i;

#ifdef ROCKBOX
    (void) linewidth;
    (void) shift;
    (void) dbl;
#endif

    if (!array_getfields(elemtemplatesym, &elemtemplatecanvas,
    	&elemtemplate, &elemsize, &xonset, &yonset, &wonset))
    {
    	float best = 100;
	int incr;
	    /* if it has more than 2000 points, just check 300 of them. */
	if (array->a_n < 2000)
	    incr = 1;
	else incr = array->a_n / 300;
	for (i = 0; i < array->a_n; i += incr)
    	{
	    float pxpix, pypix, pwpix, dx, dy;
    	    array_getcoordinate(glist, (char *)(array->a_vec) + i * elemsize,
    		xonset, yonset, wonset, i, xloc, yloc, xinc,
    		&pxpix, &pypix, &pwpix);
	    if (pwpix < 4)
	    	pwpix = 4;
	    dx = pxpix - xpix;
	    if (dx < 0) dx = -dx;
	    if (dx > 8)
	    	continue;   
	    dy = pypix - ypix;
	    if (dy < 0) dy = -dy;
	    if (dx + dy < best)
	    	best = dx + dy;
	    if (wonset >= 0)
	    {
		dy = (pypix + pwpix) - ypix;
		if (dy < 0) dy = -dy;
		if (dx + dy < best)
	    	    best = dx + dy;
		dy = (pypix - pwpix) - ypix;
		if (dy < 0) dy = -dy;
		if (dx + dy < best)
	    	    best = dx + dy;
	    }
	}
	if (best > 8)
	    return (0);
	best += 0.001;  /* add truncation error margin */
	for (i = 0; i < array->a_n; i += incr)
    	{
	    float pxpix, pypix, pwpix, dx, dy, dy2, dy3;
    	    array_getcoordinate(glist, (char *)(array->a_vec) + i * elemsize,
    		xonset, yonset, wonset, i, xloc, yloc, xinc,
    		&pxpix, &pypix, &pwpix);
	    if (pwpix < 4)
	    	pwpix = 4;
	    dx = pxpix - xpix;
	    if (dx < 0) dx = -dx;
	    dy = pypix - ypix;
	    if (dy < 0) dy = -dy;
	    if (wonset >= 0)
	    {
		dy2 = (pypix + pwpix) - ypix;
		if (dy2 < 0) dy2 = -dy2;
		dy3 = (pypix - pwpix) - ypix;
		if (dy3 < 0) dy3 = -dy3;
		if (yonset <= 0)
		    dy = 100;
	    }
	    else dy2 = dy3 = 100;
	    if (dx + dy <= best || dx + dy2 <= best || dx + dy3 <= best)
	    {
	    	if (dy < dy2 && dy < dy3)
		    array_motion_fatten = 0;
    		else if (dy2 < dy3)
		    array_motion_fatten = -1;
		else array_motion_fatten = 1;
		if (doit)
		{
		    char *elem = (char *)array->a_vec;
    	    	    array_motion_elemsize = elemsize;
		    array_motion_glist = glist;
		    array_motion_gobj = gobj;
		    array_motion_template = elemtemplate;
		    array_motion_xperpix = glist_dpixtodx(glist, 1);
		    array_motion_yperpix = glist_dpixtody(glist, 1);
		    if (alt && xpix < pxpix) /* delete a point */
		    {
		    	if (array->a_n <= 1)
			    return (0);
		    	memmove((char *)(array->a_vec) + elemsize * i, 
			    (char *)(array->a_vec) + elemsize * (i+1),
			    	(array->a_n - 1 - i) * elemsize);
			array_resize(array, elemtemplate, array->a_n - 1);
			glist_redrawitem(array_motion_glist, array_motion_gobj);
			return (0);
		    }
		    else if (alt)
		    {
			/* add a point (after the clicked-on one) */
			array_resize(array, elemtemplate, array->a_n + 1);
		    	elem = (char *)array->a_vec;
			memmove(elem + elemsize * (i+1), 
			    elem + elemsize * i,
			    	(array->a_n - i - 1) * elemsize);
			i++;
		    }
		    if (xonset >= 0)
		    {
		    	array_motion_xfield = gensym("x");
			array_motion_xcumulative = 
			    *(float *)((elem + elemsize * i) + xonset);
		    	array_motion_wp = (t_word *)(elem + i * elemsize);
			array_motion_npoints = array->a_n - i;
		    }
		    else
		    {
		    	array_motion_xfield = &s_;
			array_motion_xcumulative = 0;
			array_motion_wp = (t_word *)elem;
			array_motion_npoints = array->a_n;

			array_motion_initx = i;
			array_motion_lastx = i;
			array_motion_xperpix *= (xinc == 0 ? 1 : 1./xinc);
		    }
		    if (array_motion_fatten)
		    {
		    	array_motion_yfield = gensym("w");
			array_motion_ycumulative = 
			    *(float *)((elem + elemsize * i) + wonset);
			array_motion_yperpix *= array_motion_fatten;
		    }
		    else if (yonset >= 0)
		    {
		    	array_motion_yfield = gensym("y");
			array_motion_ycumulative = 
			    *(float *)((elem + elemsize * i) + yonset);
		    }
		    else
		    {
		    	array_motion_yfield = &s_;
			array_motion_ycumulative = 0;
		    }
		    glist_grab(glist, 0, array_motion, 0, xpix, ypix);
		}
		if (alt)
		{
		    if (xpix < pxpix)
		    	return (CURSOR_EDITMODE_DISCONNECT);
		    else return (CURSOR_RUNMODE_ADDPOINT);
		}
    		else return (array_motion_fatten ?
		    CURSOR_RUNMODE_THICKEN : CURSOR_RUNMODE_CLICKME);
    	    }
    	}   
    }
    return (0);
}

/* -------------------- widget behavior for garray ------------ */

static void garray_getrect(t_gobj *z, t_glist *glist,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_garray *x = (t_garray *)z;
    float x1 = 0x7fffffff, y1 = 0x7fffffff, x2 = -0x7fffffff, y2 = -0x7fffffff;
    t_canvas *elemtemplatecanvas;
    t_template *elemtemplate;
    int elemsize, yonset, wonset, xonset, i;

    if (!array_getfields(x->x_templatesym, &elemtemplatecanvas,
    	&elemtemplate, &elemsize, &xonset, &yonset, &wonset))
    {
    	int incr;
	    /* if it has more than 2000 points, just check 300 of them. */
	if (x->x_array.a_n < 2000)
	    incr = 1;
	else incr = x->x_array.a_n / 300;
	for (i = 0; i < x->x_array.a_n; i += incr)
    	{
#ifdef ROCKBOX
            float pxpix, pypix, pwpix;
#else /* ROCKBOX */
	    float pxpix, pypix, pwpix, dx, dy;
#endif /* ROCKBOX */
    	    array_getcoordinate(glist, (char *)(x->x_array.a_vec) +
	    	i * elemsize,
    		xonset, yonset, wonset, i, 0, 0, 1,
    		&pxpix, &pypix, &pwpix);
	    if (pwpix < 2)
	    	pwpix = 2;
	    if (pxpix < x1)
		x1 = pxpix;
	    if (pxpix > x2)
		x2 = pxpix;
	    if (pypix - pwpix < y1)
		y1 = pypix - pwpix;
	    if (pypix + pwpix > y2)
		y2 = pypix + pwpix;
	}
    }
    *xp1 = x1;
    *yp1 = y1;
    *xp2 = x2;
    *yp2 = y2;
}

static void garray_displace(t_gobj *z, t_glist *glist, int dx, int dy)
{
#ifdef ROCKBOX
    (void) z;
    (void) glist;
    (void) dx;
    (void) dy;
#endif
    /* refuse */
}

static void garray_select(t_gobj *z, t_glist *glist, int state)
{
#ifdef ROCKBOX
    (void) z;
    (void) glist;
    (void) state;
#else /* ROCKBOX */
    t_garray *x = (t_garray *)z;
#endif /* ROCKBOX */
    /* fill in later */
}

static void garray_activate(t_gobj *z, t_glist *glist, int state)
{
#ifdef ROCKBOX
    (void) z;
    (void) glist;
    (void) state;
#endif
}

static void garray_delete(t_gobj *z, t_glist *glist)
{
#ifdef ROCKBOX
    (void) z;
    (void) glist;
#endif
    /* nothing to do */
}

static void garray_vis(t_gobj *z, t_glist *glist, int vis)
{
    t_garray *x = (t_garray *)z;
    if (vis)
    {
    	int i, xonset, yonset, type;
	t_symbol *arraytype;
	t_template *template = template_findbyname(x->x_templatesym);
	if (!template)
	    return;
    	if (!template_find_field(template, gensym("y"), &yonset, &type,
	    &arraytype) || type != DT_FLOAT)
    	{
    	    error("%s: needs floating-point 'y' field",
	    	x->x_templatesym->s_name);
#ifndef ROCKBOX
    	    sys_vgui(".x%x.c create text 50 50 -text foo\
    	    	-tags .x%x.a%x\n",
    	    	glist_getcanvas(glist), glist_getcanvas(glist), x);
#endif
    	}
    	else if (!template_find_field(template, gensym("x"), &xonset, &type,
	    &arraytype) || type != DT_FLOAT)
    	{
    	    float firsty, xcum = x->x_firstx;
    	    int lastpixel = -1, ndrawn = 0;
    	    float yval = 0, xpix;
    	    int ixpix = 0;
#ifndef ROCKBOX
    	    sys_vgui(".x%x.c create line \\\n", glist_getcanvas(glist));
#endif
    	    for (i = 0; i < x->x_n; i++)
    	    {
    	    	yval = fixtof(*(t_sample *)(x->x_vec +
		    template->t_n * i * sizeof (t_word) + yonset));
    	    	xpix = glist_xtopixels(glist, xcum);
    	    	ixpix = xpix + 0.5;
    	    	if (ixpix != lastpixel)
    	    	{
#ifndef ROCKBOX
    	    	    sys_vgui("%d %f \\\n", ixpix,
    	    	    	glist_ytopixels(glist, yval));
#endif
    	    	    ndrawn++;
    	    	}
    	    	lastpixel = ixpix;
    	    	if (ndrawn >= 1000) break;
    	    	xcum += x->x_xinc;
    	    }
    	    	/* TK will complain if there aren't at least 2 points... */
#ifndef ROCKBOX
    	    if (ndrawn == 0) sys_vgui("0 0 0 0 \\\n");
    	    else if (ndrawn == 1) sys_vgui("%d %f \\\n", ixpix,
    	    	    	glist_ytopixels(glist, yval));
    	    sys_vgui("-tags .x%x.a%x\n", glist_getcanvas(glist), x);
#endif
    	    firsty = fixtof(*(t_sample *)(x->x_vec + yonset));
#ifndef ROCKBOX
    	    sys_vgui(".x%x.c create text %f %f -text {%s} -anchor e\
    	    	 -font -*-courier-bold--normal--%d-* -tags .x%x.a%x\n",
    	    	glist_getcanvas(glist),
    	    	glist_xtopixels(glist, x->x_firstx) - 5.,
    	    	glist_ytopixels(glist, firsty),
    	    	x->x_name->s_name, glist_getfont(glist),
    	    	    glist_getcanvas(glist), x);
#endif
    	}
    	else
    	{
    	    post("x, y arrays not yet supported");
    	}
    }
    else
    {
#ifndef ROCKBOX
    	sys_vgui(".x%x.c delete .x%x.a%x\n",
    	    glist_getcanvas(glist), glist_getcanvas(glist), x);
#endif
    }
}

static int garray_click(t_gobj *z, struct _glist *glist,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    t_garray *x = (t_garray *)z;
    return (array_doclick(&x->x_array, glist, z, x->x_templatesym, 1, 0, 1, 0,
    	xpix, ypix, shift, alt, dbl, doit));
}

#define ARRAYWRITECHUNKSIZE 1000

static void garray_save(t_gobj *z, t_binbuf *b)
{
    t_garray *x = (t_garray *)z;
    binbuf_addv(b, "sssisi;", gensym("#X"), gensym("array"),
    	x->x_name, x->x_n, x->x_templatesym, x->x_saveit);
#ifdef ROCKBOX
#ifdef SIMULATOR
    printf("array save\n");
#endif /* SIMULATOR */
#else /* ROCKBOX */
    fprintf(stderr,"array save\n");
#endif /* ROCKBOX */
    if (x->x_saveit)
    {
    	int n = x->x_n, n2 = 0;
    	if (x->x_templatesym != &s_float)
	{
	    pd_error(x, "sorry, you can only save 'float' arrays now");
	    return;
	}
	if (n > 200000)
	    post("warning: I'm saving an array with %d points!\n", n);
	while (n2 < n)
	{
	    int chunk = n - n2, i;
	    if (chunk > ARRAYWRITECHUNKSIZE)
	    	chunk = ARRAYWRITECHUNKSIZE;
	    binbuf_addv(b, "si", gensym("#A"), n2);
	    for (i = 0; i < chunk; i++)
	    	binbuf_addv(b, "f", fixtof(((t_sample *)(x->x_vec))[n2+i]));
	    binbuf_addv(b, ";");
	    n2 += chunk;
    	}
    }
}

t_widgetbehavior garray_widgetbehavior =
{
    garray_getrect,
    garray_displace,
    garray_select,
    garray_activate,
    garray_delete,
    garray_vis,
    garray_click
};

/* ----------------------- public functions -------------------- */

void garray_usedindsp(t_garray *x)
{
    x->x_usedindsp = 1;
}

void garray_redraw(t_garray *x)
{
    if (glist_isvisible(x->x_glist))
    {
    	garray_vis(&x->x_gobj, x->x_glist, 0); 
    	garray_vis(&x->x_gobj, x->x_glist, 1);
    }
}

   /* This functiopn gets the template of an array; if we can't figure
   out what template an array's elements belong to we're in grave trouble
   when it's time to free or resize it.  */
t_template *garray_template(t_garray *x)
{
    t_template *template = template_findbyname(x->x_templatesym);
    if (!template)
    	bug("garray_template");
    return (template);
}

int garray_npoints(t_garray *x)	/* get the length */
{
    return (x->x_n);
}

char *garray_vec(t_garray *x) /* get the contents */
{
    return ((char *)(x->x_vec));
}

    /* routine that checks if we're just an array of floats and if
    so returns the goods */

int garray_getfloatarray(t_garray *x, int *size, t_sample **vec)
{
    t_template *template = garray_template(x);
    int yonset, type;
    t_symbol *arraytype;
    if (!template_find_field(template, gensym("y"), &yonset,
    	&type, &arraytype) || type != DT_FLOAT)
    	    error("%s: needs floating-point 'y' field",
	    	x->x_templatesym->s_name);
    else if (template->t_n != 1)
    	error("%s: has more than one field", x->x_templatesym->s_name);
    else
    {
    	*size = garray_npoints(x);
    	*vec =  (t_sample *)garray_vec(x);
    	return (1);
    }
    return (0);
}

    /* get any floating-point field of any element of an array */
float garray_get(t_garray *x, t_symbol *s, t_int indx)
{
    t_template *template = garray_template(x);
    int yonset, type;
    t_symbol *arraytype;
    if (!template_find_field(template, gensym("y"), &yonset,
    	&type, &arraytype) || type != DT_FLOAT)
    {
    	error("%s: needs floating-point '%s' field", x->x_templatesym->s_name,
    	    s->s_name);
    	return (0);
    }
    if (indx < 0) indx = 0;
    else if (indx >= x->x_n) indx = x->x_n - 1;
    return (*(float *)((x->x_vec + sizeof(t_word) * indx) + yonset));
}

    /* set the "saveit" flag */
void garray_setsaveit(t_garray *x, int saveit)
{
    if (x->x_saveit && !saveit)
    	post("warning: array %s: clearing save-in-patch flag",
	    x->x_name->s_name);
    x->x_saveit = saveit;
}

/*------------------- Pd messages ------------------------ */
static void garray_const(t_garray *x, t_floatarg g)
{
    t_template *template = garray_template(x);
    int yonset, type, i;
    t_symbol *arraytype;
    if (!template_find_field(template, gensym("y"), &yonset,
    	&type, &arraytype) || type != DT_FLOAT)
    	    error("%s: needs floating-point 'y' field",
	    	x->x_templatesym->s_name);
    else for (i = 0; i < x->x_n; i++)
    	    *(float *)(((char *)x->x_vec + sizeof(t_word) * i) + yonset) = g;
    garray_redraw(x);
}

    /* sum of Fourier components; called from routines below */
static void garray_dofo(t_garray *x, int npoints, float dcval,
    int nsin, t_float *vsin, int sineflag)
{
    t_template *template = garray_template(x);
    int yonset, type, i, j;
    t_symbol *arraytype;
    double phase, phaseincr, fj;
    if (npoints == 0)
    	npoints = 512;	/* dunno what a good default would be... */
    if (npoints != (1 << ilog2(npoints)))
    	post("%s: rounnding to %d points", x->x_templatesym->s_name,
	    (npoints = (1<<ilog2(npoints))));
    garray_resize(x, npoints + 3);
    phaseincr = 2. * 3.14159 / npoints;
    if (!template_find_field(template, gensym("y"), &yonset,
    	&type, &arraytype) || type != DT_FLOAT)
    {
    	error("%s: needs floating-point 'y' field", x->x_templatesym->s_name);
	return;
    }
    for (i = 0, phase = -phaseincr; i < x->x_n; i++, phase += phaseincr )
    {
    	double sum = dcval;
	if (sineflag)
	    for (j = 0, fj = phase; j < nsin; j++, fj += phase)
	    	sum += vsin[j] * sin(fj);
	else
	    for (j = 0, fj = 0; j < nsin; j++, fj += phase)
	    	sum += vsin[j] * cos(fj);
    	*(float *)((x->x_vec + sizeof(t_word) * i) + yonset) = sum;
    }
    garray_redraw(x);
}

static void garray_sinesum(t_garray *x, t_symbol *s, int argc, t_atom *argv)
{
#ifdef ROCKBOX
    (void) s;
#else
    t_template *template = garray_template(x);
#endif
    
    t_float *svec = (t_float *)t_getbytes(sizeof(t_float) * argc);
    int npoints, i;
    if (argc < 2)
    {
    	error("sinesum: %s: need number of points and partial strengths",
	    x->x_templatesym->s_name);
	return;
    }

    npoints = atom_getfloatarg(0, argc, argv);
    argv++, argc--;
    
    svec = (t_float *)t_getbytes(sizeof(t_float) * argc);
    if (!svec) return;
    
    for (i = 0; i < argc; i++)
    	svec[i] = atom_getfloatarg(i, argc, argv);
    garray_dofo(x, npoints, 0, argc, svec, 1);
    t_freebytes(svec, sizeof(t_float) * argc);
}

static void garray_cosinesum(t_garray *x, t_symbol *s, int argc, t_atom *argv)
{
#ifdef ROCKBOX
    (void) s;
#else
    t_template *template = garray_template(x);
#endif
    
    t_float *svec = (t_float *)t_getbytes(sizeof(t_float) * argc);
    int npoints, i;
    if (argc < 2)
    {
    	error("sinesum: %s: need number of points and partial strengths",
	    x->x_templatesym->s_name);
	return;
    }

    npoints = atom_getfloatarg(0, argc, argv);
    argv++, argc--;
    
    svec = (t_float *)t_getbytes(sizeof(t_float) * argc);
    if (!svec) return;

    for (i = 0; i < argc; i++)
    	svec[i] = atom_getfloatarg(i, argc, argv);
    garray_dofo(x, npoints, 0, argc, svec, 0);
    t_freebytes(svec, sizeof(t_float) * argc);
}

static void garray_normalize(t_garray *x, t_float f)
{
    t_template *template = garray_template(x);
#ifdef ROCKBOX
    int yonset, type, i;
#else
    int yonset, type, npoints, i;
#endif
    double maxv, renormer;
    t_symbol *arraytype;
    
    if (f <= 0)
    	f = 1;

    if (!template_find_field(template, gensym("y"), &yonset,
    	&type, &arraytype) || type != DT_FLOAT)
    {
    	error("%s: needs floating-point 'y' field", x->x_templatesym->s_name);
	return;
    }
    for (i = 0, maxv = 0; i < x->x_n; i++)
    {
    	double v = *(float *)((x->x_vec + sizeof(t_word) * i) + yonset);
	if (v > maxv)
	    maxv = v;
	if (-v > maxv)
	    maxv = -v;
    }
    if (maxv >= 0)
    {
	renormer = f / maxv;
	for (i = 0; i < x->x_n; i++)
	{
    	    *(float *)((x->x_vec + sizeof(t_word) * i) + yonset)
	    	*= renormer;
	}
    }
    garray_redraw(x);
}

    /* list -- the first value is an index; subsequent values are put in
    the "y" slot of the array.  This generalizes Max's "table", sort of. */
static void garray_list(t_garray *x, t_symbol *s, int argc, t_atom *argv)
{
    t_template *template = garray_template(x);
    int yonset, type, i;
    t_symbol *arraytype;
#ifdef ROCKBOX
    (void) s;
#endif
    if (!template_find_field(template, gensym("y"), &yonset,
    	&type, &arraytype) || type != DT_FLOAT)
    	    error("%s: needs floating-point 'y' field",
	    	x->x_templatesym->s_name);
    else if (argc < 2) return;
    else
    {
    	int firstindex = atom_getfloat(argv);
    	argc--;
    	argv++;
    	    /* drop negative x values */
    	if (firstindex < 0)
    	{
    	    argc += firstindex;
    	    argv -= firstindex;
    	    firstindex = 0;
    	    if (argc <= 0) return;
    	}
    	if (argc + firstindex > x->x_n)
    	{
    	    argc = x->x_n - firstindex;
    	    if (argc <= 0) return;
    	}
    	for (i = 0; i < argc; i++)
    	    *(t_sample *)((x->x_vec + sizeof(t_word) * (i + firstindex)) + yonset) = 
    	    	ftofix(atom_getfloat(argv + i));
    }
    garray_redraw(x);
}

    /* forward a "bounds" message to the owning graph */
static void garray_bounds(t_garray *x, t_floatarg x1, t_floatarg y1,
    t_floatarg x2, t_floatarg y2)
{
    vmess(&x->x_glist->gl_pd, gensym("bounds"), "ffff", x1, y1, x2, y2);
}

    /* same for "xticks", etc */
static void garray_xticks(t_garray *x,
    t_floatarg point, t_floatarg inc, t_floatarg f)
{
    vmess(&x->x_glist->gl_pd, gensym("xticks"), "fff", point, inc, f);
}

static void garray_yticks(t_garray *x,
    t_floatarg point, t_floatarg inc, t_floatarg f)
{
    vmess(&x->x_glist->gl_pd, gensym("yticks"), "fff", point, inc, f);
}

static void garray_xlabel(t_garray *x, t_symbol *s, int argc, t_atom *argv)
{
    typedmess(&x->x_glist->gl_pd, s, argc, argv);
}

static void garray_ylabel(t_garray *x, t_symbol *s, int argc, t_atom *argv)
{
    typedmess(&x->x_glist->gl_pd, s, argc, argv);
}
    /* change the name of a garray. */
static void garray_rename(t_garray *x, t_symbol *s)
{
    pd_unbind(&x->x_gobj.g_pd, x->x_realname);
    pd_bind(&x->x_gobj.g_pd, x->x_realname = x->x_name = s);
    garray_redraw(x);
}

static void garray_read(t_garray *x, t_symbol *filename)
{
    int nelem = x->x_n, filedesc;
#ifdef ROCKBOX
    int fd = 0;
#else
    FILE *fd;
#endif
    char buf[MAXPDSTRING], *bufptr;
    t_template *template = garray_template(x);
    int yonset, type, i;
    t_symbol *arraytype;
    if (!template_find_field(template, gensym("y"), &yonset,
    	&type, &arraytype) || type != DT_FLOAT)
    {
    	error("%s: needs floating-point 'y' field", x->x_templatesym->s_name);
    	return;
    }
    if ((filedesc = open_via_path(
    	canvas_getdir(glist_getcanvas(x->x_glist))->s_name,
    	    filename->s_name, "", buf, &bufptr, MAXPDSTRING, 0)) < 0 
#ifdef ROCKBOX
                )
#else
	    	|| !(fd = fdopen(filedesc, "r")))
#endif
    {
    	error("%s: can't open", filename->s_name);
    	return;
    }
    for (i = 0; i < nelem; i++)
    {
#ifdef ROCKBOX
        if(rb_fscanf_f(fd, (float*)((x->x_vec + sizeof(t_word) * i) + yonset)))
#else
    	if (!fscanf(fd, "%f", (float *)((x->x_vec + sizeof(t_word) * i) +
	    yonset)))
#endif
    	{
    	    post("%s: read %d elements into table of size %d",
    	    	filename->s_name, i, nelem);
    	    break;
    	}
    }
    while (i < nelem)
    	*(float *)((x->x_vec + sizeof(t_word) * i) + yonset) = 0, i++;
#ifdef ROCKBOX
    close(fd);
#else
    fclose(fd);
#endif
    garray_redraw(x);
}

    /* this should be renamed and moved... */
int garray_ambigendian(void)
{
    unsigned short s = 1;
    unsigned char c = *(char *)(&s);
    return (c==0);
}

#define BINREADMODE "rb"
#define BINWRITEMODE "wb"

static void garray_read16(t_garray *x, t_symbol *filename,
    t_symbol *endian, t_floatarg fskip)
{
    int skip = fskip, filedesc;
    int i, nelem;
    t_sample *vec;
#ifdef ROCKBOX
    int fd = 0;
#else
    FILE *fd;
#endif
    char buf[MAXPDSTRING], *bufptr;
    short s;
    int cpubig = garray_ambigendian(), swap = 0;
    char c = endian->s_name[0];
    if (c == 'b')
    {
    	if (!cpubig) swap = 1;
    }
    else if (c == 'l')
    {
    	if (cpubig) swap = 1;
    }
    else if (c)
    {
    	error("array_read16: endianness is 'l' (low byte first ala INTEL)");
    	post("... or 'b' (high byte first ala MIPS,DEC,PPC)");
    }
    if (!garray_getfloatarray(x, &nelem, &vec))
    {
    	error("%s: not a float array", x->x_templatesym->s_name);
    	return;
    }
    if ((filedesc = open_via_path(
    	canvas_getdir(glist_getcanvas(x->x_glist))->s_name,
    	    filename->s_name, "", buf, &bufptr, MAXPDSTRING, 1)) < 0 
#ifdef ROCKBOX
        )
#else
	    	|| !(fd = fdopen(filedesc, BINREADMODE)))
#endif
    {
    	error("%s: can't open", filename->s_name);
    	return;
    }
    if (skip)
    {
#ifdef ROCKBOX
        long pos = lseek(fd, (long)skip, SEEK_SET);
#else
    	long pos = fseek(fd, (long)skip, SEEK_SET);
#endif
    	if (pos < 0)
    	{
    	    error("%s: can't seek to byte %d", buf, skip);
#ifdef ROCKBOX
            close(fd);
#else
    	    fclose(fd);
#endif
    	    return;
    	}
    }

    for (i = 0; i < nelem; i++)
    {
#ifdef ROCKBOX
        if(read(fd, &s, sizeof(s)) < 1)
#else
    	if (fread(&s, sizeof(s), 1, fd) < 1)
#endif
    	{
    	    post("%s: read %d elements into table of size %d",
    	    	filename->s_name, i, nelem);
    	    break;
    	}
    	if (swap) s = ((s & 0xff) << 8) | ((s & 0xff00) >> 8);
    	vec[i] = s * (1./32768.);
    }
    while (i < nelem) vec[i++] = 0;
#ifdef ROCKBOX
    close(fd);
#else
    fclose(fd);
#endif
    garray_redraw(x);
}

static void garray_write(t_garray *x, t_symbol *filename)
{
#ifdef ROCKBOX
    int fd;
#else
    FILE *fd;
#endif
    char buf[MAXPDSTRING];
    t_template *template = garray_template(x);
    int yonset, type, i;
    t_symbol *arraytype;
    if (!template_find_field(template, gensym("y"), &yonset,
    	&type, &arraytype) || type != DT_FLOAT)
    {
    	error("%s: needs floating-point 'y' field", x->x_templatesym->s_name);
    	return;
    }
    canvas_makefilename(glist_getcanvas(x->x_glist), filename->s_name,
    	buf, MAXPDSTRING);
    sys_bashfilename(buf, buf);
#ifdef ROCKBOX
    if(!(fd = open(buf, O_WRONLY|O_CREAT|O_TRUNC, 0666)))
#else
    if (!(fd = fopen(buf, "w")))
#endif
    {
    	error("%s: can't create", buf);
    	return;
    }
    for (i = 0; i < x->x_n; i++)
    {
#ifdef ROCKBOX
        if(rb_fprintf_f(fd, 
#else /* ROCKBOX */
    	if (fprintf(fd, "%g\n",
#endif /* ROCKBOX */
    	    *(float *)((x->x_vec + sizeof(t_word) * i) + yonset)) < 1)
    	{
    	    post("%s: write error", filename->s_name);
    	    break;
    	}
    }
#ifdef ROCKBOX
    close(fd);
#else
    fclose(fd);
#endif
}

static unsigned char waveheader[] = {
0x52, 0x49, 0x46, 0x46,
0x00, 0x00, 0x00, 0x00,
0x57, 0x41, 0x56, 0x45,
0x66, 0x6d, 0x74, 0x20,

0x10, 0x00, 0x00, 0x00,
0x01, 0x00, 0x01, 0x00,
0x44, 0xac, 0x00, 0x00,
0x88, 0x58, 0x01, 0x00,

0x02, 0x00, 0x10, 0x00,
0x64, 0x61, 0x74, 0x61,
0x00, 0x00, 0x00, 0x00,
};

    /* wave format only so far */
static void garray_write16(t_garray *x, t_symbol *filename, t_symbol *format)
{
    t_template *template = garray_template(x);
    int yonset, type, i;
    t_symbol *arraytype;
#ifdef ROCKBOX
    int fd;
#else
    FILE *fd;
#endif
    int aiff = (format == gensym("aiff"));
    char filenamebuf[MAXPDSTRING], buf2[MAXPDSTRING];
    int swap = garray_ambigendian();	/* wave is only little endian */
    int intbuf;
    strncpy(filenamebuf, filename->s_name, MAXPDSTRING-10);
    filenamebuf[MAXPDSTRING-10] = 0;
    if (sizeof(int) != 4) post("write16: only works on 32-bit machines");
    if (aiff)
    {
    	if (strcmp(filenamebuf + strlen(filenamebuf)-5, ".aiff"))
    	    strcat(filenamebuf, ".aiff");
    }
    else
    {
    	if (strcmp(filenamebuf + strlen(filenamebuf)-4, ".wav"))
    	    strcat(filenamebuf, ".wav");
    }
    if (!template_find_field(template, gensym("y"), &yonset,
    	&type, &arraytype) || type != DT_FLOAT)
    {
    	error("%s: needs floating-point 'y' field", x->x_templatesym->s_name);
    	return;
    }
    canvas_makefilename(glist_getcanvas(x->x_glist), filenamebuf,
    	buf2, MAXPDSTRING);
    sys_bashfilename(buf2, buf2);
#ifdef ROCKBOX
    if(!(fd = open(buf2, O_WRONLY|O_CREAT|O_TRUNC, 0666)))
#else
    if (!(fd = fopen(buf2, BINWRITEMODE)))
#endif
    {
    	error("%s: can't create", buf2);
    	return;
    }
    intbuf = 2 * x->x_n + 36;
    if (swap)
    {
    	unsigned char *foo = (unsigned char *)&intbuf, xxx;
    	xxx = foo[0]; foo[0] = foo[3]; foo[3] = xxx;
    	xxx = foo[1]; foo[1] = foo[2]; foo[2] = xxx;
    }
    memcpy((void *)(waveheader + 4), (void *)(&intbuf), 4);
    intbuf = 2 * x->x_n;
    if (swap)
    {
    	unsigned char *foo = (unsigned char *)&intbuf, xxx;
    	xxx = foo[0]; foo[0] = foo[3]; foo[3] = xxx;
    	xxx = foo[1]; foo[1] = foo[2]; foo[2] = xxx;
    }
    memcpy((void *)(waveheader + 40), (void *)(&intbuf), 4);
#ifdef ROCKBOX
    if(write(fd, waveheader, sizeof(waveheader)) < 1)
#else
    if (fwrite(waveheader, sizeof(waveheader), 1, fd) < 1)
#endif
    {
    	post("%s: write error", buf2);
    	goto closeit;
    }
    for (i = 0; i < x->x_n; i++)
    {
    	float f = 32767. * *(float *)((x->x_vec + sizeof(t_word) * i) + yonset);
    	short sh;
    	if (f < -32768) f = -32768;
    	else if (f > 32767) f = 32767;
    	sh = f;
    	if (swap)
    	{
    	    unsigned char *foo = (unsigned char *)&sh, xxx;
    	    xxx = foo[0]; foo[0] = foo[1]; foo[1] = xxx;
    	}
#ifdef ROCKBOX
        if(write(fd, &sh, sizeof(sh)) < 1)
#else
	if (fwrite(&sh, sizeof(sh), 1, fd) < 1)
#endif
	{
    	    post("%s: write error", buf2);
    	    goto closeit;
	}
    }
closeit:
#ifdef ROCKBOX
    close(fd);
#else
    fclose(fd);
#endif
}

void garray_resize(t_garray *x, t_floatarg f)
{
    int was = x->x_n, elemsize;
    t_glist *gl;
#ifndef ROCKBOX
    int dspwas;
#endif
    int n = f;
    char *nvec;
    
    if (n < 1) n = 1;
    elemsize = template_findbyname(x->x_templatesym)->t_n * sizeof(t_word);
    nvec = t_resizebytes(x->x_vec, was * elemsize, n * elemsize);
    if (!nvec)
    {
    	pd_error(x, "array resize failed: out of memory");
	return;
    }
    x->x_vec = nvec;
    	/* LATER should check t_resizebytes result */
    if (n > was)
    	memset(x->x_vec + was*elemsize,
    	    0, (n - was) * elemsize);
    x->x_n = n;
    
    	/* if this is the only array in the graph,
    	    reset the graph's coordinates */
    gl = x->x_glist;
    if (gl->gl_list == &x->x_gobj && !x->x_gobj.g_next)
    {
    	vmess(&gl->gl_pd, gensym("bounds"), "ffff",
    	    0., gl->gl_y1, (double)(n > 1 ? n-1 : 1), gl->gl_y2);
	    	/* close any dialogs that might have the wrong info now... */
#ifndef ROCKBOX
    	gfxstub_deleteforkey(gl);
#endif
    }
    else garray_redraw(x);
    if (x->x_usedindsp) canvas_update_dsp();
}

static void garray_print(t_garray *x)
{  
    post("garray %s: template %s, length %d",
    	x->x_name->s_name, x->x_templatesym->s_name, x->x_n);
}

void g_array_setup(void)
{
    garray_class = class_new(gensym("array"), 0, (t_method)garray_free,
    	sizeof(t_garray), CLASS_GOBJ, 0);
    class_setwidget(garray_class, &garray_widgetbehavior);
    class_addmethod(garray_class, (t_method)garray_const, gensym("const"),
    	A_DEFFLOAT, A_NULL);
    class_addlist(garray_class, garray_list);
    class_addmethod(garray_class, (t_method)garray_bounds, gensym("bounds"),
    	A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_NULL);
    class_addmethod(garray_class, (t_method)garray_xticks, gensym("xticks"),
    	A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(garray_class, (t_method)garray_xlabel, gensym("xlabel"),
    	A_GIMME, 0);
    class_addmethod(garray_class, (t_method)garray_yticks, gensym("yticks"),
    	A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(garray_class, (t_method)garray_ylabel, gensym("ylabel"),
    	A_GIMME, 0);
    class_addmethod(garray_class, (t_method)garray_rename, gensym("rename"),
    	A_SYMBOL, 0);
    class_addmethod(garray_class, (t_method)garray_read, gensym("read"),
    	A_SYMBOL, A_NULL);
    class_addmethod(garray_class, (t_method)garray_read16, gensym("read16"),
    	A_SYMBOL, A_DEFFLOAT, A_DEFSYM, A_NULL);
    class_addmethod(garray_class, (t_method)garray_write, gensym("write"),
    	A_SYMBOL, A_NULL);
    class_addmethod(garray_class, (t_method)garray_write16, gensym("write16"),
    	A_SYMBOL, A_DEFSYM, A_NULL);
    class_addmethod(garray_class, (t_method)garray_resize, gensym("resize"),
    	A_FLOAT, A_NULL);
    class_addmethod(garray_class, (t_method)garray_print, gensym("print"),
    	A_NULL);
    class_addmethod(garray_class, (t_method)garray_sinesum, gensym("sinesum"),
    	A_GIMME, 0);
    class_addmethod(garray_class, (t_method)garray_cosinesum,
    	gensym("cosinesum"), A_GIMME, 0);
    class_addmethod(garray_class, (t_method)garray_normalize,
    	gensym("normalize"), A_DEFFLOAT, 0);
    class_addmethod(garray_class, (t_method)garray_arraydialog,
    	gensym("arraydialog"), A_SYMBOL, A_FLOAT, A_FLOAT, A_FLOAT, A_NULL);
    class_setsavefn(garray_class, garray_save);
}

