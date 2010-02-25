#include "../src/m_pd.h"
#include "g_canvas.h"

#ifdef NT
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#endif

/* ------------------------ image ----------------------------- */

static t_class *image_class;

typedef struct _image
{
     t_object x_obj;
     t_glist * x_glist;
     int x_width;
     int x_height;
     t_symbol*  x_fname;
} t_image;

/* widget helper functions */

void image_drawme(t_image *x, t_glist *glist, int firsttime)
{
#ifdef ROCKBOX
    (void) x;
    (void) glist;
    (void) firsttime;
#else /* ROCKBOX */
     if (firsttime) {
	  char fname[MAXPDSTRING];
	  canvas_makefilename(glist_getcanvas(x->x_glist), x->x_fname->s_name,
			 fname, MAXPDSTRING);

	  sys_vgui("image create photo img%x -file %s\n",x,fname);
	  sys_vgui(".x%x.c create image %d %d -image img%x -tags %xS\n", 
		   glist_getcanvas(glist),text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist),x,x);

	  /* TODO callback from gui
	    sys_vgui("image_size logo");
	  */
     }     
     else {
	  sys_vgui(".x%x.c coords %xS \
%d %d\n",
		   glist_getcanvas(glist), x,
		   text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist));
     }
#endif /* ROCKBOX */
}


void image_erase(t_image* x,t_glist* glist)
{
#ifdef ROCKBOX
    (void) x;
    (void) glist;
#else /* ROCKBOX */
     int n;
     sys_vgui(".x%x.c delete %xS\n",
	      glist_getcanvas(glist), x);
#endif /* ROCKBOX */
}
	


/* ------------------------ image widgetbehaviour----------------------------- */


static void image_getrect(t_gobj *z, t_glist *glist,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    int width, height;
    t_image* x = (t_image*)z;


    width = x->x_width;
    height = x->x_height;
    *xp1 = text_xpix(&x->x_obj, glist);
    *yp1 = text_ypix(&x->x_obj, glist);
    *xp2 = text_xpix(&x->x_obj, glist) + width;
    *yp2 = text_ypix(&x->x_obj, glist) + height;
}

static void image_displace(t_gobj *z, t_glist *glist,
    int dx, int dy)
{
    t_image *x = (t_image *)z;
    x->x_obj.te_xpix += dx;
    x->x_obj.te_ypix += dy;
#ifndef ROCKBOX
    sys_vgui(".x%x.c coords %xSEL %d %d %d %d\n",
		   glist_getcanvas(glist), x,
		   text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist),
		   text_xpix(&x->x_obj, glist) + x->x_width, text_ypix(&x->x_obj, glist) + x->x_height);
#endif
    image_drawme(x, glist, 0);
    canvas_fixlinesfor(glist_getcanvas(glist),(t_text*) x);
}

static void image_select(t_gobj *z, t_glist *glist, int state)
{
#ifdef ROCKBOX
    (void) z;
    (void) glist;
    (void) state;
#else /* ROCKBOX */
     t_image *x = (t_image *)z;
     if (state) {
	  sys_vgui(".x%x.c create rectangle \
%d %d %d %d -tags %xSEL -outline blue\n",
		   glist_getcanvas(glist),
		   text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist),
		   text_xpix(&x->x_obj, glist) + x->x_width, text_ypix(&x->x_obj, glist) + x->x_height,
		   x);
     }
     else {
	  sys_vgui(".x%x.c delete %xSEL\n",
		   glist_getcanvas(glist), x);
     }
#endif /* ROCKBOX */
}


static void image_activate(t_gobj *z, t_glist *glist, int state)
{
#ifdef ROCKBOX
    (void) z;
    (void) glist;
    (void) state;
#endif
/*    t_text *x = (t_text *)z;
    t_rtext *y = glist_findrtext(glist, x);
    if (z->g_pd != gatom_class) rtext_activate(y, state);*/
}

static void image_delete(t_gobj *z, t_glist *glist)
{
    t_text *x = (t_text *)z;
    canvas_deletelinesfor(glist_getcanvas(glist), x);
}

       
static void image_vis(t_gobj *z, t_glist *glist, int vis)
{
    t_image* s = (t_image*)z;
    if (vis)
	 image_drawme(s, glist, 1);
    else
	 image_erase(s,glist);
}

/* can we use the normal text save function ?? */

static void image_save(t_gobj *z, t_binbuf *b)
{
    t_image *x = (t_image *)z;
    binbuf_addv(b, "ssiiss", gensym("#X"),gensym("obj"),
		x->x_obj.te_xpix, x->x_obj.te_ypix,   
		gensym("image"),x->x_fname);
    binbuf_addv(b, ";");
}


t_widgetbehavior   image_widgetbehavior;

void image_size(t_image* x,t_floatarg w,t_floatarg h) {
     x->x_width = w;
     x->x_height = h;
}

void image_color(t_image* x,t_symbol* col)
{
#ifdef ROCKBOX
    (void) x;
    (void) col;
#endif
/*     outlet_bang(x->x_obj.ob_outlet); only bang if there was a bang .. 
       so color black does the same as bang, but doesn't forward the bang 
*/
}

static void image_setwidget(void)
{
    image_widgetbehavior.w_getrectfn =     image_getrect;
    image_widgetbehavior.w_displacefn =    image_displace;
    image_widgetbehavior.w_selectfn =   image_select;
    image_widgetbehavior.w_activatefn =   image_activate;
    image_widgetbehavior.w_deletefn =   image_delete;
    image_widgetbehavior.w_visfn =   image_vis;
#if defined(PD_VERSION_MINOR) && (PD_VERSION_MINOR > 31) 
    image_widgetbehavior.w_clickfn = NULL;
    image_widgetbehavior.w_propertiesfn = NULL; 
#endif
#if defined(PD_VERSION_MINOR) && PD_MINOR_VERSION < 37
    image_widgetbehavior.w_savefn =   image_save;
#endif
}


static void *image_new(t_symbol* fname)
{
    t_image *x = (t_image *)pd_new(image_class);

    x->x_glist = (t_glist*) canvas_getcurrent();

    x->x_width = 15;
    x->x_height = 15;

    x->x_fname = fname;
    outlet_new(&x->x_obj, &s_float);
    return (x);
}

void image_setup(void)
{
    image_class = class_new(gensym("image"), (t_newmethod)image_new, 0,
				sizeof(t_image),0, A_DEFSYM,0);

/*
    class_addmethod(image_class, (t_method)image_size, gensym("size"),
    	A_FLOAT, A_FLOAT, 0);

    class_addmethod(image_class, (t_method)image_color, gensym("color"),
    	A_SYMBOL, 0);
*/
/*
    class_addmethod(image_class, (t_method)image_open, gensym("open"),
    	A_SYMBOL, 0);
*/
    image_setwidget();
    class_setwidget(image_class,&image_widgetbehavior);
#if PD_MINOR_VERSION >= 37
    class_setsavefn(image_class,&image_save);
#endif
}


