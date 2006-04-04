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
// $Log$
// Revision 1.5  2006/04/04 23:58:37  kkurbjun
// Make savegame strings more informative
//
// Revision 1.4  2006-04-04 23:13:50  kkurbjun
// Fix up configurable keys, edit exit string, more work needs to be done on menu keys
//
// Revision 1.3  2006-04-03 20:03:02  kkurbjun
// Updates doom menu w/ new graphics, now requires rockdoom.wad: http://alamode.mines.edu/~kkurbjun/rockdoom.wad
//
// Revision 1.2  2006-04-03 00:28:13  kkurbjun
// Fixes graphic errors in scaling code, note sure about the fix in hu_lib.c though.  I havn't seen any corrupted text but it may still need a proper fix.
//
// Revision 1.1  2006-03-28 15:44:01  dave
// Patch #2969 - Doom!  Currently only working on the H300.
//
//
// DESCRIPTION:
// DOOM selection menu, options, episode etc.
// Sliders and icons. Kinda widget stuff.
//
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "dstrings.h"

#include "d_main.h"

#include "i_system.h"
#include "i_video.h"
#include "z_zone.h"
#include "v_video.h"
#include "w_wad.h"

#include "r_main.h"

#include "hu_stuff.h"

#include "g_game.h"

#include "m_argv.h"
#include "m_swap.h"
#include "s_sound.h"

#include "doomstat.h"

// Data.
#include "sounds.h"

#include "m_menu.h"
#include "rockmacros.h"


extern patchnum_t  hu_font[HU_FONTSIZE];
extern boolean  message_dontfuckwithme;

extern boolean  chat_on;  // in heads-up code

//
// defaulted values
//
int   mouseSensitivity;       // has default

// Show messages has default, 0 = off, 1 = on
int   showMessages;

// Blocky mode, has default, 0 = high, 1 = normal
int   screenblocks;  // has default

// temp for screenblocks (0-9)
int   screenSize;

// -1 = no quicksave slot picked!
int   quickSaveSlot;

// 1 = message to be printed
int   messageToPrint;
// ...and here is the message string!
char*   messageString;

// message x & y
int   messx;
int   messy;
int   messageLastMenuActive;

// timed message = no input from user
boolean   messageNeedsInput;

void    (*messageRoutine)(int response);

#define SAVESTRINGSIZE  24

char gammamsg[5][26] =
   {
      GAMMALVL0,
      GAMMALVL1,
      GAMMALVL2,
      GAMMALVL3,
      GAMMALVL4
   };

// we are going to be entering a savegame string
int   saveStringEnter;
int              saveSlot; // which slot to save in
int   saveCharIndex; // which char we're editing
// old save description before edit
char   saveOldString[SAVESTRINGSIZE];

boolean   inhelpscreens;
boolean   menuactive;

#define SKULLXOFF  -32
#define LINEHEIGHT  16

extern boolean  sendpause;
char   savegamestrings[10][SAVESTRINGSIZE];

char endstring[170];


//
// MENU TYPEDEFS
//
typedef struct
{
   // 0 = no cursor here, 1 = ok, 2 = arrows ok
   short status;

   char name[10];

   // choice = menu item #.
   // if status = 2,
   //   choice=0:leftarrow,1:rightarrow
   void (*routine)(int choice);

   // hotkey in menu
   char alphaKey;
}
menuitem_t;



typedef struct menu_s
{
   short  numitems; // # of menu items
   struct menu_s* prevMenu; // previous menu
   menuitem_t*  menuitems; // menu items
   void  (*routine)(void); // draw routine ROCKBOX
   short  x;
   short  y;  // x,y of menu
   short  lastOn;  // last item user was on in menu
}
menu_t;

short  itemOn;   // menu item skull is on
short  skullAnimCounter; // skull animation counter
short  whichSkull;  // which skull to draw
int systemvol;

// graphic name of skulls
// warning: initializer-string for array of chars is too long
char    skullName[2][/*8*/9] = {"M_SKULL1","M_SKULL2"};

// current menudef
menu_t* currentMenu;

//
// PROTOTYPES
//
void M_NewGame(int choice);
void M_Episode(int choice);
void M_ChooseSkill(int choice);
void M_LoadGame(int choice);
void M_SaveGame(int choice);
void M_Options(int choice);
void M_EndGame(int choice);
void M_ReadThis(int choice);
void M_ReadThis2(int choice);
void M_QuitDOOM(int choice);

void M_ChangeMessages(int choice);
void M_ChangeGamma(int choice);
void M_SfxVol(int choice);
void M_MusicVol(int choice);
void M_SystemVol(int choice);
void M_SizeDisplay(int choice);
void M_StartGame(int choice);
void M_Sound(int choice);

void M_FinishReadThis(int choice);
void M_LoadSelect(int choice);
void M_SaveSelect(int choice);
void M_ReadSaveStrings(void);
void M_QuickSave(void);
void M_QuickLoad(void);

void M_DrawMainMenu(void);
void M_DrawReadThis1(void);
void M_DrawReadThis2(void);
void M_DrawNewGame(void);
void M_DrawEpisode(void);
void M_DrawOptions(void);
void M_DrawSound(void);
void M_DrawLoad(void);
void M_DrawSave(void);

