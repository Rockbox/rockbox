// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//   Menu widget stuff, episode selection and such.
//
//-----------------------------------------------------------------------------


#ifndef __M_MENU__
#define __M_MENU__



#include "d_event.h"

//
// MENUS
//
// Called by main loop,
// saves config file and calls I_Quit when user exits.
// Even when the menu is not displayed,
// this can resize the view and change game parameters.
// Does all the real work of the menu interaction.
boolean M_Responder (event_t *ev);


// Called by main loop,
// only used for menu (skull cursor) animation.
void M_Ticker (void);

// Called by main loop,
// draws the menus directly into the screen buffer.
void M_Drawer (void);

// Called by D_DoomMain,
// loads the config file.
void M_Init (void);

// Called by intro code to force menu up upon a keypress,
// does nothing if menu is already up.
void M_StartControlPanel (void);

/****************************
 *
 * The setup_group enum is used to show which 'groups' keys fall into so
 * that you can bind a key differently in each 'group'.
 */

typedef enum {
   m_null,       // Has no meaning; not applicable
   m_scrn,       // A key can not be assigned to more than one action
   m_map,        // in the same group. A key can be assigned to one
   m_menu,       // action in one group, and another action in another.
} setup_group;

/****************************
 *
 * phares 4/17/98:
 * State definition for each item.
 * This is the definition of the structure for each setup item. Not all
 * fields are used by all items.
 *
 * A setup screen is defined by an array of these items specific to
 * that screen.
 *
 * killough 11/98:
 *
 * Restructured to allow simpler table entries,
 * and to Xref with defaults[] array in m_misc.c.
 * Moved from m_menu.c to m_menu.h so that m_misc.c can use it.
 */

typedef struct setup_menu_s
{
   const char  *m_text;  /* text to display */
   int         m_flags;  /* phares 4/17/98: flag bits S_* (defined above) */
   setup_group m_group;  /* Group */
   short       m_x;      /* screen x position (left is 0) */
   short       m_y;      /* screen y position (top is 0) */

   union  /* killough 11/98: The first field is a union of several types */
   {
      const void          *var;   /* generic variable */
      int                 *m_key; /* key value, or 0 if not shown */
      const char          *name;  /* name */
      struct default_s    *def;   /* default[] table entry */
      struct setup_menu_s *menu;  /* next or prev menu */
   } var;

   int         *m_mouse; /* mouse button value, or 0 if not shown */
   int         *m_joy;   /* joystick button value, or 0 if not shown */
   void (*action)(void); /* killough 10/98: function to call after changing */
} setup_menu_t;




#endif
//-----------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2006/03/28 15:44:01  dave
// Patch #2969 - Doom!  Currently only working on the H300.
//
//
//-----------------------------------------------------------------------------
