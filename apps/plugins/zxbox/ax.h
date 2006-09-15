/* 
 * Copyright (C) 1996-1998 Szeredi Miklos
 * Email: mszeredi@inf.bme.hu
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. See the file COPYING. 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*             ax.h
 *
 *     Header of the AX module
 *
 *     
 *     Created:   94/11/11    Szeredi Miklos
 */


#ifndef _AX_H
#define _AX_H

#include <X11/Xlib.h>
#include <X11/Xresource.h>

/*typedef  XrmOptionDescRec aX_options; */

typedef struct {
    const char        *option;        /* Option abbreviation in argv        */
    const char        *specifier;     /* Resource specifier            */
    XrmOptionKind   argKind;        /* Which style of option it is        */
    const char      *value;        /* Value to provide if XrmoptionNoArg   */
} aX_options;


#define  AX_NO_EVENT      LASTEvent
#define  AX_ANY_EVENT     0
#define  AX_LOOK_EVENTS    -1

typedef  struct{
         Display          *disp;
     int              scr;
     Screen           *scr_ptr;
         unsigned int     width;
     unsigned int     height;
     int              x;
     int              y;
     unsigned int     border_width;
     unsigned long    foreground;
     unsigned long    background;
     unsigned long    border_color;
     const char       *font_name;
     const char       *fallback_font_name;
     XFontStruct      *font;
     long             sflags;
     const char             *prog_name;
     const char             *class_name;
     char             *window_name;
     char             *icon_name;
       } aX_default_resources;


#ifdef __cplusplus
extern "C" {
#endif


extern   void   aX_open_disp(aX_options *useropt, int useroptlen, 
                 int *argcp, char *argv[],
                 aX_default_resources *defres);
extern   void   aX_open_second_disp(char *display_name,
                    aX_default_resources *defres);
extern   void   aX_wait_event(int eventtype);
extern   void   aX_look_events(void);
extern   char   *aX_get_prog_res(const char *resname, const char* resclass);
extern   char   *aX_get_resource(const char *resname, const char* resclass);

extern   void   aX_add_event_proc(Display *disp,
                  Window win,
                  int eventtype,
                  void (*eventproc)(XEvent *, void *),
                  unsigned long eventmask, 
                  void *ptr);

extern   void   aX_remove_event_proc(Display *disp,
                     Window win,
                     int eventtype,
                     void (*eventproc)(XEvent *, void *));
         
extern   void   aX_close_one_disp(Display *disp);
extern   void   aX_close_disp(void);
extern   unsigned long aX_get_color(Display *disp, int scr, 
                 unsigned long def_col, const char *color_name);


#ifdef __cplusplus
}
#endif


#endif /* _AX_H */

/*         End of ax.h                 */

