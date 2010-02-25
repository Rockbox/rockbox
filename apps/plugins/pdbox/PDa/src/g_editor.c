/* Copyright (c) 1997-2001 Miller Puckette and others.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#ifdef ROCKBOX
#include "plugin.h"
#include "../../pdbox.h"
#include "m_pd.h"
#include "m_imp.h"
#include "s_stuff.h"
#include "g_canvas.h"
#else /* ROCKBOX */
#include <stdlib.h>
#include <stdio.h>
#include "m_pd.h"
#include "m_imp.h"
#include "s_stuff.h"
#include "g_canvas.h"
#include <string.h>
#endif /* ROCKBOX */

void glist_readfrombinbuf(t_glist *x, t_binbuf *b, char *filename,
    int selectem);

void open_via_helppath(const char *name, const char *dir);
char *class_gethelpdir(t_class *c);

/* ------------------ forward declarations --------------- */
static void canvas_doclear(t_canvas *x);
static void glist_setlastxy(t_glist *gl, int xval, int yval);
static void glist_donewloadbangs(t_glist *x);
static t_binbuf *canvas_docopy(t_canvas *x);
static void canvas_dopaste(t_canvas *x, t_binbuf *b);
static void canvas_paste(t_canvas *x);
static void canvas_clearline(t_canvas *x);
static t_binbuf *copy_binbuf;

/* ---------------- generic widget behavior ------------------------- */

void gobj_getrect(t_gobj *x, t_glist *glist, int *x1, int *y1,
    int *x2, int *y2)
{
    if (x->g_pd->c_wb && x->g_pd->c_wb->w_getrectfn)
    	(*x->g_pd->c_wb->w_getrectfn)(x, glist, x1, y1, x2, y2);
}

void gobj_displace(t_gobj *x, t_glist *glist, int dx, int dy)
{
    if (x->g_pd->c_wb && x->g_pd->c_wb->w_displacefn)
    	(*x->g_pd->c_wb->w_displacefn)(x, glist, dx, dy);
}

void gobj_select(t_gobj *x, t_glist *glist, int state)
{
    if (x->g_pd->c_wb && x->g_pd->c_wb->w_selectfn)
    	(*x->g_pd->c_wb->w_selectfn)(x, glist, state);
}

void gobj_activate(t_gobj *x, t_glist *glist, int state)
{
    if (x->g_pd->c_wb && x->g_pd->c_wb->w_activatefn)
    	(*x->g_pd->c_wb->w_activatefn)(x, glist, state);
}

void gobj_delete(t_gobj *x, t_glist *glist)
{
    if (x->g_pd->c_wb && x->g_pd->c_wb->w_deletefn)
    	(*x->g_pd->c_wb->w_deletefn)(x, glist);
}

void gobj_vis(t_gobj *x, struct _glist *glist, int flag)
{
    if (x->g_pd->c_wb && x->g_pd->c_wb->w_visfn)
    	(*x->g_pd->c_wb->w_visfn)(x, glist, flag);
}

int gobj_click(t_gobj *x, struct _glist *glist,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    if (x->g_pd->c_wb && x->g_pd->c_wb->w_clickfn)
    	return ((*x->g_pd->c_wb->w_clickfn)(x,
	    glist, xpix, ypix, shift, alt, dbl, doit));
    else return (0);
}

/* ------------------------ managing the selection ----------------- */

void glist_selectline(t_glist *x, t_outconnect *oc, int index1,
    int outno, int index2, int inno)
{
    if (x->gl_editor)
    {
    	glist_noselect(x);
    	x->gl_editor->e_selectedline = 1;
	x->gl_editor->e_selectline_index1 = index1;
	x->gl_editor->e_selectline_outno = outno;
	x->gl_editor->e_selectline_index2 = index2;
	x->gl_editor->e_selectline_inno = inno;
	x->gl_editor->e_selectline_tag = oc;
#ifndef ROCKBOX
	sys_vgui(".x%x.c itemconfigure l%x -fill blue\n",
	    x, x->gl_editor->e_selectline_tag);
#endif
    }    
}

void glist_deselectline(t_glist *x)
{
    if (x->gl_editor)
    {
    	x->gl_editor->e_selectedline = 0;
#ifndef ROCKBOX
	sys_vgui(".x%x.c itemconfigure l%x -fill black\n",
	    x, x->gl_editor->e_selectline_tag);
#endif
    }    
}

int glist_isselected(t_glist *x, t_gobj *y)
{
    if (x->gl_editor)
    {
    	t_selection *sel;
    	for (sel = x->gl_editor->e_selection; sel; sel = sel->sel_next)
    	    if (sel->sel_what == y) return (1);
    }
    return (0);
}

    /* call this for unselected objects only */
void glist_select(t_glist *x, t_gobj *y)
{
    if (x->gl_editor)
    {
	t_selection *sel = (t_selection *)getbytes(sizeof(*sel));
	if (x->gl_editor->e_selectedline)
	    glist_deselectline(x);
	    /* LATER #ifdef out the following check */
	if (glist_isselected(x, y)) bug("glist_select");
	sel->sel_next = x->gl_editor->e_selection;
	sel->sel_what = y;
	x->gl_editor->e_selection = sel;
	gobj_select(y, x, 1);
    }
}

    /* call this for selected objects only */
void glist_deselect(t_glist *x, t_gobj *y)
{
    int fixdsp = 0;
    static int reenter = 0;
    if (reenter) return;
    reenter = 1;
    if (x->gl_editor)
    {
	t_selection *sel, *sel2;
	t_rtext *z = 0;
	if (!glist_isselected(x, y)) bug("glist_deselect");
	if (x->gl_editor->e_textedfor)
	{
	    t_rtext *fuddy = glist_findrtext(x, (t_text *)y);
	    if (x->gl_editor->e_textedfor == fuddy)
	    {
	    	if (x->gl_editor->e_textdirty)
	    	{
		    z = fuddy;
    	    	    canvas_stowconnections(glist_getcanvas(x));
    	    	}
    	    	gobj_activate(y, x, 0);
	    }
	    if (zgetfn(&y->g_pd, gensym("dsp")))
	    	fixdsp = 1;
	}
	if ((sel = x->gl_editor->e_selection)->sel_what == y)
	{
     	    x->gl_editor->e_selection = x->gl_editor->e_selection->sel_next;
    	    gobj_select(sel->sel_what, x, 0);
    	    freebytes(sel, sizeof(*sel));
	}
	else
	{
    	    for(sel = x->gl_editor->e_selection; (sel2 = sel->sel_next);
    	    	sel = sel2)
    	    {
    		if (sel2->sel_what == y)
    		{
    	    	    sel->sel_next = sel2->sel_next;
     	    	    gobj_select(sel2->sel_what, x, 0);
    	    	    freebytes(sel2, sizeof(*sel2));
    	    	    break;
    		}
    	    }
	}
	if (z)
    	{
    	    char *buf;
    	    int bufsize;

    	    rtext_gettext(z, &buf, &bufsize);
    	    text_setto((t_text *)y, x, buf, bufsize);
	    canvas_fixlinesfor(glist_getcanvas(x), (t_text *)y);
    	    x->gl_editor->e_textedfor = 0;
    	}
	if (fixdsp)
	    canvas_update_dsp();
    }
    reenter = 0;
}

void glist_noselect(t_glist *x)
{
    if (x->gl_editor)
    {
    	while (x->gl_editor->e_selection)
    	    glist_deselect(x, x->gl_editor->e_selection->sel_what);
	if (x->gl_editor->e_selectedline)
	    glist_deselectline(x);
    }
}

void glist_selectall(t_glist *x)
{
    if (x->gl_editor)
    {
    	glist_noselect(x);
    	if (x->gl_list)
    	{
    	    t_selection *sel = (t_selection *)getbytes(sizeof(*sel));
    	    t_gobj *y = x->gl_list;
    	    x->gl_editor->e_selection = sel;
    	    sel->sel_what = y;
    	    gobj_select(y, x, 1);
    	    while((y = y->g_next))
    	    {
    	    	t_selection *sel2 = (t_selection *)getbytes(sizeof(*sel2));
    	    	sel->sel_next = sel2;
    	    	sel = sel2;
    	    	sel->sel_what = y;
    	    	gobj_select(y, x, 1);
    	    }
    	    sel->sel_next = 0;
    	}
    }
}

    /* get the index of a gobj in a glist.  If y is zero, return the
    total number of objects. */
int glist_getindex(t_glist *x, t_gobj *y)
{
    t_gobj *y2;
    int indx;

    for (y2 = x->gl_list, indx = 0; y2 && y2 != y; y2 = y2->g_next)
    	indx++;
    return (indx);
}

    /* get the index of the object, among selected items, if "selected"
    is set; otherwise, among unselected ones.  If y is zero, just
    counts the selected or unselected objects. */
int glist_selectionindex(t_glist *x, t_gobj *y, int selected)
{
    t_gobj *y2;
    int indx;

    for (y2 = x->gl_list, indx = 0; y2 && y2 != y; y2 = y2->g_next)
    	if (selected == glist_isselected(x, y2))
	    indx++;
    return (indx);
}

static t_gobj *glist_nth(t_glist *x, int n)
{
    t_gobj *y;
    int indx;
    for (y = x->gl_list, indx = 0; y; y = y->g_next, indx++)
    	if (indx == n)
	    return (y);
    return (0);
}

/* ------------------- support for undo/redo  -------------------------- */

static t_undofn canvas_undo_fn;     	/* current undo function if any */
static int canvas_undo_whatnext; 	/* whether we can now UNDO or REDO */
static void *canvas_undo_buf;    	/* data private to the undo function */
static t_canvas *canvas_undo_canvas;    /* which canvas we can undo on */
static const char *canvas_undo_name;

void canvas_setundo(t_canvas *x, t_undofn undofn, void *buf,
    const char *name)
{
    int hadone = 0;
    	/* blow away the old undo information.  In one special case the
	old undo info is re-used; if so we shouldn't free it here. */
    if (canvas_undo_fn && canvas_undo_buf && (buf != canvas_undo_buf))
    {
    	(*canvas_undo_fn)(canvas_undo_canvas, canvas_undo_buf, UNDO_FREE);
	hadone = 1;
    }
    canvas_undo_canvas = x;
    canvas_undo_fn = undofn;
    canvas_undo_buf = buf;
    canvas_undo_whatnext = UNDO_UNDO;
    canvas_undo_name = name;
#ifndef ROCKBOX
    if (x && glist_isvisible(x) && glist_istoplevel(x))
	    /* enable undo in menu */
    	sys_vgui("pdtk_undomenu .x%x %s no\n", x, name);
    else if (hadone)
    	sys_vgui("pdtk_undomenu nobody no no\n");
#endif
}

    /* clear undo if it happens to be for the canvas x.
     (but if x is 0, clear it regardless of who owns it.) */
