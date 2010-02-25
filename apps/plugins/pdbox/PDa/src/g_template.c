/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#ifdef ROCKBOX
#include "plugin.h"
#include "../../pdbox.h"
#include "m_pd.h"
#include "s_stuff.h"
#include "g_canvas.h"
#define snprintf rb->snprintf
#else /* ROCKBOX */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "m_pd.h"
#include "s_stuff.h"  	/* for sys_hostfontsize */
#include "g_canvas.h"
#endif /* ROCKBOX */

/*
This file contains text objects you would put in a canvas to define a
template.  Templates describe objects of type "array" (g_array.c) and
"scalar" (g_scalar.c).
*/

/* T.Grill - changed the _template.t_pd member to t_pdobj to avoid name clashes
with the t_pd type */

    /* the structure of a "struct" object (also the obsolete "gtemplate"
    you get when using the name "template" in a box.) */

struct _gtemplate
{
    t_object x_obj;
    t_template *x_template;
    t_canvas *x_owner;
    t_symbol *x_sym;
    struct _gtemplate *x_next;
    int x_argc;
    t_atom *x_argv;
};

/* ---------------- forward definitions ---------------- */

static void template_conformarray(t_template *tfrom, t_template *tto,
    int *conformaction, t_array *a);
static void template_conformglist(t_template *tfrom, t_template *tto,
    t_glist *glist,  int *conformaction);

/* ---------------------- storage ------------------------- */

static t_class *gtemplate_class;
static t_class *template_class;

/* there's a pre-defined "float" template.  LATER should we bind this
to a symbol such as "pd-float"??? */

static t_dataslot template_float_vec =
{
    DT_FLOAT,
    &s_y,
    &s_
};

static t_template template_float =
{
    0,	    	    /* class -- fill in in setup routine */
    0,	    	    /* list of "struct"/t_gtemplate objects */
    &s_float,	    /* name */
    1,	    	    /* number of items */
    &template_float_vec
};

    /* return true if two dataslot definitions match */
static int dataslot_matches(t_dataslot *ds1, t_dataslot *ds2,
    int nametoo)
{
    return ((!nametoo || ds1->ds_name == ds2->ds_name) &&
    	ds1->ds_type == ds2->ds_type &&
    	    (ds1->ds_type != DT_ARRAY ||
		ds1->ds_arraytemplate == ds2->ds_arraytemplate));
}

/* -- templates, the active ingredient in gtemplates defined below. ------- */

t_template *template_new(t_symbol *templatesym, int argc, t_atom *argv)
{
    t_template *x = (t_template *)pd_new(template_class);
    x->t_n = 0;
    x->t_vec = (t_dataslot *)t_getbytes(0);
    while (argc > 0)
    {
    	int newtype, oldn, newn;
	t_symbol *newname, *newarraytemplate = &s_, *newtypesym;
    	if (argc < 2 || argv[0].a_type != A_SYMBOL ||
	    argv[1].a_type != A_SYMBOL)
	    	goto bad;
    	newtypesym = argv[0].a_w.w_symbol;
	newname = argv[1].a_w.w_symbol;
	if (newtypesym == &s_float)
	    newtype = DT_FLOAT;
	else if (newtypesym == &s_symbol)
	    newtype = DT_SYMBOL;
	else if (newtypesym == &s_list)
    	    newtype = DT_LIST;
	else if (newtypesym == gensym("array"))
	{
	    if (argc < 3 || argv[2].a_type != A_SYMBOL)
	    {
	    	pd_error(x, "array lacks element template or name");
		goto bad;
	    }
	    newarraytemplate = canvas_makebindsym(argv[2].a_w.w_symbol);
	    newtype = DT_ARRAY;
	    argc--;
	    argv++;
	}
	else
	{
    	    pd_error(x, "%s: no such type", newtypesym->s_name);
    	    return (0);
	}
	newn = (oldn = x->t_n) + 1;
	x->t_vec = (t_dataslot *)t_resizebytes(x->t_vec,
	    oldn * sizeof(*x->t_vec), newn * sizeof(*x->t_vec));
	x->t_n = newn;
	x->t_vec[oldn].ds_type = newtype;
	x->t_vec[oldn].ds_name = newname;
	x->t_vec[oldn].ds_arraytemplate = newarraytemplate;
    bad: 
    	argc -= 2; argv += 2;
    }
    if (templatesym->s_name)
    {
    	x->t_sym = templatesym;
    	pd_bind(&x->t_pdobj, x->t_sym);
    }
    else x->t_sym = templatesym;
    return (x);
}

int template_size(t_template *x)
{
    return (x->t_n * sizeof(t_word));
}

int template_find_field(t_template *x, t_symbol *name, int *p_onset,
    int *p_type, t_symbol **p_arraytype)
{
#ifndef ROCKBOX
    t_template *t;
#endif
    int i, n;
    if (!x)
    {
    	bug("template_find_field");
    	return (0);
    }
    n = x->t_n;
    for (i = 0; i < n; i++)
    	if (x->t_vec[i].ds_name == name)
    {
    	*p_onset = i * sizeof(t_word);
    	*p_type = x->t_vec[i].ds_type;
    	*p_arraytype = x->t_vec[i].ds_arraytemplate;
	return (1);
    }
    return (0);
}

t_float template_getfloat(t_template *x, t_symbol *fieldname, t_word *wp,
    int loud)
{
    int onset, type;
    t_symbol *arraytype;
    t_sample val = 0;
    if (template_find_field(x, fieldname, &onset, &type, &arraytype))
    {
    	if (type == DT_FLOAT)
	    val = *(t_sample *)(((char *)wp) + onset);
    	else if (loud) error("%s.%s: not a number",
    	    x->t_sym->s_name, fieldname->s_name);
    }
    else if (loud) error("%s.%s: no such field",
    	x->t_sym->s_name, fieldname->s_name);
    return (fixtof(val));
}

void template_setfloat(t_template *x, t_symbol *fieldname, t_word *wp, 
    t_float f, int loud)
{
    int onset, type;
    t_symbol *arraytype;
    if (template_find_field(x, fieldname, &onset, &type, &arraytype))
     {
    	if (type == DT_FLOAT)
    	    *(t_sample *)(((char *)wp) + onset) = ftofix(f);
    	else if (loud) error("%s.%s: not a number",
    	    x->t_sym->s_name, fieldname->s_name);
    }
    else if (loud) error("%s.%s: no such field",
    	x->t_sym->s_name, fieldname->s_name);
}

t_symbol *template_getsymbol(t_template *x, t_symbol *fieldname, t_word *wp,
    int loud)
{
    int onset, type;
    t_symbol *arraytype;
    t_symbol *val = &s_;
    if (template_find_field(x, fieldname, &onset, &type, &arraytype))
    {
    	if (type == DT_SYMBOL)
	    val = *(t_symbol **)(((char *)wp) + onset);
    	else if (loud) error("%s.%s: not a symbol",
    	    x->t_sym->s_name, fieldname->s_name);
    }
    else if (loud) error("%s.%s: no such field",
    	x->t_sym->s_name, fieldname->s_name);
    return (val);
}

void template_setsymbol(t_template *x, t_symbol *fieldname, t_word *wp, 
    t_symbol *s, int loud)
{
    int onset, type;
    t_symbol *arraytype;
    if (template_find_field(x, fieldname, &onset, &type, &arraytype))
     {
    	if (type == DT_SYMBOL)
    	    *(t_symbol **)(((char *)wp) + onset) = s;
    	else if (loud) error("%s.%s: not a symbol",
    	    x->t_sym->s_name, fieldname->s_name);
    }
    else if (loud) error("%s.%s: no such field",
    	x->t_sym->s_name, fieldname->s_name);
}

    /* stringent check to see if a "saved" template, x2, matches the current
    	one (x1).  It's OK if x1 has additional scalar elements but not (yet)
	arrays or lists.  This is used for reading in "data files". */
