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

/*
 *
 *       ax.c
 *
 *
 *
 *       Created: 94/11/11 Szeredi Miklos
 *
 *       Version: 0.1  94/11/11
 *                0.2  95/06/12
 *
 */

/* #define DEBUG_EVENTS */

#include "ax.h"
#include <X11/Xutil.h>
#include <X11/Xresource.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>


#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
typedef int boolean;
#define max(x, y) ((x) < (y) ? (y) : (x))
#define min(x, y) ((x) < (y) ? (x) : (y))
#define minmax(a, l, u) ((a) < (l) ? (l) : ((a) > (u) ? (u) : (a)))

#define MAX_PROG_NAME_LEN 128
#define MAX_CLASS_NAME_LEN MAX_PROG_NAME_LEN 
#define MAX_PRES_LEN 256
#define MAX_FILENAME_LEN 1024

static const char *empty_str = "";

typedef void (*eventproc_t)(XEvent *, void *);

typedef struct _event_proc_struct {
  eventproc_t                  event_proc;
  unsigned long                event_mask;
  Display                      *disp;
  void                         *ptr;
  Window                       event_win;
  boolean                      done_proc;
  struct _event_proc_struct    *next_proc;
} event_proc_struct;

typedef struct {
  const char        *event_name;
  boolean           win_given;
  event_proc_struct *next_proc;
} event_struct;

typedef struct _disp_struct{
  Display *disp;
  XFontStruct *font;
  struct _disp_struct *next_disp;
} disp_struct;

     
#define  EVENT_NUM     LASTEvent

static event_struct event_info[EVENT_NUM];

static disp_struct disp_start = {NULL, NULL, NULL};

static boolean disp_chain_modified;

static char prog_name[MAX_PROG_NAME_LEN + 1];
static char class_name[MAX_CLASS_NAME_LEN + 1];

static XrmDatabase rDB = NULL; /* Has to be global otherwise Xlib hangs */

static int opTableEntries = 19;
static aX_options opTable[] = {
  {"-display",       ".display",      XrmoptionSepArg,     NULL},
  {"-geometry",      ".geometry",     XrmoptionSepArg,     NULL},
  {"-bg",            ".background",   XrmoptionSepArg,     NULL},
  {"-background",    ".background",   XrmoptionSepArg,     NULL},
  {"-bd",            ".borderColor",  XrmoptionSepArg,     NULL},
  {"-bordercolor",   ".borderColor",  XrmoptionSepArg,     NULL},
  {"-bw",            ".borderWidth",  XrmoptionSepArg,     NULL},
  {"-borderwidth",   ".borderWidth",  XrmoptionSepArg,     NULL},
  {"-fg",            ".foreground",   XrmoptionSepArg,     NULL},
  {"-foreground",    ".foreground",   XrmoptionSepArg,     NULL},
  {"-fn",            ".font",         XrmoptionSepArg,     NULL},
  {"-font",          ".font",         XrmoptionSepArg,     NULL},
  {"-name",          ".name",         XrmoptionSepArg,     NULL},
  {"-rv",            ".reverseVideo", XrmoptionNoArg,      "on"},
  {"-reverse",       ".reverseVideo", XrmoptionNoArg,      "on"},
  {"+rv",            ".reverseVideo", XrmoptionNoArg,      "off"},
  {"-bg",            ".background",   XrmoptionSepArg,     NULL},
  {"-title",         ".title",        XrmoptionSepArg,     NULL},
  {"-xrm",           NULL,            XrmoptionResArg,     NULL}
};

static char *addstr(const char str1[], const char str2[], char str12[],
            unsigned int str12len)
{
  unsigned int i, j, k;
  
  str12[str12len-1] = '\0';

  for(i=0, j=0, k=0; i + 1 < str12len; ) {
    if(str1[j]) str12[i] = str1[j++];
    else        str12[i] = str2[k++];
    if(! str12[i++]) break;
  }
  return str12;
}


static char *pname(const char *resource) {
  static char pnameres[MAX_PRES_LEN];

  return addstr(prog_name, resource, pnameres, MAX_PRES_LEN);
}

