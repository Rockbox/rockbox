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

#include <stdio.h>

const char *spcf_keynames_ascii[95] = {
  "space",
  "exclam",
  "quotedbl",
  "numbersign",
  "dollar",
  "percent",
  "ampersand",
  "apostrophe",
  "parenleft",
  "parenright",
  "asterisk",
  "plus",
  "comma",
  "minus",
  "period",
  "slash",
  "0",
  "1",
  "2",
  "3",
  "4",
  "5",
  "6",
  "7",
  "8",
  "9",
  "colon",
  "semicolon",
  "less",
  "equal",
  "greater",
  "question",
  "at",
  "A",
  "B",
  "C",
  "D",
  "E",
  "F",
  "G",
  "H",
  "I",
  "J",
  "K",
  "L",
  "M",
  "N",
  "O",
  "P",
  "Q",
  "R",
  "S",
  "T",
  "U",
  "V",
  "W",
  "X",
  "Y",
  "Z",
  "bracketleft",
  "backslash",
  "bracketright",
  "asciicircum",
  "underscore",
  "grave",
  "a",
  "b",
  "c",
  "d",
  "e",
  "f",
  "g",
  "h",
  "i",
  "j",
  "k",
  "l",
  "m",
  "n",
  "o",
  "p",
  "q",
  "r",
  "s",
  "t",
  "u",
  "v",
  "w",
  "x",
  "y",
  "z",
  "braceleft",
  "bar",
  "braceright",
  "asciitilde"
};

const char *spcf_keynames_misc[256] = {
  NULL,             NULL,             NULL,             NULL, 
  NULL,             NULL,             NULL,             NULL, 
  "BackSpace",      "Tab",            "Linefeed",       "Clear",
  NULL,             "Return",         NULL,             NULL, 

  NULL,             NULL,             NULL,             "Pause",
  "Scroll_Lock",    "Sys_Req",        NULL,             NULL, 
  NULL,             NULL,             NULL,             "Escape",
  NULL,             NULL,             NULL,             NULL, 

  "Multi_key",      NULL,             NULL,             NULL, 
  NULL,             NULL,             NULL,             NULL, 
  NULL,             NULL,             NULL,             NULL, 
  NULL,             NULL,             NULL,             NULL, 

  NULL,             NULL,             NULL,             NULL, 
  NULL,             NULL,             NULL,             NULL, 
  NULL,             NULL,             NULL,             NULL, 
  NULL,             NULL,             NULL,             NULL, 

  NULL,             NULL,             NULL,             NULL, 
  NULL,             NULL,             NULL,             NULL, 
  NULL,             NULL,             NULL,             NULL, 
  NULL,             NULL,             NULL,             NULL, 

  "Home",           "Left",           "Up",             "Right",
  "Down",           "Page_Up",        "Page_Down",      "End",
  "Begin",          NULL,             NULL,             NULL,
  NULL,             NULL,             NULL,             NULL, 

  "Select",         "Print",          "Execute",        "Insert",
  NULL,             "Undo",           "Redo",           "Menu",
  "Find",           "Cancel",         "Help",           "Break",
  NULL,             NULL,             NULL,             NULL, 

  NULL,             NULL,             NULL,             NULL, 
  NULL,             NULL,             NULL,             NULL, 
  NULL,             NULL,             NULL,             NULL, 
  NULL,             NULL,             "Mode_switch",    "Num_Lock",

  "KP_Space",       NULL,             NULL,             NULL,
  NULL,             NULL,             NULL,             NULL, 
  "KP_Tab",         NULL,             NULL,             NULL, 
  NULL,             "KP_Enter",       NULL,             NULL, 

  NULL,             "KP_F1",          "KP_F2",          "KP_F3",
  "KP_F4",          "KP_Home",        "KP_Left",        "KP_Up",
  "KP_Right",       "KP_Down",        "KP_Page_Up",     "KP_Page_Down",
  "KP_End",         "KP_Begin",       "KP_Insert",      "KP_Delete",

  NULL,             NULL,             NULL,             NULL, 
  NULL,             NULL,             NULL,             NULL, 
  NULL,             NULL,             "KP_Multiply",    "KP_Add",
  "KP_Separator",   "KP_Subtract",    "KP_Decimal",     "KP_Divide",

  "KP_0",           "KP_1",           "KP_2",           "KP_3",
  "KP_4",           "KP_5",           "KP_6",           "KP_7",
  "KP_8",           "KP_9",           NULL,             NULL, 
  NULL,             "KP_Equal",       "F1",             "F2",

  "F3",             "F4",             "F5",             "F6",
  "F7",             "F8",             "F9",             "F10",
  "F11",            "F12",            "F13",            "F14",
  "F15",            "F16",            "F17",            "F18",

  "F19",            "F20",            "F21",            "F22",
  "F23",            "F24",            "F25",            "F26",
  "F27",            "F28",            "F29",            "F30",
  "F31",            "F32",            "F33",            "F34",

  "F35",            "Shift_L",        "Shift_R",        "Control_L",
  "Control_R",      "Caps_Lock",      "Shift_Lock",     "Meta_L",
  "Meta_R",         "Alt_L",          "Alt_R",          "Super_L",
  "Super_R",        "Hyper_L",        "Hyper_R",        NULL,

  NULL,             NULL,             NULL,             NULL, 
  NULL,             NULL,             NULL,             NULL, 
  NULL,             NULL,             NULL,             NULL, 
  NULL,             NULL,             NULL,             "Delete"
};
