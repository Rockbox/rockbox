/*
    screen.c - Screen functions for Ballerburg

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

#include <SDL.h>

#include "i18n.h"
#include "screen.h"
#include "sdlgui.h"
#include "sdlgfx.h"

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

#if WITH_SDL2

static SDL_Window *sdlWindow;

#define USE_SDL2_RENDERER 0
#if USE_SDL2_RENDERER
static SDL_Renderer *sdlRenderer;
static SDL_Texture *sdlTexture;
#else
static SDL_Surface *windowSurf;
#endif

#endif

SDL_Surface *surf;
static Uint32 the_color, fill_color;
static Uint32 bg_color;

static int fill_style, fill_interior;

static SGOBJ donebuttondlg[] =
{
	{ SGBOX, 0, 0, 36,29, 8,1, NULL },
	{ SGBUTTON, SG_EXIT, 0, 0,0, 8,1, N_("Done") }
};

#if WITH_SDL2
static int fullscreenflag = 0;
static void scr_sdl2_init(void)
{
	sdlWindow = SDL_CreateWindow("Ballerburg SDL",
	                             SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	                             640, 480, fullscreenflag);
#if USE_SDL2_RENDERER
	sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, 0);
	if (!sdlWindow || !sdlRenderer)
	{
		fprintf(stderr,"Failed to create window or renderer!\n");
		exit(-1);
	}
	SDL_RenderSetLogicalSize(sdlRenderer, 640, 480);
	sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_RGB888,
					SDL_TEXTUREACCESS_STREAMING, 640, 480);
#else
	windowSurf = SDL_GetWindowSurface(sdlWindow);
#endif
}
#endif

void scr_init(void)
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		fprintf(stderr, "Could not initialize the SDL library:\n %s\n",
			SDL_GetError() );
		exit(-1);
	}

#if WITH_SDL2
	scr_sdl2_init();
	surf = SDL_CreateRGBSurface(SDL_SWSURFACE, 640, 480, 32, 0x00FF0000,
				    0x0000FF00, 0x000000FF, 0);
#else
	surf = SDL_SetVideoMode(LCD_WIDTH, LCD_HEIGHT, LCD_DEPTH, SDL_SWSURFACE);
	SDL_WM_SetCaption("Ballerburg SDL", "Ballerburg");
#endif

	if (!surf)
	{
		fprintf(stderr, "Could not initialize the SDL library:\n %s\n",
			SDL_GetError() );
		exit(-1);
	}

	bg_color = SDL_MapRGB(surf->format,0xe0,0xf0,0xff);

	SDLGui_Init();
	SDLGui_SetScreen(surf);
}

void scr_exit(void)
{
	SDLGui_UnInit();

#if WITH_SDL2
	if (surf)
		SDL_FreeSurface(surf);
#if USE_SDL2_RENDERER
	if (sdlTexture)
		SDL_DestroyTexture(sdlTexture);
	if (sdlRenderer)
		SDL_DestroyRenderer(sdlRenderer);
#endif
	if (sdlWindow)
		SDL_DestroyWindow(sdlWindow);
#endif
	SDL_Quit();
}

void scr_togglefullscreen(void)
{
#if WITH_SDL2
#if USER_SDL2_RENDERER
	SDL_DestroyTexture(sdlTexture);
	SDL_DestroyRenderer(sdlRenderer);
	fullscreenflag ^= SDL_WINDOW_FULLSCREEN_DESKTOP;
#else
	fullscreenflag ^= SDL_WINDOW_FULLSCREEN;
#endif
	SDL_DestroyWindow(sdlWindow);
	scr_sdl2_init();
	SDL_UpdateRect(surf, 0, 0, 640, 480);
#else
	SDL_WM_ToggleFullScreen(surf);
#endif
}

void scr_clear(void)
{
	SDL_Rect rect;
	Uint32 white;
	int i;

	white = SDL_MapRGB(surf->format,0xff,0xff,0xff);

	rect.x = 0;
	rect.y = 0;
	rect.w = 640;
	rect.h = 400;
	SDL_FillRect(surf, &rect, bg_color);

	for (i = 0; i < 80; i += 1)
	{
		rect.x = 0;
		rect.y = 479-i;
		rect.w = 640;
		rect.h = 1;
		SDL_FillRect(surf, &rect, SDL_MapRGB(surf->format,8+i/2,32+i,8+i/2));
	}

	/* Left vane box: */
	rect.x = 5; rect.y = 410;
	rect.w = 104; rect.h = 48+16;
	for (i = 1; i < 5; i++)
	{
		rectangleRGBA(surf, rect.x-i, rect.y-i,
			rect.x+rect.w-1+i, rect.y+rect.h-1+i,
			0xf0, 0xff, 0xf0, 0xff-i*0x3c);
	}
	SDL_FillRect(surf, &rect, white);

	/* Right vane box: */
	rect.x = 5+(629-104); rect.y = 410;
	rect.w = 104; rect.h = 48+16;
	for (i = 1; i < 5; i++)
	{
		rectangleRGBA(surf, rect.x-i, rect.y-i,
			rect.x+rect.w-1+i, rect.y+rect.h-1+i,
			0xf0, 0xff, 0xf0, 0xff-i*0x3c);
	}
	SDL_FillRect(surf, &rect, white);
}