static char *pclass(const char *resource) {
  static char pclassres[MAX_PRES_LEN];

  return addstr(class_name, resource, pclassres, MAX_PRES_LEN);
}


static void fill_event_info(void)
{
  int i;

  for(i = 0; i < EVENT_NUM; i++) {
    event_info[i].event_name = empty_str;
    event_info[i].win_given = TRUE;
    event_info[i].next_proc = NULL;
  }
  
  event_info[MappingNotify].win_given = FALSE;

  event_info[ButtonPress].event_name = "ButtonPress";
  event_info[ButtonRelease].event_name = "ButtonRelease";
  event_info[CirculateNotify].event_name = "CirculateNotify";
  event_info[CirculateRequest].event_name = "CirculateRequest";
  event_info[ClientMessage].event_name = "ClientMessage";
  event_info[ColormapNotify].event_name = "ColormapNotify";
  event_info[ConfigureNotify].event_name = "ConfigureNotify";
  event_info[ConfigureRequest].event_name = "ConfigureRequest";
  event_info[CreateNotify].event_name = "CreateNotify";
  event_info[DestroyNotify].event_name = "DestroyNotify";
  event_info[EnterNotify].event_name = "EnterNotify";
  event_info[LeaveNotify].event_name = "LeaveNotify";
  event_info[Expose].event_name = "Expose";
  event_info[FocusIn].event_name = "FocusIn";
  event_info[FocusOut].event_name = "FocusOut";
  event_info[GraphicsExpose].event_name = "GraphicsExpose";
  event_info[NoExpose].event_name = "NoExpose";
  event_info[GravityNotify].event_name = "GravityNotify";
  event_info[KeymapNotify].event_name = "KeymapNotify";
  event_info[KeyPress].event_name = "KeyPress";
  event_info[KeyRelease].event_name = "KeyRelease";
  event_info[MapNotify].event_name = "MapNotify";
  event_info[UnmapNotify].event_name = "UnmapNotify";
  event_info[MappingNotify].event_name = "MappingNotify";
  event_info[MapRequest].event_name = "MapRequest";
  event_info[MotionNotify].event_name = "MotionNotify";
  event_info[PropertyNotify].event_name = "PropertyNotify";
  event_info[ReparentNotify].event_name = "ReparentNotify";
  event_info[ResizeRequest].event_name = "ResizeRequest";
  event_info[SelectionClear].event_name = "SelectionClear";
  event_info[SelectionNotify].event_name = "SelectionNotify";
  event_info[SelectionRequest].event_name = "SelectionRequest";
  event_info[VisibilityNotify].event_name = "VisibilityNotify";

}