int template_match(t_template *x1, t_template *x2)
{
    int i;
    if (x1->t_n < x2->t_n)
    	return (0);
    for (i = x2->t_n; i < x1->t_n; i++)
    {
    	if (x1->t_vec[i].ds_type == DT_ARRAY || 
	    x1->t_vec[i].ds_type == DT_LIST)
	    	return (0);
    }
    if (x2->t_n > x1->t_n)
    	post("add elements...");
    for (i = 0; i < x2->t_n; i++)
    	if (!dataslot_matches(&x1->t_vec[i], &x2->t_vec[i], 1))
    	    return (0);
    return (1);
}

/* --------------- CONFORMING TO CHANGES IN A TEMPLATE ------------ */

/* the following routines handle updating scalars to agree with changes
in their template.  The old template is assumed to be the "installed" one
so we can delete old items; but making new ones we have to avoid scalar_new
which would make an old one whereas we will want a new one (but whose array
elements might still be old ones. 
    LATER deal with graphics updates too... */

    /* conform the word vector of a scalar to the new template */    
static void template_conformwords(t_template *tfrom, t_template *tto,
    int *conformaction, t_word *wfrom, t_word *wto)
{
#ifdef ROCKBOX
    int nto = tto->t_n, i;

    (void) tfrom;
#else
    int nfrom = tfrom->t_n, nto = tto->t_n, i;
#endif
    for (i = 0; i < nto; i++)
    {
    	if (conformaction[i] >= 0)
	{
	    	/* we swap the two, in case it's an array or list, so that
		when "wfrom" is deleted the old one gets cleaned up. */
	    t_word wwas = wto[i];
	    wto[i] = wfrom[conformaction[i]];
	    wfrom[conformaction[i]] = wwas;
	}
    }
}

    /* conform a scalar, recursively conforming sublists and arrays  */
static t_scalar *template_conformscalar(t_template *tfrom, t_template *tto,
    int *conformaction, t_glist *glist, t_scalar *scfrom)
{
    t_scalar *x;
    t_gpointer gp;
#ifdef ROCKBOX
    int i;
#else
    int nto = tto->t_n, nfrom = tfrom->t_n, i;
#endif
    t_template *scalartemplate;
    /* post("conform scalar"); */
    	/* possibly replace the scalar */
    if (scfrom->sc_template == tfrom->t_sym)
    {
    	    /* see scalar_new() for comment about the gpointer. */
	gpointer_init(&gp);
	x = (t_scalar *)getbytes(sizeof(t_scalar) +
    	    (tto->t_n - 1) * sizeof(*x->sc_vec));
	x->sc_gobj.g_pd = scalar_class;
	x->sc_template = tfrom->t_sym;
	gpointer_setglist(&gp, glist, x);
    	    /* Here we initialize to the new template, but array and list
	    elements will still belong to old template. */
	word_init(x->sc_vec, tto, &gp);

    	template_conformwords(tfrom, tto, conformaction,
	    scfrom->sc_vec, x->sc_vec);
	    
	    /* replace the old one with the new one in the list */
	if (glist->gl_list == &scfrom->sc_gobj)
	{
	    glist->gl_list = &x->sc_gobj;
	    x->sc_gobj.g_next = scfrom->sc_gobj.g_next;
	}
	else
	{
	    t_gobj *y, *y2;
	    for (y = glist->gl_list; (y2 = y->g_next); y = y2)
	    	if (y2 == &scfrom->sc_gobj)
	    {
	    	x->sc_gobj.g_next = y2->g_next;
		y->g_next = &x->sc_gobj;
		goto nobug;
	    }
	    bug("template_conformscalar");
	nobug: ;
	}
	    /* burn the old one */
	pd_free(&scfrom->sc_gobj.g_pd);
    }
    else x = scfrom;
    scalartemplate = template_findbyname(x->sc_template);
    	/* convert all array elements and sublists */
    for (i = 0; i < scalartemplate->t_n; i++)
    {
    	t_dataslot *ds = scalartemplate->t_vec + i;
    	if (ds->ds_type == DT_LIST)
	{
	    t_glist *gl2 = x->sc_vec[i].w_list;
	    template_conformglist(tfrom, tto, gl2, conformaction);
	}
	else if (ds->ds_type == DT_ARRAY)
	{
	    template_conformarray(tfrom, tto, conformaction, 
	    	x->sc_vec[i].w_array);
	}
    }
    return (x);
}

    /* conform an array, recursively conforming sublists and arrays  */
static void template_conformarray(t_template *tfrom, t_template *tto,
    int *conformaction, t_array *a)
{
    int i;
    if (a->a_templatesym == tfrom->t_sym)
    {
	/* the array elements must all be conformed */
	int oldelemsize = sizeof(t_word) * tfrom->t_n,
	    newelemsize = sizeof(t_word) * tto->t_n;
	char *newarray = getbytes(sizeof(t_word) * tto->t_n * a->a_n);
	char *oldarray = a->a_vec;
	if (a->a_elemsize != oldelemsize)
	    bug("template_conformarray");
	for (i = 0; i < a->a_n; i++)
	{
    	    t_word *wp = (t_word *)(newarray + newelemsize * i);
    	    word_init(wp, tto, &a->a_gp);
	    template_conformwords(tfrom, tto, conformaction,
		(t_word *)(oldarray + oldelemsize * i), wp);
	}
    }
    bug("template_conformarray: this part not written");
    	/* go through item by item conforming subarrays and sublists... */
}

    /* this routine searches for every scalar in the glist that belongs
    to the "from" template and makes it belong to the "to" template.  Descend
    glists recursively.
    We don't handle redrawing here; this is to be filled in LATER... */

static void template_conformglist(t_template *tfrom, t_template *tto,
    t_glist *glist,  int *conformaction)
{
    t_gobj *g;
    /* post("conform glist %s", glist->gl_name->s_name); */
    for (g = glist->gl_list; g; g = g->g_next)
    {
    	if (pd_class(&g->g_pd) == scalar_class)
	    g = &template_conformscalar(tfrom, tto, conformaction,
	    	glist, (t_scalar *)g)->sc_gobj;
    	else if (pd_class(&g->g_pd) == canvas_class)
	    template_conformglist(tfrom, tto, (t_glist *)g, conformaction);
    }
}

    /* globally conform all scalars from one template to another */ 
void template_conform(t_template *tfrom, t_template *tto)
{
    int nto = tto->t_n, nfrom = tfrom->t_n, i, j,
    	*conformaction = (int *)getbytes(sizeof(int) * nto),
	*conformedfrom = (int *)getbytes(sizeof(int) * nfrom), doit = 0;
    for (i = 0; i < nto; i++)
    	conformaction[i] = -1;
    for (i = 0; i < nfrom; i++)
    	conformedfrom[i] = 0;
    for (i = 0; i < nto; i++)
    {
    	t_dataslot *dataslot = &tto->t_vec[i];
    	for (j = 0; j < nfrom; j++)
	{
	    t_dataslot *dataslot2 = &tfrom->t_vec[j];
	    if (dataslot_matches(dataslot, dataslot2, 1))
	    {
	    	conformaction[i] = j;
		conformedfrom[j] = 1;
	    }
	}
    }
    for (i = 0; i < nto; i++)
    	if (conformaction[i] < 0)
    {
    	t_dataslot *dataslot = &tto->t_vec[i];
    	for (j = 0; j < nfrom; j++)
	    if (!conformedfrom[j] &&
	    	dataslot_matches(dataslot, &tfrom->t_vec[j], 1))
	{
	    conformaction[i] = j;
	    conformedfrom[j] = 1;
	}
    }
    if (nto != nfrom)
    	doit = 1;
    else for (i = 0; i < nto; i++)
    	if (conformaction[i] != i)
    	    doit = 1;

    if (doit)
    {
    	t_glist *gl;
    	/* post("conforming template '%s' to new structure",
	    tfrom->t_sym->s_name);
	for (i = 0; i < nto; i++)
	    post("... %d", conformaction[i]); */
    	for (gl = canvas_list; gl; gl = gl->gl_next)
    	    template_conformglist(tfrom, tto, gl, conformaction);
    }
    freebytes(conformaction, sizeof(int) * nto);
    freebytes(conformedfrom, sizeof(int) * nfrom);
}

