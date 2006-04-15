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
 *  DOOM main program (D_DoomMain) and game loop (D_DoomLoop),
 *  plus functions to determine game mode (shareware, registered),
 *  parse command line parameters, configure game parameters (turbo),
 *  and call the startup functions.
 *
 *-----------------------------------------------------------------------------
 */


#include "rockmacros.h"

#include "doomdef.h"
#include "doomtype.h"
#include "doomstat.h"
#include "dstrings.h"
#include "sounds.h"
#include "z_zone.h"
#include "w_wad.h"
#include "s_sound.h"
#include "v_video.h"
#include "f_finale.h"
#include "f_wipe.h"
#include "m_argv.h"
#include "m_misc.h"
#include "m_menu.h"
#include "i_system.h"
#include "i_sound.h"
#include "i_video.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "wi_stuff.h"
#include "st_stuff.h"
#include "am_map.h"
#include "p_setup.h"
#include "r_draw.h"
#include "r_main.h"
#include "d_main.h"
#include "am_map.h"
#include "m_swap.h"

// CPhipps - removed wadfiles[] stuff

boolean devparm;        // started game with -devparm

// jff 1/24/98 add new versions of these variables to remember command line
boolean clnomonsters;   // checkparm of -nomonsters
boolean clrespawnparm;  // checkparm of -respawn
boolean clfastparm;     // checkparm of -fast
// jff 1/24/98 end definition of command line version of play mode switches

boolean nomonsters;     // working -nomonsters
boolean respawnparm;    // working -respawn
boolean fastparm;       // working -fast

boolean singletics = false; // debug flag to cancel adaptiveness

bool doomexit;

//jff 1/22/98 parms for disabling music and sound
boolean nomusicparm=0;

//jff 4/18/98
extern boolean inhelpscreens;

skill_t startskill;
int     startepisode;
int     startmap;
boolean autostart;
int    debugfile;
int ffmap;

boolean advancedemo;

extern boolean timingdemo, singledemo, demoplayback, fastdemo; // killough

int      basetic;

void D_DoAdvanceDemo (void);

/*
 * D_PostEvent - Event handling
 *
 * Called by I/O functions when an event is received.
 * Try event handlers for each code area in turn.
 * cph - in the true spirit of the Boom source, let the 
 *  short ciruit operator madness begin!
 */

void D_PostEvent(event_t *ev)
{
   /* cph - suppress all input events at game start
    * FIXME: This is a lousy kludge */
   if (gametic < 3) return;
   M_Responder(ev) ||
   (gamestate == GS_LEVEL && (
       HU_Responder(ev) ||
       ST_Responder(ev) ||
       AM_Responder(ev)
    )
   ) ||
   G_Responder(ev);
}

//
// D_Wipe
//
// CPhipps - moved the screen wipe code from D_Display to here
// The screens to wipe between are already stored, this just does the timing
// and screen updating

static void D_Wipe(void)
{
   boolean done;
   int wipestart = I_GetTime () - 1;

   do
   {
      int nowtime, tics;
      do
      {
         //I_uSleep(5000); // CPhipps - don't thrash cpu in this loop
         nowtime = I_GetTime();
         tics = nowtime - wipestart;
      }
      while (!tics);
      wipestart = nowtime;

      done = wipe_ScreenWipe(0,0,SCREENWIDTH,SCREENHEIGHT,tics);
      M_Drawer();                   // menu is drawn even on top of wipes
      I_FinishUpdate();             // page flip or blit buffer
   }
   while (!done);
}

//
// D_Display
//  draw current display, possibly wiping it from the previous
//

// wipegamestate can be set to -1 to force a wipe on the next draw
gamestate_t    wipegamestate = GS_DEMOSCREEN;
extern boolean setsizeneeded;
extern int     showMessages;