static void get_def_res(aX_default_resources *defres)
{
  XrmValue value;
  char *str_type;
  int flags;
  XColor color_def;
  unsigned long tmp_pixel;
  Colormap def_map;
  int font_spec;


  defres->window_name = prog_name;
  defres->icon_name = prog_name;


  defres->scr = DefaultScreen(defres->disp);  
  defres->scr_ptr = ScreenOfDisplay(defres->disp, defres->scr);
  def_map = DefaultColormapOfScreen(defres->scr_ptr);


  if(XrmGetResource(rDB, pname(".title"), pclass(".Title"), 
            &str_type, &value)) 
    defres->window_name = (char *) value.addr;  

  defres->sflags = PSize;
  if(XrmGetResource(rDB, pname(".geometry"), pclass(".Geometry"), 
            &str_type, &value)) {
    flags = XParseGeometry((char *) value.addr, &(defres->x), &(defres->y),
               &(defres->width), &(defres->height));
    if((XValue | YValue) & flags) defres->sflags |= USPosition;
    if((WidthValue | HeightValue) & flags) 
      defres->sflags = (defres->sflags & ~PSize) | USSize;
  }

  defres->background = defres->background ? 
    WhitePixel(defres->disp, defres->scr) : 
      BlackPixel(defres->disp, defres->scr);

  if(XrmGetResource(rDB, pname(".background"), pclass(".Background"), 
            &str_type, &value)) {
    if(XParseColor(defres->disp, def_map, value.addr, &color_def)) {
      if(XAllocColor(defres->disp, def_map, &color_def)) 
    defres->background = color_def.pixel;
    }
    else fprintf(stderr, "%s: aX: warning: Invalid color specification %s\n", 
         prog_name, value.addr);
  }
      
  defres->foreground = defres->foreground ? 
    WhitePixel(defres->disp, defres->scr) : 
      BlackPixel(defres->disp, defres->scr);

  if(XrmGetResource(rDB, pname(".foreground"), pclass(".Foreground"), 
            &str_type, &value)) {
    if(XParseColor(defres->disp, def_map, value.addr, &color_def)) {
      if(XAllocColor(defres->disp, def_map, &color_def)) 
    defres->foreground = color_def.pixel;
    }
    else fprintf(stderr, "%s: aX: warning: Invalid color specification %s\n", 
         prog_name, value.addr);
  }

  if(XrmGetResource(rDB, pname(".borderWidth"), pclass(".BorderWidth"),
                   &str_type, &value)) {
    defres->border_width = atoi(value.addr);
  }

  defres->border_color = defres->foreground;
  if(XrmGetResource(rDB, pname(".borderColor"), pclass(".BorderColor"), 
            &str_type, &value)) {
    if(XParseColor(defres->disp, def_map, value.addr, &color_def)) {
      if(XAllocColor(defres->disp, def_map, &color_def)) 
    defres->border_color = color_def.pixel;
    }
    else fprintf(stderr, "%s: aX: warning: Invalid color specification %s\n", 
         prog_name, value.addr);
  }

  font_spec = 0;
  if(XrmGetResource(rDB, pname(".font"), pclass(".Font"),
            &str_type, &value)) {
    defres->font_name = value.addr;
    if(defres->font_name != NULL) font_spec = 1;

  }

  if(XrmGetResource(rDB, pname(".fallbackFont"), pclass(".Font"),
            &str_type, &value)) 
    defres->fallback_font_name = value.addr;

  if(defres->font_name == NULL || 
     (defres->font = XLoadQueryFont(defres->disp, defres->font_name)) 
     == NULL) {
    
    if(font_spec) 
      fprintf(stderr, "%s: aX: warning: cannot open %s font, ",
          prog_name, defres->font_name);
    
    defres->font_name = defres->fallback_font_name;

    if(font_spec && defres->font_name != NULL) 
      fprintf(stderr, "trying %s...\n",defres->font_name);

    if(defres->font_name == NULL || 
       (defres->font = 
    XLoadQueryFont(defres->disp, defres->fallback_font_name)) == NULL) {
      
      if(defres->font_name != NULL) {
    
    fprintf(stderr, "%s: aX: warning: cannot open %s font, ",
        prog_name, defres->font_name);
      }

      defres->font_name = "fixed";
      
      fprintf(stderr, "trying %s...\n",defres->font_name);

      if((defres->font = XLoadQueryFont(defres->disp, defres->font_name)) 
     == NULL) {
    
    fprintf(stderr, "%s: aX: warning: cannot open %s font\n",
        prog_name, defres->font_name);
    
    exit(-1);
      }
    }
    else defres->font_name = defres->fallback_font_name;
  }
  
  if(XrmGetResource(rDB, pname(".reverseVideo"), pclass(".ReverseVideo"),
                   &str_type, &value)) 
    if(strcmp(value.addr, "on") == 0) {
      tmp_pixel = defres->foreground;
      defres->foreground = defres->background;
      defres->background = tmp_pixel;
    } 

}


static void add_disp(aX_default_resources *defres)
{
  disp_struct *last;

  for(last = &disp_start; last->next_disp != NULL; last = last->next_disp);
  
  if((last->next_disp = malloc(sizeof(disp_struct))) == NULL) {
    fprintf(stderr, "%s: aX: Not enough memory.\n", prog_name);
    exit(-1);
  };

  last = last->next_disp;
  
  last->disp = defres->disp;
  last->font = defres->font;
  last->next_disp = NULL;


  disp_chain_modified = TRUE;
}