void M_DrawSaveLoadBorder(int x,int y);
void M_SetupNextMenu(menu_t *menudef);
void M_DrawThermo(int x,int y,int thermWidth,int thermDot);
void M_DrawEmptyCell(menu_t *menu,int item);
void M_DrawSelCell(menu_t *menu,int item);
void M_WriteText(int x, int y, char *string);
int M_StringWidth(const char* string);
int M_StringHeight(const char* string);
void M_StartControlPanel(void);
void M_StartMessage(char *string,void *routine,boolean input);
void M_StopMessage(void);
void M_ClearMenus (void);




//
// DOOM MENU
//
enum
{
   newgame = 0,
   options,
   loadgame,
   savegame,
   readthis,
   quitdoom,
   main_end
} main_e;

menuitem_t MainMenu[]=
   {
      {1,"M_NGAME",M_NewGame,'n'},
      {1,"M_OPTION",M_Options,'o'},
      {1,"M_LOADG",M_LoadGame,'l'},
      {1,"M_SAVEG",M_SaveGame,'s'},
      // Another hickup with Special edition.
      {1,"M_RDTHIS",M_ReadThis,'r'},
      {1,"M_QUITG",M_QuitDOOM,'q'}
   };

menu_t  MainDef =
   {
      main_end,
      NULL,
      MainMenu,
      M_DrawMainMenu,
      97,64,
      0
   };


//
// EPISODE SELECT
//
enum
{
   ep1,
   ep2,
   ep3,
   ep4,
   ep_end
} episodes_e;

menuitem_t EpisodeMenu[]=
   {
      {1,"M_EPI1", M_Episode,'k'},
      {1,"M_EPI2", M_Episode,'t'},
      {1,"M_EPI3", M_Episode,'i'},
      {1,"M_EPI4", M_Episode,'t'}
   };

menu_t  EpiDef =
   {
      ep_end,  // # of menu items
      &MainDef,  // previous menu
      EpisodeMenu, // menuitem_t ->
      M_DrawEpisode, // drawing routine ->
      48,63,              // x,y
      ep1   // lastOn
   };

//
// NEW GAME
//
enum
{
   killthings,
   toorough,
   hurtme,
   violence,
   nightmare,
   newg_end
} newgame_e;

menuitem_t NewGameMenu[]=
   {
      {1,"M_JKILL", M_ChooseSkill, 'i'},
      {1,"M_ROUGH", M_ChooseSkill, 'h'},
      {1,"M_HURT", M_ChooseSkill, 'h'},
      {1,"M_ULTRA", M_ChooseSkill, 'u'},
      {1,"M_NMARE", M_ChooseSkill, 'n'}
   };

menu_t  NewDef =
   {
      newg_end,  // # of menu items
      &EpiDef,  // previous menu
      NewGameMenu, // menuitem_t ->
      M_DrawNewGame, // drawing routine ->
      48,63,              // x,y
      hurtme  // lastOn
   };



//
// OPTIONS MENU
//
enum
{
   endgame,
   messages,
   scrnsize,
   option_empty1,
   gamasens,
   option_empty2,
   soundvol,
   opt_end
} options_e;

menuitem_t OptionsMenu[]=
   {
      {1,"M_ENDGAM", M_EndGame,'e'},
      {1,"M_MESSG", M_ChangeMessages,'m'},
      {2,"M_SCRNSZ", M_SizeDisplay,'s'},
      {-1,"",0,0},
      {2,"M_GAMMA", M_ChangeGamma,'m'},
      {-1,"",0,0},
      {1,"M_SVOL", M_Sound,'s'}
   };

menu_t  OptionsDef =
   {
      opt_end,
      &MainDef,
      OptionsMenu,
      M_DrawOptions,
      60,37,
      0
   };

//
// Read This! MENU 1 & 2
//
enum
{
   rdthsempty1,
   read1_end
} read_e;

menuitem_t ReadMenu1[] =
   {
      {1,"",M_ReadThis2,0}
   };

menu_t  ReadDef1 =
   {
      read1_end,
      &MainDef,
      ReadMenu1,
      M_DrawReadThis1,
      280,185,
      0
   };

enum
{
   rdthsempty2,
   read2_end
} read_e2;

menuitem_t ReadMenu2[]=
   {
      {1,"",M_FinishReadThis,0}
   };

menu_t  ReadDef2 =
   {
      read2_end,
      &ReadDef1,
      ReadMenu2,
      M_DrawReadThis2,
      330,175,
      0
   };

//
// SOUND VOLUME MENU
//
enum
{
   sfx_vol,
   sfx_empty1,
   music_vol,
   sfx_empty2,
   system_vol,
   sfx_empty3,
   sound_end
} sound_e;

menuitem_t SoundMenu[]=
   {
      {2,"M_SFXVOL",M_SfxVol,'s'},
      {-1,"",0,0}, //ROCKBOX
      {2,"M_MUSVOL",M_MusicVol,'m'},
      {-1,"",0,0}, //ROCKBOX
      {2,"M_SYSVOL",M_SystemVol,'z'},
      {-1,"",0,0} //ROCKBOX
   };

menu_t  SoundDef =
   {
      sound_end,
      &OptionsDef,
      SoundMenu,
      M_DrawSound,
      80,64,
      0
   };

//
// LOAD GAME MENU
//
enum
{
   load1,
   load2,
   load3,
   load4,
   load5,
   load6,
   load_end
} load_e;

menuitem_t LoadMenu[]=
   {
      {1,"", M_LoadSelect,'1'},
      {1,"", M_LoadSelect,'2'},
      {1,"", M_LoadSelect,'3'},
      {1,"", M_LoadSelect,'4'},
      {1,"", M_LoadSelect,'5'},
      {1,"", M_LoadSelect,'6'}
   };