void canvas_noundo(t_canvas *x)
{
    if (!x || (x == canvas_undo_canvas))
    	canvas_setundo(0, 0, 0, "foo");
}

static void canvas_undo(t_canvas *x)
{
    if (x != canvas_undo_canvas)
    	bug("canvas_undo 1");
    else if (canvas_undo_whatnext != UNDO_UNDO)
    	bug("canvas_undo 2");
    else
    {
	/* post("undo"); */
	(*canvas_undo_fn)(canvas_undo_canvas, canvas_undo_buf, UNDO_UNDO);
    	    /* enable redo in menu */
#ifndef ROCKBOX
	if (glist_isvisible(x) && glist_istoplevel(x))
	    sys_vgui("pdtk_undomenu .x%x no %s\n", x, canvas_undo_name);
#endif
	canvas_undo_whatnext = UNDO_REDO;
    }
}

static void canvas_redo(t_canvas *x)
{
    if (x != canvas_undo_canvas)
    	bug("canvas_undo 1");
    else if (canvas_undo_whatnext != UNDO_REDO)
    	bug("canvas_undo 2");
    else
    {
	/* post("redo"); */
	(*canvas_undo_fn)(canvas_undo_canvas, canvas_undo_buf, UNDO_REDO);
    	    /* enable undo in menu */
#ifndef ROCKBOX
	if (glist_isvisible(x) && glist_istoplevel(x))
	    sys_vgui("pdtk_undomenu .x%x %s no\n", x, canvas_undo_name);
#endif
	canvas_undo_whatnext = UNDO_UNDO;
    }
}

/* ------- specific undo methods: 1. connect and disconnect -------- */

typedef struct _undo_connect	
{
    int u_index1;
    int u_outletno;
    int u_index2;
    int u_inletno;
} t_undo_connect;

static void *canvas_undo_set_disconnect(t_canvas *x,
    int index1, int outno, int index2, int inno)
{
#ifdef ROCKBOX
    (void) x;
#endif
    t_undo_connect *buf = (t_undo_connect *)getbytes(sizeof(*buf));
    buf->u_index1 = index1;
    buf->u_outletno = outno;
    buf->u_index2 = index2;
    buf->u_inletno = inno;
    return (buf);
}

void canvas_disconnect(t_canvas *x,
    float index1, float outno, float index2, float inno)
{
    t_linetraverser t;
    t_outconnect *oc;
    linetraverser_start(&t, x);
    while((oc = linetraverser_next(&t)))
    {
    	int srcno = canvas_getindex(x, &t.tr_ob->ob_g);
	int sinkno = canvas_getindex(x, &t.tr_ob2->ob_g);
    	if (srcno == index1 && t.tr_outno == outno &&
	    sinkno == index2 && t.tr_inno == inno)
    	{
#ifndef ROCKBOX
    	    sys_vgui(".x%x.c delete l%x\n", x, oc);
#endif
    	    obj_disconnect(t.tr_ob, t.tr_outno, t.tr_ob2, t.tr_inno);
	    break;
    	}
    }
}

static void canvas_undo_disconnect(t_canvas *x, void *z, int action)
{
    t_undo_connect *buf = z;
    if (action == UNDO_UNDO)
    {
    	canvas_connect(x, buf->u_index1, buf->u_outletno,
	    buf->u_index2, buf->u_inletno);
    }
    else if (action == UNDO_REDO)
    {
    	canvas_disconnect(x, buf->u_index1, buf->u_outletno,
	    buf->u_index2, buf->u_inletno);
    }
    else if (action == UNDO_FREE)
    	t_freebytes(buf, sizeof(*buf));
}

    /* connect just calls disconnect actions backward... */
static void *canvas_undo_set_connect(t_canvas *x,
    int index1, int outno, int index2, int inno)
{
    return (canvas_undo_set_disconnect(x, index1, outno, index2, inno));
}

static void canvas_undo_connect(t_canvas *x, void *z, int action)
{
    int myaction;
    if (action == UNDO_UNDO)
    	myaction = UNDO_REDO;
    else if (action == UNDO_REDO)
    	myaction = UNDO_UNDO;
    else myaction = action;
    canvas_undo_disconnect(x, z, myaction);
}

/* ---------- ... 2. cut, clear, and typing into objects: -------- */

#define UCUT_CUT 1  	    /* operation was a cut */
#define UCUT_CLEAR 2	    /* .. a clear */
#define UCUT_TEXT 3 	    /* text typed into a box */

typedef struct _undo_cut	
{
    t_binbuf *u_objectbuf;  	/* the object cleared or typed into */
    t_binbuf *u_reconnectbuf;   /* connections into and out of object */
    t_binbuf *u_redotextbuf;	/* buffer to paste back for redo if TEXT */
    int u_mode;   	    	/* from flags above */
} t_undo_cut;

static void *canvas_undo_set_cut(t_canvas *x, int mode)
{
    t_undo_cut *buf;
#ifndef ROCKBOX
    t_gobj *y;
#endif
    t_linetraverser t;
    t_outconnect *oc;
    int nnotsel= glist_selectionindex(x, 0, 0);
    buf = (t_undo_cut *)getbytes(sizeof(*buf));
    buf->u_mode = mode;
    buf->u_redotextbuf = 0;

    	/* store connections into/out of the selection */
    buf->u_reconnectbuf = binbuf_new();
    linetraverser_start(&t, x);
    while((oc = linetraverser_next(&t)))
    {
	int issel1 = glist_isselected(x, &t.tr_ob->ob_g);
	int issel2 = glist_isselected(x, &t.tr_ob2->ob_g);
    	if (issel1 != issel2)
    	{
    	    binbuf_addv(buf->u_reconnectbuf, "ssiiii;",
		gensym("#X"), gensym("connect"),
		(issel1 ? nnotsel : 0)
		    + glist_selectionindex(x, &t.tr_ob->ob_g, issel1),
		t.tr_outno,
		(issel2 ? nnotsel : 0) +
		    glist_selectionindex(x, &t.tr_ob2->ob_g, issel2),
		t.tr_inno);
    	}
    }
    if (mode == UCUT_TEXT)
    {
	buf->u_objectbuf = canvas_docopy(x);
    }
    else if (mode == UCUT_CUT)
    {
    	buf->u_objectbuf = 0;
    }
    else if (mode == UCUT_CLEAR)
    {
	buf->u_objectbuf = canvas_docopy(x);
    }
    return (buf);
}

static void canvas_undo_cut(t_canvas *x, void *z, int action)
{
    t_undo_cut *buf = z;
    int mode = buf->u_mode;
    if (action == UNDO_UNDO)
    {
    	if (mode == UCUT_CUT)
    	    canvas_dopaste(x, copy_binbuf);
    	else if (mode == UCUT_CLEAR)
	    canvas_dopaste(x, buf->u_objectbuf);
	else if (mode == UCUT_TEXT)
	{
	    t_gobj *y1, *y2;
	    glist_noselect(x);
	    for(y1 = x->gl_list; (y2 = y1->g_next); y1 = y2)
	    	;
	    if (y1)
	    {
	    	if (!buf->u_redotextbuf)
		{
	    	    glist_noselect(x);
		    glist_select(x, y1);
		    buf->u_redotextbuf = canvas_docopy(x);
	    	    glist_noselect(x);
		}
	    	glist_delete(x, y1);
	    }
	    canvas_dopaste(x, buf->u_objectbuf);
	}
	pd_bind(&x->gl_pd, gensym("#X"));
	binbuf_eval(buf->u_reconnectbuf, 0, 0, 0);
	pd_unbind(&x->gl_pd, gensym("#X"));
    }
    else if (action == UNDO_REDO)
    {
    	if (mode == UCUT_CUT || mode == UCUT_CLEAR)
	    canvas_doclear(x);
    	else if (mode == UCUT_TEXT)
	{
	    t_gobj *y1, *y2;
	    for(y1 = x->gl_list; (y2 = y1->g_next); y1 = y2)
	    	;
	    if (y1)
	    	glist_delete(x, y1);
	    canvas_dopaste(x, buf->u_redotextbuf);
	    pd_bind(&x->gl_pd, gensym("#X"));
	    binbuf_eval(buf->u_reconnectbuf, 0, 0, 0);
	    pd_unbind(&x->gl_pd, gensym("#X"));
	}
    }
    else if (action == UNDO_FREE)
    {
    	if (buf->u_objectbuf)
	    binbuf_free(buf->u_objectbuf);
    	if (buf->u_reconnectbuf)
	    binbuf_free(buf->u_reconnectbuf);
    	if (buf->u_redotextbuf)
	    binbuf_free(buf->u_redotextbuf);
    	t_freebytes(buf, sizeof(*buf));
    }
}

/* --------- 3. motion, including "tidy up" and stretching ----------- */

typedef struct _undo_move_elem	
{
    int e_index;
    int e_xpix;
    int e_ypix;
} t_undo_move_elem;

typedef struct _undo_move	
{
    t_undo_move_elem *u_vec;
    int u_n;
} t_undo_move;

static int canvas_undo_already_set_move;

static void *canvas_undo_set_move(t_canvas *x, int selected)
{
    int x1, y1, x2, y2, i, indx;
    t_gobj *y;
    t_undo_move *buf =  (t_undo_move *)getbytes(sizeof(*buf));
    buf->u_n = selected ? glist_selectionindex(x, 0, 1) : glist_getindex(x, 0);
    buf->u_vec = (t_undo_move_elem *)getbytes(sizeof(*buf->u_vec) *
    	(selected ? glist_selectionindex(x, 0, 1) : glist_getindex(x, 0)));
    if (selected)
    {
    	for (y = x->gl_list, i = indx = 0; y; y = y->g_next, indx++)
	    if (glist_isselected(x, y))
	{
    	    gobj_getrect(y, x, &x1, &y1, &x2, &y2);
	    buf->u_vec[i].e_index = indx;
	    buf->u_vec[i].e_xpix = x1;
	    buf->u_vec[i].e_ypix = y1;
	    i++;
    	}
    }
    else
    {
    	for (y = x->gl_list, indx = 0; y; y = y->g_next, indx++)
	{
    	    gobj_getrect(y, x, &x1, &y1, &x2, &y2);
	    buf->u_vec[indx].e_index = indx;
	    buf->u_vec[indx].e_xpix = x1;
	    buf->u_vec[indx].e_ypix = y1;
    	}
    }
    canvas_undo_already_set_move = 1;
    return (buf);
}

