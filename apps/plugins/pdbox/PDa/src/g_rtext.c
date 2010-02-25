/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* changes by Thomas Musil IEM KUG Graz Austria 2001 */
/* have to insert gui-objects into editor-list */
/* all changes are labeled with      iemlib      */

#ifdef ROCKBOX
#include "plugin.h"
#include "ctype.h"
#include "../../pdbox.h"
#include "m_pd.h"
#include "g_canvas.h"
#define snprintf rb->snprintf
#else /* ROCKBOX */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "m_pd.h"
#include "s_stuff.h"
#include "g_canvas.h"
#include "t_tk.h"
#endif /* ROCKBOX */

#define LMARGIN 1
#define RMARGIN 1
#define TMARGIN 2
#define BMARGIN 2

#define SEND_FIRST 1
#define SEND_UPDATE 2
#define SEND_CHECK 0

struct _rtext
{
    char *x_buf;
    int x_bufsize;
    int x_selstart;
    int x_selend;
    int x_active;
    int x_dragfrom;
    int x_height;
    int x_drawnwidth;
    int x_drawnheight;
    t_text *x_text;
    t_glist *x_glist;
    char x_tag[50];
    struct _rtext *x_next;
};

t_rtext *rtext_new(t_glist *glist, t_text *who)
{
    t_rtext *x = (t_rtext *)getbytes(sizeof *x);
#ifndef ROCKBOX
    int w = 0, h = 0, indx;
#endif
    x->x_height = -1;
    x->x_text = who;
    x->x_glist = glist;
    x->x_next = glist->gl_editor->e_rtext;
    x->x_selstart = x->x_selend = x->x_active =
    	x->x_drawnwidth = x->x_drawnheight = 0;
    binbuf_gettext(who->te_binbuf, &x->x_buf, &x->x_bufsize);
    glist->gl_editor->e_rtext = x;
#ifdef ROCKBOX
    snprintf(x->x_tag, strlen(x->x_tag),
        ".x%lx.t%lx", (unsigned long) (t_int) glist_getcanvas(x->x_glist),
                      (unsigned long) (t_int) x);
#else /* ROCKBOX */
    sprintf(x->x_tag, ".x%x.t%x", (t_int)glist_getcanvas(x->x_glist),
    	(t_int)x);
#endif /* ROCKBOX */
    return (x);
}

static t_rtext *rtext_entered;

void rtext_free(t_rtext *x)
{
    if (x->x_glist->gl_editor->e_textedfor == x)
    	x->x_glist->gl_editor->e_textedfor = 0;
    if (x->x_glist->gl_editor->e_rtext == x)
    	x->x_glist->gl_editor->e_rtext = x->x_next;
    else
    {
    	t_rtext *e2;
    	for (e2 = x->x_glist->gl_editor->e_rtext; e2; e2 = e2->x_next)
    	    if (e2->x_next == x)
    	{
    	    e2->x_next = x->x_next;
    	    break;
    	}
    }
    if (rtext_entered == x) rtext_entered = 0;
    freebytes(x->x_buf, x->x_bufsize);
    freebytes(x, sizeof *x);
}

char *rtext_gettag(t_rtext *x)
{
    return (x->x_tag);
}

void rtext_gettext(t_rtext *x, char **buf, int *bufsize)
{
    *buf = x->x_buf;
    *bufsize = x->x_bufsize;
}


/* LATER deal with tcl-significant characters */

static int firstone(char *s, int c, int n)
{
    char *s2 = s + n;
    int i = 0;
    while (s != s2)
    {
    	if (*s == c) return (i);
    	i++;
    	s++;
    }
    return (-1);
}

static int lastone(char *s, int c, int n)
{
    char *s2 = s + n;
    while (s2 != s)
    {
    	s2--;
    	n--;
    	if (*s2 == c) return (n);
    }
    return (-1);
}

    /* the following routine computes line breaks and carries out
    some action which could be:
    	SEND_FIRST - draw the box  for the first time
	SEND_UPDATE - redraw the updated box
	otherwise - don't draw, just calculate.
    Called with *widthp and *heightpas coordinates of
    a test point, the routine reports the index of the character found
    there in *indexp.  *widthp and *heightp are set to the width and height
    of the entire text in pixels.
    */

    /* LATER get this and sys_vgui to work together properly,
    	breaking up messages as needed.  As of now, there's
    	a limit of 1950 characters, imposed by sys_vgui(). */
#define UPBUFSIZE 4000
#define BOXWIDTH 60

/* Older (pre-8.3.4) TCL versions handle text selection differently; this
flag is set from the GUI if this happens.  LATER take this out: early 2006? */

extern int sys_oldtclversion;	    	