void scr_l_text(int x, int y, const char *text)
{
	SDL_Rect rect;

	// printf("v_gtext: %s\n", text);

	y -= 12;

	rect.x = x;
	rect.y = y;
	rect.w = strlen(text) * sdlgui_fontwidth;
	rect.h = sdlgui_fontheight;
	SDL_FillRect(surf, &rect, SDL_MapRGB(surf->format,0xff,0xff,0xff));

	SDLGui_Text(x, y, text);

	SDL_UpdateRects(surf, 1, &rect);
}

/**
 * Draw centered text
 */
void scr_ctr_text(int cx, int y, const char *text)
{
	SDL_Rect rect;

	rect.w = strlen(text) * sdlgui_fontwidth;
	rect.h = sdlgui_fontheight;
	rect.x = cx - rect.w / 2;
	rect.y = y;
	SDL_FillRect(surf, &rect, SDL_MapRGB(surf->format,0xff,0xff,0xff));

	SDLGui_Text(rect.x, rect.y, text);

	SDL_UpdateRects(surf, 1, &rect);
}

void scr_circle(int x, int y, int w)
{
	SDL_Rect rect;

	filledCircleColor(surf, x, y, w, the_color);

	rect.x = max(x-w, 0);
	rect.y = max(y-w, 0);
	rect.w = min(2*w, 640-x+w);
	rect.h = min(2*w, 480-y+w);
	SDL_UpdateRects(surf, 1, &rect);
}

static void update_fill_color(void)
{
	if (fill_interior == 0)
	{
		fill_color = 0xffffffff;
	}
	else if (fill_interior == 1)
	{
		fill_color = 0x000000ff;
	}
	else if (fill_interior == 2)
	{
		switch (fill_style)
		{
		case 1:  fill_color = 0xc0b0a0ff; break;   // Table color
		case 2:  fill_color = 0x602060ff; break;   // King color
		case 9:  fill_color = 0x909080ff; break;   // Wall color
		case 11:  fill_color = 0xc04020ff; break;  // Roof color
		}
	}
	else
	{
		puts("unknown fill interior");
	}
}

void scr_sf_style(short val)
{
	fill_style = val;
	update_fill_color();
}

void scr_sf_interior(short val)
{
	fill_interior = val;
	update_fill_color();
}

void scr_bar(short *xy)
{
	SDL_Rect rect;
	Uint8 r, g, b;

	r = fill_color >> 24;
	g = fill_color >> 16;
	b = fill_color >> 8;

	rect.x = xy[0];
	rect.y = xy[1];
	rect.w = xy[2]-xy[0]+1;
	rect.h = xy[3]-xy[1]+1;
	SDL_FillRect(surf, &rect, SDL_MapRGB(surf->format,r,g,b));
	rectangleColor(surf, xy[0], xy[1], xy[2], xy[3], the_color);

	SDL_UpdateRects(surf, 1, &rect);
}


