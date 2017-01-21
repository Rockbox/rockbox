/*
    baller1.c

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

/**********************************************************************
 *                  B a l l e r b u r g          Modul 1              *
 * Dies ist der Hauptteil von Ballerburg. Die Routinen dieses Teils   *
 * dienen im großen und ganzen der Steuerung des Programmes, sowie    *
 * der Ausführung elementarer Grafikoperationen.                      *
 **********************************************************************/

#include "baller1.h"
#include "baller2.h"
#include "ballergui.h"
#include "screen.h"
#include "psg.h"
#include "market.h"
#include "music.h"
#include "paths.h"
#include "i18n.h"

#define Min(a,b)  ((a)<(b)?(a):(b))
#define Max(a,b)  ((a)>(b)?(a):(b))
#define menu(a)   /* wind_update(3-a) */
#define hide()    /* graf_mouse(256,0) */
#define show()    /* graf_mouse(257,0) */

#define COMP_NUM  7

short mx, my, bt, xy[100],
	bur[2],bx[2],by[2], ge[2],pu[2],ku[2],vo[2],st[2],kn[2],
	wx[2],wy[2], *bg, zug,n, p[6], max_rund,
	*burgen[30], b_anz;
static short ws, wc, t_gew[COMP_NUM][COMP_NUM+4];
int ftx, fty, ftw, fth;     /* Koordinaten von "Fertig" */
void *bur_ad;
const char *l_nam, *r_nam;
int f;
char  mod, wnd, end, txt[4], an_erl, au_kap;

int cw[2] = { 2, 2 };		/* Computer strategy */
int cx[2] = { 1, 1 };		/* Computer strength */

/* Computer player names (i.e. the strategies) */
const char *cn[COMP_NUM] = {
	N_("Oaf"), N_("Yokel"), N_("Boor"), N_("Doofus"),
	N_("Fumbler"), N_("Geezer"), N_("Ruffian")
};

char nsp1[22], nsp2[22];	/* Names of the players */

ka_t ka[2][10];
ft_t ft[2][5];


/**
 * Initialize internationalization support
 */
static void i18n_init(void)
{
#if HAVE_LIBINTL_H
	setlocale(LC_ALL, "");
	bindtextdomain("ballerburg", Paths_GetLocaleDir());
	textdomain("ballerburg");
#endif
}


/*****************************************************************************/
int ballerburg_main(int argc, char **argv)
{
    rb->splash(HZ, "point 1");
	Paths_Init(argv[0]);
	i18n_init();

	scr_init();
	scr_init_done_button(&ftx, &fty, &ftw, &fth);

	m_laden("/baller.mus");  /* Laden der Musikdatei mit Funktion aus music.c */

    rb->splash(HZ, "point 2");

	bur_ad = malloc(32000); /* Speicher für Burgdaten */
	if (!bur_ad)
	{
		puts(_("Not enough memory for loading the castles."));
		exit(-1);
	}
    rb->splash(HZ, "point 3");

	an_erl=1;
	max_rund=32767;
	au_kap=1;
	t_load();
	burgen_laden();
    rb->splash(HZ, "point 4");

	psg_audio_init();
    rb->splash(HZ, "point 5");

	strcpy(nsp1, _("William"));
	strcpy(nsp2, _("Frederick"));
	l_nam = nsp1;
	r_nam = nsp2;

	mod=0;                        /* Spielmodus ( wer gegen wen ) */

	neues();
    rb->splash(HZ, "point 6");
	while ( ein_zug() );

	t_save();

	scr_exit();
	m_quit();
	free(bur_ad);

	return 0;
}