t_template *template_findbyname(t_symbol *s)
{
#ifndef ROCKBOX
    int i;
#endif
    if (s == &s_float)
    	return (&template_float);
    else return ((t_template *)pd_findbyclass(s, template_class));
}

t_canvas *template_findcanvas(t_template *template)
{
    t_gtemplate *gt;
    if (!template) 
    	bug("template_findcanvas");
    if (!(gt = template->t_list))
    	return (0);
    return (gt->x_owner);
    /* return ((t_canvas *)pd_findbyclass(template->t_sym, canvas_class)); */
}

    /* call this when reading a patch from a file to declare what templates
    we'll need.  If there's already a template, check if it matches.
    If it doesn't it's still OK as long as there are no "struct" (gtemplate)
    objects hanging from it; we just conform everyone to the new template.
    If there are still struct objects belonging to the other template, we're
    in trouble.  LATER we'll figure out how to conform the new patch's objects
    to the pre-existing struct. */
static void *template_usetemplate(void *dummy, t_symbol *s,
    int argc, t_atom *argv)
{
    t_template *x;
    t_symbol *templatesym =
    	canvas_makebindsym(atom_getsymbolarg(0, argc, argv));
#ifdef ROCKBOX
    (void) dummy;
    (void) s;
#endif
    if (!argc)
    	return (0);
    argc--; argv++;
    	    /* check if there's already a template by this name. */
    if ((x = (t_template *)pd_findbyclass(templatesym, template_class)))
    {
    	t_template *y = template_new(&s_, argc, argv);
	    /* If the new template is the same as the old one,
	    there's nothing to do.  */
	if (!template_match(x, y))
	{
	    	/* Are there "struct" objects upholding this template? */
	    if (x->t_list)
	    {
	    	    /* don't know what to do here! */
	    	error("%s: template mismatch",
		    templatesym->s_name);
	    }
	    else
	    {
	    	    /* conform everyone to the new template */
		template_conform(x, y);
		pd_free(&x->t_pdobj);
	    	template_new(templatesym, argc, argv);
	    }
	}
	pd_free(&y->t_pdobj);
    }
    	/* otherwise, just make one. */
    else template_new(templatesym, argc, argv);
    return (0);
}

    /* here we assume someone has already cleaned up all instances of this. */
void template_free(t_template *x)
{
    if (*x->t_sym->s_name)
    	pd_unbind(&x->t_pdobj, x->t_sym);
    t_freebytes(x->t_vec, x->t_n * sizeof(*x->t_vec));
}

static void template_setup(void)
{
    template_class = class_new(gensym("template"), 0, (t_method)template_free,
    	sizeof(t_template), CLASS_PD, 0);
    class_addmethod(pd_canvasmaker, (t_method)template_usetemplate,
    	gensym("struct"), A_GIMME, 0);
    	
}

/* ---------------- gtemplates.  One per canvas. ----------- */

/* this is a "text" object that searches for, and if necessary creates, 
a "template" (above).  Other objects in the canvas then can give drawing
instructions for the template.  The template doesn't go away when the
gtemplate is deleted, so that you can replace it with
another one to add new fields, for example. */

static void *gtemplate_donew(t_symbol *sym, int argc, t_atom *argv)
{
    t_gtemplate *x = (t_gtemplate *)pd_new(gtemplate_class);
    t_template *t = template_findbyname(sym);
    int i;
#ifndef ROCKBOX
    t_symbol *sx = gensym("x");
#endif
    x->x_owner = canvas_getcurrent();
    x->x_next = 0;
    x->x_sym = sym;
    x->x_argc = argc;
    x->x_argv = (t_atom *)getbytes(argc * sizeof(t_atom));
    for (i = 0; i < argc; i++)
    	x->x_argv[i] = argv[i];

    	/* already have a template by this name? */
    if (t)
    {
    	x->x_template = t;
    	    /* if it's already got a "struct" or "gtemplate" object we
	    just tack this one to the end of the list and leave it
	    there. */
    	if (t->t_list)
	{
    	    t_gtemplate *x2, *x3;
	    for(x2 = x->x_template->t_list; (x3 = x2->x_next); x2 = x3)
		;
	    x2->x_next = x;
	    post("template %s: warning: already exists.", sym->s_name);
	}
	else
	{
	    	/* if there's none, we just replace the template with
		our own and conform it. */
    	    t_template *y = template_new(&s_, argc, argv);
		/* Unless the new template is different from the old one,
		there's nothing to do.  */
	    if (!template_match(t, y))
	    {
	    	    /* conform everyone to the new template */
		template_conform(t, y);
		pd_free(&t->t_pdobj);
	    	t = template_new(sym, argc, argv);
	    }
	    pd_free(&y->t_pdobj);
    	    t->t_list = x;
	}
    }
    else
    {
    	    /* otherwise make a new one and we're the only struct on it. */
    	x->x_template = t = template_new(sym, argc, argv);
	t->t_list = x;
    }
    return (x);
}

static void *gtemplate_new(t_symbol *s, int argc, t_atom *argv)
{
#ifndef ROCKBOX
    t_gtemplate *x = (t_gtemplate *)pd_new(gtemplate_class);
#endif
    t_symbol *sym = atom_getsymbolarg(0, argc, argv);
#ifdef ROCKBOX
    (void) s;
#endif
    if (argc >= 1)
    	argc--; argv++;
    return (gtemplate_donew(canvas_makebindsym(sym), argc, argv));
}

    /* old version (0.34) -- delete 2003 or so */
static void *gtemplate_new_old(t_symbol *s, int argc, t_atom *argv)
{
#ifndef ROCKBOX
    t_gtemplate *x = (t_gtemplate *)pd_new(gtemplate_class);
#endif
    t_symbol *sym = canvas_makebindsym(canvas_getcurrent()->gl_name);
    static int warned;
#ifdef ROCKBOX
    (void) s;
#endif
    if (!warned)
    {
    	post("warning -- 'template' (%s) is obsolete; replace with 'struct'",
	    sym->s_name);
	warned = 1;
    }
    return (gtemplate_donew(sym, argc, argv));
}

t_template *gtemplate_get(t_gtemplate *x)
{
    return (x->x_template);
}

static void gtemplate_free(t_gtemplate *x)
{
    	/* get off the template's list */
    t_template *t = x->x_template;
    if (x == t->t_list)
    {
    	if (x->x_next)
	{
	    	/* if we were first on the list, and there are others on
		the list, make a new template corresponding to the new
		first-on-list and replace teh existing template with it. */
	    t_template *z = template_new(&s_, x->x_argc, x->x_argv);
	    template_conform(t, z);
	    pd_free(&t->t_pdobj);
	    pd_free(&z->t_pdobj);
	    z = template_new(x->x_sym, x->x_argc, x->x_argv);
	    z->t_list = x->x_next;
	}
    	else t->t_list = 0;
    }
    else
    {
    	t_gtemplate *x2, *x3;
	for(x2 = t->t_list; (x3 = x2->x_next); x2 = x3)
	{
	    if (x == x3)
	    {
	    	x2->x_next = x3->x_next;
		break;
	    }
	}
    }
    freebytes(x->x_argv, sizeof(t_atom) * x->x_argc);
}

static void gtemplate_setup(void)
{
    gtemplate_class = class_new(gensym("struct"),
    	(t_newmethod)gtemplate_new, (t_method)gtemplate_free,
    	sizeof(t_gtemplate), CLASS_NOINLET, A_GIMME, 0);
    class_addcreator((t_newmethod)gtemplate_new_old, gensym("template"),
    	A_GIMME, 0);
}

/* ---------------  FIELD DESCRIPTORS ---------------------- */

/* a field descriptor can hold a constant or a variable; if a variable,
it's the name of a field in the template we belong to.  LATER, we might
want to cache the offset of the field so we don't have to search for it
every single time we draw the object.
*/

typedef struct _fielddesc
{
    char fd_type;   	/* LATER consider removing this? */
    char fd_var;
    union
    {
    	t_float fd_float;   	/* the field is a constant float */
    	t_symbol *fd_symbol;	/* the field is a constant symbol */
    	t_symbol *fd_varsym;	/* the field is variable and this is the name */
    } fd_un;
} t_fielddesc;

