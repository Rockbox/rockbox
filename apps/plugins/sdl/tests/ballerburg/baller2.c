/*
    baller2.c

    Copyright (C) 1987, 1989  Eckhard Kruse
    Copyright (C) 2010, 2014  Thomas Huth

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

/*****************************************************************************
 *                         B a l l e r b u r g               Modul 2         *
 * Dies ist der zweite Teil des Ballerburg Sourcecode. Hier werden die       *
 * Computer gesteuert und der Flug der Kugel dargestellt. Außerdem sind      *
 * alle Routinen in Zusammenhang mit dem König beinhaltet.                   *
 *****************************************************************************/

#include "i18n.h"
#include "baller1.h"
#include "baller2.h"
#include "screen.h"
#include "psg.h"
#include "market.h"

#define Min(a,b)  ((a)<(b)?(a):(b))
#define Max(a,b)  ((a)>(b)?(a):(b))
#define hide()    /*graf_mouse(256,0)*/
#define show()    /*graf_mouse(257,0)*/


void expls(int x, int y, int w, int h, int d);

/***************** Grafikdaten für Trohn, Kanone... **************************/
short trohn[]={ -2,2,2, 8,3,5,12,8,12,12,16,12,17,14,15,
   15,15,17,17,17,16,21,11,24,11,21,3,-9, -4, 3,0,5,2,24,2,26,0,-9,
   19,4,18,9,-9, 10,4,11,9,-9, 14,4,15,4,-9, 8,10,11,11,-9, 21,10,18,11,-9,
   14,17,15,17,-9, 13,19,13,19,-9, 16,19,16,19,-9, 12,22,12,21,15,21,14,22,
   17,21,17,22,-9, 6,13,6,17,5,16,5,17,7,17,7,16,-9, -1 },
      kanon[]={ -2,1,1, 1,0,3,3,5,0,3,3,3,11,2,11,2,9,4,9,4,11,3,11,3,8,0,5,
   3,8,6,5,3,8,3,3,-9, 11,0,13,2,15,0,16,0,17,1,17,2,15,4,19,8,16,11,8,3,-9,
   -1 },
      sack[]={ -4, 1,1,1,4,2,5,2,0,3,0,3,8,2,8,4,8,3,8,3,6,4,6,4,0,5,0,
   5,6,6,5,6,1,-9, -1 },
      fass[]={ -4, 2,0,6,0,8,3,8,5,6,8,2,8,0,5,0,3,2,0,3,0,3,3,2,4,3,5,3,8,
   5,8,5,5,6,4,5,3,5,1,-9, -1 },
      kuge[]={ -2,1,1, 1,0,0,1,0,3,1,4,3,4,4,3,4,1,3,0,-9,-1 },
      turm[]={ -4, 23,30,27,0,3,10,24,20,6,30,23,30,5,20,26,10,2,0,6,30,-9,
   8,0,8,31,9,34,11,36,14,37,15,37,18,36,20,34,21,31,21,29,20,26,18,24,15,23,
   14,23,11,24,9,26,8,29,-9, 14,23,15,37,-9, 10,25,19,35,-9, 10,35,19,25,-9,
   21,30,21,0,-9, -1 };
/************************ Der Computer agiert: *******************************/
int zx,zy;  /* Koordinaten des Zieles */
short *bh;

static double vvx,vvy;