menu_t  LoadDef =
   {
      load_end,
      &MainDef,
      LoadMenu,
      M_DrawLoad,
      80,54,
      0
   };

//
// SAVE GAME MENU
//
menuitem_t SaveMenu[]=
   {
      {1,"", M_SaveSelect,'1'},
      {1,"", M_SaveSelect,'2'},
      {1,"", M_SaveSelect,'3'},
      {1,"", M_SaveSelect,'4'},
      {1,"", M_SaveSelect,'5'},
      {1,"", M_SaveSelect,'6'}
   };

menu_t  SaveDef =
   {
      load_end,
      &MainDef,
      SaveMenu,
      M_DrawSave,
      80,54,
      0
   };


//
// M_ReadSaveStrings
//  read the strings from the savegame files
//
void M_ReadSaveStrings(void)
{
   int             handle;
   int             count;
   int             i;
   char    name[256];

   for (i = 0;i < load_end;i++)
   {
      if (M_CheckParm("-cdrom"))
         snprintf(name,sizeof(name),"c:\\doomdata\\"SAVEGAMENAME"%d.dsg",i);
      else
         snprintf(name,sizeof(name),SAVEGAMENAME"%d.dsg",i);

      handle = open (name, O_RDONLY | 0);
      if (handle == -1)
      {
         strcpy(&savegamestrings[i][0],EMPTYSTRING);
         LoadMenu[i].status = 0;
         continue;
      }
      count = read (handle, &savegamestrings[i], SAVESTRINGSIZE);
      close (handle);
      LoadMenu[i].status = 1;
   }
}

#define LOADGRAPHIC_Y 8
//
// M_LoadGame & Cie.
//
void M_DrawLoad(void)
{
   int             i;

   V_DrawNamePatch(72 ,LOADGRAPHIC_Y, 0, "M_LOADG", CR_DEFAULT, VPT_STRETCH);
   for (i = 0;i < load_end; i++)
   {
      M_DrawSaveLoadBorder(LoadDef.x,LoadDef.y+LINEHEIGHT*i);
      M_WriteText(LoadDef.x,LoadDef.y+LINEHEIGHT*i,savegamestrings[i]);
   }
}



//
// Draw border for the savegame description
//
void M_DrawSaveLoadBorder(int x,int y)
{
   int             i;

   V_DrawNamePatch(x-8, y+7, 0, "M_LSLEFT", CR_DEFAULT, VPT_STRETCH);
   for (i = 0;i < 24;i++)
   {
      V_DrawNamePatch(x, y+7, 0, "M_LSCNTR", CR_DEFAULT, VPT_STRETCH);
      x += 8;
   }
   V_DrawNamePatch(x, y+7, 0, "M_LSRGHT", CR_DEFAULT, VPT_STRETCH);
}



//
// User wants to load this game
//
void M_LoadSelect(int choice)
{
   char    name[256];

   if (M_CheckParm("-cdrom"))
      snprintf(name,sizeof(name),"c:\\doomdata\\"SAVEGAMENAME"%d.dsg",choice);
   else
      snprintf(name,sizeof(name),SAVEGAMENAME"%d.dsg",choice);
   G_LoadGame (choice, false);
   M_ClearMenus ();
}

//
// Selected from DOOM menu
//
void M_LoadGame (int choice)
{
   (void)choice;
   if (netgame)
   {
      M_StartMessage(LOADNET,NULL,false);
      return;
   }

   M_SetupNextMenu(&LoadDef);
   M_ReadSaveStrings();
}


//
//  M_SaveGame & Cie.
//
void M_DrawSave(void)
{
   int             i;

   V_DrawNamePatch(72, LOADGRAPHIC_Y, 0, "M_SAVEG", CR_DEFAULT, VPT_STRETCH);
   for (i = 0;i < load_end; i++)
   {
      M_DrawSaveLoadBorder(LoadDef.x,LoadDef.y+LINEHEIGHT*i);
      M_WriteText(LoadDef.x,LoadDef.y+LINEHEIGHT*i,savegamestrings[i]);
   }

   if (saveStringEnter)
   {
      i = M_StringWidth(savegamestrings[saveSlot]);
      M_WriteText(LoadDef.x + i,LoadDef.y+LINEHEIGHT*saveSlot,"_");
   }
}

//
// M_Responder calls this when user is finished
//
void M_DoSave(int slot)
{
   G_SaveGame (slot,savegamestrings[slot]);
   M_ClearMenus ();

   // PICK QUICKSAVE SLOT YET?
   if (quickSaveSlot == -2)
      quickSaveSlot = slot;
}

//
// User wants to save. Start string input for M_Responder
//
void M_SaveSelect(int choice)
{
   // we are going to be intercepting all chars
   saveStringEnter = 1;

   saveSlot = choice;
   snprintf(savegamestrings[choice], sizeof(savegamestrings[choice]), 
      (gamemode==shareware||gamemode==registered||gamemode==retail) ? 
      mapnames[(gameepisode-1)*9+gamemap-1]  : (gamemission==doom2)     ?
      mapnames2[gamemap-1] : (gamemission==pack_plut) ?
      mapnamesp[gamemap-1] : (gamemission==pack_tnt)  ?
      mapnamest[gamemap-1] : "Unknown Location", choice);
   if (!strcmp(savegamestrings[choice],EMPTYSTRING))
      savegamestrings[choice][0] = 0;
   saveCharIndex = strlen(savegamestrings[choice]);
}