static void canvas_undo_move(t_canvas *x, void *z, int action)
{
    t_undo_move *buf = z;
    if (action == UNDO_UNDO || action == UNDO_REDO)
    {
    	int i;
	for (i = 0; i < buf->u_n; i++)
	{
	    int x1, y1, x2, y2, newx, newy;
	    t_gobj *y;
	    newx = buf->u_vec[i].e_xpix;
	    newy = buf->u_vec[i].e_ypix;
	    y = glist_nth(x, buf->u_vec[i].e_index);
	    if (y)
	    {
	    	gobj_getrect(y, x, &x1, &y1, &x2, &y2);
		gobj_displace(y, x, newx-x1, newy - y1);
	    	buf->u_vec[i].e_xpix = x1;
	    	buf->u_vec[i].e_ypix = y1;
	    }
	}
    }
    else if (action == UNDO_FREE)
    {
    	t_freebytes(buf->u_vec, buf->u_n * sizeof(*buf->u_vec));
    	t_freebytes(buf, sizeof(*buf));
    }
}

/* --------- 4. paste (also duplicate) ----------- */

typedef struct _undo_paste	
{
    int u_index;    /* index of first object pasted */	
} t_undo_paste;

static void *canvas_undo_set_paste(t_canvas *x)
{
    t_undo_paste *buf =  (t_undo_paste *)getbytes(sizeof(*buf));
    buf->u_index = glist_getindex(x, 0);
    return (buf);
}

static void canvas_undo_paste(t_canvas *x, void *z, int action)
{
    t_undo_paste *buf = z;
    if (action == UNDO_UNDO)
    {
    	t_gobj *y;
    	glist_noselect(x);
	for (y = glist_nth(x, buf->u_index); y; y = y->g_next)
	    glist_select(x, y);
	canvas_doclear(x);
    }
    else if (action == UNDO_REDO)
    {
    	t_selection *sel;
    	canvas_dopaste(x, copy_binbuf);
	    /* if it was "duplicate" have to re-enact the displacement. */
	if (canvas_undo_name && canvas_undo_name[0] == 'd')
    	    for (sel = x->gl_editor->e_selection; sel; sel = sel->sel_next)
    	    	gobj_displace(sel->sel_what, x, 10, 10);
    }
else if (action == UNDO_FREE)
    	t_freebytes(buf, sizeof(*buf));
}

    /* recursively check for abstractions to reload as result of a save. 
    Don't reload the one we just saved ("except") though. */
    /*  LATER try to do the same trick for externs. */
static void glist_doreload(t_glist *gl, t_symbol *name, t_symbol *dir,
    t_gobj *except)
{
    t_gobj *g;
    int i, nobj = glist_getindex(gl, 0);  /* number of objects */
    for (g = gl->gl_list, i = 0; g && i < nobj; i++)
    {
    	if (g != except && pd_class(&g->g_pd) == canvas_class &&
	    canvas_isabstraction((t_canvas *)g) &&
	    	((t_canvas *)g)->gl_name == name &&
		    canvas_getdir((t_canvas *)g) == dir)
	{
	    	/* we're going to remake the object, so "g" will go stale.
		Get its index here, and afterward restore g.  Also, the
		replacement will be at teh end of the list, so we don't
		do g = g->g_next in this case. */
	    int j = glist_getindex(gl, g);
	    if (!gl->gl_havewindow)
	    	canvas_vis(glist_getcanvas(gl), 1);
	    glist_noselect(gl);
	    glist_select(gl, g);
	    canvas_setundo(gl, canvas_undo_cut,
		canvas_undo_set_cut(gl, UCUT_CLEAR), "clear");
	    canvas_doclear(gl);
	    canvas_undo(gl);
	    glist_noselect(gl);
	    g = glist_nth(gl, j);
	}
    	else
	{
	    if (g != except && pd_class(&g->g_pd) == canvas_class)
    	    	glist_doreload((t_canvas *)g, name, dir, except);
	     g = g->g_next;
	}
    }
}

    /* call canvas_doreload on everyone */
void canvas_reload(t_symbol *name, t_symbol *dir, t_gobj *except)
{
    t_canvas *x;
    	/* find all root canvases */
    for (x = canvas_list; x; x = x->gl_next)
    	glist_doreload(x, name, dir, except);
}

/* ------------------------ event handling ------------------------ */

#define CURSOR_RUNMODE_NOTHING 0
#define CURSOR_RUNMODE_CLICKME 1
#define CURSOR_RUNMODE_THICKEN 2
#define CURSOR_RUNMODE_ADDPOINT 3
#define CURSOR_EDITMODE_NOTHING 4
#define CURSOR_EDITMODE_CONNECT 5
#define CURSOR_EDITMODE_DISCONNECT 6

static char *cursorlist[] = {
#ifdef MSW
    "right_ptr",     	/* CURSOR_RUNMODE_NOTHING */
#else
    "left_ptr",     	/* CURSOR_RUNMODE_NOTHING */
#endif
    "arrow",	    	/* CURSOR_RUNMODE_CLICKME */
    "sb_v_double_arrow", /* CURSOR_RUNMODE_THICKEN */
    "plus",	    	/* CURSOR_RUNMODE_ADDPOINT */
    "hand2",	    	/* CURSOR_EDITMODE_NOTHING */
    "circle",	    	/* CURSOR_EDITMODE_CONNECT */
    "X_cursor"	    	/* CURSOR_EDITMODE_DISCONNECT */
};

void canvas_setcursor(t_canvas *x, unsigned int cursornum)
{
    static t_canvas *xwas;
    static unsigned int cursorwas;
    if (cursornum >= sizeof(cursorlist)/sizeof *cursorlist)
    {
    	bug("canvas_setcursor");
	return;
    }
    if (xwas != x || cursorwas != cursornum)
    {
#ifndef ROCKBOX
    	sys_vgui(".x%x configure -cursor %s\n", x, cursorlist[cursornum]);
#endif
    	xwas = x;
    	cursorwas = cursornum;
    }
}

    /* check if a point lies in a gobj.  */
int canvas_hitbox(t_canvas *x, t_gobj *y, int xpos, int ypos,
    int *x1p, int *y1p, int *x2p, int *y2p)
{
    int x1, y1, x2, y2;
    t_text *ob;
    if ((ob = pd_checkobject(&y->g_pd)) && 
    	!text_shouldvis(ob, x))
	    return (0);
    gobj_getrect(y, x, &x1, &y1, &x2, &y2);
    if (xpos >= x1 && xpos <= x2 && ypos >= y1 && ypos <= y2)
    {
    	*x1p = x1;
    	*y1p = y1;
    	*x2p = x2;
    	*y2p = y2;
    	return (1);
    }
    else return (0);
}

    /* find the last gobj, if any, containing the point. */
static t_gobj *canvas_findhitbox(t_canvas *x, int xpos, int ypos,
    int *x1p, int *y1p, int *x2p, int *y2p)
{
    t_gobj *y, *rval = 0;
    for (y = x->gl_list; y; y = y->g_next)
    {
    	if (canvas_hitbox(x, y, xpos, ypos, x1p, y1p, x2p, y2p))
	    rval = y; 
    }
    return (rval);
}

    /* right-clicking on a canvas object pops up a menu. */
static void canvas_rightclick(t_canvas *x, int xpos, int ypos, t_gobj *y)
{
    int canprop, canopen;
    canprop = (!y || (y && class_getpropertiesfn(pd_class(&y->g_pd))));
    canopen = (y && zgetfn(&y->g_pd, gensym("menu-open")));
#ifdef ROCKBOX
    (void) x;
    (void) xpos;
    (void) ypos;
#else /* ROCKBOX */
    sys_vgui("pdtk_canvas_popup .x%x %d %d %d %d\n",
    	x, xpos, ypos, canprop, canopen);
#endif /* ROCKBOX */
}

    /* tell GUI to create a properties dialog on the canvas.  We tell
    the user the negative of the "pixel" y scale to make it appear to grow
    naturally upward, whereas pixels grow downward. */
static void canvas_properties(t_glist *x)
{
#ifdef ROCKBOX
    (void) x;
#else /* ROCKBOX */
    char graphbuf[200];
    sprintf(graphbuf, "pdtk_canvas_dialog %%s %g %g %g %g \n",
	glist_dpixtodx(x, 1), -glist_dpixtody(x, 1),
	(float)glist_isgraph(x), (float)x->gl_stretch);
    gfxstub_new(&x->gl_pd, x, graphbuf);
#endif /* ROCKBOX */
}


void canvas_setgraph(t_glist *x, int flag)
{
    if (!flag && glist_isgraph(x))
    {
	if (x->gl_owner && !x->gl_loading && glist_isvisible(x->gl_owner))
    	    gobj_vis(&x->gl_gobj, x->gl_owner, 0);
	x->gl_isgraph = 0;
	if (x->gl_owner && !x->gl_loading && glist_isvisible(x->gl_owner))
	{
    	    gobj_vis(&x->gl_gobj, x->gl_owner, 1);
    	    canvas_fixlinesfor(x->gl_owner, &x->gl_obj);
	}
    }
    else if (flag && !glist_isgraph(x))
    {
	if (x->gl_pixwidth <= 0)
	    x->gl_pixwidth = GLIST_DEFGRAPHWIDTH;

	if (x->gl_pixheight <= 0)
	    x->gl_pixheight = GLIST_DEFGRAPHHEIGHT;

	if (x->gl_owner && !x->gl_loading && glist_isvisible(x->gl_owner))
    	    gobj_vis(&x->gl_gobj, x->gl_owner, 0);
	x->gl_isgraph = 1;
    	/* if (x->gl_owner && glist_isvisible(x->gl_owner))
    	    canvas_vis(x, 1); */
	if (x->gl_loading && x->gl_owner && glist_isvisible(x->gl_owner))
	    canvas_create_editor(x, 1);
	if (x->gl_owner && !x->gl_loading && glist_isvisible(x->gl_owner))
	{
    	    gobj_vis(&x->gl_gobj, x->gl_owner, 1);
    	    canvas_fixlinesfor(x->gl_owner, &x->gl_obj);
	}
    }
}

    /* called from the gui when "OK" is selected on the canvas properties
    	dialog.  Again we negate "y" scale. */
static void canvas_donecanvasdialog(t_glist *x,  t_floatarg xperpix,
    t_floatarg yperpix, t_floatarg fgraphme)
{
    int graphme = (fgraphme != 0), redraw = 0;
    yperpix = -yperpix;
    if (xperpix == 0)
    	xperpix = 1;
    if (yperpix == 0)
    	yperpix = 1;
    canvas_setgraph(x, graphme);
    if (!x->gl_isgraph && (xperpix != glist_dpixtodx(x, 1)))
    {
	if (xperpix > 0)
	{
	    x->gl_x1 = 0;
	    x->gl_x2 = xperpix;
	}
	else
	{
	    x->gl_x1 = -xperpix * (x->gl_screenx2 - x->gl_screenx1);
	    x->gl_x2 = x->gl_x1 + xperpix;
	}
	redraw = 1;
    } 	
    if (!x->gl_isgraph && (yperpix != glist_dpixtody(x, 1)))
    {
	if (yperpix > 0)
	{
	    x->gl_y1 = 0;
	    x->gl_y2 = yperpix;
	}
	else
	{
	    x->gl_y1 = -yperpix * (x->gl_screeny2 - x->gl_screeny1);
	    x->gl_y2 = x->gl_y1 + yperpix;
	}
	redraw = 1;
    }
    if (redraw)
    	canvas_redraw(x);
}

    /* called from the gui when a popup menu comes back with "properties,"
    	"open," or "help." */
