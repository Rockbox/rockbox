/*
    market.c

    Copyright (C) 1987, 1989  Eckhard Kruse
    Copyright (C) 2010, 2013  Thomas Huth

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


#include "i18n.h"
#include "baller1.h"
#include "baller2.h"
#include "ballergui.h"
#include "screen.h"
#include "sdlgui.h"
#include "market.h"


static char dlg_geld[6];
static char dlg_geschuetze[6];
static char dlg_pulver[6];
static char dlg_volk[6];
static char dlg_ftuerme[6];
static char dlg_fahne[6];
static char dlg_kugeln[6];
static char dlg_steuern[6];

static char dlg_anbauenpreis[6];
static char dlg_fturmpreis[6];
static char dlg_geschuetzpreis[6];
static char dlg_fahnepreis[6];
static char dlg_pulverpreis[6];
static char dlg_kugelpreis[6];

#define SH1	11	/* Geld */
#define SH2	12	/* Fördertürme */
#define SH3	13	/* Geschütze */
#define SH4	14	/* Windfahne */
#define SH5	15	/* Pulver */
#define SH6	16	/* Kugeln */
#define SH7	17	/* Volk */
#define SH8	18	/* Steuern */
#define SHK	19	/* Steuern runter */
#define SHG	20	/* Steuern rauf */

#define SM1	29	/* Preis für Anbauen */
#define SM2	30	/* Preis für Förderturm */
#define SM3	31	/* Preis für Geschütz */
#define SM4	32	/* Preis für Windfahne */
#define SM5	33	/* Preis für Pulver */
#define SM6	34	/* Preis für Kugeln */

#define FERTIG	35

static SGOBJ marktdlg[] =
{
	{ SGBOX, 0, 0, 0,0, 54,18, NULL },

	{ SGBOX, 0, 0, 1,1, 52,7, NULL },
	{ SGTEXT, 0, 0, 23,1, 8,1, N_("You have:") },

	{ SGTEXT, 0, 0, 2,3, 10,1, N_("Funds:") },
	{ SGTEXT, 0, 0, 28,3, 12,1, N_("Shaft towers:") },
	{ SGTEXT, 0, 0, 2,4, 10,1, N_("Cannons:") },
	{ SGTEXT, 0, 0, 28,4, 12,1, N_("Weather vane:") },
	{ SGTEXT, 0, 0, 2,5, 10,1, N_("Gunpowder:") },
	{ SGTEXT, 0, 0, 28,5, 12,1, N_("Cannonballs:") },
	{ SGTEXT, 0, 0, 2,6, 10,1, N_("Population:") },
	{ SGTEXT, 0, 0, 28,6, 12,1, N_("Taxes in %") },
	{ SGTEXT, 0, 0, 19,3, 5,1, dlg_geld },
	{ SGTEXT, 0, 0, 47,3, 5,1, dlg_ftuerme },
	{ SGTEXT, 0, 0, 19,4, 5,1, dlg_geschuetze },
	{ SGTEXT, 0, 0, 47,4, 5,1, dlg_fahne },
	{ SGTEXT, 0, 0, 19,5, 5,1, dlg_pulver },
	{ SGTEXT, 0, 0, 47,5, 5,1, dlg_kugeln },
	{ SGTEXT, 0, 0, 19,6, 5,1, dlg_volk },
	{ SGTEXT, 0, 0, 45,6, 5,1, dlg_steuern },
	{ SGBUTTON, SG_EXIT, 0, 46,6, 1,1, SGARROWLEFTSTR },
	{ SGBUTTON, SG_EXIT, 0, 51,6, 1,1, SGARROWRIGHTSTR },

	{ SGBOX, 0, 0, 1,9, 52,6, NULL },
	{ SGTEXT, 0, 0, 23,9, 8,1, N_("Market:") },

	{ SGTEXT, 0, 0, 2,11, 10,1, N_("Lay bricks:") },
	{ SGTEXT, 0, 0, 28,11, 12,1, N_("Shaft tower:") },
	{ SGTEXT, 0, 0, 2,12, 10,1, N_("Cannon:") },
	{ SGTEXT, 0, 0, 28,12, 12,1, N_("Weather vane:") },
	{ SGTEXT, 0, 0, 2,13, 10,1, N_("Powder x30:") },
	{ SGTEXT, 0, 0, 28,13, 12,1, N_("2 cannonballs:") },
	{ SGTEXT, 0, 0, 19,11, 5,1, dlg_anbauenpreis },
	{ SGTEXT, 0, 0, 47,11, 5,1, dlg_fturmpreis },
	{ SGTEXT, 0, 0, 19,12, 5,1, dlg_geschuetzpreis },
	{ SGTEXT, 0, 0, 47,12, 5,1, dlg_fahnepreis },
	{ SGTEXT, 0, 0, 19,13, 5,1, dlg_pulverpreis },
	{ SGTEXT, 0, 0, 47,13, 5,1, dlg_kugelpreis },

	{ SGBUTTON, SG_DEFAULT, 0, 21,16, 12,1, N_("Done") },

	{ -1, 0, 0, 0,0, 0,0, NULL }
};