static void rtext_senditup(t_rtext *x, int action, int *widthp, int *heightp,
    int *indexp)
{
    float dispx, dispy;
    char tempbuf[UPBUFSIZE], *tp = tempbuf, *bp = x->x_buf;
    int outchars, inchars = x->x_bufsize, nlines = 0, ncolumns = 0,
    	pixwide, pixhigh;
#ifdef ROCKBOX
    int fontwidth = 8, fontheight = 10;
#else
    int font = glist_getfont(x->x_glist);
    int fontwidth = sys_fontwidth(font), fontheight = sys_fontheight(font);
#endif
    int findx = (*widthp + (fontwidth/2)) / fontwidth,
    	findy = *heightp / fontheight;
    int reportedindex = 0;
#ifndef ROCKBOX
    t_canvas *canvas = glist_getcanvas(x->x_glist);
#endif
    int widthspec = x->x_text->te_width;
    int widthlimit = (widthspec ? widthspec : BOXWIDTH);
    while (inchars)
    {
    	int maxindex = (inchars > widthlimit ? widthlimit : inchars);
    	int eatchar = 1;
    	int foundit = firstone(bp, '\n', maxindex);
    	if (foundit < 0)
    	{
    	    if (inchars > widthlimit)
    	    {
    		foundit = lastone(bp, ' ', maxindex);
    		if (foundit < 0)
    		{
    	    	    foundit = maxindex;
    	    	    eatchar = 0;
    		}
   	    }
   	    else
   	    {
   	    	foundit = inchars;
   	    	eatchar = 0;
   	    }
   	}
	if (nlines == findy)
	{
	    int actualx = (findx < 0 ? 0 :
	    	(findx > foundit ? foundit : findx));
	    *indexp = (bp - x->x_buf) + actualx;
	    reportedindex = 1;
	}
   	strncpy(tp, bp, foundit);
    	tp += foundit;
    	bp += (foundit + eatchar);
    	inchars -= (foundit + eatchar);
    	if (inchars) *tp++ = '\n';
    	if (foundit > ncolumns) ncolumns = foundit;
    	nlines++;
    }
    outchars = tp - tempbuf;
    if (outchars > 1950) outchars = 1950;
    if (!reportedindex)
    	*indexp = outchars;
    dispx = text_xpix(x->x_text, x->x_glist);
    dispy = text_ypix(x->x_text, x->x_glist);
    if (nlines < 1) nlines = 1;
    if (!widthspec)
    {
    	while (ncolumns < 3)
    	{
    	    tempbuf[outchars++] = ' ';
    	    ncolumns++;
    	}
    }
    else ncolumns = widthspec;
    pixwide = ncolumns * fontwidth + (LMARGIN + RMARGIN);
    pixhigh = nlines * fontheight + (TMARGIN + BMARGIN);

    if (action == SEND_FIRST)
#ifdef ROCKBOX
        ;
#else /* ROCKBOX */
    	sys_vgui("pdtk_text_new .x%x.c %s %f %f {%.*s} %d %s\n",
	    canvas, x->x_tag,
	    dispx + LMARGIN, dispy + TMARGIN,
	    outchars, tempbuf, sys_hostfontsize(font),
	    (glist_isselected(x->x_glist,
	    	&x->x_glist->gl_gobj)? "blue" : "black"));
#endif /* ROCKBOX */
    else if (action == SEND_UPDATE)
    {
#ifndef ROCKBOX
    	sys_vgui("pdtk_text_set .x%x.c %s {%.*s}\n",
    	    canvas, x->x_tag, outchars, tempbuf);
#endif
    	if (pixwide != x->x_drawnwidth || pixhigh != x->x_drawnheight) 
	    text_drawborder(x->x_text, x->x_glist, x->x_tag,
	    	pixwide, pixhigh, 0);
    	if (x->x_active)
	{
    	    if (x->x_selend > x->x_selstart)
	    {
#ifndef ROCKBOX
    		sys_vgui(".x%x.c select from %s %d\n", canvas, 
    	    	    x->x_tag, x->x_selstart);
    		sys_vgui(".x%x.c select to %s %d\n", canvas, 
    	    	    x->x_tag, x->x_selend + (sys_oldtclversion ? 0 : -1));
	    	sys_vgui(".x%x.c focus \"\"\n", canvas);	
#endif
    	    }
	    else
	    {
#ifndef ROCKBOX
    		sys_vgui(".x%x.c select clear\n", canvas);
	    	sys_vgui(".x%x.c icursor %s %d\n", canvas, x->x_tag,
		    x->x_selstart);
	    	sys_vgui(".x%x.c focus %s\n", canvas, x->x_tag);	
#endif
	    }
	}
    }
    x->x_drawnwidth = pixwide;
    x->x_drawnheight = pixhigh;
    
    *widthp = pixwide;
    *heightp = pixhigh;
}