//
// Selected from DOOM menu
//
void M_SaveGame (int choice)
{
   (void)choice;
   if (!usergame)
   {
      M_StartMessage(SAVEDEAD,NULL,false);
      return;
   }

   if (gamestate != GS_LEVEL)
      return;

   M_SetupNextMenu(&SaveDef);
   M_ReadSaveStrings();
}



//
//      M_QuickSave
//
char    tempstring[80];

void M_QuickSaveResponse(int ch)
{
   if (ch == 'y')
   {
      M_DoSave(quickSaveSlot);

      S_StartSound(NULL,sfx_swtchx);

   }
}

void M_QuickSave(void)
{
   if (!usergame)
   {
      S_StartSound(NULL,sfx_oof);
      return;
   }

   if (gamestate != GS_LEVEL)
      return;

   if (quickSaveSlot < 0)
   {
      M_StartControlPanel();
      M_ReadSaveStrings();
      M_SetupNextMenu(&SaveDef);
      quickSaveSlot = -2; // means to pick a slot now
      return;
   }
   snprintf(tempstring,sizeof(tempstring),QSPROMPT,savegamestrings[quickSaveSlot]);
   M_StartMessage(tempstring,M_QuickSaveResponse,true);
}



//
// M_QuickLoad
//
void M_QuickLoadResponse(int ch)
{
   if (ch == 'y')
   {
      M_LoadSelect(quickSaveSlot);
      S_StartSound(NULL,sfx_swtchx);
   }
}


void M_QuickLoad(void)
{
   if (netgame)
   {
      M_StartMessage(QLOADNET,NULL,false);
      return;
   }

   if (quickSaveSlot < 0)
   {
      M_StartMessage(QSAVESPOT,NULL,false);
      return;
   }
   snprintf(tempstring, sizeof(tempstring), QLPROMPT,savegamestrings[quickSaveSlot]);
   M_StartMessage(tempstring,M_QuickLoadResponse,true);
}




//
// Read This Menus
// Had a "quick hack to fix romero bug"
//
void M_DrawReadThis1(void)
{
   inhelpscreens = true;
   switch ( gamemode )
   {
   case commercial:
      V_DrawNamePatch(0, 0, 0, "HELP", CR_DEFAULT, VPT_STRETCH);
      break;
   case shareware:
   case registered:
   case retail:
      V_DrawNamePatch(0, 0, 0, "HELP1", CR_DEFAULT, VPT_STRETCH);
      break;
   default:
      break;
   }
   return;
}



//
// Read This Menus - optional second page.
//
void M_DrawReadThis2(void)
{
   inhelpscreens = true;
   switch ( gamemode )
   {
   case retail:
   case commercial:
      // This hack keeps us from having to change menus.
      V_DrawNamePatch(0, 0, 0, "CREDIT", CR_DEFAULT, VPT_STRETCH);
      break;
   case shareware:
   case registered:
      V_DrawNamePatch(0, 0, 0, "HELP2", CR_DEFAULT, VPT_STRETCH);
      break;
   default:
      break;
   }
   return;
}


//
// Change Sfx & Music volumes
//
void M_DrawSound(void)
{
   int sysmax=(rb->sound_max(SOUND_VOLUME)-rb->sound_min(SOUND_VOLUME));
   V_DrawNamePatch(60, 38, 0, "M_SVOL", CR_DEFAULT, VPT_STRETCH);

   M_DrawThermo(SoundDef.x,SoundDef.y+LINEHEIGHT*(sfx_vol+1),
                16,snd_SfxVolume);

   M_DrawThermo(SoundDef.x,SoundDef.y+LINEHEIGHT*(music_vol+1),
                16,snd_MusicVolume);

   M_DrawThermo(SoundDef.x,SoundDef.y+LINEHEIGHT*(system_vol+1),
                16,(sysmax+systemvol)/5);
}

void M_Sound(int choice)
{
   (void) choice;
   M_SetupNextMenu(&SoundDef);
}

void M_SfxVol(int choice)
{
   switch(choice)
   {
   case 0:
      if (snd_SfxVolume)
         snd_SfxVolume--;
      break;
   case 1:
      if (snd_SfxVolume < 15)
         snd_SfxVolume++;
      break;
   }

   S_SetSfxVolume(snd_SfxVolume /* *8 */);
}

void M_MusicVol(int choice)
{
   switch(choice)
   {
   case 0:
      if (snd_MusicVolume)
         snd_MusicVolume--;
      break;
   case 1:
      if (snd_MusicVolume < 15)
         snd_MusicVolume++;
      break;
   }

   S_SetMusicVolume(snd_MusicVolume /* *8 */);
}

void M_SystemVol(int choice)
{
   switch(choice)
   {
   case 0:
      if (systemvol-5>rb->sound_min(SOUND_VOLUME))
      {
         systemvol-=5;
         rb->sound_set(SOUND_VOLUME, systemvol);
         rb->global_settings->volume = systemvol;
      }
      break;
   case 1:
      if (systemvol+5<rb->sound_max(SOUND_VOLUME))
      {
         systemvol+=5;
         rb->sound_set(SOUND_VOLUME, systemvol);
         rb->global_settings->volume = systemvol;
      }
      break;
   }
}

//
// M_DrawMainMenu
//
void M_DrawMainMenu(void)
{
   V_DrawNamePatch(94, 2, 0, "M_DOOM", CR_DEFAULT, VPT_STRETCH);
}