void aX_open_disp(aX_options *useropt, int useroptlen, 
         int *argcp, char *argv[],
         aX_default_resources *defres)
{

  XrmValue value;
  char *str_type;
  char *disp_res;
  char *environment;
  char *display_name = NULL;
  char filename[MAX_FILENAME_LEN];
  int i;
  XrmDatabase commandlineDB = NULL, usercommandlineDB = NULL;
  XrmDatabase homeDB, serverDB, applicationDB;

/*  
  if(disp_start.next_disp != NULL) {
    fprintf(stderr, "aX_open_disp: Cannot open first display twice.\n");
    exit(-1);
  }
*/

  XrmInitialize();

  class_name[0] = '\0';
  class_name[MAX_CLASS_NAME_LEN] = '\0';
  if(defres->class_name != NULL) 
    strncpy(class_name, defres->class_name, MAX_CLASS_NAME_LEN);


  fill_event_info();

  for(i = 1; i < *argcp; i++) 
    if(strcmp(argv[i], "-name") == 0 && ++i < *argcp){
      defres->prog_name = argv[i];
      break;
    }


  prog_name[0] = '\0';
  prog_name[MAX_PROG_NAME_LEN] = '\0';
  if(defres->prog_name != NULL) 
    strncpy(prog_name, defres->prog_name, MAX_PROG_NAME_LEN);
  else
    strncpy(prog_name, argv[0], MAX_PROG_NAME_LEN);

  defres->prog_name = prog_name;

  XrmParseCommand(&commandlineDB, (XrmOptionDescRec *) opTable, 
          opTableEntries, prog_name, argcp, argv);

  if(useropt != NULL)
    XrmParseCommand(&usercommandlineDB, (XrmOptionDescRec *) useropt, 
            useroptlen, prog_name, argcp, argv);
  else usercommandlineDB = NULL;

/*
  if(*argcp != 1) {
    fprintf(stderr, 
    "%s: aX_open_disp: Unrecognised options in command line!\n", 
        prog_name);
    exit(-1);
  }
*/

  if(XrmGetResource(commandlineDB, pname(".display"), pclass(".Display"),
            &str_type, &value)) display_name = (char *) value.addr;
  
  if((defres->disp = XOpenDisplay(display_name)) == NULL) {
    fprintf(stderr, "%s: aX_open_disp: cannot connect to X server %s\n", 
        prog_name, XDisplayName(display_name));
    exit(-1);
  }

  applicationDB = XrmGetFileDatabase(
                     addstr("/usr/lib/X11/app-defaults/",
                        class_name,
                        filename,
                        MAX_FILENAME_LEN));
/*  
  if(defres->disp->xdefaults) 
  serverDB = XrmGetStringDatabase(defres->disp->xdefaults);
  else serverDB = NULL;
*/


  disp_res = XResourceManagerString(defres->disp);

  if(disp_res) serverDB = XrmGetStringDatabase(disp_res);
  else serverDB = NULL;

  
  if((environment = getenv("XENVIRONMENT")) != NULL) 
    homeDB = XrmGetFileDatabase(environment);
  else homeDB = NULL;

  
  XrmMergeDatabases(applicationDB, &rDB);
  XrmMergeDatabases(serverDB, &rDB);
  XrmMergeDatabases(homeDB, &rDB);
  XrmMergeDatabases(commandlineDB, &rDB);
  XrmMergeDatabases(usercommandlineDB, &rDB);

  get_def_res(defres);

  add_disp(defres);

} 

 
void aX_open_second_disp(char *display_name, 
             aX_default_resources *defres)
{
  char *disp_res;

  XrmDatabase serverDB;


  if((defres->disp = XOpenDisplay(display_name)) == NULL) {
    fprintf(stderr, 
        "%s: aX_open_second_disp: cannot connect to X server %s\n", 
        prog_name, XDisplayName(display_name));
    exit(-1);
  }


  disp_res = XResourceManagerString(defres->disp);

  if(disp_res) serverDB = XrmGetStringDatabase(disp_res);
  else serverDB = NULL;

  
  XrmMergeDatabases(serverDB, &rDB);

  get_def_res(defres);

  add_disp(defres);
} 

/*
  
void smallwait(boolean event_prev)
{
  event_prev = event_prev;
  sleep(1);
}

*/


static void sigalrm_handler(int i)
{
  i = i;
  signal(SIGALRM, SIG_IGN);
}