void rtext_retext(t_rtext *x)
{
    int w = 0, h = 0, indx;
    t_text *text = x->x_text;
    t_freebytes(x->x_buf, x->x_bufsize);
    binbuf_gettext(text->te_binbuf, &x->x_buf, &x->x_bufsize);
    	/* special case: for number boxes, try to pare the number down
	to the specified width of the box. */
    if (text->te_width > 0 && text->te_type == T_ATOM &&
    	x->x_bufsize > text->te_width)
    {
    	t_atom *atomp = binbuf_getvec(text->te_binbuf);
	int natom = binbuf_getnatom(text->te_binbuf);
	int bufsize = x->x_bufsize;
	if (natom == 1 && atomp->a_type == A_FLOAT)
	{
	    	/* try to reduce size by dropping decimal digits */
	    int wantreduce = bufsize - text->te_width;
	    char *decimal = 0, *nextchar, *ebuf = x->x_buf + bufsize,
		*s1, *s2;
#ifndef ROCKBOX
	    int ndecimals;
#endif
	    for (decimal = x->x_buf; decimal < ebuf; decimal++)
	    	if (*decimal == '.')
		    break;
	    if (decimal >= ebuf)
	    	goto giveup;
	    for (nextchar = decimal + 1; nextchar < ebuf; nextchar++)
	    	if (*nextchar < '0' || *nextchar > '9')
		    break;
	    if (nextchar - decimal - 1 < wantreduce)
	    	goto giveup;
	    for (s1 = nextchar - wantreduce, s2 = s1 + wantreduce;
	    	s2 < ebuf; s1++, s2++)
		    *s1 = *s2;
	    t_resizebytes(x->x_buf, bufsize, text->te_width);
	    bufsize = text->te_width;
	    goto done;
	giveup:
		/* give up and bash it to "+" or "-" */
	    x->x_buf[0] = (atomp->a_w.w_float < 0 ? '-' : '+');
	    t_resizebytes(x->x_buf, bufsize, 1);
	    bufsize = 1;
	}
	else if (bufsize > text->te_width)
	{
	    x->x_buf[text->te_width - 1] = '>';
	    t_resizebytes(x->x_buf, bufsize, text->te_width);
	    bufsize = text->te_width;
	}
    done:
	x->x_bufsize = bufsize;
    }
    rtext_senditup(x, SEND_UPDATE, &w, &h, &indx);
}

/* find the rtext that goes with a text item */
t_rtext *glist_findrtext(t_glist *gl, t_text *who)
{
    t_rtext *x = gl->gl_editor->e_rtext;
    while (x && x->x_text != who) x = x->x_next;
    if (!x) bug("glist_findrtext");
    return (x);
}

int rtext_width(t_rtext *x)
{
    int w = 0, h = 0, indx;
    rtext_senditup(x, SEND_CHECK, &w, &h, &indx);
    return (w);
}

int rtext_height(t_rtext *x)
{
    int w = 0, h = 0, indx;
    rtext_senditup(x, SEND_CHECK, &w, &h, &indx);
    return (h);
}

void rtext_draw(t_rtext *x)
{
    int w = 0, h = 0, indx;
    rtext_senditup(x, SEND_FIRST, &w, &h, &indx);
}

void rtext_erase(t_rtext *x)
{
#ifdef ROCKBOX
    (void) x;
#else
    sys_vgui(".x%x.c delete %s\n", glist_getcanvas(x->x_glist), x->x_tag);
#endif
}

void rtext_displace(t_rtext *x, int dx, int dy)
{
#ifdef ROCKBOX
    (void) x;
    (void) dx;
    (void) dy;
#else /* ROCKBOX */
    sys_vgui(".x%x.c move %s %d %d\n", glist_getcanvas(x->x_glist), 
    	x->x_tag, dx, dy);
#endif /* ROCKBOX */
}

void rtext_select(t_rtext *x, int state)
{
    t_glist *glist = x->x_glist;
    t_canvas *canvas = glist_getcanvas(glist);
#ifdef ROCKBOX
    (void) state;
#else /* ROCKBOX */
    sys_vgui(".x%x.c itemconfigure %s -fill %s\n", canvas, 
    	x->x_tag, (state? "blue" : "black"));
#endif /* ROCKBOX */
    canvas_editing = canvas;
}

