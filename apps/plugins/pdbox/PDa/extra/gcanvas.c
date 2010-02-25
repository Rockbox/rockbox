/* (C) Guenter Geiger <geiger@xdv.org> */


#include "../src/m_pd.h"
#include "g_canvas.h"

/* ------------------------ gcanvas ----------------------------- */


#define BACKGROUNDCOLOR "grey"

#define DEFAULTSIZE 80

static t_class *gcanvas_class;

typedef struct _gcanvas
{
     t_object x_obj;
     t_glist * x_glist;
     t_outlet* out2;
     t_outlet* out3;
     int x_width;
     int x_height;
     int x;
     int y;
     int x_xgrid;
     int x_ygrid;
} t_gcanvas;


static void rectangle(void* cv,void* o,char c,int x, int y,int w,int h,char* color) {
#ifdef ROCKBOX
    (void) cv;
    (void) o;
    (void) c;
    (void) x;
    (void) y;
    (void) w;
    (void) h;
    (void) color;
#else /* ROCKBOX */
     sys_vgui(".x%x.c create rectangle \
                 %d %d %d %d -tags %x%c -fill %s\n",cv,x,y,x+w,y+h,o,c,color);
#endif /* ROCKBOX */
}

static void move_object(void* cv,void* o,char c,int x, int y,int w,int h) {
#ifdef ROCKBOX
    (void) cv;
    (void) o;
    (void) c;
    (void) x;
    (void) y;
    (void) w;
    (void) h;
#else /* ROCKBOX */
	  sys_vgui(".x%x.c coords %x%c %d %d %d %d\n",
                   cv,o,c,x,y,x+w,y+h);
#endif /* ROCKBOX */
}

static void color_object(void* cv,void* o,char c,char* color) {
#ifdef ROCKBOX
    (void) cv;
    (void) o;
    (void) c;
    (void) color;
#else /* ROCKBOX */
     sys_vgui(".x%x.c itemconfigure %x%c -fill %s\n", cv, 
	     o, c,color);
#endif /* ROCKBOX */
}

static void delete_object(void* cv,void* o,char c) {
#ifdef ROCKBOX
    (void) cv;
    (void) o;
    (void) c;
#else /* ROCKBOX */
     sys_vgui(".x%x.c delete %x%c\n",
	      cv, o,c);
#endif /* ROCKBOX */
}

static void line(void* cv,void* o,char c,int x,int y,int w,int h,char* color) {
#ifdef ROCKBOX
    (void) cv;
    (void) o;
    (void) c;
    (void) x;
    (void) y;
    (void) w;
    (void) h;
    (void) color;
#else /* ROCKBOX */
     sys_vgui(".x%x.c create line \
                 %d %d %d %d -tags %x%c -fill %s\n",cv,x,y,x+w,y+h,o,c,color);
#endif /* ROCKBOX */
}


/* widget helper functions */

void gcanvas_drawme(t_gcanvas *x, t_glist *glist, int firsttime)
{
     int i;
     if (firsttime) {
	  rectangle(glist_getcanvas(glist),x,'a',
                    x->x_obj.te_xpix, x->x_obj.te_ypix,
                    x->x_width, x->x_height,BACKGROUNDCOLOR);
          for (i=1;i<x->x_xgrid;i++) 
               line(glist_getcanvas(glist),x,'b'+ i,
                    x->x_obj.te_xpix + x->x_width*i/x->x_xgrid, 
                    x->x_obj.te_ypix,
                    0, x->x_height,"red");                    
          for (i=1;i<x->x_ygrid;i++) 
               line(glist_getcanvas(glist),x,'B'+ i,
                    x->x_obj.te_xpix, 
                    x->x_obj.te_ypix + x->x_height*i/x->x_ygrid,
                    x->x_width, 0,"blue");                    
     }     
     else {
          move_object(
               glist_getcanvas(glist),x,'a',
               x->x_obj.te_xpix, x->x_obj.te_ypix,
               x->x_width, x->x_height);
          for (i=1;i<x->x_xgrid;i++) 
               move_object(glist_getcanvas(glist),x,'b'+ i,
                    x->x_obj.te_xpix + x->x_width*i/x->x_xgrid, 
                    x->x_obj.te_ypix,
                    0, x->x_height);                    
          for (i=1;i<x->x_ygrid;i++) 
               move_object(glist_getcanvas(glist),x,'B'+ i,
                    x->x_obj.te_xpix, 
                    x->x_obj.te_ypix + x->x_height*i/x->x_ygrid,
                    x->x_width, 0);                    
     }
     
     {
       /* outlets */
	  int n = 3;
	  int nplus, i;
	  nplus = (n == 1 ? 1 : n-1);
	  for (i = 0; i < n; i++)
	  {
#ifndef ROCKBOX
	       int onset = x->x_obj.te_xpix + (x->x_width - IOWIDTH) * i / nplus;
	       if (firsttime)
		    sys_vgui(".x%x.c create rectangle %d %d %d %d -tags %xo%d\n",
			     glist_getcanvas(glist),
			     onset, x->x_obj.te_ypix + x->x_height - 1,
			     onset + IOWIDTH, x->x_obj.te_ypix + x->x_height,
			     x, i);
	       else
		    sys_vgui(".x%x.c coords %xo%d %d %d %d %d\n",
			     glist_getcanvas(glist), x, i,
			     onset, x->x_obj.te_ypix + x->x_height - 1,
			     onset + IOWIDTH, x->x_obj.te_ypix + x->x_height);
#endif /* ROCKBOX */
	  }
	  /* inlets */
	  n = 0; 
	  nplus = (n == 1 ? 1 : n-1);
	  for (i = 0; i < n; i++)
	  {
#ifndef ROCKBOX
	       int onset = x->x_obj.te_xpix + (x->x_width - IOWIDTH) * i / nplus;
	       if (firsttime)
		    sys_vgui(".x%x.c create rectangle %d %d %d %d -tags %xi%d\n",
			     glist_getcanvas(glist),
			     onset, x->x_obj.te_ypix,
			     onset + IOWIDTH, x->x_obj.te_ypix + 1,
			     x, i);
	       else
		    sys_vgui(".x%x.c coords %xi%d %d %d %d %d\n",
			     glist_getcanvas(glist), x, i,
			     onset, x->x_obj.te_ypix,
			     onset + IOWIDTH, x->x_obj.te_ypix + 1);
#endif /* ROCKBOX */
	  }
     }

}




