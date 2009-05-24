/* ------------------------ fatom ----------------------------- */

#define x_val a_pos.a_w.w_float
#define DEBUG(x) 

#include <string.h>
#include <stdio.h>

typedef struct _fatom
{
     t_object x_obj;
     t_atom a_pos;  /* the value of the fatom */

     t_symbol* x_send;
     t_symbol* x_receive;
     t_glist * x_glist; /* value of the current canvas, intialized in _new */
     int x_rect_width; /* width of the widget */
     int x_rect_height; /* height of the widget */
     t_symbol*  x_sym; /* symbol for receiving callbacks from GUI */
     t_symbol*  x_type; /* type of fatom (vslider, hslider, checkbutton) */

     t_symbol*  x_text; /* associated widget text */
     int x_max; /* maximum value of a_pos (x_val) */
     int x_min; /* minimum value of a_pos (x_val) */
     int x_width; /* width of widget (e.g x_rect_height + 15 for hslider, x_rect_width + 15 for slider) */
     t_symbol* x_color;
     t_symbol* x_bgcolor;
} t_fatom;

/* widget helper functions */




static void draw_inlets(t_fatom *x, t_glist *glist, int firsttime, int nin, int nout)
{
     int n = nin;
     int nplus, i;
     nplus = (n == 1 ? 1 : n-1);
     DEBUG(post("draw inlet");)
     for (i = 0; i < n; i++)
     {
	  int onset = text_xpix(&x->x_obj, glist) + (x->x_rect_width - IOWIDTH) * i / nplus;
	  if (firsttime)
	       sys_vgui(".x%x.c create rectangle %d %d %d %d -tags %xo%d\n",
			glist_getcanvas(glist),
			onset, text_ypix(&x->x_obj, glist) + x->x_rect_height - 1,
			onset + IOWIDTH, text_ypix(&x->x_obj, glist) + x->x_rect_height,
			x, i);
	  else
	       sys_vgui(".x%x.c coords %xo%d %d %d %d %d\n",
			glist_getcanvas(glist), x, i,
			onset, text_ypix(&x->x_obj, glist) + x->x_rect_height - 1,
			onset + IOWIDTH, text_ypix(&x->x_obj, glist) + x->x_rect_height);
     }
     n = nout; 
     nplus = (n == 1 ? 1 : n-1);
     for (i = 0; i < n; i++)
     {
	  int onset = text_xpix(&x->x_obj, glist) + (x->x_rect_width - IOWIDTH) * i / nplus;
	  if (firsttime)
	       sys_vgui(".x%x.c create rectangle %d %d %d %d -tags %xi%d\n",
			glist_getcanvas(glist),
			onset, text_ypix(&x->x_obj, glist),
			     onset + IOWIDTH, text_ypix(&x->x_obj, glist) + 1,
			x, i);
	  else
	       sys_vgui(".x%x.c coords %xi%d %d %d %d %d\n",
			glist_getcanvas(glist), x, i,
			onset, text_ypix(&x->x_obj, glist),
			onset + IOWIDTH, text_ypix(&x->x_obj, glist) + 1);
	  
     }
     DEBUG(post("draw inlet end");)
}


static void draw_handle(t_fatom *x, t_glist *glist, int firsttime) {
  int onset = text_xpix(&x->x_obj, glist) + (x->x_rect_width - IOWIDTH+2);

  if (firsttime)
    sys_vgui(".x%x.c create rectangle %d %d %d %d -tags %xhandle\n",
	     glist_getcanvas(glist),
	     onset, text_ypix(&x->x_obj, glist) + x->x_rect_height - 12,
	     onset + IOWIDTH, text_ypix(&x->x_obj, glist) + x->x_rect_height-4,
	     x);
  else
    sys_vgui(".x%x.c coords %xhandle %d %d %d %d\n",
	     glist_getcanvas(glist), x, 
	     onset, text_ypix(&x->x_obj, glist) + x->x_rect_height - 12,
	     onset + IOWIDTH, text_ypix(&x->x_obj, glist) + x->x_rect_height-4);
}

