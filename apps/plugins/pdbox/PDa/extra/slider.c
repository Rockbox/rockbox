#include <stdio.h>
#include "../src/m_pd.h"
#include "g_canvas.h" /* for widgetbehaviour */
#include "fatom.h"

static t_class *slider_class;

static void slider_save(t_gobj *z, t_binbuf *b)
{
    t_fatom *x = (t_fatom *)z;

    binbuf_addv(b, "ssiisiiisss", gensym("#X"),gensym("obj"),
		x->x_obj.te_xpix, x->x_obj.te_ypix ,  
		gensym("slider"),x->x_max,x->x_min,x->x_width,x->x_send,x->x_color,x->x_bgcolor);
    binbuf_addv(b, ";");
}


static void *slider_new(t_symbol* s,t_int argc, t_atom* argv)
{
    t_fatom *x = (t_fatom *)pd_new(slider_class);
    x->x_type = gensym("vslider");
    return fatom_new(x,argc,argv);
}


t_widgetbehavior   slider_widgetbehavior;
 

void slider_setup(void) {
    slider_class = class_new(gensym("slider"), (t_newmethod)slider_new, 0,
				sizeof(t_fatom),0,A_GIMME,0);

    slider_widgetbehavior.w_getrectfn = fatom_getrect;
    slider_widgetbehavior.w_displacefn = fatom_displace;
    slider_widgetbehavior.w_selectfn =  fatom_select;
    slider_widgetbehavior.w_activatefn = fatom_activate;
    slider_widgetbehavior.w_deletefn =   fatom_delete;
    slider_widgetbehavior.w_visfn=     fatom_vis;
    slider_widgetbehavior.w_clickfn =   NULL;
    
    fatom_setup_common(slider_class);
    class_setwidget(slider_class,&slider_widgetbehavior);
    
#if PD_MINOR_VERSION < 37
    slider_widgetbehavior.w_savefn =    slider_save;
    slider_widgetbehavior.w_propertiesfn = NULL;
#else
    class_setsavefn(slider_class,&slider_save);
    class_setpropertiesfn(slider_class,&fatom_properties);
#endif

}