//
// M_NewGame
//
void M_DrawNewGame(void)
{
   // CPhipps - patch drawing updated
   V_DrawNamePatch(96, 14, 0, "M_NEWG", CR_DEFAULT, VPT_STRETCH);
   V_DrawNamePatch(54, 38, 0, "M_SKILL",CR_DEFAULT, VPT_STRETCH);
}

void M_NewGame(int choice)
{
   (void) choice;
   if (netgame && !demoplayback)
   {
      M_StartMessage(NEWGAME,NULL,false);
      return;
   }

   if ( gamemode == commercial )
      M_SetupNextMenu(&NewDef);
   else
      M_SetupNextMenu(&EpiDef);
}


//
//      M_Episode
//
int     epi;

void M_DrawEpisode(void)
{
   // CPhipps - patch drawing updated
   V_DrawNamePatch(54, 38, 0, "M_EPISOD", CR_DEFAULT, VPT_STRETCH);
}

void M_VerifyNightmare(int ch)
{
   if (ch != key_menu_enter)
      return;

   G_DeferedInitNew(nightmare,epi+1,1);
   M_ClearMenus ();
}

void M_ChooseSkill(int choice)
{
   if (choice == nightmare)
   {
      M_StartMessage(NIGHTMARE,M_VerifyNightmare,true);
      return;
   }

   //jff 3/24/98 remember last skill selected
   // killough 10/98 moved to here
   defaultskill = choice+1;

   G_DeferedInitNew(choice,epi+1,1);
   M_ClearMenus ();
}

void M_Episode(int choice)
{
   if ( (gamemode == shareware)
         && choice)
   {
      M_StartMessage(SWSTRING,NULL,false);
      M_SetupNextMenu(&ReadDef1);
      return;
   }

   // Yet another hack...
   if ( (gamemode == registered)
         && (choice > 2))
   {
      /* Digita */
      // fprintf( stderr,
      //     "M_Episode: 4th episode requires UltimateDOOM\n");
      choice = 0;
   }

   epi = choice;
   M_SetupNextMenu(&NewDef);
}



//
// M_Options
//
char    detailNames[2][9] = {"M_GDHIGH","M_GDLOW"};
char msgNames[2][9]  = {"M_MSGOFF","M_MSGON"};


void M_DrawOptions(void)
{
   // CPhipps - patch drawing updated
   V_DrawNamePatch(108, 15, 0, "M_OPTTTL", CR_DEFAULT, VPT_STRETCH);

   V_DrawNamePatch(OptionsDef.x + 120, OptionsDef.y+LINEHEIGHT*messages, 0,
                   msgNames[showMessages], CR_DEFAULT, VPT_STRETCH);

   M_DrawThermo(OptionsDef.x,OptionsDef.y+LINEHEIGHT*(gamasens+1),
                4,usegamma);

   M_DrawThermo(OptionsDef.x,OptionsDef.y+LINEHEIGHT*(scrnsize+1),
                9,screenSize);
}

void M_Options(int choice)
{
   (void)choice;
   M_SetupNextMenu(&OptionsDef);
}



//
//      Toggle messages on/off
//
void M_ChangeMessages(int choice)
{
   // warning: unused parameter `int choice'
   choice = 0;
   showMessages = 1 - showMessages;

   if (!showMessages)
      players[consoleplayer].message = MSGOFF;
   else
      players[consoleplayer].message = MSGON ;

   message_dontfuckwithme = true;
}


//
// M_EndGame
//
void M_EndGameResponse(int ch)
{
   if (ch != key_menu_enter)
      return;

   // killough 5/26/98: make endgame quit if recording or playing back demo
   if (demorecording || singledemo)
      G_CheckDemoStatus();

   currentMenu->lastOn = itemOn;
   M_ClearMenus ();
   D_StartTitle ();
}

void M_EndGame(int choice)
{
   choice = 0;
   if (!usergame)
   {
      S_StartSound(NULL,sfx_oof);
      return;
   }

   if (netgame)
   {
      M_StartMessage(NETEND,NULL,false);
      return;
   }

   M_StartMessage(ENDGAME,M_EndGameResponse,true);
}




//
// M_ReadThis
//
void M_ReadThis(int choice)
{
   choice = 0;
   M_SetupNextMenu(&ReadDef1);
}

void M_ReadThis2(int choice)
{
   choice = 0;
   M_SetupNextMenu(&ReadDef2);
}

void M_FinishReadThis(int choice)
{
   choice = 0;
   M_SetupNextMenu(&MainDef);
}




//
// M_QuitDOOM
//
int     quitsounds[8] =
   {
      sfx_pldeth,
      sfx_dmpain,
      sfx_popain,
      sfx_slop,
      sfx_telept,
      sfx_posit1,
      sfx_posit3,
      sfx_sgtatk
   };

int     quitsounds2[8] =
   {
      sfx_vilact,
      sfx_getpow,
      sfx_boscub,
      sfx_slop,
      sfx_skeswg,
      sfx_kntdth,
      sfx_bspact,
      sfx_sgtatk
   };



void M_QuitResponse(int ch)
{
   if (ch != key_menu_enter)
      return;
   if (!netgame)
   {
      if (gamemode == commercial)
         S_StartSound(NULL,quitsounds2[(gametic>>2)&7]);
      else
         S_StartSound(NULL,quitsounds[(gametic>>2)&7]);
      I_WaitVBL(105);
   }
   I_Quit ();
}




void M_QuitDOOM(int choice)
{
   (void)choice;
   // We pick index 0 which is language sensitive,
   //  or one at random, between 1 and maximum number.
   if (language != english )
      snprintf(endstring,sizeof(endstring),"%s\n\n"DOSY, endmsg[0] );
   else
      snprintf(endstring,sizeof(endstring),"%s\n\n%s", endmsg[gametic%(NUM_QUITMESSAGES-1)+1], DOSY);

   M_StartMessage(endstring,M_QuitResponse,true);
}