void clr(short x, short y, short w, short h)
{
	SDL_Rect rect;

	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
	SDL_FillRect(surf, &rect, SDL_MapRGB(surf->format,0xff,0xff,0xff));

	SDL_UpdateRect(surf, x,y, w,h);
}


void clr_bg(short x, short y, short w, short h)
{
	SDL_Rect rect;

	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
	SDL_FillRect(surf, &rect, bg_color);

	SDL_UpdateRect(surf, x,y, w,h);
}


void scr_fillarea(short num, short *xy)
{
	int i;
	Sint16 vx[512], vy[512];

	//printf("v_fillarea %i\n", num);

	if (num > 512) {
		puts("v_fillarea overlow");
		exit(-2);
	}

	for (i = 0; i < num; i++) {
		vx[i] = xy[i*2];
		vy[i] = xy[i*2+1];
	}

	filledPolygonColor(surf, vx, vy, num, fill_color);

	SDL_UpdateRect(surf, 0,0, 640,480);
}

void scr_pline(short num, short *xy)
{
	int i;
	int maxx, maxy;
	int minx, miny;

	minx = 639; miny = 399;
	maxx = 0; maxy = 0;

	// printf("v_pline %i\n", num);

	for (i = 0; i < num-1; i++)
	{
		if (xy[i*2] < 0 || xy[i*2] > 639
		    || xy[i*2+1] < 0 || xy[i*2+1] > 479) {
			printf("bad coordinates!\n");
			return;
		}

		if (xy[i*2] > maxx)  maxx = xy[i*2];
		if (xy[i*2+1] > maxy)  maxy = xy[i*2+1];
		if (xy[i*2] < minx)  minx = xy[i*2];
		if (xy[i*2+1] < miny)  miny = xy[i*2+1];
		lineColor(surf, xy[i*2], xy[i*2+1], xy[i*2+2], xy[i*2+3], the_color|0x1000);
	}

	if (xy[i*2] > maxx)  maxx = xy[i*2];
	if (xy[i*2+1] > maxy)  maxy = xy[i*2+1];
	if (xy[i*2] < minx)  minx = xy[i*2];
	if (xy[i*2+1] < miny)  miny = xy[i*2+1];

	//printf("blit %i %i %i %i\n", minx,miny, maxx-minx+1,maxy-miny+1);
	SDL_UpdateRect(surf, minx,miny, maxx-minx+1,maxy-miny+1);
}


void scr_line(int x1, int y1, int x2, int y2, int rgb)
{
	lineColor(surf, x1, y1, x2, y2, (rgb<<8)|0xff);
}


int scr_getpixel(int x, int y)
{
	Uint32 *p = surf->pixels;
	Uint32 c;
	SDL_PixelFormat *fmt = surf->format;

	c = p[y*640+x];
	c = ((((c & fmt->Rmask) >> fmt->Rshift) << fmt->Rloss) << 16)
	    | ((((c & fmt->Gmask) >> fmt->Gshift) << fmt->Gloss) << 8)
	    | (((c & fmt->Bmask) >> fmt->Bshift) << fmt->Bloss);

	return c;
}

void scr_color(int c)
{
	the_color = (c << 8) | 0xff;
}

void scr_fillcolor(int c)
{
	fill_color = (c << 8) | 0xff;
}


/**
 * Select foreground (1) or background (0) color
 */
void color(int c)
{
	if (c)
		the_color = 0x000000ff;
	else
		the_color = (bg_color<<8) | 0x0ff;
}


