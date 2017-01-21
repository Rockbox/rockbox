/*
    cannoneer.c

    Copyright (C) 1987, 1989  Eckhard Kruse
    Copyright (C) 2010  Thomas Huth

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <SDL.h>

#include "i18n.h"
#include "ballergui.h"
#include "baller1.h"
#include "sdlgui.h"
#include "screen.h"
#include "sdlgfx.h"


static void draw_cannoneer(int x, int y, int w, int h);


static short wi, pv;    /* Winkel und Pulver */

/* The cannoneer dialog data: */

#define WL2 3		/* Winkel um 10 verkleinern */
#define WL1 4		/* Winkel um 1 verkleinern */
#define WR1 6
#define WR2 7
#define PL2 9
#define PL1 10
#define PR1 12
#define PR2 13
#define SOK 14
#define SAB 15

// #define WINK TBD
// #define PULV TBD

static char dlg_winkel[4];
static char dlg_pulver[3];

static SGOBJ cannoneerdlg[] =
{
	{ SGBOX, 0, 0, 0,0, 42,15, NULL },
	{ SGTEXT, 0, 0, 18,1, 6,1, N_("Cannon") },

	{ SGTEXT, 0, 0, 2,3, 7,1, N_("Angle:") },
	{ SGBUTTON, SG_EXIT, 0, 12,3, 4,1, "\x04\04" },   // 2 arrows left
	{ SGBUTTON, SG_EXIT, 0, 17,3, 3,1, "\x04" },      // Arrow left
	{ SGTEXT, 0, 0, 22,3, 4,1, dlg_winkel },
	{ SGBUTTON, SG_EXIT, 0, 26,3, 3,1, "\x03" },      // Arrow right
	{ SGBUTTON, SG_EXIT, 0, 30,3, 4,1, "\x03\x03" },  // 2 arrows right

	{ SGTEXT, 0, 0, 2,5, 7,1, N_("Gunpowder:") },
	{ SGBUTTON, SG_EXIT, 0, 12,5, 4,1, "\x04\04" },   // 2 arrows left
	{ SGBUTTON, SG_EXIT, 0, 17,5, 3,1, "\x04" },      // Arrow left
	{ SGTEXT, 0, 0, 22,5, 4,1, dlg_pulver },
	{ SGBUTTON, SG_EXIT, 0, 26,5, 3,1, "\x03" },      // Arrow right
	{ SGBUTTON, SG_EXIT, 0, 30,5, 4,1, "\x03\x03" },  // 2 arrows right

	{ SGBUTTON, SG_DEFAULT, 0, 33,11, 8,1, "OK" },
	{ SGBUTTON, SG_CANCEL, 0, 33,13, 8,1, "Cancel" },

	{ SGBOX, 0, 0, 2,7, 30,7, NULL },
	{ SGUSER, 0, 0, 2,7, 30,7, (void*)draw_cannoneer },

	{ -1, 0, 0, 0,0, 0,0, NULL }
};


/**
 * Draw cannoneer
 */
static void draw_cannoneer(int x, int y, int w, int h)
{
	static short fig[]={ 0,0,15,20,30,20,20,15,10,0,10,-30,18,-18,20,-5,24,-6,
	                     20,-25,10,-40,0,-45,
	                     -10,-40,-20,-25,-24,-6,-20,-5,-18,-18,-10,-30,-10,0,
	                     -20,15,-30,20,-15,20, -1,-1
	                   }; /* Daten für das Männchen */
	int xk,yk;
	int fl;
	int i;
	double s,c;
	SDL_Rect rect;

	rect.x = x + 1;
	rect.y = y + 1;
	rect.w = w - 2;
	rect.h = h - 2;
	SDL_FillRect(surf, &rect, SDL_MapRGB(surf->format,0xff,0xff,0xff));

	xk = x + w / 2;
	yk = y + h / 2 + 40;

	/* Draw the cannoneer man */
	color(1);
	filledCircleColor(surf, xk-88*f, yk-60, 15, 0x000000ff);

	i=0;
	while ( fig[i]!=-1 )
	{
		xy[i]=xk-88*f+fig[i];
		i++;
		xy[i]=yk-5+fig[i];
		i++;
	}
	xy[i++]=xy[0];
	xy[i++]=xy[1];
	scr_fillarea(i/2-1, xy);

	/* Draw the cannon */
	color( 1 );
	filledCircleColor(surf, xk, yk, 15, 0x000000ff);

	s=sin(wi/P57);
	c=cos(wi/P57);
	fl=-f;
	if ( wi>90 )
	{
		fl=-fl;
		c=-c;
	}
	xy[0]=xk+fl*(c*14+s*14);
	xy[1]=yk+s*14-c*14;
	xy[2]=xk+fl*(c*14+s*40);
	xy[3]=yk+s*14-c*40;
	xy[4]=xk-fl*(c*55-s*40);
	xy[5]=yk-s*55-c*40;
	xy[6]=xk-fl*(c*55-s*14);
	xy[7]=yk-s*55-c*14;
	xy[8]=xy[0];
	xy[9]=xy[1];
	scr_fillarea(4, xy);
}


/**
 * Kanonenobjektbaum, Wahl von Winkel und Pulver
 */
int sch_obj(short k)
{
	short i = 0;
	char *aw,*ap;

	dlg_winkel[0] = dlg_pulver[0] = 0;
	aw = dlg_winkel;
	ap = dlg_pulver;
	*(ap+2)=0;

	SDLGui_CenterDlg(cannoneerdlg);

	wi=ka[n][k].w;
	pv=ka[n][k].p;

	scr_sf_interior(1);

	do
	{
		if (pv > pu[n])
		{
			pv=pu[n];
		}
		*aw=48+wi/100;
		*(aw+1)=48+wi%100/10;
		*(aw+2)=48+wi%10;
		if (wi < 100)
		{
			*aw=*(aw+1);
			*(aw+1)=*(aw+2);
			*(aw+2)=0;
		}
		*ap=48+pv/10;
		*(ap+1)=48+pv%10;
		if (pv<10)
		{
			*ap=*(ap+1);
			*(ap+1)=0;
		}

		i = SDLGui_DoDialog(cannoneerdlg, NULL);

		wi-=10*(i==WL2)-10*(i==WR2)+(i==WL1)-(i==WR1);
		if ( wi<0 ) wi=0;
		if ( wi>180 ) wi=180;
		pv-= 3*(i==PL2)- 3*(i==PR2)+(i==PL1)-(i==PR1);
		if ( pv<5 ) pv=5;
		if ( pv>20 ) pv=20;
	}
	while (i != SOK && i != SAB);

	SDL_UpdateRect(surf, 0,0, 0,0);

	ka[n][k].w=wi;
	ka[n][k].p=pv;

	return (i == SOK);
}