static void smallwait(boolean event_prev)
{
  struct itimerval value;

  signal(SIGALRM, sigalrm_handler);

  event_prev = event_prev;

  value.it_interval.tv_sec = 0L;
  value.it_interval.tv_usec = 0L;
  value.it_value.tv_sec = 0L;
  value.it_value.tv_usec = 100000L;
  setitimer(ITIMER_REAL, &value, NULL);

  pause();
}

/* This aX_wait_all_muck needs to be cleared out! */

void aX_wait_event(int eventtype)
{
  XEvent ev;
  event_proc_struct **curr;
  int i;
  disp_struct *dsp, *dsp1;

  boolean event_prev;


start:;

  if(disp_start.next_disp == NULL) {
    fprintf(stderr, "%s: aX_wait_event: No connection to any display\n", 
        prog_name);
    exit(-1);
  }

  dsp = disp_start.next_disp;


  do {
    event_prev = TRUE;

    dsp1 = dsp;
    if((disp_start.next_disp)->next_disp != NULL || 
       eventtype == AX_LOOK_EVENTS)
      while(XPending(dsp->disp) == 0) {
    dsp = dsp->next_disp;
    if(dsp == NULL) dsp = disp_start.next_disp;

    if(dsp == dsp1) {
      if(eventtype == AX_LOOK_EVENTS) return;
      
      smallwait(event_prev);
      event_prev = FALSE;
      dsp = dsp1 = disp_start.next_disp;
    }
      }    

    XNextEvent(dsp->disp, &ev);
#ifdef DEBUG_EVENTS
      fprintf(stderr,"Event: %s (%i) in win: %li)\n",
          event_info[ev.type].event_name,
          ev.type,
          ev.xany.window);
#endif
    

    if(dsp->disp != ev.xany.display) 
      fprintf(stderr, "Ha! Event read from wrong display! Stupid XLib!!!\n");
    
    curr = &(event_info[ev.type].next_proc);
    i = 0;
    while(*curr != NULL) {
      if((*curr)->disp == dsp->disp && 
     (!event_info[ev.type].win_given || 
      ev.xany.window == (*curr)->event_win) && 
     !(*curr)->done_proc) {
    i++;
    (*curr)->done_proc = TRUE;
    disp_chain_modified = FALSE;

    if((*curr)->event_proc != NULL) {
        (*((*curr)->event_proc))(&ev, (*curr)->ptr);
    }

    if(disp_chain_modified) goto start;
    curr = &(event_info[ev.type].next_proc);
      }
      else curr = &((*curr)->next_proc);
    }
    curr = &(event_info[ev.type].next_proc);
    while(*curr != NULL) {
      (*curr)->done_proc = FALSE;
      curr = &((*curr)->next_proc);
    }

    if(i == 0) 
      fprintf(stderr, "%s: aX_wait_event: warning: "
          "Unexpected event: %s (%i) in win: %li)\n",
          prog_name,
          event_info[ev.type].event_name,
          ev.type,
          ev.xany.window);

  } while(eventtype != ev.type && eventtype != AX_ANY_EVENT);
}

  
void aX_look_events(void)
{
  aX_wait_event(AX_LOOK_EVENTS);
}

char *aX_get_prog_res(const char *resname, const char* resclass)
{
  XrmValue value;
  char *str_type;

  if(XrmGetResource(rDB, pname(resname), pclass(resclass),
                   &str_type, &value)) 
    return (char *)value.addr;
  else return NULL;
}

char *aX_get_resource(const char *resname, const char* resclass)
{
  XrmValue value;
  char *str_type;

  if(XrmGetResource(rDB, resname, resclass,
                   &str_type, &value)) 
    return (char *)value.addr;
  else return NULL;
}

static long get_win_mask(Display *disp, Window win)
{
  int evt;
  event_proc_struct *ep;
  long winmask;

  for(evt = 0, winmask = 0; evt < EVENT_NUM; evt++) {
    ep = event_info[evt].next_proc;
    while(ep != NULL) {
      if(ep->event_win == win && ep->disp == disp) winmask |= ep->event_mask;
      ep = ep->next_proc;
    }
  }
  return winmask;
} 