void scr_init_done_button(int *bx, int *by, int *bw, int *bh)
{
	int fontw, fonth;

	/* Calculate the "Done" button coordinates */
	SDLGui_GetFontSize(&fontw, &fonth);
	*bx = donebuttondlg[0].x * fontw;
	*by = donebuttondlg[0].y * fonth;
	*bw = donebuttondlg[0].w * fontw;
	*bh = donebuttondlg[0].h * fonth;

}

void scr_draw_done_button(int selected)
{
	if (selected)
		donebuttondlg[1].state |= SG_SELECTED;
	else
		donebuttondlg[1].state &= ~SG_SELECTED;

	SDLGui_DrawButton(donebuttondlg, 1);

	SDL_UpdateRect(surf, 0,0, 0,0);
}


/**
 * Draws a fast cannonball
 */
void scr_cannonball(int x, int y)
{
	lineColor(surf, x-1,y-2, x+1,y-2, the_color);
	lineColor(surf, x-2,y-1, x+2,y-1, the_color);
	lineColor(surf, x-2,y  , x+2,y  , the_color);
	lineColor(surf, x-2,y+1, x+2,y+1, the_color);
	lineColor(surf, x-1,y+2, x+1,y+2, the_color);
}


/**
 * Stores information about saved background area
 */
struct savebg
{
	SDL_Rect rect;
	SDL_Rect bgrect;
	SDL_Surface *bgsurf;
};

/**
 * Save a part of the background of the screen
 */
void *scr_save_bg(int x, int y, int w, int h)
{
	struct savebg *s;

	s = malloc(sizeof(struct savebg));
	if (!s)
		return NULL;

	s->rect.x = x; s->rect.y = y;
	s->rect.w = w; s->rect.h = h;
	s->bgrect.x = s->bgrect.y = 0;
	s->bgrect.w = s->rect.w;
	s->bgrect.h = s->rect.h;
	s->bgsurf = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, surf->format->BitsPerPixel,
	                              surf->format->Rmask, surf->format->Gmask,
	                              surf->format->Bmask, surf->format->Amask);
	if (s->bgsurf != NULL)
	{
		/* Save background */
		SDL_BlitSurface(surf, &s->rect, s->bgsurf, &s->bgrect);
	}
	else
	{
		fprintf(stderr, "scr_save_bg: CreateRGBSurface failed: %s\n",
		        SDL_GetError());
	}

	return s;
}

/**
 * Restore part of the background
 */
void scr_restore_bg(void *ps)
{
	struct savebg *s = ps;

	/* Restore background */
	if (s != NULL && s->bgsurf != NULL)
	{
		SDL_BlitSurface(s->bgsurf, &s->bgrect, surf,  &s->rect);
		SDL_FreeSurface(s->bgsurf);
	}

	SDL_UpdateRects(surf, 1, &s->rect);

	free(ps);
}

#if WITH_SDL2
void SDL_UpdateRects(SDL_Surface *screen, int numrects, SDL_Rect *rects)
{
#if USE_SDL2_RENDERER
	SDL_UpdateTexture(sdlTexture, NULL, screen->pixels, screen->pitch);
	SDL_RenderClear(sdlRenderer);
	SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);
	SDL_RenderPresent(sdlRenderer);
#else
	SDL_BlitSurface(surf, rects, windowSurf, rects);
	SDL_UpdateWindowSurfaceRects(sdlWindow, rects, numrects);
#endif
}

void SDL_UpdateRect(SDL_Surface *screen, Sint32 x, Sint32 y, Sint32 w, Sint32 h)
{
	SDL_Rect rect;
	if (w == 0 && h == 0) {
		w = 640;
		h = 480;
	}
	rect.x = x; rect.y = y;
	rect.w = w; rect.h = h;
	SDL_UpdateRects(screen, 1, &rect);
}
#endif

void scr_update(int x, int y, int w, int h)
{
	if (x >= 640 || y >= 480)
		return;
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;
	if (x + w > 640)
		w = 640 - x;
	if (y + h > 480)
		h = 480 - y;
	SDL_UpdateRect(surf, x, y, w, h);
}