void D_Display (void)
{
   static boolean isborderstate        IDATA_ATTR= false;
   static boolean borderwillneedredraw IDATA_ATTR= false;
   static  boolean  inhelpscreensstate IDATA_ATTR= false;
   static  gamestate_t  oldgamestate IDATA_ATTR= -1;
   boolean wipe;
   boolean viewactive = false, isborder = false;

   if (nodrawers)                   // for comparative timing / profiling
      return; 

   // save the current screen if about to wipe
   if ((wipe = gamestate != wipegamestate))
      wipe_StartScreen(0, 0, SCREENWIDTH, SCREENHEIGHT);

   if (gamestate != GS_LEVEL) { // Not a level
      switch (oldgamestate) {
      case -1:
      case GS_LEVEL:
         V_SetPalette(0); // cph - use default (basic) palette
      default:
         break;
      }

      switch (gamestate) {
      case GS_INTERMISSION:
         WI_Drawer();
         break;
      case GS_FINALE:
         F_Drawer();
         break;
      case GS_DEMOSCREEN:
         D_PageDrawer();
         break;
      default:
         break;
      }
   } else if (gametic != basetic) { // In a level
      boolean redrawborderstuff;

      HU_Erase();

      if (setsizeneeded) {               // change the view size if needed
         R_ExecuteSetViewSize();
         oldgamestate = -1;            // force background redraw
      }

      // Work out if the player view is visible, and if there is a border
      viewactive = (!(automapmode & am_active) || (automapmode & am_overlay)) && !inhelpscreens;
      isborder = viewactive ? (viewheight != SCREENHEIGHT) : (!inhelpscreens && (automapmode & am_active));

      if (oldgamestate != GS_LEVEL) {
         R_FillBackScreen ();    // draw the pattern into the back screen
         redrawborderstuff = isborder;
      } else {
         // CPhipps -
         // If there is a border, and either there was no border last time,
         // or the border might need refreshing, then redraw it.
         redrawborderstuff = isborder && (!isborderstate || borderwillneedredraw);
         // The border may need redrawing next time if the border surrounds the screen,
         // and there is a menu being displayed
         borderwillneedredraw = menuactive && isborder && viewactive && (viewwidth != SCREENWIDTH);
      }

      if (redrawborderstuff)
         R_DrawViewBorder();

      // Now do the drawing
      if (viewactive)
         R_RenderPlayerView (&players[displayplayer]);
      if (automapmode & am_active)
         AM_Drawer();
      ST_Drawer((viewheight != SCREENHEIGHT) || ((automapmode & am_active) && !(automapmode & am_overlay)), redrawborderstuff);
      R_DrawViewBorder();

      HU_Drawer();
   }

   inhelpscreensstate = inhelpscreens;
   isborderstate      = isborder;
   oldgamestate = wipegamestate = gamestate;

   // draw pause pic
   if (paused) {
      static int x;

      if (!x) { // Cache results of x pos calc
         int lump = W_GetNumForName("M_PAUSE");
         const patch_t* p = W_CacheLumpNum(lump);
         x = (320 - SHORT(p->width))/2;
         W_UnlockLumpNum(lump);
      }

      // CPhipps - updated for new patch drawing
      V_DrawNamePatch(x, (!(automapmode & am_active) || (automapmode & am_overlay))
                      ? 4+(viewwindowy*200/SCREENHEIGHT) : 4, // cph - Must un-stretch viewwindowy
                      0, "M_PAUSE", CR_DEFAULT, VPT_STRETCH);
   }

   // menus go directly to the screen
   M_Drawer();          // menu is drawn even on top of everything
   D_BuildNewTiccmds();

   // normal update
   if (!wipe)
      I_FinishUpdate ();              // page flip or blit buffer
   else {
      // wipe update
      wipe_EndScreen(0, 0, SCREENWIDTH, SCREENHEIGHT);
      D_Wipe();
   }
}

//
//  D_DoomLoop()
//
// Not a globally visible function,
//  just included for source reference,
//  called by D_DoomMain, never exits.
// Manages timing and IO,
//  calls all ?_Responder, ?_Ticker, and ?_Drawer,
//  calls I_GetTime, I_StartFrame, and I_StartTic
//

extern  boolean demorecording;

static void D_DoomLoop (void)
{
   basetic = gametic;

   I_SubmitSound();

   while (!doomexit)
   {
      // process one or more tics
      if (singletics)
      {
         I_StartTic ();
         G_BuildTiccmd (&netcmds[consoleplayer][maketic%BACKUPTICS]);
         if (advancedemo)
            D_DoAdvanceDemo ();
         M_Ticker ();
         G_Ticker ();
         gametic++;
         maketic++;
      }
      else
         TryRunTics (); // will run at least one tic

      // killough 3/16/98: change consoleplayer to displayplayer
      if (players[displayplayer].mo) // cph 2002/08/10
         S_UpdateSounds(players[displayplayer].mo);// move positional sounds

      // Update display, next frame, with current state.
      D_Display();

      // Give the system some time
      rb->yield();
   }
}

//
//  DEMO LOOP
//

static int  demosequence;         // killough 5/2/98: made static
static int  pagetic;
static const char *pagename; // CPhipps - const

//
// D_PageTicker
// Handles timing for warped projection
//
void D_PageTicker(void)
{
   if (--pagetic < 0)
      D_AdvanceDemo();
}

