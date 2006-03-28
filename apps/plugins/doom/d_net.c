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
 *    Network client. Passes information to/from server, staying
 *    synchronised.
 *    Contains the main wait loop, waiting for network input or
 *    time before doing the next tic.
 *    Rewritten for LxDoom, but based around bits of the old code.
 *
 *-----------------------------------------------------------------------------
 */

#include "m_menu.h"
#include "i_system.h"
#include "i_video.h"
#include "i_sound.h"
#include "g_game.h"
#include "doomdef.h"
#include "doomstat.h"

#include "rockmacros.h"

#define NCMD_EXIT  0x80000000
#define NCMD_RETRANSMIT  0x40000000
#define NCMD_SETUP  0x20000000
#define NCMD_KILL  0x10000000 // kill game
#define NCMD_CHECKSUM   0x0fffffff


doomcom_t* doomcom;

static boolean   server=0;
static int       remotetic; // Tic expected from the remote
static int       remotesend; // Tic expected by the remote

//
// NETWORKING
//
// gametic is the tic about to (or currently being) run
// maketic is the tick that hasn't had control made for it yet
// nettics[] has the maketics for all players
//
// a gametic cannot be run until nettics[] > gametic for all players
//

static ticcmd_t* localcmds;

ticcmd_t        netcmds[MAXPLAYERS][BACKUPTICS];

int             maketic;
int  ticdup=1;

void G_BuildTiccmd (ticcmd_t *cmd);
void D_DoAdvanceDemo (void);

void D_InitNetGame (void)
{
   int i;

   doomcom = Z_Malloc(sizeof *doomcom, PU_STATIC, NULL);
   doomcom->consoleplayer = 0;
   doomcom->numnodes = 0; doomcom->numplayers = 1;
   localcmds = netcmds[consoleplayer];

   for (i=0; i<doomcom->numplayers; i++)
      playeringame[i] = true;
   for (; i<MAXPLAYERS; i++)
      playeringame[i] = false;

   consoleplayer = displayplayer = doomcom->consoleplayer;
}

void D_BuildNewTiccmds()
{
   static int lastmadetic;
   int newtics = I_GetTime() - lastmadetic;
   lastmadetic += newtics;
   while (newtics--)
   {
      I_StartTic();
      if (maketic - gametic > BACKUPTICS/2) break;
      G_BuildTiccmd(&localcmds[maketic%BACKUPTICS]);
      maketic++;
   }
}

//
// TryRunTics
//
extern boolean advancedemo;

void TryRunTics (void)
{
   int runtics;
   int entertime = I_GetTime();

   // Wait for tics to run
   while (1) {
      D_BuildNewTiccmds();
      runtics = (server ? remotetic : maketic) - gametic;
      if (!runtics) {
//         if (server) I_WaitForPacket(ms_to_next_tick);
//         else I_uSleep(ms_to_next_tick*1000);
//         rb->sleep(ms_to_next_tick);
         if (I_GetTime() - entertime > 10) {
            remotesend--;
//            {
//               char buf[sizeof(packet_header_t)+1];
//               packet_set((packet_header_t *)buf, PKT_RETRANS, remotetic);
//               buf[sizeof(buf)-1] = consoleplayer;
//               I_SendPacket((packet_header_t *)buf, sizeof buf);
//            }
            M_Ticker(); return;
         }
      } else break;
   }

   while (runtics--) {
      if (advancedemo)
         D_DoAdvanceDemo ();
      M_Ticker ();
      G_Ticker ();
      gametic++;
   }
}