int comp(void)  /* Führt einen Zug des Computers durch */
{
	char wd;
	int i,t;
	register double x2=0,x,y,vx2,vx,vy=0,wi;

	st[n]=16;
	for ( i=0;i<10;i++ ) if ( ka[n][i].x>-1 ) break;
	if ( pu[n]<20 && ge[n]>=p[4] )
	{
		pu[n]+=30;        /* Pulv. kaufen*/
		ge[n]-=p[4];
	}
	if ( !ku[n] && ge[n]>=p[5] )
	{
		ku[n]+=2;         /* Kug.  kaufen*/
		ge[n]-=p[5];
	}
	if ( i>9 && ge[n]>=p[2] )
	{
		init_ka(i=0,639*n);        /* Ka. kauf.*/
		ge[n]-=p[2];
	}
	if ( (ft[n][0].x<0 || ft[n][1].x<0 || ft[n][2].x<0 ) && ge[n]>=p[1] &&
	                cw[n]>2 )
	{
		ge[n]-=p[1];        /* Förderturm kaufen */
		fturm();
	}
	drw_all();

	if ( i>9 || pu[n]<20 || !ku[n] ) return(-1);

	/* Jetzt kommen die Berechnungen für den Schuss: */
	bh=burgen[bur[!n]];
	do i=rand()%10;
	while ( ka[n][i].x==-1 );

	zx=639*!n-f*(rand()%bh[0]);
	zy=by[!n];
	t=rand()%100;
	if ( cw[n] ) z_kn();
	switch ( cw[n] ) /* Die verschiedenen Strategien */
	{
	case 1:
		if ( t<30 ) z_ge();
		if ( t>60 ) z_pk();
		break;
	case 2:
		z_ka();
		if ( t<90 ) z_ge();
		break;
	case 3:
		if ( t<50 )z_ka();
		else if ( t<70 )z_ge();
		else if ( t<90 )z_pk();
		if ( !(rand()%3) ) z_ft();
		break;
	case 4:
		z_ka();
		break;
	case 5:
		z_ka();
		if ( t<90 ) z_ft();
	}

	y=ka[n][i].y-10-zy;
	t=49-cx[n]*16;   /* Berechnen der Distanz */
	x=ka[n][i].x+10+7*f-zx -t/2+rand()%t;
	if ( x<0 ) x=-x;

	wd=n? -wnd:wnd;

	x2=-1;
	for ( vx=.5; vx<7 && ( x2<x || vy/3.5>vx ); ) /* Zeitberechnung bei */
	{                              /* verschiedenen X-Geschwindigkeiten */
		vx+=.4;
		t=x2=0;
		vx2=vx;
		while ( vx2>0 && x2<x )
		{
			x2+=vx2;
			vx2+=(wd/2-vx2)/5000;
			t++;
		}
		vy=y/t+.5*G*t;
	}

	if ( n ) vx=-vx;
	vvx=vx;
	vvy=vy;  /* Werte für Schuss übergeben */

	if ( vx<0 ) vx=-vx;
	wi=atan2(vy,vx);
	ka[n][i].w=wi*P57;
	ka[n][i].p=(int)(4.0*vx/cos(wi)-1.6);

	return(i);
}
/********************* Routinen zur Zielerkennung: ***************************/
/* Die Routinen berechnen die Zielkoordinaten für den König, Kanonen, Geld...*/
void z_kn(void)
{
	zx=!n*639-f*(bh[21]+15);
	zy=by[!n]-bh[22]-5;
}

void z_ka(void)
{
	short i;

	for ( i=0;i<10;i++ ) if ( ka[!n][i].x>-1 ) break;
	if ( i<10 )
	{
		do i=rand()%10;
		while ( ka[!n][i].x==-1 );
		zx=ka[!n][i].x+10;
		zy=ka[!n][i].y;
	}
}

void z_ft(void)
{
	short i;

	for ( i=0;i<5;i++ ) if ( ft[!n][i].x>-1 ) break;
	if ( i<5 )
	{
		zx=ft[!n][i].x-15*f;
		zy=ft[!n][i].y-10;
	}
}

void z_ge(void)
{
	if ( ge[!n]>100 )
	{
		zx=!n*639-f*(bh[25]+bh[31]/2);
		zy=by[!n]-bh[26];
	}
}

void z_pk(void)
{
	short i;

	if ( ku[!n] || pu[!n]>19 )
	{
		i=rand()&2;
		zx=!n*639-f*(bh[27+i]+bh[33+i]/2);
		zy=by[!n]-bh[28+i];
	}
}