static void create_widget(t_fatom *x, t_glist *glist)
{
  t_canvas *canvas=glist_getcanvas(glist);

  if (!strcmp(x->x_type->s_name,"vslider")) {
    x->x_rect_width = x->x_width+15;
    x->x_rect_height =  x->x_max-x->x_min+26;
    
    sys_vgui("scale .x%x.c.s%x \
                    -sliderlength 10 \
                    -showvalue 0 \
                    -length %d \
                    -resolution 0.01 \
                    -repeatinterval 20 \
                    -from %d -to %d \
                    -width %d \
                    -bg %s \
                    -activebackground %s \
                    -troughcolor %s \
                    -command fatom_cb%x\n",canvas,x,
	     x->x_max-x->x_min+14,
	     x->x_max,
	     x->x_min,
	     x->x_width,
             x->x_color->s_name,
             x->x_color->s_name,
             x->x_bgcolor->s_name,
	     x);
  } else  if (!strcmp(x->x_type->s_name,"hslider")) {
    x->x_rect_width =  x->x_max-x->x_min + 24;
    x->x_rect_height = x->x_width + 15;
    sys_vgui("scale .x%x.c.s%x \
                    -sliderlength 10 \
                    -showvalue 0 \
                    -length %d \
                    -resolution 0.01 \
                    -orient horizontal \
                    -repeatinterval 20 \
                    -from %d -to %d \
                    -width %d \
                    -bg %s \
                    -activebackground %s \
                    -troughcolor %s \
                    -command fatom_cb%x\n",canvas,x,
	     x->x_max-x->x_min+14,
	     x->x_min,
	     x->x_max,
	     x->x_width,
             x->x_color->s_name,
             x->x_color->s_name,
             x->x_bgcolor->s_name,
	     x);
  } else if (!strcmp(x->x_type->s_name,"checkbutton")) {
       x->x_rect_width = 32;
       x->x_rect_height = 28;
       sys_vgui("checkbutton .x%x.c.s%x \
                    -command { fatom_cb%x $fatom_val%x} -variable fatom_val%x -text \"%s\" \
		     -bg %s \
		     -activebackground %s \
		     \n",canvas,x,x,x,x,
		x->x_text->s_name,
		x->x_color->s_name,
		x->x_bgcolor->s_name);
  } else if (!strcmp(x->x_type->s_name,"hradio")) {
    int i;
    x->x_rect_width = 8*20;
       x->x_rect_height = 25;
       for (i=0;i<8;i++) {
	 sys_vgui("radiobutton .x%x.c.s%x%d \
                    -command { fatom_cb%x $fatom_val%x} -variable fatom_val%x -value %d\n",canvas,x,i,x,x,x,i);
       }
       /* TODO pack them */
     } else if (!strcmp(x->x_type->s_name,"vradio")) {
       int i;
       x->x_rect_width = 30;
       x->x_rect_height = 20*8+5;
       for (i=0;i<8;i++) {
	 sys_vgui("radiobutton .x%x.c.s%x%d \
                    -command { fatom_cb%x $fatom_val%x} -variable fatom_val%x -value %d\n",canvas,x,i,x,x,x,i);
       }
       /* TODO pack them */
     } else {
       x->x_rect_width = 32;
       x->x_rect_height = 140;
       sys_vgui("scale .x%x.c.s%x \
                    -sliderlength 10 \
                    -showvalue 0 \
                    -length 131 \
                    -from 127 -to 0 \
                    -command fatom_cb%x\n",canvas,x,x);
     }     

  /* set the start value */
     if (!strcmp(x->x_type->s_name,"checkbutton")) {
       if (x->x_val)
	 sys_vgui(".x%x.c.s%x select\n",canvas,x,x->x_val);
       else
	 sys_vgui(".x%x.c.s%x deselect\n",canvas,x,x->x_val);
     } else
       sys_vgui(".x%x.c.s%x set %f\n",canvas,x,x->x_val);

}





static void fatom_drawme(t_fatom *x, t_glist *glist, int firsttime)
{
  t_canvas *canvas=glist_getcanvas(glist);// x->x_glist;//glist_getcanvas(glist);
  DEBUG(post("drawme %d",firsttime);)
     if (firsttime) {
       DEBUG(post("glist %x canvas %x",x->x_glist,canvas));
       create_widget(x,glist);	       
       x->x_glist = canvas;
       sys_vgui(".x%x.c create window %d %d -anchor nw -window .x%x.c.s%x -tags %xS\n", 
		canvas,text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist)+2,x->x_glist,x,x);
              
     }     
     else {
       sys_vgui(".x%x.c coords %xS \
%d %d\n",
		canvas, x,
		text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist)+2);
     }
     draw_inlets(x, glist, firsttime, 1,1);
     //     draw_handle(x, glist, firsttime);

}