#define FIELDDESC_SETFLOAT(x, f) \
    ((x)->fd_type = A_FLOAT, (x)->fd_var = 0, (x)->fd_un.fd_float = (f))
#define FIELDDESC_SETSYMBOL(x, s) \
    ((x)->fd_type = A_SYMBOL, (x)->fd_var = 0, (x)->fd_un.fd_symbol = (s))
#define FIELDDESC_SETVAR(x, s, type) \
    ((x)->fd_type = type, (x)->fd_var = 1, (x)->fd_un.fd_varsym = (s))

#define CLOSED 1
#define BEZ 2
#define A_ARRAY 55  	/* LATER decide whether to enshrine this in m_pd.h */

static void fielddesc_setfloatarg(t_fielddesc *fd, int argc, t_atom *argv)
{
    	if (argc <= 0) FIELDDESC_SETFLOAT(fd, 0);
    	else if (argv->a_type == A_SYMBOL)
    	    FIELDDESC_SETVAR(fd, argv->a_w.w_symbol, A_FLOAT);
    	else FIELDDESC_SETFLOAT(fd, argv->a_w.w_float);
}

static void fielddesc_setarrayarg(t_fielddesc *fd, int argc, t_atom *argv)
{
    	if (argc <= 0) FIELDDESC_SETFLOAT(fd, 0);
    	else if (argv->a_type == A_SYMBOL)
    	    FIELDDESC_SETVAR(fd, argv->a_w.w_symbol, A_ARRAY);
    	else FIELDDESC_SETFLOAT(fd, argv->a_w.w_float);
}

static t_float fielddesc_getfloat(t_fielddesc *f, t_template *template,
    t_word *wp, int loud)
{
    if (f->fd_type == A_FLOAT)
    {
    	if (f->fd_var)
   	    return (template_getfloat(template, f->fd_un.fd_varsym, wp, loud));
    	else return (f->fd_un.fd_float);
    }
    else
    {
    	if (loud)
	    error("symbolic data field used as number");
    	return (0);
    }
}

static t_symbol *fielddesc_getsymbol(t_fielddesc *f, t_template *template,
    t_word *wp, int loud)
{
    if (f->fd_type == A_SYMBOL)
    {
    	if (f->fd_var)
   	    return(template_getsymbol(template, f->fd_un.fd_varsym, wp, loud));
    	else return (f->fd_un.fd_symbol);
    }
    else
    {
    	if (loud)
	    error("numeric data field used as symbol");
    	return (&s_);
    }
}

/* ---------------- curves and polygons (joined segments) ---------------- */

/*
curves belong to templates and describe how the data in the template are to
be drawn.  The coordinates of the curve (and other display features) can
be attached to fields in the template.
*/

t_class *curve_class;

typedef struct _curve
{
    t_object x_obj;
    int x_flags;     	    /* CLOSED and/or BEZ */
    t_fielddesc x_fillcolor;
    t_fielddesc x_outlinecolor;
    t_fielddesc x_width;
    int x_npoints;
    t_fielddesc *x_vec;
} t_curve;

static void *curve_new(t_symbol *classsym, t_int argc, t_atom *argv)
{
    t_curve *x = (t_curve *)pd_new(curve_class);
    char *classname = classsym->s_name;
    int flags = 0;
    int nxy, i;
    t_fielddesc *fd;
    if (classname[0] == 'f')
    {
    	classname += 6;
    	flags |= CLOSED;
    	if (argc) fielddesc_setfloatarg(&x->x_fillcolor, argc--, argv++);
    	else FIELDDESC_SETFLOAT(&x->x_outlinecolor, 0); 
    }
    else classname += 4;
    if (classname[0] == 'c') flags |= BEZ;
    x->x_flags = flags;
    if (argc) fielddesc_setfloatarg(&x->x_outlinecolor, argc--, argv++);
    else FIELDDESC_SETFLOAT(&x->x_outlinecolor, 0);
    if (argc) fielddesc_setfloatarg(&x->x_width, argc--, argv++);
    else FIELDDESC_SETFLOAT(&x->x_width, 1);
    if (argc < 0) argc = 0;
    nxy =  (argc + (argc & 1));
    x->x_npoints = (nxy>>1);
    x->x_vec = (t_fielddesc *)t_getbytes(nxy * sizeof(t_fielddesc));
    for (i = 0, fd = x->x_vec; i < argc; i++, fd++, argv++)
    	fielddesc_setfloatarg(fd, 1, argv);
    if (argc & 1) FIELDDESC_SETFLOAT(fd, 0);

    return (x);
}

/* -------------------- widget behavior for curve ------------ */

static void curve_getrect(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, float basex, float basey,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_curve *x = (t_curve *)z;
    int i, n = x->x_npoints;
    t_fielddesc *f = x->x_vec;
    int x1 = 0x7fffffff, x2 = -0x7fffffff, y1 = 0x7fffffff, y2 = -0x7fffffff;
    for (i = 0, f = x->x_vec; i < n; i++, f += 2)
    {
    	int xloc = glist_xtopixels(glist,
    	    basex + fielddesc_getfloat(f, template, data, 0));
    	int yloc = glist_ytopixels(glist,
    	    basey + fielddesc_getfloat(f+1, template, data, 0));
    	if (xloc < x1) x1 = xloc;
    	if (xloc > x2) x2 = xloc;
    	if (yloc < y1) y1 = yloc;
    	if (yloc > y2) y2 = yloc;
    }
    *xp1 = x1;
    *yp1 = y1;
    *xp2 = x2;
    *yp2 = y2; 
}

static void curve_displace(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, float basex, float basey,
    int dx, int dy)
{
#ifdef ROCKBOX
    (void) z;
    (void) glist;
    (void) data;
    (void) template;
    (void) basex;
    (void) basey;
    (void) dx;
    (void) dy;
#endif
    /* refuse */
}

static void curve_select(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, float basex, float basey,
    int state)
{
#ifdef ROCKBOX
    (void) z;
    (void) glist;
    (void) data;
    (void) template;
    (void) basex;
    (void) basey;
    (void) state;
#endif
    /* fill in later */
}

static void curve_activate(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, float basex, float basey,
    int state)
{
#ifdef ROCKBOX
    (void) z;
    (void) glist;
    (void) data;
    (void) template;
    (void) basex;
    (void) basey;
    (void) state;
#endif
    /* fill in later */
}

static int rangecolor(int n)	/* 0 to 9 in 5 steps */
{
    int n2 = n/2;   	    	/* 0 to 4 */
    int ret = (n2 << 6);     	/* 0 to 256 in 5 steps */
    if (ret > 255) ret = 255;
    return (ret);
}

static void numbertocolor(int n, char *s)
{
    int red, blue, green;
    if (n < 0) n = 0;
    red = n / 100;
    blue = ((n / 10) % 10);
    green = n % 10;
#ifdef ROCKBOX
    snprintf(s, 10, "#%2.2x%2.2x%2.2x",
            rangecolor(red), rangecolor(blue), rangecolor(green));
#else
    sprintf(s, "#%2.2x%2.2x%2.2x", rangecolor(red), rangecolor(blue),
    	rangecolor(green));
#endif
}