void aX_add_event_proc(Display *disp, 
                  Window win, 
                  int eventtype,
                  void (*eventproc)(XEvent *, void *),
                  unsigned long eventmask,
                  void *ptr)
{
  event_proc_struct **epp;
  long winmask;
  
  epp = &(event_info[eventtype].next_proc);
  while(*epp != NULL) epp = &((*epp)->next_proc);
  
  if((*epp = (event_proc_struct *)
      malloc((size_t) sizeof(event_proc_struct))) == NULL) {
    fprintf(stderr, 
        "%s: aX_add_event_proc_disp: Not enough memory.\n", prog_name);
    exit(-1);
  }

  (*epp)->event_proc = eventproc;
  (*epp)->ptr = ptr;
  (*epp)->event_mask = eventmask;
  (*epp)->disp = disp;
  (*epp)->event_win = win;
  (*epp)->done_proc = FALSE;
  (*epp)->next_proc = NULL;

  if(win) {
    winmask = get_win_mask(disp, win);
    XSelectInput(disp, win, winmask);
  } 
}

void aX_remove_event_proc(Display *disp, 
                 Window win, 
                 int eventtype,
                 void (*eventproc)(XEvent *, void *))
{
  event_proc_struct **epp;
  event_proc_struct *tmp;
  long winmask;
  
  epp = &(event_info[eventtype].next_proc);
  while(*epp != NULL) {
    if((*epp)->disp == disp && 
       (*epp)->event_win == win && 
       (*epp)->event_proc == eventproc) {
      tmp = (*epp)->next_proc;
      free(*epp);
      *epp = tmp;
      if(win) {
    winmask = get_win_mask(disp, win);
    XSelectInput(disp, win, winmask);
      }
      return;
    }
    else epp = &((*epp)->next_proc);
  }
  fprintf(stderr, "%s: aX_remove_event_proc_disp: warning: "
      "Could not remove event proc (event: %s (%i), window: %lX)\n",
          prog_name, event_info[eventtype].event_name, eventtype, win);
 
}

void aX_close_one_disp(Display *disp)
{
/*  int evt;
  event_proc_struct **curr; */
  disp_struct *dsp, *dsp_tmp;

/*
  for(evt = 0; evt < EVENT_NUM; evt++) {
    curr = &(event_info[evt].next_proc);
    while(*curr != NULL) {
      if(disp == (*curr)->disp) {
    aX_remove_event_proc_disp((*curr)->disp, (*curr)->event_win,
                  evt, (*curr)->event_proc);
    curr = &(event_info[evt].next_proc);      
      }
      else curr = &((*curr)->next_proc);
    }
  }

*/

  for(dsp = &disp_start; dsp->next_disp->disp != disp; dsp = dsp->next_disp)
    if(dsp->next_disp == NULL) {
      fprintf(stderr,
          "%s: aX_close_one_disp: warning: Trying to close unopened display.\n", 
          prog_name);
      return;
    }

  XUnloadFont(dsp->next_disp->disp, dsp->next_disp->font->fid);
  XCloseDisplay(dsp->next_disp->disp);

  dsp_tmp = dsp->next_disp;
  dsp->next_disp = dsp->next_disp->next_disp;
  free(dsp_tmp);

  disp_chain_modified = TRUE;

}

void aX_close_disp(void)
{

  while(disp_start.next_disp != NULL) 
    aX_close_one_disp(disp_start.next_disp->disp);

}


unsigned long aX_get_color(Display *disp, int scr, unsigned long def_col, 
               const char *color_name)
{
  XColor color_def;
  Colormap def_map;
  Screen *scr_ptr;


  if(color_name == NULL) return def_col;

  scr_ptr = ScreenOfDisplay(disp, scr);
  def_map = DefaultColormapOfScreen(scr_ptr);

  if(XParseColor(disp, def_map, color_name, &color_def)) {
    if(XAllocColor(disp, def_map, &color_def)) 
      return color_def.pixel;
  }
  else fprintf(stderr, 
           "%s: aX_get_color: warning: Invalid color specification %s\n", 
           prog_name, color_name);

  return def_col;

}