static void canvas_done_popup(t_canvas *x, float which, float xpos, float ypos)
{
#ifdef ROCKBOX
    char namebuf[MAXPDSTRING];
#else
    char pathbuf[MAXPDSTRING], namebuf[MAXPDSTRING];
#endif
    t_gobj *y;
    for (y = x->gl_list; y; y = y->g_next)
    {
    	int x1, y1, x2, y2;
    	if (canvas_hitbox(x, y, xpos, ypos, &x1, &y1, &x2, &y2))
	{
    	    if (which == 0) 	/* properties */
	    {
		if (!class_getpropertiesfn(pd_class(&y->g_pd)))
		    continue;
    	    	(*class_getpropertiesfn(pd_class(&y->g_pd)))(y, x);
		return;
	    }
	    else if (which == 1)    /* open */
	    {
		if (!zgetfn(&y->g_pd, gensym("menu-open")))
		    continue;
		vmess(&y->g_pd, gensym("menu-open"), "");
		return;
	    }
	    else    /* help */
	    {
		char *dir;
	    	if (pd_class(&y->g_pd) == canvas_class &&
		    canvas_isabstraction((t_canvas *)y))
		{
		    t_object *ob = (t_object *)y;
		    int ac = binbuf_getnatom(ob->te_binbuf);
		    t_atom *av = binbuf_getvec(ob->te_binbuf);
		    if (ac < 1)
		    	return;
		    atom_string(av, namebuf, MAXPDSTRING);
		    dir = canvas_getdir((t_canvas *)y)->s_name;
		}
    		else
		{
		    strcpy(namebuf, class_gethelpname(pd_class(&y->g_pd)));
		    dir = class_gethelpdir(pd_class(&y->g_pd));
    		}
		if (strcmp(namebuf + strlen(namebuf) - 3, ".pd"))
    		    strcat(namebuf, ".pd");
		open_via_helppath(namebuf, dir);
		return;
    	    }
	}
    }
    if (which == 0)
    	canvas_properties(x);
    else if (which == 2)
    {
#ifndef ROCKBOX
	strcpy(pathbuf, sys_libdir->s_name);
	strcat(pathbuf, "/doc/5.reference/0.INTRO.txt");
	sys_vgui("menu_opentext %s\n", pathbuf);
#endif
    }
}

#define NOMOD 0
#define SHIFTMOD 1
#define CTRLMOD 2
#define ALTMOD 4
#define RIGHTCLICK 8

/* on one-button-mouse machines, you can use double click to
    mean right click (which gets the popup menu.)  Do this for Mac. */
#ifdef MACOSX
#define SIMULATERIGHTCLICK
#endif

#ifdef SIMULATERIGHTCLICK
static double canvas_upclicktime;
static int canvas_upx, canvas_upy;
#define DCLICKINTERVAL 0.25
#endif

    /* mouse click */
void canvas_doclick(t_canvas *x, int xpos, int ypos, int which,
    int mod, int doit)
{
    t_gobj *y;
    int shiftmod, runmode, altmod, rightclick;
    int x1, y1, x2, y2, clickreturned = 0;

#ifdef ROCKBOX
    (void) which;
#endif

    if (!x->gl_editor)
    {
    	bug("editor");
    	return;
    }
    
    shiftmod = (mod & SHIFTMOD);
    runmode = ((mod & CTRLMOD) || (!x->gl_edit));
    altmod = (mod & ALTMOD);
    rightclick = (mod & RIGHTCLICK);

    canvas_undo_already_set_move = 0;

    	    /* if keyboard was grabbed, notify grabber and cancel the grab */
    if (doit && x->gl_editor->e_grab && x->gl_editor->e_keyfn)
    {
    	(* x->gl_editor->e_keyfn) (x->gl_editor->e_grab, 0);
    	glist_grab(x, 0, 0, 0, 0, 0);
    }

#ifdef SIMULATERIGHTCLICK
    if (doit && !runmode && xpos == canvas_upx && ypos == canvas_upy &&
    	sys_getrealtime() - canvas_upclicktime < DCLICKINTERVAL)
	    rightclick = 1;
#endif
    	
    x->gl_editor->e_lastmoved = 0;
    if (doit)
    {
    	x->gl_editor->e_grab = 0;
    	x->gl_editor->e_onmotion = MA_NONE;
    }
    /* post("click %d %d %d %d", xpos, ypos, which, mod); */
    
    if (x->gl_editor->e_onmotion != MA_NONE)
    	return;

    x->gl_editor->e_xwas = xpos;
    x->gl_editor->e_ywas = ypos;

    if (runmode && !rightclick)
    {
    	for (y = x->gl_list; y; y = y->g_next)
	{
	    	/* check if the object wants to be clicked */
    	    if (canvas_hitbox(x, y, xpos, ypos, &x1, &y1, &x2, &y2)
	    	&& (clickreturned = gobj_click(y, x, xpos, ypos,
		    shiftmod, altmod, 0, doit)))
    	    	    	break;
	}
	if (!doit)
	{
	    if (y)
	    	canvas_setcursor(x, clickreturned);
	    else canvas_setcursor(x, CURSOR_RUNMODE_NOTHING);
	}
	return;
    }
    	/* if not a runmode left click, fall here. */
    if((y = canvas_findhitbox(x, xpos, ypos, &x1, &y1, &x2, &y2)))
    {
    	t_object *ob = pd_checkobject(&y->g_pd);
    	    /* check you're in the rectangle */
    	ob = pd_checkobject(&y->g_pd);
    	if (rightclick)
	    canvas_rightclick(x, xpos, ypos, y);
    	else if (shiftmod)
    	{
    	    if (doit)
    	    {
		t_rtext *rt;
		if (ob && (rt = x->gl_editor->e_textedfor) &&
		    rt == glist_findrtext(x, ob))
    	    	{
		    rtext_mouse(rt, xpos - x1, ypos - y1, RTEXT_SHIFT);
		    x->gl_editor->e_onmotion = MA_DRAGTEXT;
		    x->gl_editor->e_xwas = x1;
		    x->gl_editor->e_ywas = y1;
    	    	}
		else
		{
		    if (glist_isselected(x, y))
    	    	    	glist_deselect(x, y);
    	    	    else glist_select(x, y);
    	    	}
	    }
    	}
    	else
    	{
    	    	/* look for an outlet */
    	    int noutlet;
    	    if (ob && (noutlet = obj_noutlets(ob)) && ypos >= y2-4)
    	    {
    	    	int width = x2 - x1;
    	    	int nout1 = (noutlet > 1 ? noutlet - 1 : 1);
    	    	int closest = ((xpos-x1) * (nout1) + width/2)/width;
    	    	int hotspot = x1 +
    	    	    (width - IOWIDTH) * closest / (nout1);
		if (closest < noutlet &&
    	    	    xpos >= (hotspot-1) && xpos <= hotspot + (IOWIDTH+1))
    	    	{
    	    	    if (doit)
    	    	    {
#ifndef ROCKBOX
		    	int issignal = obj_issignaloutlet(ob, closest);
#endif
    	    	    	x->gl_editor->e_onmotion = MA_CONNECT;
    	    	    	x->gl_editor->e_xwas = xpos;
    	    	    	x->gl_editor->e_ywas = ypos;
#ifndef ROCKBOX
    	    	    	sys_vgui(
    	    	    	  ".x%x.c create line %d %d %d %d -width %d -tags x\n",
    	    	    	    	x, xpos, ypos, xpos, ypos,
				    (issignal ? 2 : 1));
#endif
    	    	    }    	    	    	    	
    	    	    else canvas_setcursor(x, CURSOR_EDITMODE_CONNECT);
    	    	}
		else if (doit)
		    goto nooutletafterall;
    	    }
    	    	/* not in an outlet; select and move */
    	    else if (doit)
    	    {
		t_rtext *rt;
		    /* check if the box is being text edited */
	    nooutletafterall:
		if (ob && (rt = x->gl_editor->e_textedfor) &&
		    rt == glist_findrtext(x, ob))
    	    	{
		    rtext_mouse(rt, xpos - x1, ypos - y1, RTEXT_DOWN);
		    x->gl_editor->e_onmotion = MA_DRAGTEXT;
		    x->gl_editor->e_xwas = x1;
		    x->gl_editor->e_ywas = y1;
		}
    	    	else
		{
			/* otherwise select and drag to displace */
		    if (!glist_isselected(x, y))
    	    	    {
    	    		glist_noselect(x);
    	    		glist_select(x, y);
    	    	    }
		    x->gl_editor->e_onmotion = MA_MOVE;
		}
	    }
    	    else canvas_setcursor(x, CURSOR_EDITMODE_NOTHING); 
    	}
    	return;
    }
    	/* if right click doesn't hit any boxes, call rightclick
	    routine anyway */
    if (rightclick)
    	canvas_rightclick(x, xpos, ypos, 0);

    	/* if not an editing action, and if we didn't hit a
    	box, set cursor and return */
    if (runmode || rightclick)
    {
    	canvas_setcursor(x, CURSOR_RUNMODE_NOTHING);
    	return;
    }
    	/* having failed to find a box, we try lines now. */
    if (!runmode && !altmod && !shiftmod)
    {
    	t_linetraverser t;
    	t_outconnect *oc;
    	float fx = xpos, fy = ypos;
	t_glist *glist2 = glist_getcanvas(x);
    	linetraverser_start(&t, glist2);
    	while((oc = linetraverser_next(&t)))
    	{
    	    float lx1 = t.tr_lx1, ly1 = t.tr_ly1,
    	    	lx2 = t.tr_lx2, ly2 = t.tr_ly2;
    	    float area = (lx2 - lx1) * (fy - ly1) -
    		(ly2 - ly1) * (fx - lx1);
    	    float dsquare = (lx2-lx1) * (lx2-lx1) + (ly2-ly1) * (ly2-ly1);
    	    if (area * area >= 50 * dsquare) continue;
    	    if ((lx2-lx1) * (fx-lx1) + (ly2-ly1) * (fy-ly1) < 0) continue;
    	    if ((lx2-lx1) * (lx2-fx) + (ly2-ly1) * (ly2-fy) < 0) continue;
    	    if (doit)
    	    {
	    	glist_selectline(glist2, oc, 
		    canvas_getindex(glist2, &t.tr_ob->ob_g), t.tr_outno,
		    canvas_getindex(glist2, &t.tr_ob2->ob_g), t.tr_inno);
    	    }
    	    canvas_setcursor(x, CURSOR_EDITMODE_DISCONNECT);
    	    return;
    	}
    }
    canvas_setcursor(x, CURSOR_EDITMODE_NOTHING);
    if (doit)
    {
	if (!shiftmod) glist_noselect(x);
#ifndef ROCKBOX
    	sys_vgui(".x%x.c create rectangle %d %d %d %d -tags x\n",
    	      x, xpos, ypos, xpos, ypos);
#endif
	x->gl_editor->e_xwas = xpos;
	x->gl_editor->e_ywas = ypos;
    	x->gl_editor->e_onmotion = MA_REGION;
    }
}