//
// D_PageDrawer
//
void D_PageDrawer(void)
{
   // CPhipps - updated for new patch drawing
   V_DrawNamePatch(0, 0, 0, pagename, CR_DEFAULT, VPT_STRETCH);
}

//
// D_AdvanceDemo
// Called after each demo or intro demosequence finishes
//
void D_AdvanceDemo (void)
{
   advancedemo = true;
}

/* killough 11/98: functions to perform demo sequences
 * cphipps 10/99: constness fixes
 */

static void D_SetPageName(const char *name)
{
   pagename = name;
}

static void D_DrawTitle1(const char *name)
{
   S_StartMusic(mus_intro);
   pagetic = (TICRATE*170)/35;
   D_SetPageName(name);
}

static void D_DrawTitle2(const char *name)
{
   S_StartMusic(mus_dm2ttl);
   D_SetPageName(name);
}

/* killough 11/98: tabulate demo sequences
 */

static struct
{
   void (*func)(const char *);
   const char *name;
} const demostates[][4] =
   {
      {
         {D_DrawTitle1, "TITLEPIC"},
         {D_DrawTitle1, "TITLEPIC"},
         {D_DrawTitle2, "TITLEPIC"},
         {D_DrawTitle1, "TITLEPIC"},
      },

      {
         {G_DeferedPlayDemo, "demo1"},
         {G_DeferedPlayDemo, "demo1"},
         {G_DeferedPlayDemo, "demo1"},
         {G_DeferedPlayDemo, "demo1"},
      },
      {
         {D_SetPageName, "CREDIT"},
         {D_SetPageName, "CREDIT"},
         {D_SetPageName, "CREDIT"},
         {D_SetPageName, "CREDIT"},
      },

      {
         {G_DeferedPlayDemo, "demo2"},
         {G_DeferedPlayDemo, "demo2"},
         {G_DeferedPlayDemo, "demo2"},
         {G_DeferedPlayDemo, "demo2"},
      },

      {
         {D_SetPageName, "HELP2"},
         {D_SetPageName, "HELP2"},
         {D_SetPageName, "CREDIT"},
         {D_DrawTitle1,  "TITLEPIC"},
      },

      {
         {G_DeferedPlayDemo, "demo3"},
         {G_DeferedPlayDemo, "demo3"},
         {G_DeferedPlayDemo, "demo3"},
         {G_DeferedPlayDemo, "demo3"},
      },

      {
         {NULL,0},
         {NULL,0},
         {NULL,0},
         {D_SetPageName, "CREDIT"},
      },

      {
         {NULL,0},
         {NULL,0},
         {NULL,0},
         {G_DeferedPlayDemo, "demo4"},
      },

      {
         {NULL,0},
         {NULL,0},
         {NULL,0},
         {NULL,0},
      }
   };

/*
 * This cycles through the demo sequences.
 * killough 11/98: made table-driven
 */

void D_DoAdvanceDemo(void)
{
   players[consoleplayer].playerstate = PST_LIVE;  /* not reborn */
   advancedemo = usergame = paused = false;
   gameaction = ga_nothing;

   pagetic = TICRATE * 11;         /* killough 11/98: default behavior */
   gamestate = GS_DEMOSCREEN;

   if (netgame && !demoplayback) {
      demosequence = 0;
   } else
      if (!demostates[++demosequence][gamemode].func)
         demosequence = 0;
   demostates[demosequence][gamemode].func(demostates[demosequence][gamemode].name);
}

//
// D_StartTitle
//
void D_StartTitle (void)
{
   gameaction = ga_nothing;
   demosequence = -1;
   D_AdvanceDemo();
}

//
// D_AddFile
//
// Rewritten by Lee Killough
//
// Ty 08/29/98 - add source parm to indicate where this came from
// CPhipps - static, const char* parameter
//         - source is an enum
//         - modified to allocate & use new wadfiles array
void D_AddFile (const char *file, wad_source_t source)
{
   wadfiles = realloc(wadfiles, sizeof(*wadfiles)*(numwadfiles+1));
   wadfiles[numwadfiles].name =
      AddDefaultExtension(strcpy(malloc(strlen(file)+5), file), ".wad");
   wadfiles[numwadfiles].src = source; // Ty 08/29/98
   numwadfiles++;
}