void gcanvas_erase(t_gcanvas* x,t_glist* glist)
{
     int n,i;
     delete_object(glist_getcanvas(glist),x,'a');
     for (i=1;i<x->x_xgrid;i++) 
          delete_object(glist_getcanvas(glist),x,'b'+ i);
     for (i=1;i<x->x_ygrid;i++) 
          delete_object(glist_getcanvas(glist),x,'B'+ i);

     n = 2;
     while (n--) {
#ifndef ROCKBOX
	  sys_vgui(".x%x.c delete %xo%d\n",glist_getcanvas(glist),x,n);
#endif
     }
}
	


/* ------------------------ gcanvas widgetbehaviour----------------------------- */


static void gcanvas_getrect(t_gobj *z, t_glist *owner,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
#ifdef ROCKBOX
    (void) owner;
#endif
    int width, height;
    t_gcanvas* s = (t_gcanvas*)z;


    width = s->x_width;
    height = s->x_height;
    *xp1 = s->x_obj.te_xpix;
    *yp1 = s->x_obj.te_ypix;
    *xp2 = s->x_obj.te_xpix + width;
    *yp2 = s->x_obj.te_ypix + height;
}

static void gcanvas_displace(t_gobj *z, t_glist *glist,
    int dx, int dy)
{
    t_gcanvas *x = (t_gcanvas *)z;
    x->x_obj.te_xpix += dx;
    x->x_obj.te_ypix += dy;
    gcanvas_drawme(x, glist, 0);
    canvas_fixlinesfor(glist_getcanvas(glist),(t_text*) x);
}

static void gcanvas_select(t_gobj *z, t_glist *glist, int state)
{
     t_gcanvas *x = (t_gcanvas *)z;
     color_object(glist,x,'a',state ? "blue" : BACKGROUNDCOLOR);
}


static void gcanvas_activate(t_gobj *z, t_glist *glist, int state)
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

static void gcanvas_delete(t_gobj *z, t_glist *glist)
{
    t_text *x = (t_text *)z;
    canvas_deletelinesfor(glist_getcanvas(glist), x);
}

       
static void gcanvas_vis(t_gobj *z, t_glist *glist, int vis)
{
    t_gcanvas* s = (t_gcanvas*)z;
    if (vis)
	 gcanvas_drawme(s, glist, 1);
    else
	 gcanvas_erase(s,glist);
}

/* can we use the normal text save function ?? */

static void gcanvas_save(t_gobj *z, t_binbuf *b)
{
    t_gcanvas *x = (t_gcanvas *)z;
    binbuf_addv(b, "ssiisiiii", gensym("#X"),gensym("obj"),
		(t_int)x->x_obj.te_xpix, (t_int)x->x_obj.te_ypix,  
		gensym("gcanvas"),x->x_width,x->x_height,
                x->x_xgrid,
                x->x_ygrid);
    binbuf_addv(b, ";");
}


t_widgetbehavior   gcanvas_widgetbehavior;

static void gcanvas_motion(t_gcanvas *x, t_floatarg dx, t_floatarg dy)
{
  x->x += dx;
  x->y += dy;
  outlet_float(x->out2,x->y);
  outlet_float(x->x_obj.ob_outlet,x->x);
}

void gcanvas_key(t_gcanvas *x, t_floatarg f)
{
#ifdef ROCKBOX
  (void) x;
  (void) f;
#endif
  post("key");
}