/**************************** Tabelle ****************************************/
void tabelle(void)
{
	short i,j;
	void *save_area;

	save_area = scr_save_bg(17, 55, 640-17*2+1, 400-55*2+24+1);

	scr_sf_interior(0);
	box(17,56, 623,367, 1);
	box(19,58, 621,365, 1);
	box(20,59, 620,364, 1);
	line(108,59, 108,364);

	for (i = 116; i < 584; i += 72)
		line(i,59,i,364);
	line(20,84, 620,84);
	for (i = 92; i < 264; i += 24)
		line(20,i, 620,i);
	for (i = 268; i < 364; i += 24)
		line(20,i, 620,i);
	for (i = 0; i < COMP_NUM; i++)
		scr_l_text(124+i*72, 78, _(cn[i]));
	for (i = 0; i < COMP_NUM; i++)
		scr_l_text(36, 110+i*24, _(cn[i]));

	scr_sf_interior(2);
	scr_sf_style(1);
	for (i = 0; i < COMP_NUM; i++)
	{
		for (j = 0; j < COMP_NUM+4; j++)
		{
			z_txt(t_gew[i][j]);
			if (j == COMP_NUM+3 && !t_gew[i][COMP_NUM])
			{
				txt[0]=32;
				txt[1]='-';
				txt[2]=0;
			}
			scr_l_text(140+i*72, 110+j*24+8*(j>=COMP_NUM), txt);
			if (i == j)
				box(116+i*72, 92+j*24, 188+i*72, 116+j*24, 1);
		}
	}

	scr_l_text(28, 286, _("Games"));
	scr_l_text(28, 310, _("Total won"));
	scr_l_text(28, 334, _("Total lost"));
	scr_l_text(28, 358, _("Won in %"));

	line(20, 59, 108, 84);
	scr_l_text(36, 80, "-");
	scr_l_text(88, 72, "+");

	while (bt == 0 && event(1, 0) == 0);
	while (bt != 0 && event(1, 0) == 0);

	scr_restore_bg(save_area);
}


void z_txt(short a)
{
	txt[0]=a/100+48;
	txt[1]=a%100/10+48;
	txt[2]=a%10+48;
	if (a<100)
	{
		txt[0]=32;
		if (a<10) txt[1]=32;
	}
}


/******************** Initialisierung vor neuem Spiel ************************/
void neues(void)
{
	static short pr[6]={ 200,500,400,150,50,50 };   /* Preise zu Beginn */
	short j;

	wnd=rand()%60-30;
	st[0]=st[1]=20;
	kn[0]=kn[1]=0;
	for ( j=0;j<6;j++ )
		p[j]=pr[j]*(95+rand()%11)/100;
	bild();
	for ( n=0;n<2;n++ )
	{
		bg=burgen[bur[n]];
		wx[n]=n? 639-bg[23]:bg[23];
		wy[n]=by[n]-bg[24];
		for ( f=0;f<5; ) ft[n][f++].x=-1;
	}
	zug=n=end=0;
	f=1;
}


/************************* Durchführen eines Zuges ***************************/
int ein_zug(void)
{
	short i = 0, fl, a;

	// puts("ein zug ...");

	n=zug&1;
	fn();
	kn[n]&=~16;
	wnd=wnd*9/10+rand()%12-6;

	werdran(1);

	do
	{
		fl=0;
		menu(1);
		do
		{
			if (event(!(mod&(2-n)), 1) != 0)
				return(0);
		}
		while (!bt && !(mod&(2-n)));
		//printf("ein zug %i %i %i\n", bt, mod, a);
		menu(0);
		bg=burgen[bur[n]];

		if ( mod&(2-n) )
		{
			hide();
			i=comp();
			show();
			fl=1+(i<0);
		}
		else if (mx > ftx && mx < ftx+ftw && my > fty && my < fty+fth)
		{
			/* "Fertig"-Knopf wurde gedrueckt */
			scr_draw_done_button(1);
			do
			{
				event(1, 0);
			} while (bt);
			scr_draw_done_button(0);
			fl=2;
		}
		else
		{
			for ( i=0;i<10;i++ )
				if ( ka[n][i].x<=mx && ka[n][i].x+20>=mx && ka[n][i].x!=-1 &&
				                ka[n][i].y>=my && ka[n][i].y-14<=my ) break;
			if ( i>9 )
			{
				if (drin( bg[25],bg[26],bg[31],bg[32],0,mx,my)
				    || drin( bg[27],bg[28],bg[33],bg[34],0,mx,my)
				    || drin( bg[29],bg[30],bg[35],bg[36],0,mx,my))
					markt();
				else if (drin( bg[21],bg[22],30,25,0,mx,my))
					koenig();
			}
			else if (pu[n] < 5)
			{
				DlgAlert_Notice(_("You don't have enough gunpowder."), _("Cancel"));
			}
			else if (ku[n] == 0)
			{
				DlgAlert_Notice(_("You don't have any cannonballs left."), _("Cancel"));
			}
			else fl=sch_obj(i);
		}
	}
	while ( !fl );

	werdran(0);
	if ( fl<2 ) schuss(i);

	if ( ~kn[n]&16 ) kn[n]&=~15;
	rechnen();
	zug++;

	for ( i=0;i<10;i++ ) if ( ka[n][i].x>-1 ) break;
	n=zug&1;
	bg=burgen[bur[n]];
	for ( a=0;a<10;a++ ) if ( ka[n][a].x>-1 ) break;
	if ( a==10 && i<10 && bg[40]>vo[n] && ge[n]<p[2]/3 && au_kap &&
	                ft[n][0].x+ft[n][1].x+ft[n][2].x+ft[n][3].x+ft[n][4].x==-5 )
		end=n+33;

	if ( !end && zug/2>=max_rund )
	{
		static int h[2];
		for (n=0;n<2;n++)
		{
			h[n]=ge[n]+pu[n]*p[4]/30+ku[n]*p[5]/2+(wx[n]>-1)*p[3]+vo[n]*4;
			for (i=0;i<5;i++) if ( ft[n][i].x>-1 ) h[n]+=p[1];
			for (i=0;i<10;i++) if ( ka[n][i].x>-1 ) h[n]+=p[2];
		}
		end=65+(h[1]<h[0]);
	}
	if ( end )
	{
		ende();
		menu(1);
		while (!bt)
		{
			if (event(1, 0) != 0)
				return 0;
		}
		neues();
		menu(0);
	}
	return(1);
}