static void curve_vis(t_gobj *z, t_glist *glist, 
    t_word *data, t_template *template, float basex, float basey,
    int vis)
{
    t_curve *x = (t_curve *)z;
    int i, n = x->x_npoints;
    t_fielddesc *f = x->x_vec;

#ifdef ROCKBOX
    (void) glist;
    (void) basex;
    (void) basey;
#endif
    
    if (vis)
    {
    	if (n > 1)
    	{
#ifdef ROCKBOX
            int flags = x->x_flags;
#else
    	    int flags = x->x_flags, closed = (flags & CLOSED);
#endif
    	    float width = fielddesc_getfloat(&x->x_width, template, data, 1);
    	    char outline[20], fill[20];
    	    if (width < 1) width = 1;
    	    numbertocolor(
    	    	fielddesc_getfloat(&x->x_outlinecolor, template, data, 1),
    	    	outline);
    	    if (flags & CLOSED)
    	    {
    	    	numbertocolor(
    	    	    fielddesc_getfloat(&x->x_fillcolor, template, data, 1),
    	    	    fill);
#ifndef ROCKBOX
    	    	sys_vgui(".x%x.c create polygon\\\n",
    	    	    glist_getcanvas(glist));
#endif
    	    }
#ifndef ROCKBOX
    	    else sys_vgui(".x%x.c create line\\\n",
    	    	    glist_getcanvas(glist));
#endif
    	    for (i = 0, f = x->x_vec; i < n; i++, f += 2)
    	    {
#ifndef ROCKBOX
    		float xloc = glist_xtopixels(glist,
    	    	    basex + fielddesc_getfloat(f, template, data, 1));
    		float yloc = glist_ytopixels(glist,
    	    	    basey + fielddesc_getfloat(f+1, template, data, 1));
    		sys_vgui("%d %d\\\n", (int)xloc, (int)yloc);
#endif
    	    }
#ifndef ROCKBOX
    	    sys_vgui("-width %f\\\n",
    	    	fielddesc_getfloat(&x->x_width, template, data, 1));
    	    if (flags & CLOSED) sys_vgui("-fill %s -outline %s\\\n",
    	    	fill, outline);
    	    else sys_vgui("-fill %s\\\n", outline);
    	    if (flags & BEZ) sys_vgui("-smooth 1\\\n");
    	    sys_vgui("-tags curve%x\n", data);
#endif
    	}
    	else post("warning: curves need at least two points to be graphed");
    }
    else
    {
#ifndef ROCKBOX
    	if (n > 1) sys_vgui(".x%x.c delete curve%x\n",
    	    glist_getcanvas(glist), data);    	
#endif
    }
}

static int curve_motion_field;
static float curve_motion_xcumulative;
static float curve_motion_xbase;
static float curve_motion_xper;
static float curve_motion_ycumulative;
static float curve_motion_ybase;
static float curve_motion_yper;
static t_glist *curve_motion_glist;
static t_gobj *curve_motion_gobj;
static t_word *curve_motion_wp;
static t_template *curve_motion_template;

    /* LATER protect against the template changing or the scalar disappearing
    probably by attaching a gpointer here ... */

static void curve_motion(void *z, t_floatarg dx, t_floatarg dy)
{
    t_curve *x = (t_curve *)z;
    t_fielddesc *f = x->x_vec + curve_motion_field;
    curve_motion_xcumulative += dx;
    curve_motion_ycumulative += dy;
    if (f->fd_var)
    {
	template_setfloat(curve_motion_template,
	    f->fd_un.fd_varsym,
	    curve_motion_wp, 
    	    curve_motion_xbase + curve_motion_xcumulative * curve_motion_xper,
	    	1);
    }
    if ((f+1)->fd_var)
    {
	template_setfloat(curve_motion_template,
	    (f+1)->fd_un.fd_varsym,
	    curve_motion_wp, 
    	    curve_motion_ybase + curve_motion_ycumulative * curve_motion_yper,
	    	1);
    }
    glist_redrawitem(curve_motion_glist, curve_motion_gobj);
}

static int curve_click(t_gobj *z, t_glist *glist, 
    t_scalar *sc, t_template *template, float basex, float basey,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    t_curve *x = (t_curve *)z;
    int i, n = x->x_npoints;
    int bestn = -1;
    int besterror = 0x7fffffff;
    t_fielddesc *f = x->x_vec;
    t_word *data = sc->sc_vec;

#ifdef ROCKBOX
    (void) shift;
    (void) alt;
    (void) dbl;
#endif

    for (i = 0, f = x->x_vec; i < n; i++, f += 2)
    {
    	int xloc = glist_xtopixels(glist,
    	    basex + fielddesc_getfloat(f, template, data, 0));
    	int yloc = glist_ytopixels(glist,
    	    basey + fielddesc_getfloat(f+1, template, data, 0));
    	int xerr = xloc - xpix, yerr = yloc - ypix;
	if (!f->fd_var && !(f+1)->fd_var)
	    continue;
	if (xerr < 0)
	    xerr = -xerr;
	if (yerr < 0)
	    yerr = -yerr;
	if (yerr > xerr)
	    xerr = yerr;
	if (xerr < besterror)
	{
	    besterror = xerr;
	    bestn = i;
	    curve_motion_xbase = fielddesc_getfloat(f, template, data, 0);
	    curve_motion_ybase = fielddesc_getfloat(f+1, template, data, 0);
	}
    }
    if (besterror > 10)
    	return (0);
    if (doit)
    {
    	curve_motion_xper = glist_pixelstox(glist, 1)
	    - glist_pixelstox(glist, 0);
    	curve_motion_yper = glist_pixelstoy(glist, 1)
	    - glist_pixelstoy(glist, 0);
	curve_motion_xcumulative = curve_motion_ycumulative = 0;
	curve_motion_glist = glist;
	curve_motion_gobj = &sc->sc_gobj;
	curve_motion_wp = data;
	curve_motion_field = 2*bestn;
	curve_motion_template = template;
    	glist_grab(glist, z, curve_motion, 0, xpix, ypix);
    }
    return (1);
}

t_parentwidgetbehavior curve_widgetbehavior =
{
    curve_getrect,
    curve_displace,
    curve_select,
    curve_activate,
    curve_vis,
    curve_click,
};

static void curve_free(t_curve *x)
{
    t_freebytes(x->x_vec, 2 * x->x_npoints * sizeof(*x->x_vec));
}

static void curve_setup(void)
{
    curve_class = class_new(gensym("drawpolygon"), (t_newmethod)curve_new,
    	(t_method)curve_free, sizeof(t_curve), CLASS_NOINLET, A_GIMME, 0);
    class_setdrawcommand(curve_class);
    class_addcreator((t_newmethod)curve_new, gensym("drawcurve"),
    	A_GIMME, 0);
    class_addcreator((t_newmethod)curve_new, gensym("filledpolygon"),
    	A_GIMME, 0);
    class_addcreator((t_newmethod)curve_new, gensym("filledcurve"),
    	A_GIMME, 0);
    class_setparentwidget(curve_class, &curve_widgetbehavior);
}

/* --------- plots for showing arrays --------------- */

t_class *plot_class;

typedef struct _plot
{
    t_object x_obj;
    int x_flags;
    t_fielddesc x_outlinecolor;
    t_fielddesc x_width;
    t_fielddesc x_xloc;
    t_fielddesc x_yloc;
    t_fielddesc x_xinc;
    t_fielddesc x_data;
} t_plot;

static void *plot_new(t_symbol *classsym, t_int argc, t_atom *argv)
{
    t_plot *x = (t_plot *)pd_new(plot_class);
    int flags = 0;
#ifndef ROCKBOX
    int nxy, i;
    t_fielddesc *fd;
#endif
    t_symbol *firstarg = atom_getsymbolarg(0, argc, argv);

#ifdef ROCKBOX
    (void) classsym;
#endif

    if (!strcmp(firstarg->s_name, "curve"))
    {
    	flags |= BEZ;
    	argc--, argv++;
    }
    if (argc) fielddesc_setarrayarg(&x->x_data, argc--, argv++);
    else FIELDDESC_SETFLOAT(&x->x_data, 1);
    if (argc) fielddesc_setfloatarg(&x->x_outlinecolor, argc--, argv++);
    else FIELDDESC_SETFLOAT(&x->x_outlinecolor, 0);
    if (argc) fielddesc_setfloatarg(&x->x_width, argc--, argv++);
    else FIELDDESC_SETFLOAT(&x->x_width, 1);
    if (argc) fielddesc_setfloatarg(&x->x_xloc, argc--, argv++);
    else FIELDDESC_SETFLOAT(&x->x_xloc, 1);
    if (argc) fielddesc_setfloatarg(&x->x_yloc, argc--, argv++);
    else FIELDDESC_SETFLOAT(&x->x_yloc, 1);
    if (argc) fielddesc_setfloatarg(&x->x_xinc, argc--, argv++);
    else FIELDDESC_SETFLOAT(&x->x_xinc, 1);
    x->x_flags = flags;
    return (x);
}

/* -------------------- widget behavior for plot ------------ */


    /* get everything we'll need from the owner template of the array being
    plotted. Not used for garrays, but see below */