static void gcanvas_click(t_gcanvas *x,
    t_floatarg xpos, t_floatarg ypos, t_floatarg shift, t_floatarg ctrl,
    t_floatarg doit,int up)
{
#ifdef ROCKBOX
    (void) shift;
    (void) ctrl;
    (void) doit;
    (void) up;
#endif
    glist_grab(x->x_glist, &x->x_obj.te_g, (t_glistmotionfn) gcanvas_motion,
		(t_glistkeyfn) NULL, xpos, ypos);

    x->x = xpos - x->x_obj.te_xpix;
    x->y = ypos - x->x_obj.te_ypix;
    outlet_float(x->out2,x->y);
    outlet_float(x->x_obj.ob_outlet,x->x);
    outlet_float(x->out3,0);
}

static int gcanvas_newclick(t_gobj *z, struct _glist *glist,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
#ifdef ROCKBOX
    (void) glist;
#endif
    	if (doit)
	    gcanvas_click((t_gcanvas *)z, (t_floatarg)xpix, (t_floatarg)ypix,
	    	(t_floatarg)shift, 0, (t_floatarg)alt,dbl);

    if (dbl) outlet_float(((t_gcanvas*)z)->out3,1);
    return (1);
}

void gcanvas_size(t_gcanvas* x,t_floatarg w,t_floatarg h) {
     x->x_width = w;
     x->x_height = h;
     gcanvas_drawme(x, x->x_glist, 0);
}

static void gcanvas_setwidget(void)
{
    gcanvas_widgetbehavior.w_getrectfn =     gcanvas_getrect;
    gcanvas_widgetbehavior.w_displacefn =    gcanvas_displace;
    gcanvas_widgetbehavior.w_selectfn =   gcanvas_select;
    gcanvas_widgetbehavior.w_activatefn =   gcanvas_activate;
    gcanvas_widgetbehavior.w_deletefn =   gcanvas_delete;
    gcanvas_widgetbehavior.w_visfn =   gcanvas_vis;
    gcanvas_widgetbehavior.w_clickfn = gcanvas_newclick;
    class_setsavefn(gcanvas_class,gcanvas_save);
}


static void *gcanvas_new(t_symbol* s,t_int ac,t_atom* at)
{
#ifdef ROCKBOX
    (void) s;
#endif
    t_gcanvas *x = (t_gcanvas *)pd_new(gcanvas_class);

    x->x_glist = (t_glist*) canvas_getcurrent();


    /* Fetch the width */

    x->x_width = DEFAULTSIZE;
    if (ac-- > 0) {
         if (at->a_type != A_FLOAT)
              error("gcanvas: wrong argument type");
         else
              x->x_width = atom_getfloat(at++);
         
         if (x->x_width < 0 || x->x_width > 2000) {
              error("gcanvas: unallowed width %f",x->x_width);
              x->x_width = DEFAULTSIZE;
         }
    }

    /* Fetch the height */

    x->x_height = DEFAULTSIZE;
    if (ac-- > 0) {
         if (at->a_type != A_FLOAT)
              error("gcanvas: wrong argument type");
         else 
              x->x_height = atom_getfloat(at++);
         
         if (x->x_height < 0 || x->x_height > 2000) {
              error("gcanvas: unallowed height %f",x->x_height);
              x->x_width = DEFAULTSIZE;
         }
    }

    /* Fetch the xgrid */

    x->x_xgrid = 0;
    if (ac-- > 0) {
         if (at->a_type != A_FLOAT)
              error("gcanvas: wrong argument type");
         else
              x->x_xgrid = atom_getfloat(at++);
         
         if (x->x_xgrid < 0 || x->x_xgrid > x->x_width/2) {
              error("gcanvas: unallowed xgrid %f",x->x_xgrid);
              x->x_xgrid = 0;
         }
    }

    /* Fetch the ygrid */

    x->x_ygrid = 0;
    if (ac-- > 0) {
         if (at->a_type != A_FLOAT)
              error("gcanvas: wrong argument type");
         else
              x->x_ygrid = atom_getfloat(at++);
         
         if (x->x_ygrid < 0 || x->x_ygrid > x->x_height/2) {
              error("gcanvas: unallowed xgrid %f",x->x_ygrid);
              x->x_ygrid = 0;
         }
    }

    outlet_new(&x->x_obj, &s_float);
    x->out2 = outlet_new(&x->x_obj, &s_float);
    x->out3 = outlet_new(&x->x_obj, &s_float);
    return (x);
}



void gcanvas_setup(void)
{
    gcanvas_class = class_new(gensym("gcanvas"), (t_newmethod)gcanvas_new, 0,
				sizeof(t_gcanvas),0, A_GIMME,0);

    class_addmethod(gcanvas_class, (t_method)gcanvas_click, gensym("click"),
    	A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(gcanvas_class, (t_method)gcanvas_size, gensym("size"),
    	A_FLOAT, A_FLOAT, 0);

    gcanvas_setwidget();
    class_setwidget(gcanvas_class,&gcanvas_widgetbehavior);
}