static void fatom_erase(t_fatom* x,t_glist* glist)
{
     int n;

     DEBUG(post("erase");)
       sys_vgui("destroy .x%x.c.s%x\n",glist_getcanvas(glist),x);

     sys_vgui(".x%x.c delete %xS\n",glist_getcanvas(glist), x);

     /* inlets and outlets */
     
     sys_vgui(".x%x.c delete %xi%d\n",glist_getcanvas(glist),x,0);
     sys_vgui(".x%x.c delete %xo%d\n",glist_getcanvas(glist),x,0);
     sys_vgui(".x%x.c delete  %xhandle\n",glist_getcanvas(glist),x,0);
}
	


/* ------------------------ fatom widgetbehaviour----------------------------- */


static void fatom_getrect(t_gobj *z, t_glist *owner,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    int width, height;
    t_fatom* s = (t_fatom*)z;

    width = s->x_rect_width;
    height = s->x_rect_height;
    *xp1 = text_xpix(&s->x_obj, owner);
    *yp1 = text_ypix(&s->x_obj, owner);
    *xp2 = text_xpix(&s->x_obj, owner) + width;
    *yp2 = text_ypix(&s->x_obj, owner) + height;
}

static void fatom_displace(t_gobj *z, t_glist *glist,
    int dx, int dy)
{
    t_fatom *x = (t_fatom *)z;
    DEBUG(post("displace");)
    x->x_obj.te_xpix += dx;
    x->x_obj.te_ypix += dy;
    if (glist_isvisible(glist))
    {
      sys_vgui(".x%x.c coords %xSEL %d %d %d %d\n",
	       glist_getcanvas(glist), x,
	       text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist),
	       text_xpix(&x->x_obj, glist) + x->x_rect_width, text_ypix(&x->x_obj, glist) + x->x_rect_height);
      
      fatom_drawme(x, glist, 0);
      canvas_fixlinesfor(glist_getcanvas(glist),(t_text*) x);
    }
    DEBUG(post("displace end");)
}

static void fatom_select(t_gobj *z, t_glist *glist, int state)
{
     t_fatom *x = (t_fatom *)z;
     if (state) {
	  sys_vgui(".x%x.c create rectangle \
%d %d %d %d -tags %xSEL -outline blue\n",
		   glist_getcanvas(glist),
		   text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist),
		   text_xpix(&x->x_obj, glist) + x->x_rect_width, text_ypix(&x->x_obj, glist) + x->x_rect_height,
		   x);
     }
     else {
	  sys_vgui(".x%x.c delete %xSEL\n",
		   glist_getcanvas(glist), x);
     }



}


static void fatom_activate(t_gobj *z, t_glist *glist, int state)
{
/*    t_text *x = (t_text *)z;
    t_rtext *y = glist_findrtext(glist, x);
    if (z->g_pd != gatom_class) rtext_activate(y, state);*/
}

static void fatom_delete(t_gobj *z, t_glist *glist)
{
    t_text *x = (t_text *)z;
    canvas_deletelinesfor(glist_getcanvas(glist), x);
}

       
static void fatom_vis(t_gobj *z, t_glist *glist, int vis)
{
    t_fatom* s = (t_fatom*)z;
    t_rtext *y;
    DEBUG(post("vis: %d",vis);)
    if (vis) {
#ifdef PD_MINOR_VERSION
      	y = (t_rtext *) rtext_new(glist, (t_text *)z);
#else
        y = (t_rtext *) rtext_new(glist, (t_text *)z,0,0);
#endif
	 fatom_drawme(s, glist, 1);
    }
    else {
	y = glist_findrtext(glist, (t_text *)z);
	 fatom_erase(s,glist);
	rtext_free(y);
    }
}

static void fatom_save(t_gobj *z, t_binbuf *b);

t_widgetbehavior   fatom_widgetbehavior;

 


static void fatom_size(t_fatom* x,t_floatarg w,t_floatarg h) {
     x->x_rect_width = w;
     x->x_rect_height = h;
}

static void fatom_color(t_fatom* x,t_symbol* col)
{

}


static void fatom_f(t_fatom* x,t_floatarg f) 
{
     x->x_val = f;
     if (x->x_send == &s_)
          outlet_float(x->x_obj.ob_outlet,f);
     else
          if (x->x_send->s_thing) pd_float(x->x_send->s_thing,f);
}