void M_ChangeGamma(int choice)
{
   switch(choice)
   {
   case 0:
      if (usegamma)
         usegamma--;
      break;
   case 1:
      if (usegamma < 4)
         usegamma++;
      break;
   }
   V_SetPalette (0);
}

void M_SizeDisplay(int choice)
{
   switch(choice)
   {
   case 0:
      if (screenSize > 0)
      {
         screenblocks--;
         screenSize--;
      }
      break;
   case 1:
      if (screenSize < 8)
      {
         screenblocks++;
         screenSize++;
      }
      break;
   }


   R_SetViewSize (screenblocks);
}




//
//      Menu Functions
//
void
M_DrawThermo
( int x,
  int y,
  int thermWidth,
  int thermDot )
{
   int  xx;
   int  i;

   xx = x;
   V_DrawNamePatch(xx, y, 0, "M_THERML", CR_DEFAULT, VPT_STRETCH);
   xx += 8;
   for (i=0;i<thermWidth;i++)
   {
      V_DrawNamePatch(xx, y, 0, "M_THERMM", CR_DEFAULT, VPT_STRETCH);
      xx += 8;
   }
   V_DrawNamePatch(xx, y, 0, "M_THERMR", CR_DEFAULT, VPT_STRETCH);
   V_DrawNamePatch((x+8)+thermDot*8,y,0,"M_THERMO",CR_DEFAULT,VPT_STRETCH);
}



void
M_DrawEmptyCell
( menu_t* menu,
  int  item )
{
   // CPhipps - patch drawing updated
   V_DrawNamePatch(menu->x - 10, menu->y+item*LINEHEIGHT - 1, 0,
                   "M_CELL1", CR_DEFAULT, VPT_STRETCH);
}

void
M_DrawSelCell
( menu_t* menu,
  int  item )
{
   // CPhipps - patch drawing updated
   V_DrawNamePatch(menu->x - 10, menu->y+item*LINEHEIGHT - 1, 0,
                   "M_CELL2", CR_DEFAULT, VPT_STRETCH);
}


void
M_StartMessage
( char*  string,
  void*  routine,
  boolean input )
{
   messageLastMenuActive = menuactive;
   messageToPrint = 1;
   messageString = string;
   messageRoutine = routine;
   messageNeedsInput = input;
   menuactive = true;
   return;
}



void M_StopMessage(void)
{
   menuactive = messageLastMenuActive;
   messageToPrint = 0;
}



//
// Find string width from hu_font chars
//
int M_StringWidth(const char* string)
{
   int i, c, w = 0;
   for (i = 0;(size_t)i < strlen(string);i++)
      w += (c = toupper(string[i]) - HU_FONTSTART) < 0 || c >= HU_FONTSIZE ?
           4 : SHORT(hu_font[c].width);
   return w;
}

//
//    Find string height from hu_font chars
//

int M_StringHeight(const char* string)
{
   int i, h, height = h = SHORT(hu_font[0].height);
   for (i = 0;string[i];i++)            // killough 1/31/98
      if (string[i] == '\n')
         h += height;
   return h;
}


//
//      Write a string using the hu_font
//
void
M_WriteText
( int  x,
  int  y,
  char*  string)
{
   int  w;
   char* ch;
   int  c;
   int  cx;
   int  cy;


   ch = string;
   cx = x;
   cy = y;

   while(1)
   {
      c = *ch++;
      if (!c)
         break;
      if (c == '\n')
      {
         cx = x;
         cy += 12;
         continue;
      }

      c = toupper(c) - HU_FONTSTART;
      if (c < 0 || c>= HU_FONTSIZE)
      {
         cx += 4;
         continue;
      }

      w = SHORT (hu_font[c].width);
      if (cx+w > 320)
         break;
      // proff/nicolas 09/20/98 -- changed for hi-res
      // CPhipps - patch drawing updated
      V_DrawNumPatch(cx, cy, 0, hu_font[c].lumpnum, CR_DEFAULT, VPT_STRETCH);
      cx+=w;
   }
}



//
// CONTROL PANEL
//