void canvas_mousedown(t_canvas *x, t_floatarg xpos, t_floatarg ypos,
    t_floatarg which, t_floatarg mod)
{
    canvas_doclick(x, xpos, ypos, which, mod, 1);
}

int canvas_isconnected (t_canvas *x, t_text *ob1, int n1,
    t_text *ob2, int n2)
{
    t_linetraverser t;
    t_outconnect *oc;
    linetraverser_start(&t, x);
    while((oc = linetraverser_next(&t)))
    	if (t.tr_ob == ob1 && t.tr_outno == n1 &&
	    t.tr_ob2 == ob2 && t.tr_inno == n2) 
	    	return (1);
    return (0);
}

void canvas_doconnect(t_canvas *x, int xpos, int ypos, int which, int doit)
{
    int x11, y11, x12, y12;
    t_gobj *y1;
    int x21, y21, x22, y22;
    t_gobj *y2;
    int xwas = x->gl_editor->e_xwas,
    	ywas = x->gl_editor->e_ywas;
#ifdef ROCKBOX
    (void) which;
#endif /* ROCKBOX */
#ifndef ROCKBOX
    if (doit) sys_vgui(".x%x.c delete x\n", x);
    else sys_vgui(".x%x.c coords x %d %d %d %d\n",
    	    x, x->gl_editor->e_xwas,
    	    	x->gl_editor->e_ywas, xpos, ypos);
#endif /* ROCKBOX */

    if ((y1 = canvas_findhitbox(x, xwas, ywas, &x11, &y11, &x12, &y12))
    	&& (y2 = canvas_findhitbox(x, xpos, ypos, &x21, &y21, &x22, &y22)))
    {
    	t_object *ob1 = pd_checkobject(&y1->g_pd);
    	t_object *ob2 = pd_checkobject(&y2->g_pd);
    	int noutlet1, ninlet2;
    	if (ob1 && ob2 && ob1 != ob2 &&
    	    (noutlet1 = obj_noutlets(ob1))
    	    && (ninlet2 = obj_ninlets(ob2)))
    	{
    	    int width1 = x12 - x11, closest1, hotspot1;
    	    int width2 = x22 - x21, closest2, hotspot2;
    	    int lx1, lx2, ly1, ly2;
    	    t_outconnect *oc;

    	    if (noutlet1 > 1)
    	    {
    		closest1 = ((xwas-x11) * (noutlet1-1) + width1/2)/width1;
    		hotspot1 = x11 +
    	    	    (width1 - IOWIDTH) * closest1 / (noutlet1-1);
    	    }
    	    else closest1 = 0, hotspot1 = x11;

    	    if (ninlet2 > 1)
    	    {
    		closest2 = ((xpos-x21) * (ninlet2-1) + width2/2)/width2;
    		hotspot2 = x21 +
    	    	    (width2 - IOWIDTH) * closest2 / (ninlet2-1);
    	    }
    	    else closest2 = 0, hotspot2 = x21;

    	    if (closest1 >= noutlet1)
    	    	closest1 = noutlet1 - 1;
    	    if (closest2 >= ninlet2)
    	    	closest2 = ninlet2 - 1;

    	    if (canvas_isconnected (x, ob1, closest1, ob2, closest2))
	    {
    	    	canvas_setcursor(x, CURSOR_EDITMODE_NOTHING);
		return;
	    }
	    if (obj_issignaloutlet(ob1, closest1) &&
	    	!obj_issignalinlet(ob2, closest2))
	    {
	    	if (doit)
		    error("can't connect signal outlet to control inlet");
    	    	canvas_setcursor(x, CURSOR_EDITMODE_NOTHING);
		return;
	    }
    	    if (doit)
    	    {
    	    	oc = obj_connect(ob1, closest1, ob2, closest2);
    	    	lx1 = x11 + (noutlet1 > 1 ?
   	    		((x12-x11-IOWIDTH) * closest1)/(noutlet1-1) : 0)
   	    		     + IOMIDDLE;
    	    	ly1 = y12;
    	    	lx2 = x21 + (ninlet2 > 1 ?
   	    		((x22-x21-IOWIDTH) * closest2)/(ninlet2-1) : 0)
   	    		    + IOMIDDLE;
    	    	ly2 = y21;
#ifndef ROCKBOX
    	    	sys_vgui(".x%x.c create line %d %d %d %d -width %d -tags l%x\n",
		    glist_getcanvas(x),
		    	lx1, ly1, lx2, ly2,
			    (obj_issignaloutlet(ob1, closest1) ? 2 : 1), oc);
#endif /* ROCKBOX */
		canvas_setundo(x, canvas_undo_connect,
		    canvas_undo_set_connect(x, 
		    	canvas_getindex(x, &ob1->ob_g), closest1,
			canvas_getindex(x, &ob2->ob_g), closest2),
		    	"connect");
	    }
    	    else canvas_setcursor(x, CURSOR_EDITMODE_CONNECT);
    	    return;
    	}
    }
    canvas_setcursor(x, CURSOR_EDITMODE_NOTHING);
}

void canvas_selectinrect(t_canvas *x, int lox, int loy, int hix, int hiy)
{
    t_gobj *y;
    for (y = x->gl_list; y; y = y->g_next)
    {
    	int x1, y1, x2, y2;
    	gobj_getrect(y, x, &x1, &y1, &x2, &y2);
    	if (hix >= x1 && lox <= x2 && hiy >= y1 && loy <= y2
    	    && !glist_isselected(x, y))
    	    	glist_select(x, y);
    }
}

static void canvas_doregion(t_canvas *x, int xpos, int ypos, int doit)
{
    if (doit)
    {
    	int lox, loy, hix, hiy;
    	if (x->gl_editor->e_xwas < xpos)
    	    lox = x->gl_editor->e_xwas, hix = xpos;
    	else hix = x->gl_editor->e_xwas, lox = xpos;
    	if (x->gl_editor->e_ywas < ypos)
    	    loy = x->gl_editor->e_ywas, hiy = ypos;
    	else hiy = x->gl_editor->e_ywas, loy = ypos;
	canvas_selectinrect(x, lox, loy, hix, hiy);
#ifndef ROCKBOX
    	sys_vgui(".x%x.c delete x\n", x);
#endif
    	x->gl_editor->e_onmotion = 0;
    }
#ifndef ROCKBOX
    else sys_vgui(".x%x.c coords x %d %d %d %d\n",
    	    x, x->gl_editor->e_xwas,
    	    	x->gl_editor->e_ywas, xpos, ypos);
#endif
}

void canvas_mouseup(t_canvas *x,
    t_floatarg fxpos, t_floatarg fypos, t_floatarg fwhich)
{
    t_gobj *y;

    int xpos = fxpos, ypos = fypos, which = fwhich;

    if (!x->gl_editor)
    {
    	bug("editor");
    	return;
    }

#ifdef SIMULATERIGHTCLICK
    canvas_upclicktime = sys_getrealtime();
    canvas_upx = xpos;
    canvas_upy = ypos;
#endif

    if (x->gl_editor->e_onmotion == MA_CONNECT)
    	canvas_doconnect(x, xpos, ypos, which, 1);
    else if (x->gl_editor->e_onmotion == MA_REGION)
    	canvas_doregion(x, xpos, ypos, 1);
    else if (x->gl_editor->e_onmotion == MA_MOVE)
    {
    	    /* after motion, if there's only one item selected, activate it */
    	if (x->gl_editor->e_selection &&
    	    !(x->gl_editor->e_selection->sel_next))
    	    	gobj_activate(x->gl_editor->e_selection->sel_what,
    	    	    x, 1);
    }
    else if (x->gl_editor->e_onmotion == MA_PASSOUT)
    	x->gl_editor->e_onmotion = 0;
    x->gl_editor->e_onmotion = MA_NONE;


#if 1
    /* GG misused the (unused) dbl parameter to tell if mouseup */

    for (y = x->gl_list; y; y = y->g_next) {
        int x1, y1, x2, y2, clickreturned = 0;
        
        /* check if the object wants to be clicked */
        if (canvas_hitbox(x, y, xpos, ypos, &x1, &y1, &x2, &y2)
            && (clickreturned = gobj_click(y, x, xpos, ypos,
                                           0, 0, 1, 0)))
            break;
    }
#endif


}

    /* displace the selection by (dx, dy) pixels */
static void canvas_displaceselection(t_canvas *x, int dx, int dy)
{
    t_selection *y;
    int resortin = 0, resortout = 0;
    if (!canvas_undo_already_set_move)
    {
    	canvas_setundo(x, canvas_undo_move, canvas_undo_set_move(x, 1),
    	    "motion");
    	canvas_undo_already_set_move = 1;
    }
    for (y = x->gl_editor->e_selection; y; y = y->sel_next)
    {
    	t_class *cl = pd_class(&y->sel_what->g_pd);
    	gobj_displace(y->sel_what, x, dx, dy);
    	if (cl == vinlet_class) resortin = 1;
    	else if (cl == voutlet_class) resortout = 1;
    }
    if (resortin) canvas_resortinlets(x);
    if (resortout) canvas_resortoutlets(x);
    canvas_dirty(x, 1);
}

    /* this routine is called whenever a key is pressed or released.  "x"
    may be zero if there's no current canvas.  The first argument is true or
    fals for down/up; the second one is either a symbolic key name (e.g.,
    "Right" or an Ascii key number. */