/********************************* Ein Schuss ********************************/
/* k = Nr. der Kanone */
void schuss(int k)
{
	double x, y;
	double ox = 0.0, oy = 0.0;
	double vx, vy;
	short v,c,a, j;
	short oldn;

	hide();
	pu[n]-=ka[n][k].p;
	ku[n]--;
	drw_gpk(1);
	drw_gpk(2);

	x=ka[n][k].x+10+7*f;
	y=ka[n][k].y-10;
	c=2;
	vx=(.4+0.25*ka[n][k].p)*cos( ka[n][k].w/P57 )*f;
	vy=(.4+0.25*ka[n][k].p)*sin( ka[n][k].w/P57 );

	if ( mod&(2-n) )
	{
		vx=vvx;
		vy=vvy;
	}

	color(1);
	baller(0);
	kugel( (int)x,(int)y );
	scr_update(x - 3, y - 3, 6, 6);

	v=1;
	while ( x>3 && x<637 && y<396 &&  ( v || c ) ) /* Flugschleife */
	{
		ox=x;
		oy=y;
		x+=vx;
		y-=vy;
		vy-=G;
		vx+=(wnd/2-vx)/5000;

		color(0);
		kugel( (int)ox,(int)oy );
		if (x>=3 && x<=637 && y>= 3 && y <= 397)
		{
			int i;
			v &= !loc((int)x,(int)y);
			for (i = -1; i <= 1; i++)
			{
				v = v & !loc((int)x-2, (int)y+i)
				      & !loc((int)x+2, (int)y+i)
				      & !loc((int)x+i, (int)y+2);
			}
		}
		color(1);
		kugel( (int)x,(int)y );
		scr_update(Min(ox, x)-3, Min(oy, y)-3, abs(ox-x)+7, abs(oy-y)+7);
		SDL_Delay(8);
		SDL_PumpEvents();

		a=1000+2*y;
		if ( a<30 ) a=30;
		Giaccess( 10,137 );
		Giaccess( 244,135 );
		Giaccess( a&255,130 );
		Giaccess( a>>8,131 );

		if (c > 0)
		{
			c--;
			if (c == 0)
				draw(ka[n][k].x+20*n, ka[n][k].y, kanon);
		}
	}

	color( 0 );
	kugel( (int)x,(int)y );
	scr_update(x - 3, y - 3, 6, 6);

	draw(ka[n][k].x+20*n, ka[n][k].y, kanon);

	oldn=n;
	if ( !v ) for ( c=0;c<4;c++ )    /* Einschlag der Kugel */
		{
			if (ox > 6 && ox < 634)
				scr_circle((int)ox, (int)oy, 5);

			baller(22+c*3);

			for ( n=0;n<2;n++ )   /* Treffer ? */
			{
				bg=burgen[bur[n]];
				if ( rand()&1 && (n? x>639-bg[0] : x<bg[0]) && vo[n] ) vo[n]--;
				fn();
				for ( j=0;j<10;j++ )
					if ( ka[n][j].x<ox+2 && ka[n][j].x+22>ox && ka[n][j].x>-1 &&
					                ka[n][j].y-16<oy && ka[n][j].y>oy )
					{
						clr_bg( a=ka[n][j].x,v=ka[n][j].y-12,20,13 );
						expls( a+10,v+6,10,8,50 );
						color(0);
						ka[n][j].x=-1;
					}
				for ( j=0;j<5;j++ )
					if ( ft[n][j].x>-1 && drin( a=639*n+f*ft[n][j].x,v=by[n]-ft[n][j].y,
					                            30,37,2,(int)ox,(int)oy ) )
					{
						if ( drin( a,v,30,37,-2,(int)ox,(int)oy ) )
						{
							expls((a=ft[n][j].x-29*n)+15,(v=ft[n][j].y-40)+20,15,20,120);
							clr_bg( a,v,30,40 );
							ft[n][j].x=-1;
						}
						c=4;
					}
				if ( ox>wx[n]-11 && ox<wx[n]+11 && oy>wy[n]-16 && oy<wy[n] && wx[n]>-1)
				{
					/* Windfahne getroffen */
					clr_bg(wx[n]-10, wy[n]-15, 20, 15);
					wx[n]=-1;
				}
				if ( drin( bg[21],bg[22],30,25,0,(int)ox,(int)oy ) )
				{
					end=n+17;
					expls( a=639*n+f*(bg[21]+15),v=by[n]-bg[22]-12,17,17,40 );
					scr_circle(a, v, 17);
					expls( a,v,17,17,200 );
					c=4;
				}
				for ( j=0;j<6;j+=2 )
					if ( drin( bg[25+j],bg[26+j],a=bg[31+j],v=bg[32+j],3,
					                (int)ox,(int)oy ) ) break;
				switch ( j )
				{
				case 0:
					ge[n]-=Min(200,ge[n]);
					break;
				case 2:
					pu[n]=0;
					break;
				case 4:
					ku[n]-=Min(2,ku[n]);
				}
				if ( j<6 )
				{
					expls( 639*n+f*(bg[25+j]+a/2),by[n]-bg[26+j]-v/2,
					       a/2,v/2,60+100*(j==2) );
					drw_gpk(j/2);
					c=4;
				}
			}
			ox+=vx;
			oy-=vy;
		}
	n=oldn;
	fn();
	Giaccess( 0,137 );
	show();
}