//
// M_Responder
//
boolean M_Responder (event_t* ev)
{
   int             ch;
   int             i;
//   static  int     joywait = 0;
//   static  int     mousewait = 0;
//   static  int     mousey = 0;
//   static  int     lasty = 0;
//   static  int     mousex = 0;
//   static  int     lastx = 0;

   ch = -1;

  // Process joystick input

/*   if (ev->type == ev_joystick && joywait < I_GetTime())
   {
      if (ev->data3 == -1)
      {
         ch = KEY_UPARROW;
         joywait = I_GetTime() + 5;
      }
      else if (ev->data3 == 1)
      {
         ch = KEY_DOWNARROW;
         joywait = I_GetTime() + 5;
      }

      if (ev->data2 == -1)
      {
         ch = KEY_LEFTARROW;
         joywait = I_GetTime() + 2;
      }
      else if (ev->data2 == 1)
      {
         ch = KEY_RIGHTARROW;
         joywait = I_GetTime() + 2;
      }

      if (ev->data1&1)
      {
         ch = key_menu_enter;
         joywait = I_GetTime() + 5;
      }
      if (ev->data1&2)
      {
         ch = KEY_BACKSPACE;
         joywait = I_GetTime() + 5;
      }
   }
   else
   {
      // Process mouse input
      if (ev->type == ev_mouse && mousewait < I_GetTime())
      {
         mousey += ev->data3;
         if (mousey < lasty-30)
         {
            ch = KEY_DOWNARROW;
            mousewait = I_GetTime() + 5;
            mousey = lasty -= 30;
         }
         else if (mousey > lasty+30)
         {
            ch = KEY_UPARROW;
            mousewait = I_GetTime() + 5;
            mousey = lasty += 30;
         }

         mousex += ev->data2;
         if (mousex < lastx-30)
         {
            ch = KEY_LEFTARROW;
            mousewait = I_GetTime() + 5;
            mousex = lastx -= 30;
         }
         else if (mousex > lastx+30)
         {
            ch = KEY_RIGHTARROW;
            mousewait = I_GetTime() + 5;
            mousex = lastx += 30;
         }

         if (ev->data1&1)
         {
            ch = key_menu_enter;
            mousewait = I_GetTime() + 15;
         }

         if (ev->data1&2)
         {
            ch = KEY_BACKSPACE;
            mousewait = I_GetTime() + 15;
         }
      }
      else */if (ev->type == ev_keydown)
      {
         ch = ev->data1;
      }
//   }

   if (ch == -1)
      return false;


   // Save Game string input
   if (saveStringEnter)
   {
      switch(ch)
      {
      case KEY_BACKSPACE:
         if (saveCharIndex > 0)
         {
            saveCharIndex--;
            savegamestrings[saveSlot][saveCharIndex] = 0;
         }
         break;

      case KEY_ESCAPE:
         saveStringEnter = 0;
         strcpy(&savegamestrings[saveSlot][0],saveOldString);
         break;

      case KEY_ENTER:
         saveStringEnter = 0;
         if (savegamestrings[saveSlot][0])
            M_DoSave(saveSlot);
         break;

      default:
         ch = toupper(ch);
         if (ch != 32)
            if (ch-HU_FONTSTART < 0 || ch-HU_FONTSTART >= HU_FONTSIZE)
               break;
         if (ch >= 32 && ch <= 127 &&
               saveCharIndex < SAVESTRINGSIZE-1 &&
               M_StringWidth(savegamestrings[saveSlot]) <
               (SAVESTRINGSIZE-2)*8)
         {
            savegamestrings[saveSlot][saveCharIndex++] = ch;
            savegamestrings[saveSlot][saveCharIndex] = 0;
         }
         break;
      }
      return true;
   }

   // Take care of any messages that need input
   if (messageToPrint)
   {
      if (messageNeedsInput == true &&
            !(ch == ' ' || ch == 'n' || ch == key_menu_enter || ch == key_menu_escape))
         return false;

      menuactive = messageLastMenuActive;
      messageToPrint = 0;
      if (messageRoutine)
         messageRoutine(ch);

      menuactive = false;
      S_StartSound(NULL,sfx_swtchx);
      return true;
   }
/*
   if (ch == KEY_F1) // devparm &&
   {
      G_ScreenShot ();
      return true;
   }
*/
   // F-Keys
   if (!menuactive)
      switch(ch)
      {
      case KEY_MINUS:         // Screen size down
         if ((automapmode & am_active) || chat_on)
            return false;
         M_SizeDisplay(0);
         S_StartSound(NULL,sfx_stnmov);
         return true;

      case KEY_EQUALS:        // Screen size up
         if ((automapmode & am_active) || chat_on)
            return false;
         M_SizeDisplay(1);
         S_StartSound(NULL,sfx_stnmov);
         return true;
/*
      case KEY_F1:            // Help key
         M_StartControlPanel ();

         if ( gamemode == retail )
            currentMenu = &ReadDef2;
         else
            currentMenu = &ReadDef1;

         itemOn = 0;
         S_StartSound(NULL,sfx_swtchn);
         return true;

      case KEY_F6:            // Quicksave
         S_StartSound(NULL,sfx_swtchn);
         M_QuickSave();
         return true;

      case KEY_F9:            // Quickload
         S_StartSound(NULL,sfx_swtchn);
         M_QuickLoad();
         return true;
*/
      }


   // Pop-up menu?
   if (!menuactive)
   {
      if (ch == key_menu_escape)
      {
         M_StartControlPanel ();
         S_StartSound(NULL,sfx_swtchn);
         return true;
      }
      return false;
   }


   // Keys usable within menu
   switch (ch)
   {
   case KEY_DOWNARROW:
      do
      {
         if (itemOn+1 > currentMenu->numitems-1)
            itemOn = 0;
         else
            itemOn++;
         S_StartSound(NULL,sfx_pstop);
      }
      while(currentMenu->menuitems[itemOn].status==-1);
      return true;

   case KEY_UPARROW:
      do
      {
         if (!itemOn)
            itemOn = currentMenu->numitems-1;
         else
            itemOn--;
         S_StartSound(NULL,sfx_pstop);
      }
      while(currentMenu->menuitems[itemOn].status==-1);
      return true;

   case KEY_LEFTARROW:
      if (currentMenu->menuitems[itemOn].routine &&
            currentMenu->menuitems[itemOn].status == 2)
      {
         S_StartSound(NULL,sfx_stnmov);
         currentMenu->menuitems[itemOn].routine(0);
      }
      return true;

   case KEY_RIGHTARROW:
      if (currentMenu->menuitems[itemOn].routine &&
            currentMenu->menuitems[itemOn].status == 2)
      {
         S_StartSound(NULL,sfx_stnmov);
         currentMenu->menuitems[itemOn].routine(1);
      }
      return true;

   case KEY_ENTER:
      if (currentMenu->menuitems[itemOn].routine &&
            currentMenu->menuitems[itemOn].status)
      {
         currentMenu->lastOn = itemOn;
         if (currentMenu->menuitems[itemOn].status == 2)
         {
            currentMenu->menuitems[itemOn].routine(1);      // right arrow
            S_StartSound(NULL,sfx_stnmov);
         }
         else
         {
            currentMenu->menuitems[itemOn].routine(itemOn);
            S_StartSound(NULL,sfx_pistol);
         }
      }
      return true;

   case KEY_ESCAPE:
      currentMenu->lastOn = itemOn;
      M_ClearMenus ();
      S_StartSound(NULL,sfx_swtchx);
      return true;

   case KEY_BACKSPACE:
      currentMenu->lastOn = itemOn;
      if (currentMenu->prevMenu)
      {
         currentMenu = currentMenu->prevMenu;
         itemOn = currentMenu->lastOn;
         S_StartSound(NULL,sfx_swtchn);
      }
      return true;

   default:
      for (i = itemOn+1;i < currentMenu->numitems;i++)
         if (currentMenu->menuitems[i].alphaKey == ch)
         {
            itemOn = i;
            S_StartSound(NULL,sfx_pstop);
            return true;
         }
      for (i = 0;i <= itemOn;i++)
         if (currentMenu->menuitems[i].alphaKey == ch)
         {
            itemOn = i;
            S_StartSound(NULL,sfx_pstop);
            return true;
         }
      break;

   }

   return false;
}



