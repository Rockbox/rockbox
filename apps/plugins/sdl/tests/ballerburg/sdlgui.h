/*
 * sdlgui.h - Header for the tiny graphical user interface for the SDL library.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * (http://www.gnu.org/licenses/) for more details.
 */

#ifndef SDLGUI_H
#define SDLGUI_H

#include <SDL.h>

enum
{
  SGBOX,
  SGTEXT,
  SGEDITFIELD,
  SGBUTTON,
  SGRADIOBUT,
  SGCHECKBOX,
  SGPOPUP,
  SGSCROLLBAR,
  SGUSER
};


/* Object flags: */
#define SG_TOUCHEXIT   1   /* Exit immediately when mouse button is pressed down */
#define SG_EXIT        2   /* Exit when mouse button has been pressed (and released) */
#define SG_DEFAULT     4   /* Marks a default button, selectable with return key */
#define SG_CANCEL      8   /* Marks a cancel button, selectable with ESC key */

/* Object states: */
#define SG_SELECTED    1
#define SG_MOUSEDOWN   16
#define SG_MOUSEUP     (((int)-1) - SG_MOUSEDOWN)

/* Special characters: */
#define SGRADIOBUTTON_NORMAL    12
#define SGRADIOBUTTON_SELECTED  13
#define SGCHECKBOX_NORMAL       14
#define SGCHECKBOX_SELECTED     15
#define SGARROWUP                1
#define SGARROWDOWN              2
#define SGFOLDER                 5

#define SGARROWLEFTSTR      "\x04"
#define SGARROWRIGHTSTR     "\x03"

/* Return codes: */
#define SDLGUI_ERROR         -1
#define SDLGUI_QUIT          -2
#define SDLGUI_UNKNOWNEVENT  -3


typedef struct
{
  int type;             /* What type of object */
  int flags;            /* Object flags */
  int state;            /* Object state */
  int x, y;             /* The offset to the upper left corner */
  int w, h;             /* Width and height (for scrollbar : height and position) */
  char *txt;            /* Text string */
}  SGOBJ;

extern int sdlgui_fontwidth;	/* Width of the actual font */
extern int sdlgui_fontheight;	/* Height of the actual font */

extern int SDLGui_Init(void);
extern int SDLGui_UnInit(void);
extern int SDLGui_SetScreen(SDL_Surface *pScrn);
extern void SDLGui_GetFontSize(int *width, int *height);
extern void SDLGui_Text(int x, int y, const char *txt);
extern void SDLGui_DrawDialog(const SGOBJ *dlg);
extern int SDLGui_DoDialog(SGOBJ *dlg, SDL_Event *pEventOut);
extern void SDLGui_CenterDlg(SGOBJ *dlg);
void SDLGui_DrawButton(const SGOBJ *bdlg, int objnum);

#endif