//
// CheckIWAD
//
// Verify a file is indeed tagged as an IWAD
// Scan its lumps for levelnames and return gamemode as indicated
// Detect missing wolf levels in DOOM II
//
// The filename to check is passed in iwadname, the gamemode detected is
// returned in gmode, hassec returns the presence of secret levels
//
// jff 4/19/98 Add routine to test IWAD for validity and determine
// the gamemode from it. Also note if DOOM II, whether secret levels exist
// CPhipps - const char* for iwadname, made static
#if 0
static void CheckIWAD(const char *iwadname,GameMode_t *gmode,boolean *hassec)
{
   if ( !fileexists (iwadname) )
   {
      int ud=0,rg=0,sw=0,cm=0,sc=0;
      int handle;

      // Identify IWAD correctly
      if ( (handle = open (iwadname,O_RDONLY)) != -1)
      {
         wadinfo_t header;

         // read IWAD header
         read (handle, &header, sizeof(header));
         if (!strncmp(header.identification,"IWAD",4))
         {
            size_t length;
            filelump_t *fileinfo;

            // read IWAD directory
            header.numlumps = LONG(header.numlumps);
            header.infotableofs = LONG(header.infotableofs);
            length = header.numlumps;
            fileinfo = malloc(length*sizeof(filelump_t));
            lseek (handle, header.infotableofs, SEEK_SET);
            read (handle, fileinfo, length*sizeof(filelump_t));
            close(handle);

            // scan directory for levelname lumps
            while (length--)
               if (fileinfo[length].name[0] == 'E' &&
                     fileinfo[length].name[2] == 'M' &&
                     fileinfo[length].name[4] == 0)
               {
                  if (fileinfo[length].name[1] == '4')
                     ++ud;
                  else if (fileinfo[length].name[1] == '3')
                     ++rg;
                  else if (fileinfo[length].name[1] == '2')
                     ++rg;
                  else if (fileinfo[length].name[1] == '1')
                     ++sw;
               }
               else if (fileinfo[length].name[0] == 'M' &&
                        fileinfo[length].name[1] == 'A' &&
                        fileinfo[length].name[2] == 'P' &&
                        fileinfo[length].name[5] == 0)
               {
                  ++cm;
                  if (fileinfo[length].name[3] == '3')
                     if (fileinfo[length].name[4] == '1' ||
                           fileinfo[length].name[4] == '2')
                        ++sc;
               }

            free(fileinfo);
         }
         else // missing IWAD tag in header
            I_Error("CheckIWAD: IWAD tag %s not present", iwadname);
      }
      else // error from open call
         I_Error("CheckIWAD: Can't open IWAD %s", iwadname);

      // Determine game mode from levels present
      // Must be a full set for whichever mode is present
      // Lack of wolf-3d levels also detected here

      *gmode = indetermined;
      *hassec = false;
      if (cm>=30)
      {
         *gmode = commercial;
         *hassec = sc>=2;
      }
      else if (ud>=9)
         *gmode = retail;
      else if (rg>=18)
         *gmode = registered;
      else if (sw>=9)
         *gmode = shareware;
   }
   else // error from access call
      I_Error("CheckIWAD: IWAD %s not readable", iwadname);
}
#endif
void D_DoomMainSetup(void)
{
   int   p;

   nomonsters = M_CheckParm ("-nomonsters");
   respawnparm = M_CheckParm ("-respawn");
   fastparm = M_CheckParm ("-fast");
   devparm = M_CheckParm ("-devparm");
   if (M_CheckParm ("-altdeath"))
      deathmatch = 2;
   else if (M_CheckParm ("-deathmatch"))
      deathmatch = 1;

   printf("Welcome to Rockdoom\n");

   switch ( gamemode )
   {
   case retail:
      printf ("The Ultimate DOOM Startup v%d.%d\n",DVERSION/100,DVERSION%100);
      break;
   case shareware:
      printf ("DOOM Shareware Startup v%d.%d\n",DVERSION/100,DVERSION%100);
      break;
   case registered:
      printf ("DOOM Registered Startup v%d.%d\n",DVERSION/100,DVERSION%100);
      break;
   case commercial:
      switch (gamemission)
      {
      case pack_plut:
         printf ("DOOM 2: Plutonia Experiment v%d.%d\n",DVERSION/100,DVERSION%100);
         break;
      case pack_tnt:
         printf ("DOOM 2: TNT - Evilution v%d.%d\n",DVERSION/100,DVERSION%100);
         break;
      default:
         printf ("DOOM 2: Hell on Earth v%d.%d\n",DVERSION/100,DVERSION%100);
         break;
      }
      break;
   default:
      printf ("Public DOOM v%d.%d\n",DVERSION/100,DVERSION%100);
      break;
   }

   if (devparm)
      printf(D_DEVSTR);

   // turbo option
   if ((p=M_CheckParm ("-turbo")))
   {
      int scale = 200;
      extern int forwardmove[2];
      extern int sidemove[2];

      if (p<myargc-1)
         scale = atoi (myargv[p+1]);
      if (scale < 10)
         scale = 10;
      if (scale > 400)
         scale = 400;
      printf ("turbo scale: %d%%\n",scale);
      forwardmove[0] = forwardmove[0]*scale/100;
      forwardmove[1] = forwardmove[1]*scale/100;
      sidemove[0] = sidemove[0]*scale/100;
      sidemove[1] = sidemove[1]*scale/100;
   }

   // get skill / episode / map from parms
   startskill = sk_medium;
   startepisode = 1;
   startmap = 1;
   autostart = false;

   p = M_CheckParm ("-skill");
   if (p && p < myargc-1)
   {
      startskill = myargv[p+1][0]-'1';
      autostart = true;
   }

   p = M_CheckParm ("-episode");
   if (p && p < myargc-1)
   {
      startepisode = myargv[p+1][0]-'0';
      startmap = 1;
      autostart = true;
   }

   p = M_CheckParm ("-warp");
   if (p && p < myargc-1)
   {
      if (gamemode == commercial)
         startmap = atoi (myargv[p+1]);
      else
      {
         startepisode = myargv[p+1][0]-'0';
         startmap = myargv[p+2][0]-'0';
      }
      autostart = true;
   }

   // CPhipps - move up netgame init
   printf("D_InitNetGame: Checking for network game.\n");
   D_InitNetGame();

   // init subsystems
   printf ("V_Init: allocate screens.\n");
   V_Init ();

   printf ("W_Init: Init WADfiles.\n");
   W_Init();

   V_InitColorTranslation(); //jff 4/24/98 load color translation lumps

   // Check for -file in shareware
   if (modifiedgame)
   {
      // These are the lumps that will be checked in IWAD,
      // if any one is not present, execution will be aborted.
      const char name[23][8]=
         {
            "e2m1","e2m2","e2m3","e2m4","e2m5","e2m6","e2m7","e2m8","e2m9",
            "e3m1","e3m3","e3m3","e3m4","e3m5","e3m6","e3m7","e3m8","e3m9",
            "dphoof","bfgga0","heada1","cybra1","spida1d1"
         };
      int i;

      if ( gamemode == shareware)
         I_Error("\nYou cannot -file with the shareware version. Register!\n");

      // Check for fake IWAD with right name,
      // but w/o all the lumps of the registered version.
      if (gamemode == registered)
         for (i = 0;i < 23; i++)
            if (W_CheckNumForName(name[i])<0)
               I_Error("This is not the registered version.\n");
   }

   // Iff additonal PWAD files are used, print modified banner
   if (modifiedgame)
      printf ("ATTENTION:  This version of DOOM has been modified.\n");

   // Check and print which version is executed.
   switch ( gamemode )
   {
   case shareware:
   case indetermined:
      printf ("Shareware!\n");
      break;
   case registered:
   case retail:
   case commercial:
      printf ("Commercial product - do not distribute!\n");
      break;
   default:
      // Ouch.
      break;
   }

   printf ("M_Init: Init miscellaneous info.\n");
   M_Init ();

   printf ("R_Init: Init DOOM refresh daemon - ");
   R_Init ();

   printf ("P_Init: Init Playloop state.\n");
   P_Init ();

   printf ("I_Init: Setting up machine state.\n");
   I_Init ();

   printf ("S_Init: Setting up sound.\n");
   S_Init (snd_SfxVolume /* *8 */, snd_MusicVolume /* *8*/ );

   printf ("HU_Init: Setting up heads up display.\n");
   HU_Init ();

   I_InitGraphics ();

   printf ("ST_Init: Init status bar.\n");
   ST_Init ();

   // check for a driver that wants intermission stats
   p = M_CheckParm ("-statcopy");
   if (p && p<myargc-1)
   {
      // for statistics driver
      extern  void* statcopy;

      statcopy = (void*)(long)atoi(myargv[p+1]);
      printf ("External statistics registered.\n");
   }

   // start the apropriate game based on parms
   p = M_CheckParm ("-record");
   if (p && p < myargc-1)
   {
      G_RecordDemo (myargv[p+1]);
      autostart = true;
   }

   p = M_CheckParm ("-loadgame");
   if (p && p < myargc-1)
      G_LoadGame (atoi(myargv[p+1]), false);

   if ( gameaction != ga_loadgame )
   {
      if (!singledemo) {                  /* killough 12/98 */
         if (autostart || netgame)
            G_InitNew (startskill, startepisode, startmap);
         else
            D_StartTitle ();                // start up intro loop
      }
   }
}

//
// D_DoomMain
//
void D_DoomMain (void)
{
   D_DoomMainSetup(); // get this crap off the stack

   D_DoomLoop ();  // never returns
}
