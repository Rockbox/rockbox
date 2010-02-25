#include "../src/m_pd.h"
#include "g_canvas.h"


#ifdef NT
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#endif

#include "fatom.h"

/* can we use the normal text save function ?? */

static t_class *sliderh_class;

static void sliderh_save(t_gobj *z, t_binbuf *b)
{

    t_fatom *x = (t_fatom *)z;

    binbuf_addv(b, "ssiisiiisss", gensym("#X"),gensym("obj"),
		x->x_obj.te_xpix, x->x_obj.te_ypix ,  
		gensym("sliderh"),x->x_max,x->x_min,x->x_width,x->x_send,x->x_color,x->x_bgcolor);
    binbuf_addv(b, ";");
}


static void *sliderh_new(t_symbol* s, int argc, t_atom* argv)
{
    t_fatom *x = (t_fatom *)pd_new(sliderh_class);
    x->x_type = gensym("hslider");
    return fatom_new(x,argc,argv);
}


t_widgetbehavior   sliderh_widgetbehavior;




void sliderh_setup(void) {
    sliderh_class = class_new(gensym("sliderh"), (t_newmethod)sliderh_new, 0,
				sizeof(t_fatom),0,A_DEFFLOAT,A_DEFFLOAT,A_DEFFLOAT,0);

    fatom_setup_common(sliderh_class);

    sliderh_widgetbehavior.w_getrectfn = fatom_getrect;
    sliderh_widgetbehavior.w_displacefn= fatom_displace;
    sliderh_widgetbehavior.w_selectfn=   fatom_select;
    sliderh_widgetbehavior.w_activatefn=fatom_activate;
    sliderh_widgetbehavior.w_deletefn=   fatom_delete;
    sliderh_widgetbehavior.w_visfn=     fatom_vis;
#if PD_MINOR_VERSION < 37
    sliderh_widgetbehavior.w_savefn=    sliderh_save;
    sliderh_widgetbehavior.w_propertiesfn= NULL;
#endif
    sliderh_widgetbehavior.w_clickfn=    NULL;

	class_setwidget(sliderh_class,&sliderh_widgetbehavior);
#if PD_MINOR_VERSION >= 37
    class_setsavefn(sliderh_class,&sliderh_save);
#endif
}