/********** Berechnen von Bevölkerungszuwachs usw. nach jedem Zug ************/
void rechnen(void)
{
	short j;
	static short pmi[6]={ 98,347,302,102,30,29 },   /* Preisgrenzen */
	             pma[6]={ 302,707,498,200,89,91 },
	             psp[6]={ 10,50,50,20,10,10 };     /* max. Preisschwankung */

	j=st[n];
	ge[n]+=vo[n]*(j>65? 130-j:j)/(150-rand()%50);
	vo[n]=vo[n]*(95+rand()%11)/100+(21-j+rand()%9)*(8+rand()%5)/20;
	if ( vo[n]<0 )
	{
		vo[n]=0;
		end=n+49;
	}

	for ( j=0;j<5;j++ ) ge[n]+=(40+rand()%31)*(ft[n][j].x>-1);
	for ( j=0;j<6;j++ )
	{
		p[j]+=psp[j]*(rand()%99)/98-psp[j]/2;
		p[j]=Max(p[j],pmi[j]);
		p[j]=Min(p[j],pma[j]);
	}
	drw_gpk(0);
}


/******************************* Spielende ***********************************/
void ende(void)
{
	char s1[80], s2[80], s3[80];
	int a, b;
	int i;
	const char *loser;
	int with_s = 0;

	snprintf(s1, sizeof(s1), _("!! %s has won !!"),
	         end&2 ? _(l_nam) : _(r_nam));
	loser = (end&2) ? _(r_nam) : _(l_nam);
	if ((end&240) < 48)
	{
		a = loser[strlen(loser)-1];
		with_s = (a=='s' || a=='S');
	}

	switch ( end&240 )
	{
	case 16:
		if (with_s)
			snprintf(s2, sizeof(s2), _("( %s' king was hit,"), loser);
		else
			snprintf(s2, sizeof(s2), _("( %s's king was hit,"), loser);
		strcpy(s3, _("and upon hearing this, the people capitulated. )"));
		break;
	case 32:
		if (with_s)
			snprintf(s2, sizeof(s2), _("( %s' king has capitulated"),
			         loser);
		else
			snprintf(s2, sizeof(s2), _("( %s's king has capitulated"),
			         loser);
		strcpy(s3, _("  because of the hopeless situation. )"));
		break;
	case 48:
		snprintf(s2, sizeof(s2), _("( %s has no folk left. )"), loser);
		s3[0]=0;
		break;
	case 64:
		strcpy(s2, _("( The limit of maximum rounds has been reached."));
		snprintf(s3, sizeof(s3), _("%s is worse off. )"), loser);
	}

	for (a = 0; a < COMP_NUM && strncmp(cn[a], l_nam, 7); a++);
	for (b = 0; b < COMP_NUM && strncmp(cn[b], r_nam, 7); b++);
	if (a < COMP_NUM && b < COMP_NUM && a != b)
	{
		if (~end&2)
		{
			int c;
			c=a;
			a=b;
			b=c;
		}
		t_gew[a][b]++;
		t_gew[b][a]++;
		t_gew[b][COMP_NUM+2]++;
		t_gew[a][COMP_NUM+3] = 100 * ++t_gew[a][COMP_NUM+1]
		                       / ++t_gew[a][COMP_NUM];
		t_gew[b][COMP_NUM+3] = 100 * t_gew[b][COMP_NUM+1]
		                       / ++t_gew[b][COMP_NUM];
	}

	scr_color(0x00800000);
	for (i = 0; i < 8; i++)
	{
		short rxy[4];
		if (i!=7)
			scr_fillcolor((i*32)<<16);
		else
			scr_fillcolor(0xffffff);
		rxy[0] = 40 + i*8;
		rxy[1] = 80 + i*8;
		rxy[2] = 600 - i*8;
		rxy[3] = 320 - i*8;
		scr_bar(rxy);
	}

	scr_l_text(140, 170, s1);
	scr_l_text(140, 210, s2);
	scr_l_text(140, 230, s3);

	bt = 0;

	m_musik();

	Giaccess( 0,138 );
	Giaccess( 0,139 );
	Giaccess( 0,140 );
}