static int plot_readownertemplate(t_plot *x,
    t_word *data, t_template *ownertemplate, 
    t_symbol **elemtemplatesymp, t_array **arrayp,
    float *linewidthp, float *xlocp, float *xincp, float *ylocp)
{
    int arrayonset, type;
    t_symbol *elemtemplatesym;
    t_array *array;

    	/* find the data and verify it's an array */
    if (x->x_data.fd_type != A_ARRAY || !x->x_data.fd_var)
    {
    	error("plot: needs an array field");
    	return (-1);
    }
    if (!template_find_field(ownertemplate, x->x_data.fd_un.fd_varsym,
    	&arrayonset, &type, &elemtemplatesym))
    {
    	error("plot: %s: no such field", x->x_data.fd_un.fd_varsym->s_name);
    	return (-1);
    }
    if (type != DT_ARRAY)
    {
    	error("plot: %s: not an array", x->x_data.fd_un.fd_varsym->s_name);
    	return (-1);
    }
    array = *(t_array **)(((char *)data) + arrayonset);
    *linewidthp = fielddesc_getfloat(&x->x_width, ownertemplate, data, 1);
    *xlocp = fielddesc_getfloat(&x->x_xloc, ownertemplate, data, 1);
    *xincp = fielddesc_getfloat(&x->x_xinc, ownertemplate, data, 1);
    *ylocp = fielddesc_getfloat(&x->x_yloc, ownertemplate, data, 1);
    *elemtemplatesymp = elemtemplatesym;
    *arrayp = array;
    return (0);
}

    /* get everything else you could possibly need about a plot,
    either for plot's own purposes or for plotting a "garray" */
int array_getfields(t_symbol *elemtemplatesym,
    t_canvas **elemtemplatecanvasp,
    t_template **elemtemplatep, int *elemsizep,
    int *xonsetp, int *yonsetp, int *wonsetp)
{
#ifdef ROCKBOX
    int elemsize, yonset, wonset, xonset, type;
#else
    int arrayonset, elemsize, yonset, wonset, xonset, type;
#endif
    t_template *elemtemplate;
    t_symbol *dummy;
    t_canvas *elemtemplatecanvas = 0;

    	/* the "float" template is special in not having to have a canvas;
	template_findbyname is hardwired to return a predefined 
	template. */

    if (!(elemtemplate =  template_findbyname(elemtemplatesym)))
    {
    	error("plot: %s: no such template", elemtemplatesym->s_name);
    	return (-1);
    }
    if (!((elemtemplatesym == &s_float) ||
    	(elemtemplatecanvas = template_findcanvas(elemtemplate))))
    {
    	error("plot: %s: no canvas for this template", elemtemplatesym->s_name);
    	return (-1);
    }
    elemsize = elemtemplate->t_n * sizeof(t_word);
    if (!template_find_field(elemtemplate, gensym("y"), &yonset, &type, &dummy)
    	|| type != DT_FLOAT)	
	    yonset = -1;
    if (!template_find_field(elemtemplate, gensym("x"), &xonset, &type, &dummy)
    	|| type != DT_FLOAT) 
	    xonset = -1;
    if (!template_find_field(elemtemplate, gensym("w"), &wonset, &type, &dummy)
    	|| type != DT_FLOAT) 
	    wonset = -1;

    	/* fill in slots for return values */
    *elemtemplatecanvasp = elemtemplatecanvas;
    *elemtemplatep = elemtemplate;
    *elemsizep = elemsize;
    *xonsetp = xonset;
    *yonsetp = yonset;
    *wonsetp = wonset;
    return (0);
}

static void plot_getrect(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, float basex, float basey,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_plot *x = (t_plot *)z;
    int elemsize, yonset, wonset, xonset;
    t_canvas *elemtemplatecanvas;
    t_template *elemtemplate;
    t_symbol *elemtemplatesym;
    float linewidth, xloc, xinc, yloc;
    t_array *array;
    float x1 = 0x7fffffff, y1 = 0x7fffffff, x2 = -0x7fffffff, y2 = -0x7fffffff;
    int i;
    float xpix, ypix, wpix;

    if (!plot_readownertemplate(x, data, template, 
    	&elemtemplatesym, &array, &linewidth, &xloc, &xinc, &yloc) &&
	    !array_getfields(elemtemplatesym, &elemtemplatecanvas,
	    	&elemtemplate, &elemsize, &xonset, &yonset, &wonset))
    {
	for (i = 0; i < array->a_n; i++)
	{
    	    array_getcoordinate(glist, (char *)(array->a_vec) + i * elemsize,
    		xonset, yonset, wonset, i, basex + xloc, basey + yloc, xinc,
    		&xpix, &ypix, &wpix);
	    if (xpix < x1)
		x1 = xpix;
	    if (xpix > x2)
		x2 = xpix;
	    if (ypix - wpix < y1)
		y1 = ypix - wpix;
	    if (ypix + wpix > y2)
		y2 = ypix + wpix;
	}
    }

    *xp1 = x1;
    *yp1 = y1;
    *xp2 = x2;
    *yp2 = y2;
}

static void plot_displace(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, float basex, float basey,
    int dx, int dy)
{
#ifdef ROCKBOX
    (void) z;
    (void) glist;
    (void) data;
    (void) template;
    (void) basex;
    (void) basey;
    (void) dx;
    (void) dy;
#endif
    	/* not yet */
}

static void plot_select(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, float basex, float basey,
    int state)
{
#ifdef ROCKBOX
    (void) z;
    (void) glist;
    (void) data;
    (void) template;
    (void) basex;
    (void) basey;
    (void) state;
#endif
    /* not yet */
}

static void plot_activate(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, float basex, float basey,
    int state)
{
#ifdef ROCKBOX
    (void) z;
    (void) glist;
    (void) data;
    (void) template;
    (void) basex;
    (void) basey;
    (void) state;
#endif
    	/* not yet */
}

