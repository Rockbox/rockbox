/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *  Main loop menu stuff.
 *  Default Config File.
 *  PCX Screenshots.
 *
 *-----------------------------------------------------------------------------*/

#include "doomstat.h"
#include "m_argv.h"
#include "g_game.h"
#include "m_menu.h"
#include "am_map.h"
#include "w_wad.h"
#include "i_sound.h"
#include "i_video.h"
#include "v_video.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "dstrings.h"
#include "m_misc.h"
#include "s_sound.h"
#include "sounds.h"
#include "i_system.h"
#include "d_main.h"
#include "m_swap.h"
#include "p_pspr.h"

#include "rockmacros.h"

//
// M_DrawText
// Returns the final X coordinate
// HU_Init must have been called to init the font
//
extern patchnum_t hu_font[HU_FONTSIZE];

int M_DrawText(int x,int y,boolean direct,char* string)
{
   (void)direct;
   int c;
   int w;

   while (*string) {
      c = toupper(*string) - HU_FONTSTART;
      string++;
      if (c < 0 || c> HU_FONTSIZE) {
         x += 4;
         continue;
      }

      w = SHORT (hu_font[c].width);
      if (x+w > SCREENWIDTH)
         break;

      // proff/nicolas 09/20/98 -- changed for hi-res
      // CPhipps - patch drawing updated, reformatted
      V_DrawNumPatch(x, y, 0, hu_font[c].lumpnum, CR_DEFAULT, VPT_STRETCH);
      x+=w;
   }

   return x;
}

//
// M_WriteFile
//

boolean M_WriteFile(char const* name,void* source,int length)
{
   int handle;
   int count;

   handle = open ( name, O_WRONLY | O_CREAT | O_TRUNC);

   if (handle == -1)
      return false;

   count = write (handle, source, length);
   close (handle);

   if (count < length) {
//      unlink(name); // CPhipps - no corrupt data files around, they only confuse people.
      return false;
   }

   return true;
}


//
// M_ReadFile
//

int M_ReadFile(char const* name,byte** buffer)
{
   int handle, count, length;
   //  struct stat fileinfo;
   byte   *buf;

   handle = open (name, O_RDONLY);
   if ((handle < 0))
      I_Error ("M_ReadFile: Couldn't read file %s", name);

   length = filesize(handle);
   buf = Z_Malloc (length, PU_STATIC, NULL);
   count = read (handle, buf, length);
   close (handle);

   if (count < length)
      I_Error ("M_ReadFile: Couldn't read file %s", name);

   *buffer = buf;
   return length;
}

//
// DEFAULTS
//

int usemouse;
boolean    precache = true; /* if true, load all graphics at start */

extern int mousebfire;
extern int mousebstrafe;
extern int mousebforward;

extern int viewwidth;
extern int viewheight;
extern int fake_contrast;
extern int mouseSensitivity_horiz,mouseSensitivity_vert;  // killough

extern int realtic_clock_rate;         // killough 4/13/98: adjustable timer
extern int tran_filter_pct;            // killough 2/21/98

extern int screenblocks;
extern int showMessages;

#ifndef DJGPP
const char* musserver_filename;
const char* sndserver_filename;
const char* snd_device;
int         mus_pause_opt; // 0 = kill music, 1 = pause, 2 = continue
#endif

extern const char* chat_macros[];

extern int endoom_mode;
int X_opt;

extern const char* S_music_files[]; // cournia

/* cph - Some MBF stuff parked here for now
 * killough 10/98
 */
int map_point_coordinates;

