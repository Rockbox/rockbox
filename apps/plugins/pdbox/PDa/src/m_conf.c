/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* changes by Thomas Musil IEM KUG Graz Austria 2001 */
/* all changes are labeled with      iemlib      */

#include "m_pd.h"

void g_array_setup(void);
void g_canvas_setup(void);
void g_guiconnect_setup(void);
/* iemlib */
void g_bang_setup(void);
void g_hradio_setup(void);
void g_hslider_setup(void);
void g_mycanvas_setup(void);
void g_numbox_setup(void);
void g_toggle_setup(void);
void g_vradio_setup(void);
void g_vslider_setup(void);
void g_vumeter_setup(void);
/* iemlib */
void g_io_setup(void);
void g_scalar_setup(void);
void g_template_setup(void);
void g_text_setup(void);
void g_traversal_setup(void);
void m_pd_setup(void);
void x_acoustics_setup(void);
void x_interface_setup(void);
void x_connective_setup(void);
void x_time_setup(void);
void x_arithmetic_setup(void);
void x_midi_setup(void);
void x_misc_setup(void);
void x_net_setup(void);
void x_qlist_setup(void);
void x_gui_setup(void);
void d_arithmetic_setup(void);
void d_array_setup(void);
void d_ctl_setup(void);
void d_dac_setup(void);
void d_delay_setup(void);
void d_fft_setup(void);
void d_filter_setup(void);
void d_global_setup(void);
void d_math_setup(void);
void d_misc_setup(void);
void d_osc_setup(void);
void d_soundfile_setup(void);
void d_ugen_setup(void);

/* PD anywhere specific. -- W.B. */
void d_intern_setup(void);

void conf_init(void)
{
    g_array_setup();
    g_canvas_setup();
    g_guiconnect_setup();
/* iemlib */

    g_bang_setup();
    g_hradio_setup();
    g_hslider_setup();
    g_mycanvas_setup();
    g_numbox_setup();
    g_toggle_setup();
    g_vradio_setup();
    g_vslider_setup();
    g_vumeter_setup();
/* iemlib */

    g_io_setup();
    g_scalar_setup();
    g_template_setup();
    g_text_setup();
    g_traversal_setup();
    m_pd_setup();
    x_acoustics_setup();
    x_interface_setup();
    x_connective_setup();
    x_time_setup();
    x_arithmetic_setup();

#ifndef ROCKBOX
    x_midi_setup();
#endif
    x_misc_setup();
    x_net_setup();
    x_qlist_setup();
#ifndef ROCKBOX
    x_gui_setup();
#endif
    d_arithmetic_setup();
    d_dac_setup();
    d_fft_setup();
    d_global_setup();
    d_misc_setup();
#ifdef STATIC
    d_intern_setup();
#endif
    d_soundfile_setup();
    d_ugen_setup();
	    
}