static void fatom_float(t_fatom* x,t_floatarg f) 
{
     if (glist_isvisible(x->x_glist)) {
          if (!strcmp(x->x_type->s_name,"checkbutton")) {
               if (x->x_val)
                    sys_vgui(".x%x.c.s%x select\n",x->x_glist,x,f);
               else
                    sys_vgui(".x%x.c.s%x deselect\n",x->x_glist,x,f);
          } else
               sys_vgui(".x%x.c.s%x set %f\n",x->x_glist,x,f);
     }
     fatom_f(x,f);
}


static void fatom_bang(t_fatom* x,t_floatarg f) 
{
     outlet_float(x->x_obj.ob_outlet,x->x_val);
}


static void fatom_properties(t_gobj *z, t_glist *owner)
{
  post("N/I");
}


static void fatom_save(t_gobj *z, t_binbuf *b)
{

    t_fatom *x = (t_fatom *)z;

    binbuf_addv(b, "ssiiss", gensym("#X"),gensym("obj"),
		x->x_obj.te_xpix, x->x_obj.te_ypix ,  
		gensym("fatom"),x->x_type);
    binbuf_addv(b, ";");
}


static void *fatom_new(t_fatom* x,int argc,t_atom* argv)
{
    char buf[256];
    int n = 0;
    x->x_glist = canvas_getcurrent();

    x->x_text = gensym("");
    x->x_max = 127; 
    x->x_min = 0;
    x->x_width = 15;
    x->x_color = gensym("grey");
    x->x_bgcolor = gensym("grey");
    x->x_send = &s_;

    while (argc) {
      if (argv->a_type == A_FLOAT) {
        if (n==0) x->x_max = atom_getfloat(argv);
        if (n==1) x->x_min = atom_getfloat(argv);
        if (n==2) x->x_width = atom_getfloat(argv);
      }

      if (argv->a_type == A_SYMBOL) {
        post("%d: symbol value %s",n,atom_getsymbol(argv)->s_name);
        if (n==3) x->x_send = atom_getsymbol(argv);
        if (n==4) x->x_color = atom_getsymbol(argv);
        if (n==5) x->x_bgcolor = atom_getsymbol(argv);
      }
      argv++;
      argc--;
      n++;
    }

    /* bind to a symbol for slider callback (later make this based on the
       filepath ??) */

    sprintf(buf,"fatom%x",(t_int)x);
    x->x_sym = gensym(buf);
    pd_bind(&x->x_obj.ob_pd, x->x_sym);

    /* pipe startup code to tk */

    sys_vgui("proc fatom_cb%x {v} {\n pd [concat fatom%x f $v \\;]\n }\n",x,x);

    outlet_new(&x->x_obj, &s_float);
    return (x);
}

static void fatom_setup_common(t_class* class)
{

  fatom_widgetbehavior.w_getrectfn =  fatom_getrect;
  fatom_widgetbehavior.w_displacefn = fatom_displace;
  fatom_widgetbehavior.w_selectfn =   fatom_select;
  fatom_widgetbehavior.w_activatefn = fatom_activate;
  fatom_widgetbehavior.w_deletefn =  fatom_delete;
  fatom_widgetbehavior.w_visfn =     fatom_vis;
#if PD_MINOR_VERSION < 37
  fatom_widgetbehavior.w_savefn =    fatom_save;
  fatom_widgetbehavior.w_propertiesfn = NULL;
#endif
  fatom_widgetbehavior.w_clickfn =   NULL;
  
  class_addfloat(class, (t_method)fatom_float);
  class_addbang(class, (t_method)fatom_bang);
  class_addmethod(class, (t_method)fatom_f, gensym("f"),
                  A_FLOAT, 0);

/*
    class_addmethod(class, (t_method)fatom_size, gensym("size"),
    	A_FLOAT, A_FLOAT, 0);

    class_addmethod(class, (t_method)fatom_color, gensym("color"),
    	A_SYMBOL, 0);
*/
/*
    class_addmethod(class, (t_method)fatom_open, gensym("open"),
    	A_SYMBOL, 0);
*/

    class_setwidget(class,&fatom_widgetbehavior);
#if PD_MINOR_VERSION >= 37
    class_setsavefn(class,&fatom_save);
#endif
}