default_t defaults[] =
   {
      {"Misc settings",{NULL, NULL},{0, NULL},UL,UL,def_none,ss_none, 0, 0},
      {"default_compatibility_level",{(void *)&default_compatibility_level, NULL},
       {-1, NULL},-1,MAX_COMPATIBILITY_LEVEL-1,
       def_int,ss_none, 0, 0}, // compatibility level" - CPhipps
//      {"realtic_clock_rate",{&realtic_clock_rate, NULL},{100, NULL},0,UL,
//       def_int,ss_none, 0, 0}, // percentage of normal speed (35 fps) realtic clock runs at
      {"max_player_corpse", {&bodyquesize, NULL}, {32, NULL},-1,UL,   // killough 2/8/98
       def_int,ss_none, 0, 0}, // number of dead bodies in view supported (-1 = no limit)
      {"flashing_hom",{&flashing_hom, NULL},{0, NULL},0,1,
       def_bool,ss_none, 0, 0}, // killough 10/98 - enable flashing HOM indicator
      {"demo_insurance",{&default_demo_insurance, NULL},{2, NULL},0,2,  // killough 3/31/98
       def_int,ss_none, 0, 0}, // 1=take special steps ensuring demo sync, 2=only during recordings
//      {"endoom_mode", {&endoom_mode, NULL},{5, NULL},0,7, // CPhipps - endoom flags
//       def_hex, ss_none, 0, 0}, // 0, +1 for colours, +2 for non-ascii chars, +4 for skip-last-line
      {"level_precache",{(void*)&precache, NULL},{1, NULL},0,1,
       def_bool,ss_none, 0, 0}, // precache level data?

      {"Files",{NULL, NULL},{0, NULL},UL,UL,def_none,ss_none, 0, 0},
      /* cph - MBF-like wad/deh/bex autoload code
       * POSIX targets need to get lumps from prboom.wad */
//      {"wadfile_1",{NULL,&wad_files[0]},{0,
#ifdef ALL_IN_ONE
//                                         ""
#else
//                                         "prboom.wad"
#endif
//                                        },UL,UL,def_str,ss_none, 0, 0},
//      {"wadfile_2",{NULL,&wad_files[1]},{0,""},UL,UL,def_str,ss_none, 0, 0},
//      {"dehfile_1",{NULL,&deh_files[0]},{0,""},UL,UL,def_str,ss_none, 0, 0},
//      {"dehfile_2",{NULL,&deh_files[1]},{0,""},UL,UL,def_str,ss_none, 0, 0},

      {"Game settings",{NULL, NULL},{0, NULL},UL,UL,def_none,ss_none, 0, 0},
      {"default_skill",{&defaultskill, NULL},{3, NULL},1,5, // jff 3/24/98 allow default skill setting
       def_int,ss_none, 0, 0}, // selects default skill 1=TYTD 2=NTR 3=HMP 4=UV 5=NM
      {"weapon_recoil",{&default_weapon_recoil, NULL},{0, NULL},0,1,
       def_bool,ss_weap, &weapon_recoil, 0},
      /* killough 10/98 - toggle between SG/SSG and Fist/Chainsaw */
      {"doom_weapon_toggles",{&doom_weapon_toggles, NULL}, {1, NULL}, 0, 1,
       def_bool, ss_weap , 0, 0},
      {"player_bobbing",{&default_player_bobbing, NULL},{1, NULL},0,1,         // phares 2/25/98
       def_bool,ss_weap, &player_bobbing, 0},
      {"monsters_remember",{&default_monsters_remember, NULL},{1, NULL},0,1,   // killough 3/1/98
       def_bool,ss_enem, &monsters_remember, 0},
      /* MBF AI enhancement options */
      {"monster_infighting",{&default_monster_infighting, NULL}, {1, NULL}, 0, 1,
       def_bool, ss_enem, &monster_infighting, 0},
      {"monster_backing",{&default_monster_backing, NULL}, {0, NULL}, 0, 1,
       def_bool, ss_enem, &monster_backing, 0},
      {"monster_avoid_hazards",{&default_monster_avoid_hazards, NULL}, {1, NULL}, 0, 1,
       def_bool, ss_enem, &monster_avoid_hazards, 0},
      {"monkeys",{&default_monkeys, NULL}, {0, NULL}, 0, 1,
       def_bool, ss_enem, &monkeys, 0},
      {"monster_friction",{&default_monster_friction, NULL}, {1, NULL}, 0, 1,
       def_bool, ss_enem, &monster_friction, 0},
      {"help_friends",{&default_help_friends, NULL}, {1, NULL}, 0, 1,
       def_bool, ss_enem, &help_friends, 0},
#ifdef DOGS
      {"player_helpers",{&default_dogs, NULL}, {0, NULL}, 0, 3,
       def_bool, ss_enem , 0, 0},
      {"friend_distance",{&default_distfriend, NULL}, {128, NULL}, 0, 999,
       def_int, ss_enem, &distfriend, 0},
      {"dog_jumping",{&default_dog_jumping, NULL}, {1, NULL}, 0, 1,
       def_bool, ss_enem, &dog_jumping, 0},
#endif
      /* End of MBF AI extras */
      {"sts_always_red",{&sts_always_red, NULL},{1, NULL},0,1, // no color changes on status bar
       def_bool,ss_stat, 0, 0},
      {"sts_pct_always_gray",{&sts_pct_always_gray, NULL},{0, NULL},0,1, // 2/23/98 chg default
       def_bool,ss_stat, 0, 0}, // makes percent signs on status bar always gray
      {"sts_traditional_keys",{&sts_traditional_keys, NULL},{0, NULL},0,1,  // killough 2/28/98
       def_bool,ss_stat,0,0}, // disables doubled card and skull key display on status bar
//      {"traditional_menu",{&traditional_menu, NULL},{1, NULL},0,1,
//       def_bool,ss_none, 0, 0}, // force use of Doom's main menu ordering // killough 4/17/98
      {"show_messages",{&showMessages, NULL},{1, NULL},0,1,
       def_bool,ss_none,0,0}, // enables message display
      {"autorun",{&autorun, NULL},{0, NULL},0,1,  // killough 3/6/98: preserve autorun across games
       def_bool,ss_none,0,0},

      {"Compatibility settings",{NULL, NULL},{0, NULL},UL,UL,def_none,ss_none, 0, 0},
      {"comp_zombie",{&default_comp[comp_zombie], NULL},{0, NULL},0,1,def_bool,ss_comp,&comp[comp_zombie], 0},
      {"comp_infcheat",{&default_comp[comp_infcheat], NULL},{0, NULL},0,1,def_bool,ss_comp,&comp[comp_infcheat], 0},
      {"comp_stairs",{&default_comp[comp_stairs], NULL},{0, NULL},0,1,def_bool,ss_comp,&comp[comp_stairs], 0},
      {"comp_telefrag",{&default_comp[comp_telefrag], NULL},{0, NULL},0,1,def_bool,ss_comp,&comp[comp_telefrag], 0},
      {"comp_dropoff",{&default_comp[comp_dropoff], NULL},{0, NULL},0,1,def_bool,ss_comp,&comp[comp_dropoff], 0},
      {"comp_falloff",{&default_comp[comp_falloff], NULL},{0, NULL},0,1,def_bool,ss_comp,&comp[comp_falloff], 0},
      {"comp_staylift",{&default_comp[comp_staylift], NULL},{0, NULL},0,1,def_bool,ss_comp,&comp[comp_staylift], 0},
      {"comp_doorstuck",{&default_comp[comp_doorstuck], NULL},{0, NULL},0,1,def_bool,ss_comp,&comp[comp_doorstuck], 0},
      {"comp_pursuit",{&default_comp[comp_pursuit], NULL},{0, NULL},0,1,def_bool,ss_comp,&comp[comp_pursuit], 0},
      {"comp_vile",{&default_comp[comp_vile], NULL},{0, NULL},0,1,def_bool,ss_comp,&comp[comp_vile], 0},
      {"comp_pain",{&default_comp[comp_pain], NULL},{0, NULL},0,1,def_bool,ss_comp,&comp[comp_pain], 0},
      {"comp_skull",{&default_comp[comp_skull], NULL},{0, NULL},0,1,def_bool,ss_comp,&comp[comp_skull], 0},
      {"comp_blazing",{&default_comp[comp_blazing], NULL},{0, NULL},0,1,def_bool,ss_comp,&comp[comp_blazing], 0},
      {"comp_doorlight",{&default_comp[comp_doorlight], NULL},{0, NULL},0,1,def_bool,ss_comp,&comp[comp_doorlight], 0},
      {"comp_god",{&default_comp[comp_god], NULL},{0, NULL},0,1,def_bool,ss_comp,&comp[comp_god], 0},
      {"comp_skymap",{&default_comp[comp_skymap], NULL},{0, NULL},0,1,def_bool,ss_comp,&comp[comp_skymap], 0},
      {"comp_floors",{&default_comp[comp_floors], NULL},{0, NULL},0,1,def_bool,ss_comp,&comp[comp_floors], 0},
      {"comp_model",{&default_comp[comp_model], NULL},{0, NULL},0,1,def_bool,ss_comp,&comp[comp_model], 0},
      {"comp_zerotags",{&default_comp[comp_zerotags], NULL},{0, NULL},0,1,def_bool,ss_comp,&comp[comp_zerotags], 0},
      {"comp_moveblock",{&default_comp[comp_moveblock], NULL},{0, NULL},0,1,def_bool,ss_comp,&comp[comp_moveblock], 0},
      {"comp_sound",{&default_comp[comp_sound], NULL},{0, NULL},0,1,def_bool,ss_comp,&comp[comp_sound], 0},

      {"Sound settings",{NULL, NULL},{0, NULL},UL,UL,def_none,ss_none, 0, 0},
//      {"sound_card",{&snd_card, NULL},{-1, NULL},-1,7,       // jff 1/18/98 allow Allegro drivers
//       def_int,ss_none, 0, 0}, // select sounds driver (DOS), -1 is autodetect, 0 is none; in Linux, non-zero enables sound
//      {"music_card",{&mus_card, NULL},{-1, NULL},-1,9,       //  to be set,  -1 = autodetect
//       def_int,ss_none, 0, 0}, // select music driver (DOS), -1 is autodetect, 0 is none"; in Linux, non-zero enables music
      {"pitched_sounds",{&pitched_sounds, NULL},{0, NULL},0,1, // killough 2/21/98
       def_bool,ss_none, 0, 0}, // enables variable pitch in sound effects (from id's original code)
//      {"samplerate",{&snd_samplerate, NULL},{22050, NULL},11025,48000, def_int,ss_none, 0, 0},
      {"sfx_volume",{&snd_SfxVolume, NULL},{8, NULL},0,15, def_int,ss_none, 0, 0},
      {"music_volume",{&snd_MusicVolume, NULL},{8, NULL},0,15, def_int,ss_none, 0, 0},
      {"mus_pause_opt",{&mus_pause_opt, NULL},{2, NULL},0,2, // CPhipps - music pausing
       def_int, ss_none, 0, 0}, // 0 = kill music when paused, 1 = pause music, 2 = let music continue
      {"sounddev", {NULL,&snd_device}, {0,"/dev/dsp"},UL,UL,
       def_str,ss_none, 0, 0}, // sound output device (UNIX)
      {"snd_channels",{&default_numChannels, NULL},{4, NULL},1,32,
       def_int,ss_none, 0, 0}, // number of audio events simultaneously // killough

      {"Video settings",{NULL, NULL},{0, NULL},UL,UL,def_none,ss_none, 0, 0},
      // CPhipps - default screensize for targets that support high-res
 /*     {"screen_width",{&desired_screenwidth, NULL},{320, NULL}, 320, 1600,
       def_int,ss_none, 0, 0},
      {"screen_height",{&desired_screenheight, NULL},{200, NULL},200,1200,
       def_int,ss_none, 0, 0},*/
      {"fake_contrast",{&fake_contrast, NULL},{1, NULL},0,1,
       def_bool,ss_none, 0, 0}, /* cph - allow crappy fake contrast to be disabled */
//      {"use_fullscreen",{&use_fullscreen, NULL},{1, NULL},0,1, /* proff 21/05/2000 */
//       def_bool,ss_none, 0, 0},
//      {"use_doublebuffer",{&use_doublebuffer, NULL},{1, NULL},0,1,             // proff 2001-7-4
//       def_bool,ss_none, 0, 0}, // enable doublebuffer to avoid display tearing (fullscreen)
      {"translucency",{&default_translucency, NULL},{1, NULL},0,1,   // phares
       def_bool,ss_none, 0, 0}, // enables translucency
      {"tran_filter_pct",{&tran_filter_pct, NULL},{66, NULL},0,100,         // killough 2/21/98
       def_int,ss_none, 0, 0}, // set percentage of foreground/background translucency mix
      {"screenblocks",{&screenblocks, NULL},{9, NULL},3,11,
       def_int,ss_none, 0, 0},
      {"usegamma",{&usegamma, NULL},{1, NULL},0,4, //jff 3/6/98 fix erroneous upper limit in range
       def_int,ss_none, 0, 0}, // gamma correction level // killough 1/18/98
      {"X_options",{&X_opt, NULL},{0, NULL},0,3, // CPhipps - misc X options
       def_hex,ss_none, 0, 0}, // X options, see l_video_x.c

      {"Mouse settings",{NULL, NULL},{0, NULL},UL,UL,def_none,ss_none, 0, 0},
      {"use_mouse",{&usemouse, NULL},{1, NULL},0,1,
       def_bool,ss_none, 0, 0}, // enables use of mouse with DOOM
      //jff 4/3/98 allow unlimited sensitivity
//      {"mouse_sensitivity_horiz",{&mouseSensitivity_horiz, NULL},{10, NULL},0,UL,
//       def_int,ss_none, 0, 0}, /* adjust horizontal (x) mouse sensitivity killough/mead */
      //jff 4/3/98 allow unlimited sensitivity
//      {"mouse_sensitivity_vert",{&mouseSensitivity_vert, NULL},{10, NULL},0,UL,
//       def_int,ss_none, 0, 0}, /* adjust vertical (y) mouse sensitivity killough/mead */
      //jff 3/8/98 allow -1 in mouse bindings to disable mouse function
      {"mouseb_fire",{&mousebfire, NULL},{0, NULL},-1,MAX_MOUSEB,
       def_int,ss_keys, 0, 0}, // mouse button number to use for fire
      {"mouseb_strafe",{&mousebstrafe, NULL},{1, NULL},-1,MAX_MOUSEB,
       def_int,ss_keys, 0, 0}, // mouse button number to use for strafing
      {"mouseb_forward",{&mousebforward, NULL},{2, NULL},-1,MAX_MOUSEB,
       def_int,ss_keys, 0, 0}, // mouse button number to use for forward motion
      //jff 3/8/98 end of lower range change for -1 allowed in mouse binding

      // For key bindings, the values stored in the key_* variables       // phares
      // are the internal Doom Codes. The values stored in the default.cfg
      // file are the keyboard codes.
      // CPhipps - now they're the doom codes, so default.cfg can be portable

      {"Key bindings",{NULL, NULL},{0, NULL},UL,UL,def_none,ss_none, 0, 0},
      {"key_right",       {&key_right, NULL},          {KEY_RIGHTARROW, NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to turn right
      {"key_left",        {&key_left, NULL},           {KEY_LEFTARROW, NULL} ,
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to turn left
      {"key_up",          {&key_up, NULL},             {KEY_UPARROW, NULL}   ,
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to move forward
      {"key_down",        {&key_down, NULL},           {KEY_DOWNARROW, NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to move backward
/*      {"key_menu_right",  {&key_menu_right, NULL},     {KEY_RIGHTARROW, NULL},// phares 3/7/98
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to move right in a menu  //     |
      {"key_menu_left",   {&key_menu_left, NULL},      {KEY_LEFTARROW} ,//     V
       0,MAX_KEY,def_key,ss_keys, NULL}, // key to move left in a menu
      {"key_menu_up",     {&key_menu_up, NULL},        {KEY_UPARROW,NULL}   ,
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to move up in a menu
      {"key_menu_down",   {&key_menu_down, NULL},      {KEY_DOWNARROW, NULL} ,
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to move down in a menu
      {"key_menu_backspace",{&key_menu_backspace, NULL},{KEY_BACKSPACE, NULL} ,
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // delete key in a menu
      {"key_menu_escape", {&key_menu_escape, NULL},    {KEY_ESCAPE, NULL}    ,
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to leave a menu      ,   // phares 3/7/98
      {"key_menu_enter",  {&key_menu_enter, NULL},     {KEY_ENTER, NULL}     ,
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to select from menu
*/
      {"key_strafeleft",  {&key_strafeleft, NULL},     {',', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to strafe left
      {"key_straferight", {&key_straferight, NULL},    {'.', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to strafe right

      {"key_fire",        {&key_fire, NULL},           {KEY_RCTRL, NULL}     ,
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // duh
      {"key_use",         {&key_use, NULL},            {' ', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to open a door, use a switch
      {"key_strafe",      {&key_strafe, NULL},         {'s', NULL}      ,
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to use with arrows to strafe
      {"key_speed",       {&key_speed, NULL},          {KEY_RSHIFT, NULL}    ,
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to run

     {"key_savegame",    {&key_savegame, NULL},       {KEY_F2, NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to save current game
      {"key_loadgame",    {&key_loadgame, NULL},       {KEY_F3, NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to restore from saved games
      {"key_soundvolume", {&key_soundvolume, NULL},    {KEY_F4, NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to bring up sound controls
      {"key_hud",         {&key_hud, NULL},            {KEY_F5, NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to adjust HUD
      {"key_quicksave",   {&key_quicksave, NULL},      {KEY_F6, NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to to quicksave
      {"key_endgame",     {&key_endgame, NULL},        {KEY_F7, NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to end the game
      {"key_messages",    {&key_messages, NULL},       {KEY_F8, NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to toggle message enable
      {"key_quickload",   {&key_quickload, NULL},      {KEY_F9, NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to load from quicksave
      {"key_quit",        {&key_quit, NULL},           {KEY_F10, NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to quit game
      {"key_gamma",       {&key_gamma, NULL},          {KEY_F11, NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to adjust gamma correction
      {"key_spy",         {&key_spy, NULL},            {KEY_F12, NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to view from another coop player's view
      {"key_pause",       {&key_pause, NULL},          {KEY_PAUSE, NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to pause the game
      {"key_autorun",     {&key_autorun, NULL},        {KEY_CAPSLOCK, NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to toggle always run mode
      {"key_chat",        {&key_chat, NULL},           {'t', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to enter a chat message
      {"key_backspace",   {&key_backspace, NULL},      {KEY_BACKSPACE, NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // backspace key
      {"key_enter",       {&key_enter, NULL},          {KEY_ENTER, NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to select from menu or see last message
      {"key_map",         {&key_map, NULL},            {KEY_TAB, NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to toggle automap display
      {"key_map_right",   {&key_map_right, NULL},      {KEY_RIGHTARROW, NULL},// phares 3/7/98
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to shift automap right   //     |
      {"key_map_left",    {&key_map_left, NULL},       {KEY_LEFTARROW, NULL},//     V
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to shift automap left
      {"key_map_up",      {&key_map_up, NULL},         {KEY_UPARROW, NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to shift automap up
      {"key_map_down",    {&key_map_down, NULL},       {KEY_DOWNARROW, NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to shift automap down
      {"key_map_zoomin",  {&key_map_zoomin, NULL},      {'=', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to enlarge automap
      {"key_map_zoomout", {&key_map_zoomout, NULL},     {'-', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to reduce automap
      {"key_map_gobig",   {&key_map_gobig, NULL},       {'0', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0},  // key to get max zoom for automap
      {"key_map_follow",  {&key_map_follow, NULL},      {'f', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to toggle follow mode
      {"key_map_mark",    {&key_map_mark, NULL},        {'m', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to drop a marker on automap
      {"key_map_clear",   {&key_map_clear, NULL},       {'c', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to clear all markers on automap
      {"key_map_grid",    {&key_map_grid, NULL},        {'g', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to toggle grid display over automap
      {"key_map_rotate",  {&key_map_rotate, NULL},      {'r', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to toggle rotating the automap to match the player's orientation
      {"key_map_overlay", {&key_map_overlay, NULL},     {'o', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to toggle overlaying the automap on the rendered display
      {"key_reverse",     {&key_reverse, NULL},         {'/', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to spin 180 instantly
      {"key_zoomin",      {&key_zoomin, NULL},          {'=', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to enlarge display
      {"key_zoomout",     {&key_zoomout, NULL},         {'-', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to reduce display
      {"key_chatplayer1", {&destination_keys[0], NULL}, {'g', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to chat with player 1
      // killough 11/98: fix 'i'/'b' reversal
      {"key_chatplayer2", {&destination_keys[1], NULL}, {'i', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to chat with player 2
      {"key_chatplayer3", {&destination_keys[2], NULL}, {'b', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to chat with player 3
      {"key_chatplayer4", {&destination_keys[3], NULL}, {'r', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to chat with player 4
      {"key_weapon",{&key_weapon, NULL},    {'w', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to toggle between two most preferred weapons with ammo
      {"key_weapontoggle",{&key_weapontoggle, NULL},    {'0', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to toggle between two most preferred weapons with ammo
      {"key_weapon1",     {&key_weapon1, NULL},         {'1', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to switch to weapon 1 (fist/chainsaw)
      {"key_weapon2",     {&key_weapon2, NULL},         {'2', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to switch to weapon 2 (pistol)
      {"key_weapon3",     {&key_weapon3, NULL},         {'3', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to switch to weapon 3 (supershotgun/shotgun)
      {"key_weapon4",     {&key_weapon4, NULL},         {'4', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to switch to weapon 4 (chaingun)
      {"key_weapon5",     {&key_weapon5, NULL},         {'5', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to switch to weapon 5 (rocket launcher)
      {"key_weapon6",     {&key_weapon6, NULL},         {'6', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to switch to weapon 6 (plasma rifle)
      {"key_weapon7",     {&key_weapon7, NULL},         {'7', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to switch to weapon 7 (bfg9000)         //    ^
      {"key_weapon8",     {&key_weapon8, NULL},         {'8', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to switch to weapon 8 (chainsaw)        //    |
      {"key_weapon9",     {&key_weapon9, NULL},         {'9', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to switch to weapon 9 (supershotgun)    // phares

      // killough 2/22/98: screenshot key
      {"key_screenshot",  {&key_screenshot, NULL},      {'*', NULL},
       0,MAX_KEY,def_key,ss_keys, 0, 0}, // key to take a screenshot

 /*     {"Joystick settings",{NULL, NULL},{0, NULL},UL,UL,def_none,ss_none, 0, 0},
      {"use_joystick",{&usejoystick, NULL},{0, NULL},0,2,
       def_int,ss_none, 0, 0}, // number of joystick to use (0 for none)
      {"joy_left",{&joyleft, NULL},{0, NULL},  UL,UL,def_int,ss_none, 0, 0},
      {"joy_right",{&joyright, NULL},{0, NULL},UL,UL,def_int,ss_none, 0, 0},
      {"joy_up",  {&joyup, NULL},  {0, NULL},  UL,UL,def_int,ss_none, 0, 0},
      {"joy_down",{&joydown, NULL},{0, NULL},  UL,UL,def_int,ss_none, 0, 0},
      {"joyb_fire",{&joybfire, NULL},{0, NULL},0,UL,
       def_int,ss_keys, 0, 0}, // joystick button number to use for fire
      {"joyb_strafe",{&joybstrafe, NULL},{1, NULL},0,UL,
       def_int,ss_keys, 0, 0}, // joystick button number to use for strafing
      {"joyb_speed",{&joybspeed, NULL},{2, NULL},0,UL,
       def_int,ss_keys, 0, 0}, // joystick button number to use for running
      {"joyb_use",{&joybuse, NULL},{3, NULL},0,UL,
       def_int,ss_keys, 0, 0}, // joystick button number to use for use/open
*/
      {"Chat macros",{NULL, NULL},{0, NULL},UL,UL,def_none,ss_none, 0, 0},
      {"chatmacro0", {0,&chat_macros[0]}, {0,HUSTR_CHATMACRO0},UL,UL,
       def_str,ss_chat, 0, 0}, // chat string associated with 0 key
      {"chatmacro1", {0,&chat_macros[1]}, {0,HUSTR_CHATMACRO1},UL,UL,
       def_str,ss_chat, 0, 0}, // chat string associated with 1 key
      {"chatmacro2", {0,&chat_macros[2]}, {0,HUSTR_CHATMACRO2},UL,UL,
       def_str,ss_chat, 0, 0}, // chat string associated with 2 key
      {"chatmacro3", {0,&chat_macros[3]}, {0,HUSTR_CHATMACRO3},UL,UL,
       def_str,ss_chat, 0, 0}, // chat string associated with 3 key
      {"chatmacro4", {0,&chat_macros[4]}, {0,HUSTR_CHATMACRO4},UL,UL,
       def_str,ss_chat, 0, 0}, // chat string associated with 4 key
      {"chatmacro5", {0,&chat_macros[5]}, {0,HUSTR_CHATMACRO5},UL,UL,
       def_str,ss_chat, 0, 0}, // chat string associated with 5 key
      {"chatmacro6", {0,&chat_macros[6]}, {0,HUSTR_CHATMACRO6},UL,UL,
       def_str,ss_chat, 0, 0}, // chat string associated with 6 key
      {"chatmacro7", {0,&chat_macros[7]}, {0,HUSTR_CHATMACRO7},UL,UL,
       def_str,ss_chat, 0, 0}, // chat string associated with 7 key
      {"chatmacro8", {0,&chat_macros[8]}, {0,HUSTR_CHATMACRO8},UL,UL,
       def_str,ss_chat, 0, 0}, // chat string associated with 8 key
      {"chatmacro9", {0,&chat_macros[9]}, {0,HUSTR_CHATMACRO9},UL,UL,
       def_str,ss_chat, 0, 0}, // chat string associated with 9 key

      {"Automap settings",{NULL, NULL},{0, NULL},UL,UL,def_none,ss_none, 0, 0},
      //jff 1/7/98 defaults for automap colors
      //jff 4/3/98 remove -1 in lower range, 0 now disables new map features
      {"mapcolor_back", {&mapcolor_back, NULL}, {247, NULL},0,255,  // black //jff 4/6/98 new black
       def_colour,ss_auto, 0, 0}, // color used as background for automap
      {"mapcolor_grid", {&mapcolor_grid, NULL}, {104, NULL},0,255,  // dk gray
       def_colour,ss_auto, 0, 0}, // color used for automap grid lines
      {"mapcolor_wall", {&mapcolor_wall, NULL}, {23, NULL},0,255,   // red-brown
       def_colour,ss_auto, 0, 0}, // color used for one side walls on automap
      {"mapcolor_fchg", {&mapcolor_fchg, NULL}, {55, NULL},0,255,   // lt brown
       def_colour,ss_auto, 0, 0}, // color used for lines floor height changes across
      {"mapcolor_cchg", {&mapcolor_cchg, NULL}, {215, NULL},0,255,  // orange
       def_colour,ss_auto, 0, 0}, // color used for lines ceiling height changes across
      {"mapcolor_clsd", {&mapcolor_clsd, NULL}, {208, NULL},0,255,  // white
       def_colour,ss_auto, 0, 0}, // color used for lines denoting closed doors, objects
      {"mapcolor_rkey", {&mapcolor_rkey, NULL}, {175, NULL},0,255,  // red
       def_colour,ss_auto, 0, 0}, // color used for red key sprites
      {"mapcolor_bkey", {&mapcolor_bkey, NULL}, {204, NULL},0,255,  // blue
       def_colour,ss_auto, 0, 0}, // color used for blue key sprites
      {"mapcolor_ykey", {&mapcolor_ykey, NULL}, {231, NULL},0,255,  // yellow
       def_colour,ss_auto, 0, 0}, // color used for yellow key sprites
      {"mapcolor_rdor", {&mapcolor_rdor, NULL}, {175, NULL},0,255,  // red
       def_colour,ss_auto, 0, 0}, // color used for closed red doors
      {"mapcolor_bdor", {&mapcolor_bdor, NULL}, {204, NULL},0,255,  // blue
       def_colour,ss_auto, 0, 0}, // color used for closed blue doors
      {"mapcolor_ydor", {&mapcolor_ydor, NULL}, {231, NULL},0,255,  // yellow
       def_colour,ss_auto, 0, 0}, // color used for closed yellow doors
      {"mapcolor_tele", {&mapcolor_tele, NULL}, {119, NULL},0,255,  // dk green
       def_colour,ss_auto, 0, 0}, // color used for teleporter lines
      {"mapcolor_secr", {&mapcolor_secr, NULL}, {252, NULL},0,255,  // purple
       def_colour,ss_auto, 0, 0}, // color used for lines around secret sectors
      {"mapcolor_exit", {&mapcolor_exit, NULL}, {0, NULL},0,255,    // none
       def_colour,ss_auto, 0, 0}, // color used for exit lines
      {"mapcolor_unsn", {&mapcolor_unsn, NULL}, {104, NULL},0,255,  // dk gray
       def_colour,ss_auto, 0, 0}, // color used for lines not seen without computer map
      {"mapcolor_flat", {&mapcolor_flat, NULL}, {88, NULL},0,255,   // lt gray
       def_colour,ss_auto, 0, 0}, // color used for lines with no height changes
      {"mapcolor_sprt", {&mapcolor_sprt, NULL}, {112, NULL},0,255,  // green
       def_colour,ss_auto, 0, 0}, // color used as things
      {"mapcolor_item", {&mapcolor_item, NULL}, {231, NULL},0,255,  // yellow
       def_colour,ss_auto, 0, 0}, // color used for counted items
      {"mapcolor_hair", {&mapcolor_hair, NULL}, {208, NULL},0,255,  // white
       def_colour,ss_auto, 0, 0}, // color used for dot crosshair denoting center of map
      {"mapcolor_sngl", {&mapcolor_sngl, NULL}, {208, NULL},0,255,  // white
       def_colour,ss_auto, 0, 0}, // color used for the single player arrow
/*      {"mapcolor_me",   {&mapcolor_me, NULL}, {112, NULL},0,255, // green
       def_colour,ss_auto, 0, 0}, // your (player) colour*/
      {"mapcolor_frnd",   {&mapcolor_frnd, NULL}, {112, NULL},0,255,
       def_colour,ss_auto, 0, 0},
      //jff 3/9/98 add option to not show secrets til after found
      {"map_secret_after", {&map_secret_after, NULL}, {0, NULL},0,1, // show secret after gotten
       def_bool,ss_auto, 0, 0}, // prevents showing secret sectors till after entered
      {"map_point_coord", {&map_point_coordinates, NULL}, {0, NULL},0,1,
       def_bool,ss_auto, 0, 0},
      //jff 1/7/98 end additions for automap
      {"automapmode", {(void*)&automapmode, NULL}, {0, NULL}, 0, 31, // CPhipps - remember automap mode
       def_hex,ss_none, 0, 0}, // automap mode

      {"Heads-up display settings",{NULL, NULL},{0, NULL},UL,UL,def_none,ss_none, 0, 0},
      //jff 2/16/98 defaults for color ranges in hud and status
      {"hudcolor_titl", {&hudcolor_titl, NULL}, {5, NULL},0,9,  // gold range
       def_int,ss_auto, 0, 0}, // color range used for automap level title
      {"hudcolor_xyco", {&hudcolor_xyco, NULL}, {3, NULL},0,9,  // green range
       def_int,ss_auto, 0, 0}, // color range used for automap coordinates
      {"hudcolor_mesg", {&hudcolor_mesg, NULL}, {6, NULL},0,9,  // red range
       def_int,ss_mess, 0, 0}, // color range used for messages during play
      {"hudcolor_chat", {&hudcolor_chat, NULL}, {5, NULL},0,9,  // gold range
       def_int,ss_mess, 0, 0}, // color range used for chat messages and entry
      {"hudcolor_list", {&hudcolor_list, NULL}, {5, NULL},0,9,  // gold range  //jff 2/26/98
       def_int,ss_mess, 0, 0}, // color range used for message review
      {"hud_msg_lines", {&hud_msg_lines, NULL}, {1, NULL},1,16,  // 1 line scrolling window
       def_int,ss_mess, 0, 0}, // number of messages in review display (1=disable)
      {"hud_list_bgon", {&hud_list_bgon, NULL}, {0, NULL},0,1,  // solid window bg ena //jff 2/26/98
       def_bool,ss_mess, 0, 0}, // enables background window behind message review
      {"hud_distributed",{&hud_distributed, NULL},{0, NULL},0,1, // hud broken up into 3 displays //jff 3/4/98
       def_bool,ss_none, 0, 0}, // splits HUD into three 2 line displays

      {"health_red",    {&health_red, NULL}, {25, NULL},0,200, // below is red
       def_int,ss_stat, 0, 0}, // amount of health for red to yellow transition
      {"health_yellow", {&health_yellow, NULL}, {50, NULL},0,200, // below is yellow
       def_int,ss_stat, 0, 0}, // amount of health for yellow to green transition
      {"health_green",  {&health_green, NULL}, {100, NULL},0,200,// below is green, above blue
       def_int,ss_stat, 0, 0}, // amount of health for green to blue transition
      {"armor_red",     {&armor_red, NULL}, {25, NULL},0,200, // below is red
       def_int,ss_stat, 0, 0}, // amount of armor for red to yellow transition
      {"armor_yellow",  {&armor_yellow, NULL}, {50, NULL},0,200, // below is yellow
       def_int,ss_stat, 0, 0}, // amount of armor for yellow to green transition
      {"armor_green",   {&armor_green, NULL}, {100, NULL},0,200,// below is green, above blue
       def_int,ss_stat, 0, 0}, // amount of armor for green to blue transition
      {"ammo_red",      {&ammo_red, NULL}, {25, NULL},0,100, // below 25% is red
       def_int,ss_stat, 0, 0}, // percent of ammo for red to yellow transition
      {"ammo_yellow",   {&ammo_yellow, NULL}, {50, NULL},0,100, // below 50% is yellow, above green
       def_int,ss_stat, 0, 0}, // percent of ammo for yellow to green transition

      //jff 2/16/98 HUD and status feature controls
      {"hud_active",    {&hud_active, NULL}, {1, NULL},0,2, // 0=off, 1=small, 2=full
       def_int,ss_none, 0, 0}, // 0 for HUD off, 1 for HUD small, 2 for full HUD
      //jff 2/23/98
      {"hud_displayed", {&hud_displayed, NULL},  {0, NULL},0,1, // whether hud is displayed
       def_bool,ss_none, 0, 0}, // enables display of HUD
      {"hud_nosecrets", {&hud_nosecrets, NULL},  {0, NULL},0,1, // no secrets/items/kills HUD line
       def_bool,ss_stat, 0, 0}, // disables display of kills/items/secrets on HUD

      {"Weapon preferences",{NULL, NULL},{0, NULL},UL,UL,def_none,ss_none, 0, 0},
      // killough 2/8/98: weapon preferences set by user:
      {"weapon_choice_1", {&weapon_preferences[0][0], NULL}, {6, NULL}, 0,9,
       def_int,ss_weap, 0, 0}, // first choice for weapon (best)
      {"weapon_choice_2", {&weapon_preferences[0][1], NULL}, {9, NULL}, 0,9,
       def_int,ss_weap, 0, 0}, // second choice for weapon
      {"weapon_choice_3", {&weapon_preferences[0][2], NULL}, {4, NULL}, 0,9,
       def_int,ss_weap, 0, 0}, // third choice for weapon
      {"weapon_choice_4", {&weapon_preferences[0][3], NULL}, {3, NULL}, 0,9,
       def_int,ss_weap, 0, 0}, // fourth choice for weapon
      {"weapon_choice_5", {&weapon_preferences[0][4], NULL}, {2, NULL}, 0,9,
       def_int,ss_weap, 0, 0}, // fifth choice for weapon
      {"weapon_choice_6", {&weapon_preferences[0][5], NULL}, {8, NULL}, 0,9,
       def_int,ss_weap, 0, 0}, // sixth choice for weapon
      {"weapon_choice_7", {&weapon_preferences[0][6], NULL}, {5, NULL}, 0,9,
       def_int,ss_weap, 0, 0}, // seventh choice for weapon
      {"weapon_choice_8", {&weapon_preferences[0][7], NULL}, {7, NULL}, 0,9,
       def_int,ss_weap, 0, 0}, // eighth choice for weapon
      {"weapon_choice_9", {&weapon_preferences[0][8], NULL}, {1, NULL}, 0,9,
       def_int,ss_weap, 0, 0}, // ninth choice for weapon (worst)

/*      // cournia - support for arbitrary music file (defaults are mp3)
      {"Music", {NULL, NULL},{0, NULL},UL,UL,def_none,ss_none, 0, 0},
      {"mus_e1m1", {0,&S_music_files[mus_e1m1]}, {0,"e1m1.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_e1m2", {0,&S_music_files[mus_e1m2]}, {0,"e1m2.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_e1m3", {0,&S_music_files[mus_e1m3]}, {0,"e1m3.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_e1m4", {0,&S_music_files[mus_e1m4]}, {0,"e1m4.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_e1m5", {0,&S_music_files[mus_e1m5]}, {0,"e1m5.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_e1m6", {0,&S_music_files[mus_e1m6]}, {0,"e1m6.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_e1m7", {0,&S_music_files[mus_e1m7]}, {0,"e1m7.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_e1m8", {0,&S_music_files[mus_e1m8]}, {0,"e1m8.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_e1m9", {0,&S_music_files[mus_e1m9]}, {0,"e1m9.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_e2m1", {0,&S_music_files[mus_e2m1]}, {0,"e2m1.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_e2m2", {0,&S_music_files[mus_e2m2]}, {0,"e2m2.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_e2m3", {0,&S_music_files[mus_e2m3]}, {0,"e2m3.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_e2m4", {0,&S_music_files[mus_e2m4]}, {0,"e2m4.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_e2m5", {0,&S_music_files[mus_e2m5]}, {0,"e1m7.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_e2m6", {0,&S_music_files[mus_e2m6]}, {0,"e2m6.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_e2m7", {0,&S_music_files[mus_e2m7]}, {0,"e2m7.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_e2m8", {0,&S_music_files[mus_e2m8]}, {0,"e2m8.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_e2m9", {0,&S_music_files[mus_e2m9]}, {0,"e3m1.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_e3m1", {0,&S_music_files[mus_e3m1]}, {0,"e3m1.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_e3m2", {0,&S_music_files[mus_e3m2]}, {0,"e3m2.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_e3m3", {0,&S_music_files[mus_e3m3]}, {0,"e3m3.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_e3m4", {0,&S_music_files[mus_e3m4]}, {0,"e1m8.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_e3m5", {0,&S_music_files[mus_e3m5]}, {0,"e1m7.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_e3m6", {0,&S_music_files[mus_e3m6]}, {0,"e1m6.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_e3m7", {0,&S_music_files[mus_e3m7]}, {0,"e2m7.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_e3m8", {0,&S_music_files[mus_e3m8]}, {0,"e3m8.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_e3m9", {0,&S_music_files[mus_e3m9]}, {0,"e1m9.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_inter", {0,&S_music_files[mus_inter]}, {0,"e2m3.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_intro", {0,&S_music_files[mus_intro]}, {0,"intro.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_bunny", {0,&S_music_files[mus_bunny]}, {0,"bunny.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_victor", {0,&S_music_files[mus_victor]}, {0,"victor.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_introa", {0,&S_music_files[mus_introa]}, {0,"intro.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_runnin", {0,&S_music_files[mus_runnin]}, {0,"runnin.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_stalks", {0,&S_music_files[mus_stalks]}, {0,"stalks.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_countd", {0,&S_music_files[mus_countd]}, {0,"countd.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_betwee", {0,&S_music_files[mus_betwee]}, {0,"betwee.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_doom", {0,&S_music_files[mus_doom]}, {0,"doom.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_the_da", {0,&S_music_files[mus_the_da]}, {0,"the_da.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_shawn", {0,&S_music_files[mus_shawn]}, {0,"shawn.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_ddtblu", {0,&S_music_files[mus_ddtblu]}, {0,"ddtblu.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_in_cit", {0,&S_music_files[mus_in_cit]}, {0,"in_cit.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_dead", {0,&S_music_files[mus_dead]}, {0,"dead.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_stlks2", {0,&S_music_files[mus_stlks2]}, {0,"stalks.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_theda2", {0,&S_music_files[mus_theda2]}, {0,"the_da.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_doom2", {0,&S_music_files[mus_doom2]}, {0,"doom.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_ddtbl2", {0,&S_music_files[mus_ddtbl2]}, {0,"ddtblu.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_runni2", {0,&S_music_files[mus_runni2]}, {0,"runnin.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_dead2", {0,&S_music_files[mus_dead2]}, {0,"dead.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_stlks3", {0,&S_music_files[mus_stlks3]}, {0,"stalks.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_romero", {0,&S_music_files[mus_romero]}, {0,"romero.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_shawn2", {0,&S_music_files[mus_shawn2]}, {0,"shawn.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_messag", {0,&S_music_files[mus_messag]}, {0,"messag.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_count2", {0,&S_music_files[mus_count2]}, {0,"countd.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_ddtbl3", {0,&S_music_files[mus_ddtbl3]}, {0,"ddtblu.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_ampie", {0,&S_music_files[mus_ampie]}, {0,"ampie.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_theda3", {0,&S_music_files[mus_theda3]}, {0,"the_da.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_adrian", {0,&S_music_files[mus_adrian]}, {0,"adrian.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_messg2", {0,&S_music_files[mus_messg2]}, {0,"messag.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_romer2", {0,&S_music_files[mus_romer2]}, {0,"romero.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_tense", {0,&S_music_files[mus_tense]}, {0,"tense.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_shawn3", {0,&S_music_files[mus_shawn3]}, {0,"shawn.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_openin", {0,&S_music_files[mus_openin]}, {0,"openin.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_evil", {0,&S_music_files[mus_evil]}, {0,"evil.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_ultima", {0,&S_music_files[mus_ultima]}, {0,"ultima.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_read_m", {0,&S_music_files[mus_read_m]}, {0,"read_m.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_dm2ttl", {0,&S_music_files[mus_dm2ttl]}, {0,"dm2ttl.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
      {"mus_dm2int", {0,&S_music_files[mus_dm2int]}, {0,"dm2int.mp3"},UL,UL,
       def_str,ss_none, 0, 0},
*/
   };

int numdefaults;
//static const char* defaultfile; // CPhipps - static, const

//
// M_SaveDefaults
//

void M_SaveDefaults (void)
{
   int i,fd;

   fd = open (GAMEBASE"default.dfg", O_WRONLY|O_CREAT|O_TRUNC);
   if (fd<0)
      return; // can't write the file, but don't complain

   for (i=0 ; i<numdefaults ; i++)
      if(defaults[i].location.pi)
         write(fd,defaults[i].location.pi, sizeof(int));

   close (fd);
}

/*
 * M_LookupDefault
 *
 * cph - mimic MBF function for now. Yes it's crap.
 */

struct default_s *M_LookupDefault(const char *name)
{
   int i;
   for (i = 0 ; i < numdefaults ; i++)
      if ((defaults[i].type != def_none) && !strcmp(name, defaults[i].name))
         return &defaults[i];
   I_Error("M_LookupDefault: %s not found",name);
   return NULL;
}

//
// M_LoadDefaults
//

#define NUMCHATSTRINGS 10 // phares 4/13/98

void M_LoadDefaults (void)
{
     int   i;
   int fd;
     // set everything to base values

     numdefaults = sizeof(defaults)/sizeof(defaults[0]);
     for (i = 0 ; i < numdefaults ; i++) {
       if (defaults[i].location.ppsz)
         *defaults[i].location.ppsz = strdup(defaults[i].defaultvalue.psz);
       if (defaults[i].location.pi)
         *defaults[i].location.pi = defaults[i].defaultvalue.i;
     }

   fd = open (GAMEBASE"default.dfg", O_RDONLY);
   if (fd<0)
      return; // don't have anything to read

   for (i=0 ; i<numdefaults ; i++)
      if(defaults[i].location.pi)
         read(fd,defaults[i].location.pi, sizeof(int));

   close (fd);
}


//
// SCREEN SHOTS
//

// CPhipps - nasty but better than nothing
static boolean screenshot_write_error;

// jff 3/30/98 types and data structures for BMP output of screenshots
//
// killough 5/2/98:
// Changed type names to avoid conflicts with endianess functions

#define BI_RGB 0L

typedef unsigned long dword_t;
typedef long     long_t;
typedef unsigned char ubyte_t;

typedef struct tagBITMAPFILEHEADER
{
   unsigned short  bfType;
   dword_t bfSize;
   unsigned short  bfReserved1;
   unsigned short  bfReserved2;
   dword_t bfOffBits;
} PACKEDATTR BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
{
   dword_t biSize;
   long_t  biWidth;
   long_t  biHeight;
   unsigned short  biPlanes;
   unsigned short  biBitCount;
   dword_t biCompression;
   dword_t biSizeImage;
   long_t  biXPelsPerMeter;
   long_t  biYPelsPerMeter;
   dword_t biClrUsed;
   dword_t biClrImportant;
} PACKEDATTR BITMAPINFOHEADER;
#if 0
// jff 3/30/98 binary file write with error detection
// CPhipps - static, const on parameter
static void SafeWrite(const void *data, size_t size, size_t number, int st)
{
   /*  if (write(data,size,number,st)<number)
       screenshot_write_error = true; // CPhipps - made non-fatal*/
}
#endif
//
// WriteBMPfile
// jff 3/30/98 Add capability to write a .BMP file (256 color uncompressed)
//

// CPhipps - static, const on parameters
static void WriteBMPfile(const char* filename, const byte* data,
                         const int width, const int height, const byte* palette)
{
   (void)filename;
   (void)data;
   (void)width;
   (void)height;
   (void)palette;
   /*  int i,wid;
     BITMAPFILEHEADER bmfh;
     BITMAPINFOHEADER bmih;
     int fhsiz,ihsiz;
     FILE *st;
     char zero=0;
     ubyte_t c;

     fhsiz = sizeof(BITMAPFILEHEADER);
     ihsiz = sizeof(BITMAPINFOHEADER);
     wid = 4*((width+3)/4);
     //jff 4/22/98 add endian macros
     bmfh.bfType = SHORT(19778);
     bmfh.bfSize = LONG(fhsiz+ihsiz+256L*4+width*height);
     bmfh.bfReserved1 = SHORT(0);
     bmfh.bfReserved2 = SHORT(0);
     bmfh.bfOffBits = LONG(fhsiz+ihsiz+256L*4);

     bmih.biSize = LONG(ihsiz);
     bmih.biWidth = LONG(width);
     bmih.biHeight = LONG(height);
     bmih.biPlanes = SHORT(1);
     bmih.biBitCount = SHORT(8);
     bmih.biCompression = LONG(BI_RGB);
     bmih.biSizeImage = LONG(wid*height);
     bmih.biXPelsPerMeter = LONG(0);
     bmih.biYPelsPerMeter = LONG(0);
     bmih.biClrUsed = LONG(256);
     bmih.biClrImportant = LONG(256);

     st = fopen(filename,"wb");
     if (st!=NULL) {
       // write the header
       SafeWrite(&bmfh.bfType,sizeof(bmfh.bfType),1,st);
       SafeWrite(&bmfh.bfSize,sizeof(bmfh.bfSize),1,st);
       SafeWrite(&bmfh.bfReserved1,sizeof(bmfh.bfReserved1),1,st);
       SafeWrite(&bmfh.bfReserved2,sizeof(bmfh.bfReserved2),1,st);
       SafeWrite(&bmfh.bfOffBits,sizeof(bmfh.bfOffBits),1,st);

       SafeWrite(&bmih.biSize,sizeof(bmih.biSize),1,st);
       SafeWrite(&bmih.biWidth,sizeof(bmih.biWidth),1,st);
       SafeWrite(&bmih.biHeight,sizeof(bmih.biHeight),1,st);
       SafeWrite(&bmih.biPlanes,sizeof(bmih.biPlanes),1,st);
       SafeWrite(&bmih.biBitCount,sizeof(bmih.biBitCount),1,st);
       SafeWrite(&bmih.biCompression,sizeof(bmih.biCompression),1,st);
       SafeWrite(&bmih.biSizeImage,sizeof(bmih.biSizeImage),1,st);
       SafeWrite(&bmih.biXPelsPerMeter,sizeof(bmih.biXPelsPerMeter),1,st);
       SafeWrite(&bmih.biYPelsPerMeter,sizeof(bmih.biYPelsPerMeter),1,st);
       SafeWrite(&bmih.biClrUsed,sizeof(bmih.biClrUsed),1,st);
       SafeWrite(&bmih.biClrImportant,sizeof(bmih.biClrImportant),1,st);

       // write the palette, in blue-green-red order, gamma corrected
       for (i=0;i<768;i+=3) {
         c=gammatable[usegamma][palette[i+2]];
         SafeWrite(&c,sizeof(char),1,st);
         c=gammatable[usegamma][palette[i+1]];
         SafeWrite(&c,sizeof(char),1,st);
         c=gammatable[usegamma][palette[i+0]];
         SafeWrite(&c,sizeof(char),1,st);
         SafeWrite(&zero,sizeof(char),1,st);
       }

       for (i = 0 ; i < height ; i++)
         SafeWrite(data+(height-1-i)*width,sizeof(byte),wid,st);

       fclose(st);
     }*/
}

//
// M_ScreenShot
//
// Modified by Lee Killough so that any number of shots can be taken,
// the code is faster, and no annoying "screenshot" message appears.

// CPhipps - modified to use its own buffer for the image
//         - checks for the case where no file can be created (doesn't occur on POSIX systems, would on DOS)
//         - track errors better
//         - split into 2 functions

//
// M_DoScreenShot
// Takes a screenshot into the names file

void M_DoScreenShot (const char* fname)
{
   byte       *linear;
#ifndef GL_DOOM
   const byte *pal;
   int        pplump = W_GetNumForName("PLAYPAL");
#endif

   screenshot_write_error = false;

#ifdef GL_DOOM
   // munge planar buffer to linear
   // CPhipps - use a malloc()ed buffer instead of screens[2]
   gld_ReadScreen(linear = malloc(SCREENWIDTH * SCREENHEIGHT * 3));

   // save the bmp file

   WriteTGAfile
   (fname, linear, SCREENWIDTH, SCREENHEIGHT);
#else
   // munge planar buffer to linear
   // CPhipps - use a malloc()ed buffer instead of screens[2]
   I_ReadScreen(linear = malloc(SCREENWIDTH * SCREENHEIGHT));

   // killough 4/18/98: make palette stay around (PU_CACHE could cause crash)
   pal = W_CacheLumpNum (pplump);

   // save the bmp file

   WriteBMPfile
   (fname, linear, SCREENWIDTH, SCREENHEIGHT, pal);

   // cph - free the palette
   W_UnlockLumpNum(pplump);
#endif
   free(linear);
   // 1/18/98 killough: replace "SCREEN SHOT" acknowledgement with sfx

   if (screenshot_write_error)
      doom_printf("M_ScreenShot: Error writing screenshot");
}

void M_ScreenShot(void)
{
   static int shot;
   char       lbmname[32];
   int        startshot;

   screenshot_write_error = false;

   if (fileexists(".")) screenshot_write_error = true;

   startshot = shot; // CPhipps - prevent infinite loop

   do
      snprintf(lbmname,sizeof(lbmname),"DOOM%d.BMP", shot++);
   while (!fileexists(lbmname) && (shot != startshot) && (shot < 10000));

   if (!fileexists(lbmname)) screenshot_write_error = true;

   if (screenshot_write_error) {
      doom_printf ("M_ScreenShot: Couldn't create a BMP");
      // killough 4/18/98
      return;
   }

   M_DoScreenShot(lbmname); // cph

   S_StartSound(NULL,gamemode==commercial ? sfx_radio : sfx_tink);
}