/****************************** Explosion ************************************/
void expls(int x, int y, int w, int h, int d)
{
	int i,j;

	color(0);

	for ( i=0;i<32; )
	{
		xy[i++]=x;
		xy[i++]=y;
	}

	while ( d-->0 )
	{
		int fire_col;

		scr_pline(2, xy);
		memmove(xy, xy+4, 120);
		for ( j=28;j<32; )
		{
			xy[j++]=x-w+w*(rand()&511)/256;
			xy[j++]=y-h+h*(rand()&511)/256;
		}
		fire_col = (rand()&0x3f) + 0xc0;
		fire_col = (fire_col << 16) | ((rand()%fire_col) << 8);
		scr_line(xy[28], xy[29], xy[30], xy[31], fire_col);
		scr_update(x-w, y-h, w*2, h*2);
		baller((d&31)^31);
		SDL_Delay(10);
	}

	for ( i=0;i<32;i+=4 )
	{
		scr_pline(2, xy+i);
	}
}


int kugel(int x, int y)
{
	if (x < 3 || x > 637 || y < 3 || y > 397)
		return 0;

	scr_cannonball(x, y);

	return 1;
}


void baller(unsigned char r)
{
	static unsigned char s_bal[] =
	{
		0,0, 1,15, 6,31, 9,0, 11,0, 12,50,
		13,0, 7,192, 8,16, 255,0
	};

	s_bal[5] = r;
	Dosound( s_bal );

	SDL_Delay(10);
}


/**************************** Neues Bild zeichnen ****************************/
void bild(void)
{
	short y,x1,x2,v1,v2;

	hide();
	scr_clear();

	by[0] = (rand()%80 + 300) & ~3;
	by[1] = (rand()%80 + 300) & ~3;

	y=400;
	x1=0;
	x2=2556;
	color( 1 );
	v1=v2=2;

	while ( x1<x2 && --y>20 )
	{
		int green = 0x70 + (400-y)/4;
		scr_line(x1/4, y, x2/4, y, 0x300030+(green<<8));
		if (y == by[0])
		{
			x1 = *burgen[bur[0]] * 4;
		}
		if (y == by[1])
		{
			x2 = (639 - *burgen[bur[1]]) << 2;
		}
		if ( x1 )
		{
			v1=v1+rand()%5-2;
			v1=Max(0,v1);
			v1=Min(7,v1);
			x1+=v1;
		}
		if ( x2<2556 )
		{
			v2=v2+rand()%5-2;
			v2=Max(0,v2);
			v2=Min(7,v2);
			x2-=v2;
		}
	}

	memset(ka, -1, sizeof(ka));

	burg(0);
	burg(1);

	scr_update(0, 0, 640, 400);

	scr_l_text(276, 395+16, _(" Round     "));
	scr_draw_done_button(0);

	show();
}