/**
 * Anbauen
 */
static void anbau(void)
{
	short s;
	char brickstr[80];
	void *savearea;

	/* Save background */
	savearea = scr_save_bg(220, 375-12, 25*8, 38);

	color(1);
	scr_sf_interior(2);
	scr_sf_style(9);
	scr_ctr_text(320, 365, _(" Lay bricks: "));
	sprintf(brickstr,  _(" Bricks left: %02d "), 20);
	scr_ctr_text(320, 382, brickstr);
	s=20;

	scr_color(0x909080);

	do
	{
		if (event(1, 0))
		{
			scr_exit();
			exit(0);
		}

		if ( bt && (n? mx>624-bg[0] : mx<bg[0]+15 ) && my>155 )
		{
			if ( !( loc(mx,my) || loc(mx+1,my+1) || loc(mx-1,my-1) ) &&
			                ( loc(mx+3,my-1)||loc(mx+3,my+1)||loc(mx-3,my-1)||loc(mx-3,my+1)||
			                  loc(mx+1,my+2)||loc(mx-1,my+2)||loc(mx+1,my-2)||loc(mx-1,my-2) ))
			{
				xy[0]=mx-2;
				xy[1]=my-1;
				xy[2]=mx+2;
				xy[3]=my+1;
				scr_bar(xy);
				s--;
				sprintf(brickstr,  _(" Bricks left: %02d "), s);
				scr_ctr_text(320, 382, brickstr);
			}
		}
	}
	while ( s>0 && bt<2 );

	/* Restore background */
	scr_restore_bg(savearea);
}


/**
 * 5-stellige Zahl, rechtsbündig, ohne führende Nullen
 */
static void zahl(short nr, short wert)
{
	short i,a,b;
	char *adr;

	adr = marktdlg[nr].txt;

	for (b = i = 0, a = 10000; i < 5; i++, a /= 10)
	{
		*adr++ = 48 + wert/a - 16*(wert<a && i<4 && !b);
		b |= wert/a;
		wert %= a;
	}
}


/**
 * Markt dialog
 */
void markt(void)
{
	short a,k,ko,t;

	SDLGui_CenterDlg(marktdlg);

	for (a = 0; a < 6; a++)
	{
		zahl(SM1 + a, p[a]);
	}

	do
	{
		for (t = k = 0; k < 5; )
		{
			/* Fördertürme zählen */
			t += (ft[n][k++].x > -1);
		}
		for (ko = k = 0; k < 10; )
		{
			/* Kanonen zählen */
			ko += (ka[n][k++].x > -1);
		}
		for (k = 0; k < 10; k++)
		{
			if (bg[1+k*2] > -1 && ka[n][k].x == -1)
			{
				/* Platz für neue Kanone? */
				for (a = 1; a < 10; a++)
				{
					if (loc(639*n+(bg[1+k*2]+5+a)*f, by[n]-bg[2+k*2]-a))
						break;
				}
				if (a > 9)
					break;
			}
		}

		zahl(SH1, ge[n]);
		zahl(SH2, t);
		zahl(SH3, ko);
		zahl(SH4, wx[n]>-1);
		zahl(SH5, pu[n]);
		zahl(SH6, ku[n]);
		zahl(SH7, vo[n]);
		zahl(SH8, st[n]);

		for (a = 0; a < 6; a++)
		{
			// fprintf(stderr,"a=%i: ge=%i p=%i an_erl=%i bg=%i t=%i k=%i wx=%i\n",
			//	   a, ge[n], p[a], an_erl, bg[0], t, k, wx[n]);
			if (ge[n] < p[a]
			    || (!an_erl && a == 0)
			    || (a == 1 && (bg[0]+t*30 > 265 || t > 4))
			    || (a == 2 && k > 9)
			    || (a == 3 && wx[n] > -1))
			{
				marktdlg[SM1 + a].type = SGTEXT;
			}
			else
			{
				marktdlg[SM1 + a].type = SGBUTTON;
			}
		}

		a = SDLGui_DoDialog(marktdlg, NULL);

		if (a == SHK)
		{
			st[n]-=2*(st[n]>0);
		}
		else if (a == SHG)
		{
			 st[n]+=2*(st[n]<100);
		}
		else if (a != FERTIG && a >= SM1 && a <= SM6)
		{
			ge[n] -= p[a-SM1];
			if (a < SM5)
			{
				drw_all();
				if (a == SM1) anbau();
				else if (a == SM2) fturm();
				else if (a == SM3) init_ka( k,639*n );
				else if (a == SM4)
				{
					wx[n]=639*n+f*bg[23];
					wy[n]=by[n]-bg[24];
					werdran(1);
				}
			}
			else
			{
				pu[n]+=30*(a==SM5);
				ku[n]+=2*(a==SM6);
				drw_gpk(a-SM4);
				drw_gpk(0);
			}
		}
	}
	while (a != FERTIG);

	SDL_UpdateRect(surf, 0,0, 0,0);
}