void rtext_activate(t_rtext *x, int state)
{
    int w = 0, h = 0, indx;
    t_glist *glist = x->x_glist;
#ifndef ROCKBOX
    t_canvas *canvas = glist_getcanvas(glist);
#endif
    if (state)
    {
#ifndef ROCKBOX
	sys_vgui(".x%x.c focus %s\n", canvas, x->x_tag);
#endif
    	glist->gl_editor->e_textedfor = x;
    	glist->gl_editor->e_textdirty = 0;
	x->x_dragfrom = x->x_selstart = 0;
	x->x_selend = x->x_bufsize;
	x->x_active = 1;
    }
    else
    {
#ifndef ROCKBOX
    	sys_vgui("selection clear .x%x.c\n", canvas);
	sys_vgui(".x%x.c focus \"\"\n", canvas);
#endif
    	if (glist->gl_editor->e_textedfor == x)
    	    glist->gl_editor->e_textedfor = 0;
	x->x_active = 0;
    }
    rtext_senditup(x, SEND_UPDATE, &w, &h, &indx);
}

void rtext_key(t_rtext *x, int keynum, t_symbol *keysym)
{
    int w = 0, h = 0, indx, i, newsize, ndel;
#ifndef ROCKBOX
    char *s1, *s2;
#endif
    if (keynum)
    {
    	int n = keynum;
	if (n == '\r') n = '\n';
	if (n == '\b')	/* backspace */
	{
	    	    /* LATER delete the box if all text is selected...
		    this causes reentrancy problems now. */
    	    /* if ((!x->x_selstart) && (x->x_selend == x->x_bufsize))
    	    {
    		....
    	    } */
    	    if (x->x_selstart && (x->x_selstart == x->x_selend))
		x->x_selstart--;
	}
	else if (n == 127)	/* delete */
    	{
	    if (x->x_selend < x->x_bufsize && (x->x_selstart == x->x_selend))
		x->x_selend++;
	}
	
	ndel = x->x_selend - x->x_selstart;
	for (i = x->x_selend; i < x->x_bufsize; i++)
    	    x->x_buf[i- ndel] = x->x_buf[i];
	newsize = x->x_bufsize - ndel;
	x->x_buf = resizebytes(x->x_buf, x->x_bufsize, newsize);
	x->x_bufsize = newsize;

	if (n == '\n' || isprint(n))
	{
    	    newsize = x->x_bufsize+1;
    	    x->x_buf = resizebytes(x->x_buf, x->x_bufsize, newsize);
	    for (i = x->x_bufsize; i > x->x_selstart; i--)
		x->x_buf[i] = x->x_buf[i-1];
    	    x->x_buf[x->x_selstart] = n;
    	    x->x_bufsize = newsize;
	    x->x_selstart = x->x_selstart + 1;
	}
	x->x_selend = x->x_selstart;
	x->x_glist->gl_editor->e_textdirty = 1;
    }
    else if (!strcmp(keysym->s_name, "Right"))
    {
    	if (x->x_selend == x->x_selstart && x->x_selstart < x->x_bufsize)
	    x->x_selend = x->x_selstart = x->x_selstart + 1;
	else
	    x->x_selstart = x->x_selend;
    }
    else if (!strcmp(keysym->s_name, "Left"))
    {
    	if (x->x_selend == x->x_selstart && x->x_selstart > 0)
	    x->x_selend = x->x_selstart = x->x_selstart - 1;
	else
	    x->x_selend = x->x_selstart;
    }
    	/* this should be improved...  life's too short */
    else if (!strcmp(keysym->s_name, "Up"))
    {
	if (x->x_selstart)
	    x->x_selstart--;
    	while (x->x_selstart > 0 && x->x_buf[x->x_selstart] != '\n')
	    x->x_selstart--;
	x->x_selend = x->x_selstart;
    }
    else if (!strcmp(keysym->s_name, "Down"))
    {
    	while (x->x_selend < x->x_bufsize &&
	    x->x_buf[x->x_selend] != '\n')
	    x->x_selend++;
	if (x->x_selend < x->x_bufsize)
	    x->x_selend++;
	x->x_selstart = x->x_selend;
    }
    rtext_senditup(x, SEND_UPDATE, &w, &h, &indx);
}

void rtext_mouse(t_rtext *x, int xval, int yval, int flag)
{
    int w = xval, h = yval, indx;
    rtext_senditup(x, SEND_CHECK, &w, &h, &indx);
    if (flag == RTEXT_DOWN)
    {
    	x->x_dragfrom = x->x_selstart = x->x_selend = indx;
    }
    else if (flag == RTEXT_SHIFT)
    {
    	if (indx * 2 > x->x_selstart + x->x_selend)
	    x->x_dragfrom = x->x_selstart, x->x_selend = indx;
    	else
	    x->x_dragfrom = x->x_selend, x->x_selstart = indx;
    }
    else if (flag == RTEXT_DRAG)
    {
    	x->x_selstart = (x->x_dragfrom < indx ? x->x_dragfrom : indx);
    	x->x_selend = (x->x_dragfrom > indx ? x->x_dragfrom : indx);
    }
    rtext_senditup(x, SEND_UPDATE, &w, &h, &indx);
}