/***************************** Burg zeichnen *********************************/
void burg(int nn)
{
	short i,xr;
	short oldn;

	oldn=n;
	n=nn;
	fn();
	xr=n&2? bx[n&=1]:639*n;
	bg=burgen[bur[n]];
	ge[n]=bg[37];
	pu[n]=bg[38];
	ku[n]=bg[39];
	vo[n]=bg[40];

	color(1);
	draw( xr,by[n], &bg[45] );
	clr( xr+f*bg[21]-n*30,by[n]-bg[22]-25,30,25 );
	color(1);
	draw( xr+f*bg[21],by[n]-bg[22], trohn );

	n=nn;
	for ( i=0; i<10 && bg[i*2+1]>-1; i++ ) init_ka( i,xr );
	drw_all();
	n=oldn;
}


void init_ka(int k, int xr)    /* Kanone k setzen */
{
	short x,y;

	draw( x=xr+bg[1+k*2]*f, y=by[n&1]-bg[2+k*2], kanon );
	if ( ~n&2 )
	{
		ka[n][k].x=x-20*n;
		ka[n][k].y=y;
		ka[n][k].w=45;
		ka[n][k].p=12;
	}
}


void drw_all(void)  /* Geld, Pulver und Kugeln zeichnen */
{
	drw_gpk(0);
	drw_gpk(1);
	drw_gpk(2);
}


void drw_gpk(char w)
{
	short i = 0,z, x,y, xr,yr, xp = 0, yp = 0, m=n&1;
	short *a = NULL;

	switch ( w )
	{
	case 0:
		a=sack;
		xp=7;
		yp=10;
		i=(ge[m]+149)/150;
		break;
	case 1:
		a=fass;
		xp=yp=9;
		i=(pu[m]+29)/30;
		break;
	case 2:
		a=kuge;
		xp=yp=6;
		i=ku[m];
	}
	bg=burgen[bur[m]];
	w*=2;
	xr=(n&2? bx[m]:639*m)+f*bg[25+w];
	yr=by[m]-bg[26+w];

	hide();
	clr( xr-bg[31+w]*m-!n,yr-bg[32+w],bg[31+w]+1,bg[32+w]+1 );
	color(1);

	for ( y=z=0; i>0 && y<bg[32+w]; y+=yp )
		for ( x=0; i>0 && x<bg[31+w]; x+=xp,z++,i-- )
			draw( xr+f*x,yr-y, a );
	show();
	if ( i>0 )   /* Maximalbetrag überschritten ? */
		switch ( w )
		{
		case 0:
			ge[m]=z*150;
			break;
		case 2:
			pu[m]=z*30;
			break;
		case 4:
			ku[m]=z;
		}
}


/***************************** Zeichenroutine ********************************/
void draw(short x, short y, short *a)
{
	short i,fil=1;

	hide();
	scr_sf_interior(2);
	scr_sf_style(9);
	while ( *a!=-1 )
	{
		if ( *a==-2 )
		{
			scr_sf_interior(*++a);
			scr_sf_style(*++a);
		}
		else if ( *a==-4 ) fil=!fil;
		else
		{
			i=0;
			while ( *a>-1 )
			{
				xy[i++]=x+*a++*f;
				xy[i++]=y-*a++;
			}
			xy[i++]=xy[0];
			xy[i++]=xy[1];
			if (fil)
				scr_fillarea(i/2, xy);
			else
				scr_pline(i/2-1, xy);
		}
		a++;
	}
	show();
}


/**
 * Förderturm bauen
 */
void fturm(void)
{
	short x,y,yy,i,t;

	hide();
	for ( t=0;t<5;t++ ) if ( ft[n][t].x==-1 ) break;
	x=639*n+f*(bg[0]+20+30*t);
	y=380;
	do
	{
		while ( loc(x,y) && loc(x+29*f,y--) );
		yy=y;
		for ( i=0;i<40;i++,y-- ) if ( loc(x,y) && loc(x+29*f,y) ) break;
	}
	while ( i<40 );
	y=yy;
	clr_bg( x-29*n,y-70,30,70 );
	color( 1 );
	draw( ft[n][t].x=x,ft[n][t].y=y,turm );
	show();
}