/**
 * Die Routine m_wait() wird von m_musik() nach jedem 1/96 Takt
 * aufgerufen.
 * In diesem Fall macht sie nichts anderes als die eigentliche
 * Warteschleife aufzurufen. In eigenen Programmen könnten Sie hier
 * während der Musik zusätzliche Aktionen ablaufen lassen.
 */
int m_wait(void)
{
	m_wloop();

	return (event(0, 0) != 0 || bt);
}


/** Anzeige des Spielers, der am Zug ist, sowie Darstellung der Windfahnen ***/
void werdran(char c)
{
	char ad[8];
	int i;
	short x, y, w, h, c1, s1, c2, s2;
	double wk,wl;

	/* Anzahl der Spielrunden ausgeben */
	z_txt(zug/2+1);
	scr_l_text(332, 395+16, txt);

	if ( c )
	{
		x = 5+(629-104)*n; y = 410;
		w = 104; h = 48;

		/* Wind ausgeben: */
		ad[0] = ad[5] = 4+28*!wnd-(wnd>0);
		i = wnd<0? -wnd:wnd;
		ad[1] = ad[4] = ' ';
		ad[2] = 48+i/10;
		ad[3] = 48+i%10;
		ad[6] = 0;
		if ( wx[n]<0 )
		{
			ad[0]=ad[5]=32;
			ad[2]=ad[3]='?';
		}
		scr_l_text(x+4, y+h+12, _("Wind:"));
		scr_l_text(x+52, y+h+12, ad);

		c=wnd>0? 1:-1;
		wk=c*wnd/15.0;
		wl=wk*.82;
		if ( wk>1.57 ) wk=1.57;
		if ( wl>1.57 ) wl=1.57;
		s1=c*20*sin(wk);
		c1=20*cos(wk);
		s2=c*20*sin(wl);
		c2=20*cos(wl);
		ws=s1/2.0;
		ws+=!ws;
		wc=c1/2.0;

		hide();
		if ( wx[n]>-1 )
		{
			color(1);
			x+=w/2;
			line( x,y+h,x,y+5 );
			line( x+1,y+h,x+1,y+5 );
			xy[0]=xy[2]=x+1;
			xy[1]=y+5;
			xy[3]=y+11;
			if ( wk<.2 )
			{
				xy[0]=x-1;
				xy[1]=xy[3]=y+5;
				xy[2]=x+2;
			}
			xy[4]=xy[2]+s1;
			xy[5]=xy[3]+c1;
			xy[8]=xy[0]+s1;
			xy[9]=xy[1]+c1;
			xy[10]=xy[0];
			xy[11]=xy[1];
			xy[6] = ((xy[4] + xy[8]) >> 1) + s2;
			xy[7] = ((xy[5] + xy[9]) >> 1) + c2;
			scr_pline(6, xy);
		}
		fahne();
		show();
	}
	else
	{
		hide();
		clr(5+(629-104)*n,410, 104,48+16);
		show();
	}
}