void canvas_key(t_canvas *x, t_symbol *s, int ac, t_atom *av)
{
    static t_symbol *keynumsym, *keyupsym, *keynamesym;
    int keynum, fflag;
    t_symbol *gotkeysym;
    	
    int down, shift;

#ifdef ROCKBOX
    (void) s;
#endif

    if (ac < 3)
    	return;
    if (!x->gl_editor)
    {
    	bug("editor");
    	return;
    }
    canvas_undo_already_set_move = 0;
    down = (atom_getfloat(av) != 0);  /* nonzero if it's a key down */
    shift = (atom_getfloat(av+2) != 0);  /* nonzero if shift-ed */
    if (av[1].a_type == A_SYMBOL)
	gotkeysym = av[1].a_w.w_symbol;
    else if (av[1].a_type == A_FLOAT)
    {
	char buf[3];
#ifdef ROCKBOX
        snprintf(buf, sizeof(buf), "%c", (int) (av[1].a_w.w_float));
#else /* ROCKBOX */
	sprintf(buf, "%c", (int)(av[1].a_w.w_float));
#endif /* ROCKBOX */
	gotkeysym = gensym(buf);
    }
    else gotkeysym = gensym("?");
    fflag = (av[0].a_type == A_FLOAT ? av[0].a_w.w_float : 0);
    keynum = (av[1].a_type == A_FLOAT ? av[1].a_w.w_float : 0);
    if (keynum == '\\' || keynum == '{' || keynum == '}')
    {
    	post("%c: dropped", (int)keynum);
	return;
    }
    if (keynum == '\r') keynum = '\n';
    if (av[1].a_type == A_SYMBOL &&
    	!strcmp(av[1].a_w.w_symbol->s_name, "Return"))
    	    keynum = '\n';
    if (!keynumsym)
    {
    	keynumsym = gensym("#key");
    	keyupsym = gensym("#keyup");
    	keynamesym = gensym("#keyname");
    }
#ifdef MACOSX
	if (keynum == 30)
	    keynum = 0, gotkeysym = gensym("Up");
	else if (keynum == 31)
	    keynum = 0, gotkeysym = gensym("Down");
	else if (keynum == 28)
	    keynum = 0, gotkeysym = gensym("Left");
	else if (keynum == 29)
	    keynum = 0, gotkeysym = gensym("Right");
#endif
    if (keynumsym->s_thing && down)
    	pd_float(keynumsym->s_thing, (float)keynum);
    if (keyupsym->s_thing && !down)
    	pd_float(keyupsym->s_thing, (float)keynum);
    if (keynamesym->s_thing)
    {
    	t_atom at[2];
	at[0] = av[0];
	SETFLOAT(at, down);
	SETSYMBOL(at+1, gotkeysym);
	pd_list(keynamesym->s_thing, 0, 2, at);
    }
    if (!x->gl_editor)	/* if that 'invis'ed the window, we'd better stop. */
    	return;
    if (x && down)
    {
	    /* if an object has "grabbed" keys just send them on */
	if (x->gl_editor->e_grab
	    && x->gl_editor->e_keyfn && keynum)
    	    	(* x->gl_editor->e_keyfn)
		    (x->gl_editor->e_grab, (float)keynum);
	    /* if a text editor is open send it on */
	else if (x->gl_editor->e_textedfor)
	{
	    if (!x->gl_editor->e_textdirty)
	    {
	    	canvas_setundo(x, canvas_undo_cut,
		    canvas_undo_set_cut(x, UCUT_TEXT), "typing");
	    }
    	    rtext_key(x->gl_editor->e_textedfor,
	    	(int)keynum, gotkeysym);
    	    if (x->gl_editor->e_textdirty)
    	    	canvas_dirty(x, 1);
	}
	    /* check for backspace or clear */
	else if (keynum == 8 || keynum == 127)
	{
    	    if (x->gl_editor->e_selectedline)
    	    	canvas_clearline(x);
	    else if (x->gl_editor->e_selection)
	    {
	    	canvas_setundo(x, canvas_undo_cut,
		    canvas_undo_set_cut(x, UCUT_CLEAR), "clear");
	    	canvas_doclear(x);
	    }
	}
	    	/* check for arrow keys */
	else if (!strcmp(gotkeysym->s_name, "Up"))
	    canvas_displaceselection(x, 0, shift ? -10 : -1);
	else if (!strcmp(gotkeysym->s_name, "Down"))
	    canvas_displaceselection(x, 0, shift ? 10 : 1);
	else if (!strcmp(gotkeysym->s_name, "Left"))
	    canvas_displaceselection(x, shift ? -10 : -1, 0);
	else if (!strcmp(gotkeysym->s_name, "Right"))
	    canvas_displaceselection(x, shift ? 10 : 1, 0);
    }
}

void canvas_motion(t_canvas *x, t_floatarg xpos, t_floatarg ypos,
    t_floatarg fmod)
{ 
    /* post("motion %d %d", xpos, ypos); */
    int mod = fmod;
    if (!x->gl_editor)
    {
    	bug("editor");
    	return;
    }
    glist_setlastxy(x, xpos, ypos);
    if (x->gl_editor->e_onmotion == MA_MOVE)
    {
    	canvas_displaceselection(x, 
	    xpos - x->gl_editor->e_xwas, ypos - x->gl_editor->e_ywas);
    	x->gl_editor->e_xwas = xpos;
    	x->gl_editor->e_ywas = ypos;	
    }
    else if (x->gl_editor->e_onmotion == MA_REGION)
    	canvas_doregion(x, xpos, ypos, 0);
    else if (x->gl_editor->e_onmotion == MA_CONNECT)
    	canvas_doconnect(x, xpos, ypos, 0, 0);
    else if (x->gl_editor->e_onmotion == MA_PASSOUT)
    {
    	if (!x->gl_editor->e_motionfn)
	    bug("e_motionfn");
    	(*x->gl_editor->e_motionfn)(&x->gl_editor->e_grab->g_pd,
    	    xpos - x->gl_editor->e_xwas,
    	    ypos - x->gl_editor->e_ywas);
    	x->gl_editor->e_xwas = xpos;
    	x->gl_editor->e_ywas = ypos;
    }
    else if (x->gl_editor->e_onmotion == MA_DRAGTEXT)
    {
    	t_rtext *rt = x->gl_editor->e_textedfor;
	if (rt)
	    rtext_mouse(rt, xpos - x->gl_editor->e_xwas,
	    	ypos - x->gl_editor->e_ywas, RTEXT_DRAG);
    }
    else canvas_doclick(x, xpos, ypos, 0, mod, 0);
    
    x->gl_editor->e_lastmoved = 1;
}

void canvas_startmotion(t_canvas *x)
{
    int xval, yval;
    if (!x->gl_editor) return;
    glist_getnextxy(x, &xval, &yval);
    if (xval == 0 && yval == 0) return;
    x->gl_editor->e_onmotion = MA_MOVE;
    x->gl_editor->e_xwas = xval;
    x->gl_editor->e_ywas = yval; 
}

/* ----------------------------- window stuff ----------------------- */

void canvas_print(t_canvas *x, t_symbol *s)
{
#ifdef ROCKBOX
    (void) x;
    (void) s;
#else /* ROCKBOX */
    if (*s->s_name) sys_vgui(".x%x.c postscript -file %s\n", x, s->s_name);
    else sys_vgui(".x%x.c postscript -file x.ps\n", x);
#endif /* ROCKBOX */
}

void canvas_menuclose(t_canvas *x, t_floatarg force)
{
    if (x->gl_owner)
    	canvas_vis(x, 0);
    else if ((force != 0) || (!x->gl_dirty))
    	pd_free(&x->gl_pd);
#ifndef ROCKBOX
    else sys_vgui("pdtk_check {This window has been modified.  Close anyway?}\
     {.x%x menuclose 1;\n}\n", x);
#endif
}

    /* put up a dialog which may call canvas_font back to do the work */
static void canvas_menufont(t_canvas *x)
{
#ifdef ROCKBOX
    (void) x;
#else /* ROCKBOX */
    char buf[80];
    t_canvas *x2 = canvas_getrootfor(x);
    gfxstub_deleteforkey(x2);
    sprintf(buf, "pdtk_canvas_dofont %%s %d\n", x2->gl_font);
    gfxstub_new(&x2->gl_pd, &x2->gl_pd, buf);
#endif /* ROCKBOX */
}

static int canvas_find_index1, canvas_find_index2;
static t_binbuf *canvas_findbuf;
int binbuf_match(t_binbuf *inbuf, t_binbuf *searchbuf);

    /* find an atom or string of atoms */
static int canvas_dofind(t_canvas *x, int *myindex1p)
{
    t_gobj *y;
    int myindex1 = *myindex1p, myindex2;
    if (myindex1 >= canvas_find_index1)
    {
	for (y = x->gl_list, myindex2 = 0; y;
	    y = y->g_next, myindex2++)
	{
    	    t_object *ob = 0;
    	    if((ob = pd_checkobject(&y->g_pd)))
	    {
    		if (binbuf_match(ob->ob_binbuf, canvas_findbuf))
		{
	    	    if (myindex1 > canvas_find_index1 ||
			(myindex1 == canvas_find_index1 &&
		    	    myindex2 > canvas_find_index2))
		    {
			canvas_find_index1 = myindex1;
			canvas_find_index2 = myindex2;
			glist_noselect(x);
			canvas_vis(x, 1);
		    	canvas_editmode(x, 1.);
			glist_select(x, y);
			return (1);
		    }
		}
	    }
    	}
    }
    for (y = x->gl_list, myindex2 = 0; y; y = y->g_next, myindex2++)
    {
    	if (pd_class(&y->g_pd) == canvas_class)
	{
	    (*myindex1p)++;
    	    if (canvas_dofind((t_canvas *)y, myindex1p))
	    	return (1);
	}
    }
    return (0);
}

static void canvas_find(t_canvas *x, t_symbol *s, int ac, t_atom *av)
{
    int myindex1 = 0, i;
#ifdef ROCKBOX
    (void) s;
#endif
    for (i = 0; i < ac; i++)
    {
    	if (av[i].a_type == A_SYMBOL)
	{
	    if (!strcmp(av[i].a_w.w_symbol->s_name, "_semi_"))
	    	SETSEMI(&av[i]);
	    else if (!strcmp(av[i].a_w.w_symbol->s_name, "_comma_"))
	    	SETCOMMA(&av[i]);
    	}
    }
    if (!canvas_findbuf)
    	canvas_findbuf = binbuf_new();
    binbuf_clear(canvas_findbuf);
    binbuf_add(canvas_findbuf, ac, av);
    canvas_find_index1 = 0;
    canvas_find_index2 = -1;
    canvas_whichfind = x;
    if (!canvas_dofind(x, &myindex1))
    {
    	binbuf_print(canvas_findbuf);
	post("... couldn't find");
    }
}

static void canvas_find_again(t_canvas *x)
{
    int myindex1 = 0;
#ifdef ROCKBOX
    (void) x;
#endif
    if (!canvas_findbuf || !canvas_whichfind)
    	return;
    if (!canvas_dofind(canvas_whichfind, &myindex1))
    {
    	binbuf_print(canvas_findbuf);
	post("... couldn't find");
    }
}

static void canvas_find_parent(t_canvas *x)
{
    if (x->gl_owner)
    	canvas_vis(glist_getcanvas(x->gl_owner), 1);
}

static int glist_dofinderror(t_glist *gl, void *error_object)
{
    t_gobj *g;
    for (g = gl->gl_list; g; g = g->g_next)
    {
    	if ((void *)g == error_object)
	{
	    /* got it... now show it. */
	    glist_noselect(gl);
	    canvas_vis(glist_getcanvas(gl), 1);
	    canvas_editmode(glist_getcanvas(gl), 1.);
	    glist_select(gl, g);
	    return (1);
	}
    	else if (g->g_pd == canvas_class)
	{
    	    if (glist_dofinderror((t_canvas *)g, error_object))
	    	return (1);
	}
    }
    return (0);
}