/* ************************ Audience with the king ************************* */
const char
     kna[] = N_("The king says:"),
     kne[] = N_("Humbly acknowledged"),
     kn0[] = N_("Well...\n alright...\n Carry on..."),
     kn1[] = N_("We are satisfied\n with your performance!"),
     kn2[] = N_("Excellent,\n keep at it!"),
     kn3[] = N_("Maybe you ought to\n lower Our taxes..."),
     kn4[] = N_("If you keep this up,\n We shall discharge you!"),
     kn5[] = N_("Why don't you buy\n a shaft tower..."),
     kn6[] = N_("You ought to\n kindly make more of\n an effort!"),
     kn7[] = N_("You don't need to visit Us\n in each round."),
     kn8[] = N_("Are you aware\n that you have already visited Us\n %d times thus far?"),
     kn9[] = N_("So, are you certain\n that you will manage\n without a weather vane?"),
     kn10[] = N_("Nice to see you..."),
     kn11[] = N_("What are We supposed\n to say in such an\n early phase?"),
     kn12[] = N_("You ought to earn more money,\n build more shaft towers,\n and vanquish the opponent."),
     kn13[] = N_("We do not have anything new\n to say to you."),
     kn14[] = N_("We are pleased for you\n to come around and visit Us.");

void koenig(void)
{
	char a[300];
	static char kn_visited[80];
	const char *s[20];
	char k, t;
	int i, j;
	static const char *ltz[2];

	for ( j=k=0;j<10;j++ ) k+=ka[n][j].x>-1;
	for ( j=t=0;j<5;j++ )  t+=ft[n][j].x>-1;

	if ( !(rand()%20) || kn[n]&16 || (kn[n]&15)>9 )
		DlgAlert_Notice(_("The king is not in the mood\nto talk to you."), _("Too bad."));
	else
	{
		sprintf(kn_visited, _(kn8), kn[n]/256);

		j=rand();
		i=2;
		s[0]=j&1 ? _(kn0) : _(kn10);
		s[1]=j&2 ? _(kn12) : _(kn6);
		if ( j&4 ) s[i++] = _(kn14);

		if (ge[n] > p[1] && t < 3)
			s[i++] = _(kn5);
		if ((st[n] > 40 && vo[n] < bg[40]) || st[n] > 70)
			s[i++] = _(kn3);
		if (wx[n] < 0)
			s[i++] = _(kn9);
		if (ge[n] > bg[37] && vo[n] > bg[40] && k > 1)
			s[1] = t<2 ? _(kn1) : _(kn2);
		if (t > 2)
			s[i++] = _(kn2);
		if (k < 1 && ge[n] < p[2])
			s[i++] = _(kn4);
		if ((kn[n]&15) > 4)
		{
			s[i++] = _(kn7); s[i++] = _(kn7); s[i++] = _(kn7);
		}
		if ((kn[n]&15) > 6 || (!(rand()&7) && kn[n] > 2048))
		{
			s[i++] = kn_visited; s[i++] = kn_visited;
		}
		if (zug < 4)
			s[1] = s[2] = s[(i=4)-1] = _(kn11);

		do
			if (s[j=rand()%i] == ltz[n])
			{
				s[i++] = _(kn13);
				s[i++] = _(kn13);
			}
		while (s[j] == ltz[n]);
		ltz[n] = s[j];

		snprintf(a, sizeof(a), "%s\n'%s'",  _(kna), s[j]);
		DlgAlert_Notice(a, _(kne));
	}
	kn[n]|=16;
	kn[n]+=256;
	if ( ~kn[n]&15 ) kn[n]++;
}


/* Test, ob Koord. innerhalb eines Rechteckes */
int drin(short xk, short yk, short w, short h, short r, short x, short y)
{
	if ( n ) xk=639-xk-w;
	return( x>xk-r && x<xk+w+r && y<by[n]-yk+r && y>by[n]-yk-h-r );
}