//
// M_StartControlPanel
//
void M_StartControlPanel (void)
{
   // intro might call this repeatedly
   if (menuactive)
      return;

   menuactive = 1;
   currentMenu = &MainDef;         // JDC
   itemOn = currentMenu->lastOn;   // JDC
}


//
// M_Drawer
// Called after the view has been rendered,
// but before it has been blitted.
//
void M_Drawer (void)
{
   static short x;
   static short y;
   unsigned short  i;
   short  max;
   char  string[40];
   int   start;

   inhelpscreens = false;


   // Horiz. & Vertically center string and print it.
   if (messageToPrint)
   {
      start = 0;
      y = 100 - M_StringHeight(messageString)/2;
      while(*(messageString+start))
      {
         for (i = 0;i < strlen(messageString+start);i++)
            if (*(messageString+start+i) == '\n')
            {
               memset(string,0,40);
               strncpy(string,messageString+start,i);
               start += i+1;
               break;
            }
         if (i == strlen(messageString+start))
         {
            strcpy(string,messageString+start);
            start += i;
         }

         x = 160 - M_StringWidth(string)/2;
         M_WriteText(x,y,string);
         y += SHORT(hu_font[0].height);
      }
      return;
   }

   if (!menuactive)
      return;

   if (currentMenu->routine)
      currentMenu->routine();         // call Draw routine

   // DRAW MENU
   x = currentMenu->x;
   y = currentMenu->y;
   max = currentMenu->numitems;

   for (i=0;i<max;i++)
   {
      if (currentMenu->menuitems[i].name[0])
         V_DrawNamePatch(x,y,0,currentMenu->menuitems[i].name,
                         CR_DEFAULT, VPT_STRETCH);
      y += LINEHEIGHT;
   }

   // DRAW SKULL
   // CPhipps - patch drawing updated
   V_DrawNamePatch(x + SKULLXOFF, currentMenu->y - 5 + itemOn*LINEHEIGHT,0,
                   skullName[whichSkull], CR_DEFAULT, VPT_STRETCH);

}


//
// M_ClearMenus
//
void M_ClearMenus (void)
{
   menuactive = 0;
   // if (!netgame && usergame && paused)
   //       sendpause = true;
}




//
// M_SetupNextMenu
//
void M_SetupNextMenu(menu_t *menudef)
{
   currentMenu = menudef;
   itemOn = currentMenu->lastOn;
}


//
// M_Ticker
//
void M_Ticker (void)
{
   if (--skullAnimCounter <= 0)
   {
      whichSkull ^= 1;
      skullAnimCounter = 8;
   }
}


//
// M_Init
//
void M_Init (void)
{
   currentMenu = &MainDef;
   menuactive = 0;
   itemOn = currentMenu->lastOn;
   whichSkull = 0;
   skullAnimCounter = 10;
   screenSize = screenblocks - 3;
   messageToPrint = 0;
   messageString = NULL;
   messageLastMenuActive = menuactive;
   quickSaveSlot = -1;

   // Here we could catch other version dependencies,
   //  like HELP1/2, and four episodes.


   switch ( gamemode )
   {
   case commercial:
      // This is used because DOOM 2 had only one HELP
      //  page. I use CREDIT as second page now, but
      //  kept this hack for educational purposes.
      MainMenu[readthis] = MainMenu[quitdoom];
      MainDef.numitems--;
      MainDef.y += 8;
      NewDef.prevMenu = &MainDef;
      ReadDef1.routine = M_DrawReadThis1;
      ReadDef1.x = 330;
      ReadDef1.y = 165;
      ReadMenu1[0].routine = M_FinishReadThis;
      break;
   case shareware:
      // Episode 2 and 3 are handled,
      //  branching to an ad screen.
   case registered:
      // We need to remove the fourth episode.
      EpiDef.numitems--;
      break;
   case retail:
      // We are fine.
   default:
      break;
   }

}