void canvas_finderror(void *error_object)
{
    t_canvas *x;
    	/* find all root canvases */
    for (x = canvas_list; x; x = x->gl_next)
    {
	if (glist_dofinderror(x, error_object))
	    return;
    }
    post("... sorry, I couldn't find the source of that error.");
}

void canvas_stowconnections(t_canvas *x)
{
    t_gobj *selhead = 0, *seltail = 0, *nonhead = 0, *nontail = 0, *y, *y2;
    t_linetraverser t;
    t_outconnect *oc;
    if (!x->gl_editor) return;
    	/* split list to "selected" and "unselected" parts */ 
    for (y = x->gl_list; y; y = y2)
    {
    	y2 = y->g_next;
    	if (glist_isselected(x, y))
    	{
    	    if (seltail)
    	    {
    	    	seltail->g_next = y;
    	    	seltail = y;
    	    	y->g_next = 0;
    	    }
    	    else
    	    {
    	    	selhead = seltail = y;
    	    	seltail->g_next = 0;
    	    }
    	}
    	else
    	{
    	    if (nontail)
    	    {
    	    	nontail->g_next = y;
    	    	nontail = y;
    	    	y->g_next = 0;
    	    }
    	    else
    	    {
    	    	nonhead = nontail = y;
    	    	nontail->g_next = 0;
    	    }
    	}
    }
    	/* move the selected part to the end */
    if (!nonhead) x->gl_list = selhead;
    else x->gl_list = nonhead, nontail->g_next = selhead;

    	/* add connections to binbuf */
    binbuf_clear(x->gl_editor->e_connectbuf);
    linetraverser_start(&t, x);
    while((oc = linetraverser_next(&t)))
    {
    	int s1 = glist_isselected(x, &t.tr_ob->ob_g);
    	int s2 = glist_isselected(x, &t.tr_ob2->ob_g);
    	if (s1 != s2)
    	    binbuf_addv(x->gl_editor->e_connectbuf, "ssiiii;",
    	    	gensym("#X"), gensym("connect"),
    	    	    glist_getindex(x, &t.tr_ob->ob_g), t.tr_outno,
		    	glist_getindex(x, &t.tr_ob2->ob_g), t.tr_inno);
    }
}

void canvas_restoreconnections(t_canvas *x)
{
    pd_bind(&x->gl_pd, gensym("#X"));
    binbuf_eval(x->gl_editor->e_connectbuf, 0, 0, 0);
    pd_unbind(&x->gl_pd, gensym("#X"));
}

static t_binbuf *canvas_docopy(t_canvas *x)
{
    t_gobj *y;
    t_linetraverser t;
    t_outconnect *oc;
    t_binbuf *b = binbuf_new();
    for (y = x->gl_list; y; y = y->g_next)
    {
    	if (glist_isselected(x, y))
	    gobj_save(y, b);
    }
    linetraverser_start(&t, x);
    while((oc = linetraverser_next(&t)))
    {
    	if (glist_isselected(x, &t.tr_ob->ob_g)
    	    && glist_isselected(x, &t.tr_ob2->ob_g))
    	{
    	    binbuf_addv(b, "ssiiii;", gensym("#X"), gensym("connect"),
		glist_selectionindex(x, &t.tr_ob->ob_g, 1), t.tr_outno,
		glist_selectionindex(x, &t.tr_ob2->ob_g, 1), t.tr_inno);
    	}
    }
    return (b);
}

static void canvas_copy(t_canvas *x)
{
    if (!x->gl_editor || !x->gl_editor->e_selection)
    	return;
    binbuf_free(copy_binbuf);
    copy_binbuf = canvas_docopy(x);
}

static void canvas_clearline(t_canvas *x)
{
    if (x->gl_editor->e_selectedline)
    {
    	canvas_disconnect(x, x->gl_editor->e_selectline_index1,
	     x->gl_editor->e_selectline_outno,
	     x->gl_editor->e_selectline_index2,
	     x->gl_editor->e_selectline_inno);
	canvas_setundo(x, canvas_undo_disconnect,
	    canvas_undo_set_disconnect(x,
	    	x->gl_editor->e_selectline_index1,
		x->gl_editor->e_selectline_outno,
	    	x->gl_editor->e_selectline_index2,
		x->gl_editor->e_selectline_inno),
	    "disconnect");
    }
}

extern t_pd *newest;
static void canvas_doclear(t_canvas *x)
{
    t_gobj *y, *y2;
    int dspstate;

    dspstate = canvas_suspend_dsp();
    if (x->gl_editor->e_selectedline)
    {
    	canvas_disconnect(x, x->gl_editor->e_selectline_index1,
	     x->gl_editor->e_selectline_outno,
	     x->gl_editor->e_selectline_index2,
	     x->gl_editor->e_selectline_inno);
	canvas_setundo(x, canvas_undo_disconnect,
	    canvas_undo_set_disconnect(x,
	    	x->gl_editor->e_selectline_index1,
		x->gl_editor->e_selectline_outno,
	    	x->gl_editor->e_selectline_index2,
		x->gl_editor->e_selectline_inno),
	    "disconnect");
    }
    	/* if text is selected, deselecting it might remake the
    	object. So we deselect it and hunt for a "new" object on
    	the glist to reselect. */
    if (x->gl_editor->e_textedfor)
    {
    	newest = 0;
    	glist_noselect(x);
    	if (newest)
    	{
    	    for (y = x->gl_list; y; y = y->g_next)
    	    	if (&y->g_pd == newest) glist_select(x, y);
    	}
    }
    while (1)	/* this is pretty wierd...  should rewrite it */
    {
    	for (y = x->gl_list; y; y = y2)
    	{
    	    y2 = y->g_next;
    	    if (glist_isselected(x, y))
    	    {
    		glist_delete(x, y);
#if 0
    		if (y2) post("cut 5 %x %x", y2, y2->g_next);
    		else post("cut 6");
#endif
    		goto next;
    	    }
    	}
    	goto restore;
    next: ;
    }
restore:
    canvas_resume_dsp(dspstate);
    canvas_dirty(x, 1);
}

static void canvas_cut(t_canvas *x)
{
    if (x->gl_editor && x->gl_editor->e_selectedline)
    	canvas_clearline(x);
    else if (x->gl_editor && x->gl_editor->e_selection)
    {
	canvas_setundo(x, canvas_undo_cut,
	    canvas_undo_set_cut(x, UCUT_CUT), "cut");
	canvas_copy(x);
	canvas_doclear(x);
    }
}

static int paste_onset;
static t_canvas *paste_canvas;

static void glist_donewloadbangs(t_glist *x)
{
    if (x->gl_editor)
    {
    	t_selection *sel;
    	for (sel = x->gl_editor->e_selection; sel; sel = sel->sel_next)
    	    if (pd_class(&sel->sel_what->g_pd) == canvas_class)
    	    	canvas_loadbang((t_canvas *)(&sel->sel_what->g_pd));
    }
}

static void canvas_dopaste(t_canvas *x, t_binbuf *b)
{
#ifdef ROCKBOX
    t_gobj *g2;
#else /* ROCKBOX */
    t_gobj *newgobj, *last, *g2;
#endif /* ROCKBOX */
    int dspstate = canvas_suspend_dsp(), nbox, count;

    canvas_editmode(x, 1.);
    glist_noselect(x);
    for (g2 = x->gl_list, nbox = 0; g2; g2 = g2->g_next) nbox++;
    
    paste_onset = nbox;
    paste_canvas = x;
    
    pd_bind(&x->gl_pd, gensym("#X"));
    binbuf_eval(b, 0, 0, 0);
    pd_unbind(&x->gl_pd, gensym("#X"));
    for (g2 = x->gl_list, count = 0; g2; g2 = g2->g_next, count++)
    	if (count >= nbox)
    	    glist_select(x, g2);
    paste_canvas = 0;
    canvas_resume_dsp(dspstate);
    canvas_dirty(x, 1);
    glist_donewloadbangs(x);
}

static void canvas_paste(t_canvas *x)
{
    canvas_setundo(x, canvas_undo_paste, canvas_undo_set_paste(x), "paste");
    canvas_dopaste(x, copy_binbuf);
}

static void canvas_duplicate(t_canvas *x)
{
    if (x->gl_editor->e_onmotion == MA_NONE)
    {
    	t_selection *y;
    	canvas_copy(x);
    	canvas_setundo(x, canvas_undo_paste, canvas_undo_set_paste(x),
	    "duplicate");
     	canvas_dopaste(x, copy_binbuf);
    	for (y = x->gl_editor->e_selection; y; y = y->sel_next)
    	    gobj_displace(y->sel_what, x,
    	    	10, 10);
    	canvas_dirty(x, 1);
    }
}

static void canvas_selectall(t_canvas *x)
{
    t_gobj *y;
    if (!x->gl_edit)
    	canvas_editmode(x, 1);
    for (y = x->gl_list; y; y = y->g_next)
    {
    	if (!glist_isselected(x, y))
	    glist_select(x, y);
    }
}

void canvas_connect(t_canvas *x, t_floatarg fwhoout, t_floatarg foutno,
    t_floatarg fwhoin, t_floatarg finno)
{
    int whoout = fwhoout, outno = foutno, whoin = fwhoin, inno = finno;
    t_gobj *src = 0, *sink = 0;
    t_object *objsrc, *objsink;
    t_outconnect *oc;
    int nin = whoin, nout = whoout;
    if (paste_canvas == x) whoout += paste_onset, whoin += paste_onset;
    for (src = x->gl_list; whoout; src = src->g_next, whoout--)
    	if (!src->g_next) goto bad; /* bug fix thanks to Hannes */
    for (sink = x->gl_list; whoin; sink = sink->g_next, whoin--)
    	if (!sink->g_next) goto bad;
    if (!(objsrc = pd_checkobject(&src->g_pd)) ||
    	!(objsink = pd_checkobject(&sink->g_pd)))
    	    goto bad;
    if (!(oc = obj_connect(objsrc, outno, objsink, inno))) goto bad;
    if (glist_isvisible(x))
    {
#ifndef ROCKBOX
    	sys_vgui(".x%x.c create line %d %d %d %d -width %d -tags l%x\n",
	    glist_getcanvas(x), 0, 0, 0, 0,
	    (obj_issignaloutlet(objsrc, outno) ? 2 : 1),oc);
#endif
	canvas_fixlinesfor(x, objsrc);
    }
    return;

bad:
    post("%s %d %d %d %d (%s->%s) connection failed", 
    	x->gl_name->s_name, nout, outno, nin, inno,
	    (src? class_getname(pd_class(&src->g_pd)) : "???"),
	    (sink? class_getname(pd_class(&sink->g_pd)) : "???"));
}