static void plot_vis(t_gobj *z, t_glist *glist, 
    t_word *data, t_template *template, float basex, float basey,
    int vis)
{
    t_plot *x = (t_plot *)z;
    int elemsize, yonset, wonset, xonset;
    t_canvas *elemtemplatecanvas;
    t_template *elemtemplate;
    t_symbol *elemtemplatesym;
    float linewidth, xloc, xinc, yloc;
    t_array *array;
    int nelem;
    char *elem;
    if (plot_readownertemplate(x, data, template, 
    	&elemtemplatesym, &array, &linewidth, &xloc, &xinc, &yloc) ||
	    array_getfields(elemtemplatesym, &elemtemplatecanvas,
	    	&elemtemplate, &elemsize, &xonset, &yonset, &wonset))
    	    	    return;
    nelem = array->a_n;
    elem = (char *)array->a_vec;
    if (vis)
    {    	
    	char outline[20];
    	int lastpixel = -1, ndrawn = 0;
    	float xsum, yval = 0, wval = 0, xpix;
    	int ixpix = 0, i;

    	    /* draw the trace */
    	numbertocolor(fielddesc_getfloat(&x->x_outlinecolor, template, data, 1),
    	    outline);
    	if (wonset >= 0)
    	{
    	    	/* found "w" field which controls linewidth.  The trace is
    	    	a filled polygon with 2n points. */
#ifndef ROCKBOX
    	    sys_vgui(".x%x.c create polygon \\\n",
    	    	glist_getcanvas(glist));
#endif
    	    
    	    for (i = 0, xsum = xloc; i < nelem; i++)
    	    {
    	    	float usexloc;
    	    	if (xonset >= 0)
    	    	    usexloc = xloc + *(float *)((elem + elemsize * i) + xonset);
    	    	else usexloc = xsum, xsum += xinc;
		if (yonset >= 0)
    		    yval = *(float *)((elem + elemsize * i) + yonset);
		else yval = 0;
    		wval = *(float *)((elem + elemsize * i) + wonset);
    		xpix = glist_xtopixels(glist, basex + usexloc);
    		ixpix = xpix + 0.5;
    		if (xonset >= 0 || ixpix != lastpixel)
    		{
#ifndef ROCKBOX
    	    	    sys_vgui("%d %f \\\n", ixpix,
    	    		glist_ytopixels(glist,
			    basey + yloc + yval - wval));
#endif
    	    	    ndrawn++;
    		}
    		lastpixel = ixpix;
    		if (ndrawn >= 1000) goto ouch;
    	    }
	    lastpixel = -1;
    	    for (i = nelem-1; i >= 0; i--)
    	    {
    	    	float usexloc;
    	    	if (xonset >= 0)
    	    	    usexloc = xloc + *(float *)((elem + elemsize * i) + xonset);
    	    	else xsum -= xinc, usexloc = xsum;
		if (yonset >= 0)
    		    yval = *(float *)((elem + elemsize * i) + yonset);
    		else yval = 0;
		wval = *(float *)((elem + elemsize * i) + wonset);
    		xpix = glist_xtopixels(glist, basex + usexloc);
    		ixpix = xpix + 0.5;
    		if (xonset >= 0 || ixpix != lastpixel)
    		{
#ifndef ROCKBOX
    	    	    sys_vgui("%d %f \\\n", ixpix, glist_ytopixels(glist,
			    basey + yloc + yval + wval));
#endif
    	    	    ndrawn++;
    		}
    		lastpixel = ixpix;
    		if (ndrawn >= 1000) goto ouch;
     	    }
    		/* TK will complain if there aren't at least 3 points.  There
		should be at least two already. */
    	    if (ndrawn < 4)
	    {
#ifndef ROCKBOX
    	    	sys_vgui("%d %f \\\n", ixpix + 10, glist_ytopixels(glist,
			basey + yloc + yval + wval));
    	    	sys_vgui("%d %f \\\n", ixpix + 10, glist_ytopixels(glist,
			basey + yloc + yval - wval));
#endif
    	    }
	ouch:
#ifdef ROCKBOX
                ;
#else /* ROCKBOX */
    	    sys_vgui(" -width 1 -fill %s -outline %s\\\n", outline, outline);
    	    if (x->x_flags & BEZ) sys_vgui("-smooth 1\\\n");

    	    sys_vgui("-tags plot%x\n", data);
#endif /* ROCKBOX */
    	}
    	else if (linewidth > 0)
    	{
    	    	/* no "w" field.  If the linewidth is positive, draw a
    	    	segmented line with the requested width; otherwise don't
    	    	draw the trace at all. */
#ifndef ROCKBOX
    	    sys_vgui(".x%x.c create line \\\n", glist_getcanvas(glist));
#endif

    	    for (xsum = xloc, i = 0; i < nelem; i++)
    	    {
    	    	float usexloc;
    	    	if (xonset >= 0)
    	    	    usexloc = xloc + *(float *)((elem + elemsize * i) + xonset);
    	    	else usexloc = xsum, xsum += xinc;
		if (yonset >= 0)
    		    yval = *(float *)((elem + elemsize * i) + yonset);
    		else yval = 0;
		xpix = glist_xtopixels(glist, basex + usexloc);
    		ixpix = xpix + 0.5;
    		if (xonset >= 0 || ixpix != lastpixel)
    		{
#ifndef ROCKBOX
    	    	    sys_vgui("%d %f \\\n", ixpix,
    	    		glist_ytopixels(glist, basey + yloc + yval));
#endif
    	    	    ndrawn++;
    		}
    		lastpixel = ixpix;
    		if (ndrawn >= 1000) break;
    	    }
    		/* TK will complain if there aren't at least 2 points... */
#ifndef ROCKBOX
    	    if (ndrawn == 0) sys_vgui("0 0 0 0 \\\n");
    	    else if (ndrawn == 1) sys_vgui("%d %f \\\n", ixpix + 10,
    	    		glist_ytopixels(glist, basey + yloc + yval));

    	    sys_vgui("-width %f\\\n", linewidth);
    	    sys_vgui("-fill %s\\\n", outline);
    	    if (x->x_flags & BEZ) sys_vgui("-smooth 1\\\n");

    	    sys_vgui("-tags plot%x\n", data);
#endif
    	}
    	    /* We're done with the outline; now draw all the points.
    	    This code is inefficient since the template has to be
    	    searched for drawing instructions for every last point. */
    	
    	for (xsum = xloc, i = 0; i < nelem; i++)
    	{
    	    float usexloc, useyloc;
    	    t_gobj *y;
    	    if (xonset >= 0)
    	    	usexloc = basex + xloc +
    	    	    *(float *)((elem + elemsize * i) + xonset);
    	    else usexloc = basex + xsum, xsum += xinc;
    	    if (yonset >= 0)
	    	yval = *(float *)((elem + elemsize * i) + yonset);
    	    else yval = 0;
	    useyloc = basey + yloc + yval;
    	    for (y = elemtemplatecanvas->gl_list; y; y = y->g_next)
	    {
		t_parentwidgetbehavior *wb = pd_getparentwidget(&y->g_pd);
		if (!wb) continue;
		(*wb->w_parentvisfn)(y, glist,
		    (t_word *)(elem + elemsize * i),
		    	elemtemplate, usexloc, useyloc, vis);
	    }
    	}
    }
    else
    {
    	    /* un-draw the individual points */
    	int i;
    	for (i = 0; i < nelem; i++)
    	{
    	    t_gobj *y;
    	    for (y = elemtemplatecanvas->gl_list; y; y = y->g_next)
	    {
		t_parentwidgetbehavior *wb = pd_getparentwidget(&y->g_pd);
		if (!wb) continue;
		(*wb->w_parentvisfn)(y, glist,
		    (t_word *)(elem + elemsize * i), elemtemplate,
		    	0, 0, 0);
	    }
    	}
    	    /* and then the trace */
#ifndef ROCKBOX
    	sys_vgui(".x%x.c delete plot%x\n",
    	    glist_getcanvas(glist), data);
#endif
    }
}


static int plot_click(t_gobj *z, t_glist *glist, 
    t_scalar *sc, t_template *template, float basex, float basey,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    t_plot *x = (t_plot *)z;
    t_symbol *elemtemplatesym;
    float linewidth, xloc, xinc, yloc;
    t_array *array;
    t_word *data = sc->sc_vec;

    if (!plot_readownertemplate(x, data, template, 
    	&elemtemplatesym, &array, &linewidth, &xloc, &xinc, &yloc))
    {
    	return (array_doclick(array, glist, &sc->sc_gobj,
	    elemtemplatesym,
	    linewidth, basex + xloc, xinc, basey + yloc,
	    xpix, ypix, shift, alt, dbl, doit));
    }
    else return (0);
}

t_parentwidgetbehavior plot_widgetbehavior =
{
    plot_getrect,
    plot_displace,
    plot_select,
    plot_activate,
    plot_vis,
    plot_click,
};

static void plot_setup(void)
{
    plot_class = class_new(gensym("plot"), (t_newmethod)plot_new, 0,
    	sizeof(t_plot), CLASS_NOINLET, A_GIMME, 0);
    class_setdrawcommand(plot_class);
    class_setparentwidget(plot_class, &plot_widgetbehavior);
}

/* ---------------- drawnumber: draw a number ---------------- */

/*
    drawnumbers draw numeric fields at controllable locations, with
    controllable color and label  .
    	invocation: (drawnumber|drawsymbol) variable x y color label
*/

t_class *drawnumber_class;

#define DRAW_SYMBOL 1

typedef struct _drawnumber
{
    t_object x_obj;
    t_fielddesc x_value;
    t_fielddesc x_xloc;
    t_fielddesc x_yloc;
    t_fielddesc x_color;
    t_symbol *x_label;
    int x_flags;
} t_drawnumber;