/******************* Darstellung der beiden Windfahnen ***********************/
void fahne(void)
{
	int m=-1;

	while ( ++m<2 ) if ( wx[m]>-1 )
		{
			clr_bg( wx[m]-10,wy[m]-15,20,15 );
			color(1);
			line( wx[m],wy[m],wx[m],wy[m]-15 );
			if ( m==n )
			{
				line( wx[m],wy[m]-15,wx[m]+ws,wy[m]-13+wc );
				line( wx[m],wy[m]-11,wx[m]+ws,wy[m]-13+wc );
			}
		}
}


/********************** BALLER.TAB laden/speichern ***************************/
int t_load(void)
{
#if 1
	//printf("t_load does not work yet\n");
#else
	FILE *f_h;

	f_h = fopen("/baller.tab", "rb");

	if (!f_h) {
		return(1);
	}
	fread(&an_erl, 1, 1, f_h);
	fread(&au_kap, 1, 1, f_h);
	fread(&max_rund, 2, 1, f_h);
	fread(t_gew, 1, sizeof(t_gew), f_h);

	fclose(f_h);
#endif
	return(0);
}


int t_save(void)
{
#if 1
	//printf("t_save not implemented yet\n");
#else
	FILE *f_h;

	f_h = fopen("/baller.tab", "wb");
	if (!f_h) {
		return 1;
	}
	fwrite(&an_erl, 1, 1, f_h);
	fwrite(&au_kap, 1, 1, f_h);
	fwrite(&max_rund, 2, 1, f_h);
	fwrite(t_gew, 1, sizeof(t_gew), f_h);

	fclose(f_h);
#endif
	return(0);
}


/************************* BALLER.DAT laden **********************************/

/* liest ein char von der Datei */
static char zeichen(FILE *f_h)
{
	char a;

	if (fread(&a, 1, 1, f_h) != 1)
	{
		exit(-1);
	}

	return a;
}

/* liest eine Dezimalzahl von der Datei, Remarks werden überlesen */
static int rdzahl(FILE *f_h)
{
	char a,sign=1,rem=0;    /* wird durch * getoggled, und zeigt damit an, */
	/* ob man sich in einer Bemerkung befindet */
	int val=0;

	do
		if ( (a=zeichen(f_h))=='*' ) rem=!rem;
	while ((a != '-' && a < '0') || a > '9' || rem);

	if ( a=='-' )
	{
		sign=-1;
		a=zeichen(f_h);
	}
	while ( a>='0' && a<='9' )
	{
		val*=10;
		val+=a-'0';
		a=zeichen(f_h);
	}

	return( sign*val );
}


void burgen_laden(void)
{
	short *a,j;
	FILE *f_h;
	char *dat_name;

	dat_name = malloc(FILENAME_MAX);
	if (!dat_name)
	{
		exit(-1);
	}
	snprintf(dat_name, FILENAME_MAX, "%s/baller.dat",
	         Paths_GetDataDir());

	f_h = fopen(dat_name, "rb");
	free(dat_name); dat_name = NULL;
	if (!f_h) {
		/* Try to open in current directory instead */
		f_h = fopen( "/baller.dat", "rb");
		if (!f_h) {
			exit(-1);
		}
	}

	a = (short *)bur_ad;
	b_anz = 0;

	while ((j=rdzahl(f_h)) != -999
	       && b_anz < (int)(sizeof(burgen)/sizeof(burgen[0])))
	{
		burgen[b_anz++]=a;
		*a++=j;
		for ( j=0;j<40;j++ ) *a++=rdzahl(f_h);
		while ( (*a++=rdzahl(f_h))!=-1 );
	}

	fclose(f_h);
}


/* Ermittelt, ob Punkt gesetzt ist */
int loc(int x, int y)
{
	int a;

	a = scr_getpixel(x,y);
	// printf("loc %i %i = 0x%x\n", x,y, a);
	return  ((a&0xff) != 0xff);
}

void line(short x1, short y1, short x2, short y2) /* Zeichnet eine Linie */
{
	xy[0]=x1;
	xy[1]=y1;
	xy[2]=x2;
	xy[3]=y2;
	scr_pline(2, xy);
}

void box(short x, short y, short x2, short y2, short c)
{
	color(c);
	xy[0]=x;
	xy[1]=y;
	xy[2]=x2;
	xy[3]=y2;
	scr_bar(xy);
}