#define XTOLERANCE 4
#define YTOLERANCE 3
#define NHIST 15

    /* LATER might have to speed this up */
static void canvas_tidy(t_canvas *x)
{
#ifdef ROCKBOX
    t_gobj *y, *y2;
#else /* ROCKBOX */
    t_gobj *y, *y2, *y3;
#endif /* ROCKBOX */
    int ax1, ay1, ax2, ay2, bx1, by1, bx2, by2;
    int histogram[NHIST], *ip, i, besthist, bestdist;
    	/* if nobody is selected, this means do it to all boxes;
	othewise just the selection */
    int all = (x->gl_editor ? (x->gl_editor->e_selection == 0) : 1);

    canvas_setundo(x, canvas_undo_move, canvas_undo_set_move(x, !all),
    	"motion");

    	/* tidy horizontally */
    for (y = x->gl_list; y; y = y->g_next)
    	if (all || glist_isselected(x, y))
    {
    	gobj_getrect(y, x, &ax1, &ay1, &ax2, &ay2);

    	for (y2 = x->gl_list; y2; y2 = y2->g_next)
    	    if (all || glist_isselected(x, y2))
    	{
    	    gobj_getrect(y2, x, &bx1, &by1, &bx2, &by2);
    	    if (by1 <= ay1 + YTOLERANCE && by1 >= ay1 - YTOLERANCE &&
    	    	bx1 < ax1)
    	    	    goto nothorizhead;
    	}

    	for (y2 = x->gl_list; y2; y2 = y2->g_next)
    	    if (all || glist_isselected(x, y2))
    	{
    	    gobj_getrect(y2, x, &bx1, &by1, &bx2, &by2);
    	    if (by1 <= ay1 + YTOLERANCE && by1 >= ay1 - YTOLERANCE
    	    	&& by1 != ay1)
    	    	    gobj_displace(y2, x, 0, ay1-by1);
    	}
    nothorizhead: ;
    }
    	/* tidy vertically.  First guess the user's favorite vertical spacing */
    for (i = NHIST, ip = histogram; i--; ip++) *ip = 0;
    for (y = x->gl_list; y; y = y->g_next)
    	if (all || glist_isselected(x, y))
    {
    	gobj_getrect(y, x, &ax1, &ay1, &ax2, &ay2);
    	for (y2 = x->gl_list; y2; y2 = y2->g_next)
    	    if (all || glist_isselected(x, y2))
    	{
    	    gobj_getrect(y2, x, &bx1, &by1, &bx2, &by2);
    	    if (bx1 <= ax1 + XTOLERANCE && bx1 >= ax1 - XTOLERANCE)
    	    {
    	    	int distance = by1-ay2;
    	    	if (distance >= 0 && distance < NHIST)
    	    	    histogram[distance]++;
    	    }
    	}
    }
    for (i = 1, besthist = 0, bestdist = 4, ip = histogram + 1;
    	i < (NHIST-1); i++, ip++)
    {
    	int hit = ip[-1] + 2 * ip[0] + ip[1];
    	if (hit > besthist)
    	{
    	    besthist = hit;
    	    bestdist = i;
    	}
    }
    post("best vertical distance %d", bestdist);
    for (y = x->gl_list; y; y = y->g_next)
    	if (all || glist_isselected(x, y))
    {
    	int keep = 1;
    	gobj_getrect(y, x, &ax1, &ay1, &ax2, &ay2);
    	for (y2 = x->gl_list; y2; y2 = y2->g_next)
    	    if (all || glist_isselected(x, y2))
    	{
    	    gobj_getrect(y2, x, &bx1, &by1, &bx2, &by2);
    	    if (bx1 <= ax1 + XTOLERANCE && bx1 >= ax1 - XTOLERANCE &&
    	    	ay1 >= by2 - 10 && ay1 < by2 + NHIST)
    	    	    goto nothead;
    	}
    	while (keep)
    	{
    	    keep = 0;
    	    for (y2 = x->gl_list; y2; y2 = y2->g_next)
    	    	if (all || glist_isselected(x, y2))
    	    {
    		gobj_getrect(y2, x, &bx1, &by1, &bx2, &by2);
    		if (bx1 <= ax1 + XTOLERANCE && bx1 >= ax1 - XTOLERANCE &&
    	    	    by1 > ay1 && by1 < ay2 + NHIST)
    		{
    		    int vmove = ay2 + bestdist - by1;
    	    	    gobj_displace(y2, x, ax1-bx1, vmove);
    	    	    ay1 = by1 + vmove;
    	    	    ay2 = by2 + vmove;
    	    	    keep = 1;
    	    	    break;
    		}
    	    }
    	}
    nothead: ;
    }
    canvas_dirty(x, 1);
}

static void canvas_texteditor(t_canvas *x)
{
    t_rtext *foo;
    char *buf;
    int bufsize;
    if((foo = x->gl_editor->e_textedfor))
    	rtext_gettext(foo, &buf, &bufsize);
    else buf = "", bufsize = 0;
#ifndef ROCKBOX
    sys_vgui("pdtk_pd_texteditor {%.*s}\n", bufsize, buf);
#endif
}

void glob_key(void *dummy, t_symbol *s, int ac, t_atom *av)
{
#ifdef ROCKBOX
    (void) dummy;
#endif
    	/* canvas_editing can be zero; canvas_key checks for that */
    canvas_key(canvas_editing, s, ac, av);
}

void canvas_editmode(t_canvas *x, t_floatarg fyesplease)
{
    int yesplease = fyesplease;
    if (yesplease && x->gl_edit)
    	return;
    x->gl_edit = !x->gl_edit;
    if (x->gl_edit && glist_isvisible(x) && glist_istoplevel(x))
    	canvas_setcursor(x, CURSOR_EDITMODE_NOTHING);
    else
    {
    	glist_noselect(x);
	if (glist_isvisible(x) && glist_istoplevel(x))
    	    canvas_setcursor(x, CURSOR_RUNMODE_NOTHING);
    }
#ifndef ROCKBOX
    sys_vgui("pdtk_canvas_editval .x%x %d\n",
    	glist_getcanvas(x), x->gl_edit);
#endif
}

    /* called by canvas_font below */
static void canvas_dofont(t_canvas *x, t_floatarg font, t_floatarg xresize,
    t_floatarg yresize)
{
    t_gobj *y;
    x->gl_font = font;
    if (xresize != 1 || yresize != 1)
    {
    	canvas_setundo(x, canvas_undo_move, canvas_undo_set_move(x, 0),
    	    "motion");
    	for (y = x->gl_list; y; y = y->g_next)
	{
	    int x1, x2, y1, y2, nx1, ny1;
    	    gobj_getrect(y, x, &x1, &y1, &x2, &y2);
	    nx1 = x1 * xresize + 0.5;
	    ny1 = y1 * yresize + 0.5;
	    gobj_displace(y, x, nx1-x1, ny1-y1);
	}
    }
    if (glist_isvisible(x))
    	glist_redraw(x);
    for (y = x->gl_list; y; y = y->g_next)
    	if (pd_class(&y->g_pd) == canvas_class
	    && !canvas_isabstraction((t_canvas *)y))
    	    	canvas_dofont((t_canvas *)y, font, xresize, yresize);
}

    /* canvas_menufont calls up a TK dialog which calls this back */
static void canvas_font(t_canvas *x, t_floatarg font, t_floatarg resize,
    t_floatarg whichresize)
{
    float realresize, realresx = 1, realresy = 1;
    t_canvas *x2 = canvas_getrootfor(x);
    if (!resize) realresize = 1;
    else
    {
    	if (resize < 20) resize = 20;
    	if (resize > 500) resize = 500;
	realresize = resize * 0.01;
    }
    if (whichresize != 3) realresx = realresize;
    if (whichresize != 2) realresy = realresize;
    canvas_dofont(x2, font, realresx, realresy);
#ifndef ROCKBOX
    sys_defaultfont = font;
#endif
}

static t_glist *canvas_last_glist;
static int canvas_last_glist_x, canvas_last_glist_y;

void glist_getnextxy(t_glist *gl, int *xpix, int *ypix)
{
    if (canvas_last_glist == gl)
    	*xpix = canvas_last_glist_x, *ypix = canvas_last_glist_y;
    else *xpix = *ypix = 40;
}

static void glist_setlastxy(t_glist *gl, int xval, int yval)
{
    canvas_last_glist = gl;
    canvas_last_glist_x = xval;
    canvas_last_glist_y = yval;
}


void g_editor_setup(void)
{
/* ------------------------ events ---------------------------------- */
    class_addmethod(canvas_class, (t_method)canvas_mousedown, gensym("mouse"),
    	A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_mouseup, gensym("mouseup"),
    	A_FLOAT, A_FLOAT, A_FLOAT, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_key, gensym("key"),
    	A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_motion, gensym("motion"),
    	A_FLOAT, A_FLOAT, A_FLOAT, A_NULL);

/* ------------------------ menu actions ---------------------------- */
    class_addmethod(canvas_class, (t_method)canvas_menuclose,
    	gensym("menuclose"), A_DEFFLOAT, 0);
    class_addmethod(canvas_class, (t_method)canvas_cut,
    	gensym("cut"), A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_copy,
    	gensym("copy"), A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_paste,
    	gensym("paste"), A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_duplicate,
    	gensym("duplicate"), A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_selectall,
    	gensym("selectall"), A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_undo,
    	gensym("undo"), A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_redo,
    	gensym("redo"), A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_tidy,
    	gensym("tidy"), A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_texteditor,
    	gensym("texteditor"), A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_editmode,
    	gensym("editmode"), A_DEFFLOAT, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_print,
    	gensym("print"), A_SYMBOL, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_menufont,
    	gensym("menufont"), A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_font,
    	gensym("font"), A_FLOAT, A_FLOAT, A_FLOAT, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_find,
    	gensym("find"), A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_find_again,
    	gensym("findagain"), A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_find_parent,
    	gensym("findparent"), A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_done_popup,
    	gensym("done-popup"), A_FLOAT, A_FLOAT, A_FLOAT, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_donecanvasdialog,
    	gensym("donecanvasdialog"), A_FLOAT, A_FLOAT, A_FLOAT, A_NULL);
    class_addmethod(canvas_class, (t_method)glist_arraydialog,
    	gensym("arraydialog"), A_SYMBOL, A_FLOAT, A_FLOAT, A_FLOAT, A_NULL);

/* -------------- connect method used in reading files ------------------ */
    class_addmethod(canvas_class, (t_method)canvas_connect,
    	gensym("connect"), A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_NULL);

    class_addmethod(canvas_class, (t_method)canvas_disconnect,
    	gensym("disconnect"), A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_NULL);
/* -------------- copy buffer ------------------ */
    copy_binbuf = binbuf_new();
}