static void *drawnumber_new(t_symbol *classsym, t_int argc, t_atom *argv)
{
    t_drawnumber *x = (t_drawnumber *)pd_new(drawnumber_class);
    char *classname = classsym->s_name;
    int flags = 0;
    if (classname[4] == 's')
    	flags |= DRAW_SYMBOL;
    x->x_flags = flags;
    if (argc) fielddesc_setfloatarg(&x->x_value, argc--, argv++);
    else FIELDDESC_SETFLOAT(&x->x_value, 0);
    if (argc) fielddesc_setfloatarg(&x->x_xloc, argc--, argv++);
    else FIELDDESC_SETFLOAT(&x->x_xloc, 0);
    if (argc) fielddesc_setfloatarg(&x->x_yloc, argc--, argv++);
    else FIELDDESC_SETFLOAT(&x->x_yloc, 0);
    if (argc) fielddesc_setfloatarg(&x->x_color, argc--, argv++);
    else FIELDDESC_SETFLOAT(&x->x_color, 1);
    if (argc)
    	x->x_label = atom_getsymbolarg(0, argc, argv);
    else x->x_label = &s_;

    return (x);
}

/* -------------------- widget behavior for drawnumber ------------ */

#define DRAWNUMBER_BUFSIZE 80
static void drawnumber_sprintf(t_drawnumber *x, char *buf, t_atom *ap)
{
    int nchars;
    strncpy(buf, x->x_label->s_name, DRAWNUMBER_BUFSIZE);
    buf[DRAWNUMBER_BUFSIZE - 1] = 0;
    nchars = strlen(buf);
    atom_string(ap, buf + nchars, DRAWNUMBER_BUFSIZE - nchars);
}

static void drawnumber_getrect(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, float basex, float basey,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_drawnumber *x = (t_drawnumber *)z;
    t_atom at;
    int xloc = glist_xtopixels(glist,
    	basex + fielddesc_getfloat(&x->x_xloc, template, data, 0));
    int yloc = glist_ytopixels(glist,
    	basey + fielddesc_getfloat(&x->x_yloc, template, data, 0));
#ifdef ROCKBOX
    int fontwidth = 8, fontheight = 10;
#else
    int font = glist_getfont(glist);
    int fontwidth = sys_fontwidth(font), fontheight = sys_fontheight(font);
#endif
    char buf[DRAWNUMBER_BUFSIZE];
    if (x->x_flags & DRAW_SYMBOL)
    	SETSYMBOL(&at, fielddesc_getsymbol(&x->x_value, template, data, 0));
    else SETFLOAT(&at, fielddesc_getfloat(&x->x_value, template, data, 0));
    drawnumber_sprintf(x, buf, &at);
    *xp1 = xloc;
    *yp1 = yloc;
    *xp2 = xloc + fontwidth * strlen(buf);
    *yp2 = yloc + fontheight;
}

static void drawnumber_displace(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, float basex, float basey,
    int dx, int dy)
{
#ifdef ROCKBOX
    (void) z;
    (void) glist;
    (void) data;
    (void) template;
    (void) basex;
    (void) basey;
    (void) dx;
    (void) dy;
#endif
    /* refuse */
}

static void drawnumber_select(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, float basex, float basey,
    int state)
{
#ifdef ROCKBOX
    (void) z;
    (void) glist;
    (void) data;
    (void) template;
    (void) basex;
    (void) basey;
#endif
    post("drawnumber_select %d", state);
    /* fill in later */
}

static void drawnumber_activate(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, float basex, float basey,
    int state)
{
#ifdef ROCKBOX
    (void) z;
    (void) glist;
    (void) data;
    (void) template;
    (void) basex;
    (void) basey;
#endif
    post("drawnumber_activate %d", state);
}

static void drawnumber_vis(t_gobj *z, t_glist *glist, 
    t_word *data, t_template *template, float basex, float basey,
    int vis)
{
    t_drawnumber *x = (t_drawnumber *)z;

#ifdef ROCKBOX
    (void) glist;
    (void) basex;
    (void) basey;
#endif

    if (vis)
    {
    	t_atom at;
#ifndef ROCKBOX
	int xloc = glist_xtopixels(glist,
    	    basex + fielddesc_getfloat(&x->x_xloc, template, data, 0));
	int yloc = glist_ytopixels(glist,
    	    basey + fielddesc_getfloat(&x->x_yloc, template, data, 0));
#endif
	char colorstring[20], buf[DRAWNUMBER_BUFSIZE];
    	numbertocolor(fielddesc_getfloat(&x->x_color, template, data, 1),
    	    colorstring);
	if (x->x_flags & DRAW_SYMBOL)
    	    SETSYMBOL(&at, fielddesc_getsymbol(&x->x_value, template, data, 0));
	else SETFLOAT(&at, fielddesc_getfloat(&x->x_value, template, data, 0));
	drawnumber_sprintf(x, buf, &at);
#ifndef ROCKBOX
	sys_vgui(".x%x.c create text %d %d -anchor nw -fill %s -text {%s}",
    	    	glist_getcanvas(glist), xloc, yloc, colorstring, buf);
    	sys_vgui(" -font -*-courier-bold--normal--%d-*",
	    sys_hostfontsize(glist_getfont(glist)));
	sys_vgui(" -tags drawnumber%x\n", data);
#endif
    }
#ifndef ROCKBOX
    else sys_vgui(".x%x.c delete drawnumber%x\n", glist_getcanvas(glist), data);
#endif
}

static float drawnumber_motion_ycumulative;
static t_glist *drawnumber_motion_glist;
static t_gobj *drawnumber_motion_gobj;
static t_word *drawnumber_motion_wp;
static t_template *drawnumber_motion_template;

    /* LATER protect against the template changing or the scalar disappearing
    probably by attaching a gpointer here ... */

static void drawnumber_motion(void *z, t_floatarg dx, t_floatarg dy)
{
    t_drawnumber *x = (t_drawnumber *)z;
    t_fielddesc *f = &x->x_value;
    drawnumber_motion_ycumulative -= dy;
#ifdef ROCKBOX
    (void) dx;
#endif
    template_setfloat(drawnumber_motion_template,
    	f->fd_un.fd_varsym,
	    drawnumber_motion_wp, 
    	    drawnumber_motion_ycumulative,
	    	1);
    glist_redrawitem(drawnumber_motion_glist, drawnumber_motion_gobj);
}

static int drawnumber_click(t_gobj *z, t_glist *glist, 
    t_scalar *sc, t_template *template, float basex, float basey,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    t_drawnumber *x = (t_drawnumber *)z;
    int x1, y1, x2, y2;
    t_word *data = sc->sc_vec;
#ifdef ROCKBOX
    (void) shift;
    (void) alt;
    (void) dbl;
#endif
    drawnumber_getrect(z, glist,
    	sc->sc_vec, template, basex, basey,
    	&x1, &y1, &x2, &y2);
    if (xpix >= x1 && xpix <= x2 && ypix >= y1 && ypix <= y2
    	&& x->x_value.fd_var)
    {
    	if (doit)
	{
	    drawnumber_motion_glist = glist;
	    drawnumber_motion_gobj = &sc->sc_gobj;
	    drawnumber_motion_wp = data;
	    drawnumber_motion_template = template;
	    drawnumber_motion_ycumulative =
	    	fielddesc_getfloat(&x->x_value, template, data, 0);
    	    glist_grab(glist, z, drawnumber_motion, 0, xpix, ypix);
	}
    	return (1);
    }
    else return (0);
}

t_parentwidgetbehavior drawnumber_widgetbehavior =
{
    drawnumber_getrect,
    drawnumber_displace,
    drawnumber_select,
    drawnumber_activate,
    drawnumber_vis,
    drawnumber_click,
};

static void drawnumber_free(t_drawnumber *x)
{
#ifdef ROCKBOX
    (void) x;
#endif
}

static void drawnumber_setup(void)
{
    drawnumber_class = class_new(gensym("drawnumber"),
    	(t_newmethod)drawnumber_new, (t_method)drawnumber_free,
	sizeof(t_drawnumber), CLASS_NOINLET, A_GIMME, 0);
    class_setdrawcommand(drawnumber_class);
    class_addcreator((t_newmethod)drawnumber_new, gensym("drawsymbol"),
    	A_GIMME, 0);
    class_setparentwidget(drawnumber_class, &drawnumber_widgetbehavior);
}

/* ---------------------- setup function ---------------------------- */

void g_template_setup(void)
{
    template_setup();
    gtemplate_setup();
    template_float.t_pdobj = template_class;
    curve_setup();
    plot_setup();
    drawnumber_setup();
}



