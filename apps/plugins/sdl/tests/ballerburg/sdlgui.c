/*
 * sdlgui.c - A tiny graphical user interface for the SDL library.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * (http://www.gnu.org/licenses/) for more details.
 */
const char SDLGui_fileid[] = "sdlgui.c : " __DATE__ " " __TIME__;

#include <SDL.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "i18n.h"
#include "sdlgui.h"
#include "baller1.h"
#include "screen.h"

#include "font8x16.h"

#if WITH_SDL2
#define SDL_SRCCOLORKEY SDL_TRUE
#endif

int sdlgui_fontwidth;                       /* Width of the actual font */
int sdlgui_fontheight;                      /* Height of the actual font */

static SDL_Surface *pSdlGuiScrn;            /* Pointer to the actual main SDL screen surface */
//static SDL_Surface *pSmallFontGfx = NULL;   /* The small font graphics */
static SDL_Surface *pBigFontGfx = NULL;     /* The big font graphics */
static SDL_Surface *pFontGfx = NULL;        /* The actual font graphics */
static int current_object = 0;              /* Current selected object */
static bool has_utf8;

#if WITH_SDL2
static inline int SDL_SetColors(SDL_Surface *surface, SDL_Color *colors,
                                int firstcolor, int ncolors)
{
	return SDL_SetPaletteColors(surface->format->palette, colors,
	                            firstcolor, ncolors);
}
#endif

/**
 * Load an 1 plane XBM into a 8 planes SDL_Surface.
 */
static SDL_Surface *SDLGui_LoadXBM(int w, int h, const void *pXbmBits)
{
	SDL_Surface *bitmap;
	Uint8 *dstbits;
	const Uint8 *srcbits;
	int x, y, srcpitch;
	int mask;

	srcbits = pXbmBits;

	/* Allocate the bitmap */
	bitmap = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 8, 0, 0, 0, 0);
	if (bitmap == NULL)
	{
		fprintf(stderr, "Failed to allocate bitmap: %s", SDL_GetError());
		return NULL;
	}

	srcpitch = ((w + 7) / 8);
	dstbits = (Uint8 *)bitmap->pixels;
	mask = 1;

	/* Copy the pixels */
	for (y = 0 ; y < h ; y++)
	{
		for (x = 0 ; x < w ; x++)
		{
			dstbits[x] = (srcbits[x / 8] & mask) ? 1 : 0;
			mask <<= 1;
			mask |= (mask >> 8);
			mask &= 0xFF;
		}
		dstbits += bitmap->pitch;
		srcbits += srcpitch;
	}

	return bitmap;
}


/*-----------------------------------------------------------------------*/
/**
 * Initialize the GUI.
 */
int SDLGui_Init(void)
{
	char *lang;

	SDL_Color blackWhiteColors[2] = {{255, 255, 255, 0}, {0, 0, 0, 0}};

	if (/*pSmallFontGfx &&*/ pBigFontGfx)
	{
		/* already initialized */
		return 0;
	}

	/* Initialize the font graphics: */
//	pSmallFontGfx = SDLGui_LoadXBM(font5x8_width, font5x8_height, font5x8_bits);
	pBigFontGfx = SDLGui_LoadXBM(font8x16_width, font8x16_height, font8x16_bits);
	if (/*pSmallFontGfx == NULL ||*/ pBigFontGfx == NULL)
	{
		fprintf(stderr, "Error: Can not init font graphics!\n");
		return -1;
	}

	/* Set color palette of the font graphics: */
	//SDL_SetColors(pSmallFontGfx, blackWhiteColors, 0, 2);
	SDL_SetColors(pBigFontGfx, blackWhiteColors, 0, 2);

	/* Set font color 0 as transparent: */
	//SDL_SetColorKey(pSmallFontGfx, SDL_SRCCOLORKEY, 0);
	SDL_SetColorKey(pBigFontGfx, SDL_SRCCOLORKEY, 0);

	/* Check for UTF-8 locale */
	lang = getenv("LANG");
	if (lang != NULL)
	{
		has_utf8 = (strstr(lang, "utf-8") != NULL)
			   || (strstr(lang, "UTF-8") != NULL)
			   || (strstr(lang, "utf8") != NULL)
			   || (strstr(lang, "UTF8") != NULL);
	}
	else
	{
		has_utf8 = false;
	}

	return 0;
}


/*-----------------------------------------------------------------------*/
/**
 * Uninitialize the GUI.
 */
int SDLGui_UnInit(void)
{
	/*
	if (pSmallFontGfx)
	{
		SDL_FreeSurface(pSmallFontGfx);
		pSmallFontGfx = NULL;
	}
	*/

	if (pBigFontGfx)
	{
		SDL_FreeSurface(pBigFontGfx);
		pBigFontGfx = NULL;
	}

	return 0;
}


/*-----------------------------------------------------------------------*/
/**
 * Inform the SDL-GUI about the actual SDL_Surface screen pointer and
 * prepare the font to suit the actual resolution.
 */
int SDLGui_SetScreen(SDL_Surface *pScrn)
{
	pSdlGuiScrn = pScrn;

	/* Decide which font to use - small or big one: */
	// if (pSdlGuiScrn->w >= 640 && pSdlGuiScrn->h >= 400 && pBigFontGfx != NULL)
	{
		pFontGfx = pBigFontGfx;
	}
	/*
	else
	{
		pFontGfx = pSmallFontGfx;
	}
	*/

	if (pFontGfx == NULL)
	{
		fprintf(stderr, "Error: A problem with the font occured!\n");
		return -1;
	}

	/* Get the font width and height: */
	sdlgui_fontwidth = pFontGfx->w/16;
	sdlgui_fontheight = pFontGfx->h/16;

	return 0;
}

/*-----------------------------------------------------------------------*/
/**
 * Return character size for current font in given arguments.
 */
void SDLGui_GetFontSize(int *width, int *height)
{
	*width = sdlgui_fontwidth;
	*height = sdlgui_fontheight;
}

/*-----------------------------------------------------------------------*/
/**
 * Center a dialog so that it appears in the middle of the screen.
 * Note: We only store the coordinates in the root box of the dialog,
 * all other objects in the dialog are positioned relatively to this one.
 */
void SDLGui_CenterDlg(SGOBJ *dlg)
{
	dlg[0].x = (pSdlGuiScrn->w/sdlgui_fontwidth-dlg[0].w)/2;
	dlg[0].y = (pSdlGuiScrn->h/sdlgui_fontheight-dlg[0].h)/2;
}


/**
 * Convert UTF-8 character to our internal Latin1 representation and
 * increase the string index.
 */
static char SDLGui_Utf8ToLatin1(const char *txt, int *i)
{
	unsigned char c;

	c = txt[*i];
	*i += 1;
	if (c < 0x80 || !has_utf8)
	{
		return c;
	}

	/* Quick and dirty convertion for latin1 characters only... */
	if ((c & 0xc0) == 0xc0)
	{
		c = c << 6;
		c |= (txt[*i] & 0x7f);
		*i += 1;
	}
	else
	{
		printf("Unsupported character '%c' (0x%x)\n", c, c);
	}

	return c;
}

/**
 * Draw a text string.
 */
void SDLGui_Text(int x, int y, const char *txt)
{
	int i, p;
	unsigned char c;
	SDL_Rect sr, dr;

	sr.w = dr.w = sdlgui_fontwidth;
	sr.h = dr.h = sdlgui_fontheight;

	i = p = 0;
	while (txt[i] != 0)
	{
		c = SDLGui_Utf8ToLatin1(txt, &i);
		sr.x = sdlgui_fontwidth * (c % 16);
		sr.y = sdlgui_fontheight * (c / 16);
		dr.x = x + p * sdlgui_fontwidth;
		dr.y = y;
		SDL_BlitSurface(pFontGfx, &sr, pSdlGuiScrn, &dr);
		p += 1;
	}
}


/*-----------------------------------------------------------------------*/
/**
 * Draw a dialog text object.
 */
static void SDLGui_DrawText(const SGOBJ *tdlg, int objnum)
{
	int x, y;
	x = (tdlg[0].x+tdlg[objnum].x)*sdlgui_fontwidth;
	y = (tdlg[0].y+tdlg[objnum].y)*sdlgui_fontheight;
	SDLGui_Text(x, y, _(tdlg[objnum].txt));
}


/*-----------------------------------------------------------------------*/
/**
 * Draw a edit field object.
 */
static void SDLGui_DrawEditField(const SGOBJ *edlg, int objnum)
{
	int x, y;
	SDL_Rect rect;

	x = (edlg[0].x+edlg[objnum].x)*sdlgui_fontwidth;
	y = (edlg[0].y+edlg[objnum].y)*sdlgui_fontheight;
	SDLGui_Text(x, y, edlg[objnum].txt);

	rect.x = x;
	rect.y = y + edlg[objnum].h * sdlgui_fontheight;
	rect.w = edlg[objnum].w * sdlgui_fontwidth;
	rect.h = 1;
	SDL_FillRect(pSdlGuiScrn, &rect, SDL_MapRGB(pSdlGuiScrn->format,160,160,160));
}


/*-----------------------------------------------------------------------*/
/**
 * Draw a dialog box object.
 */
static void SDLGui_DrawBox(const SGOBJ *bdlg, int objnum)
{
	SDL_Rect rect;
	int x, y, w, h, offset;
	Uint32 grey = SDL_MapRGB(pSdlGuiScrn->format,192,192,192);
	Uint32 upleftc, downrightc;

	x = bdlg[objnum].x*sdlgui_fontwidth;
	y = bdlg[objnum].y*sdlgui_fontheight;
	if (objnum > 0)                 /* Since the root object is a box, too, */
	{
		/* we have to look for it now here and only */
		x += bdlg[0].x*sdlgui_fontwidth;   /* add its absolute coordinates if we need to */
		y += bdlg[0].y*sdlgui_fontheight;
	}
	w = bdlg[objnum].w*sdlgui_fontwidth;
	h = bdlg[objnum].h*sdlgui_fontheight;

	if (bdlg[objnum].state & SG_SELECTED)
	{
		upleftc = SDL_MapRGB(pSdlGuiScrn->format,128,128,128);
		downrightc = SDL_MapRGB(pSdlGuiScrn->format,255,255,255);
	}
	else
	{
		upleftc = SDL_MapRGB(pSdlGuiScrn->format,255,255,255);
		downrightc = SDL_MapRGB(pSdlGuiScrn->format,128,128,128);
	}

	/* The root box should be bigger than the screen, so we disable the offset there: */
	if (objnum != 0)
		offset = 1;
	else
		offset = 0;

	/* Draw background: */
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
	SDL_FillRect(pSdlGuiScrn, &rect, grey);

	/* Draw upper border: */
	rect.x = x;
	rect.y = y - offset;
	rect.w = w;
	rect.h = 1;
	SDL_FillRect(pSdlGuiScrn, &rect, upleftc);

	/* Draw left border: */
	rect.x = x - offset;
	rect.y = y;
	rect.w = 1;
	rect.h = h;
	SDL_FillRect(pSdlGuiScrn, &rect, upleftc);

	/* Draw bottom border: */
	rect.x = x;
	rect.y = y + h - 1 + offset;
	rect.w = w;
	rect.h = 1;
	SDL_FillRect(pSdlGuiScrn, &rect, downrightc);

	/* Draw right border: */
	rect.x = x + w - 1 + offset;
	rect.y = y;
	rect.w = 1;
	rect.h = h;
	SDL_FillRect(pSdlGuiScrn, &rect, downrightc);
}


/*-----------------------------------------------------------------------*/
/**
 * Draw a normal button.
 */
void SDLGui_DrawButton(const SGOBJ *bdlg, int objnum)
{
	int x,y;
	char *txt = _(bdlg[objnum].txt);

	SDLGui_DrawBox(bdlg, objnum);

	x = (bdlg[0].x + bdlg[objnum].x) * sdlgui_fontwidth
	    + (bdlg[objnum].w - strlen(txt)) * sdlgui_fontwidth / 2;
	y = (bdlg[0].y + bdlg[objnum].y + (bdlg[objnum].h-1)/2)
	    * sdlgui_fontheight;

	if (bdlg[objnum].state & SG_SELECTED)
	{
		x+=1;
		y+=1;
	}
	SDLGui_Text(x, y, txt);
}


/*-----------------------------------------------------------------------*/
/**
 * Draw a dialog radio button object.
 */
static void SDLGui_DrawRadioButton(const SGOBJ *rdlg, int objnum)
{
	char str[80];
	int x, y;

	x = (rdlg[0].x + rdlg[objnum].x) * sdlgui_fontwidth;
	y = (rdlg[0].y + rdlg[objnum].y) * sdlgui_fontheight;

	if (rdlg[objnum].state & SG_SELECTED)
		str[0]=SGRADIOBUTTON_SELECTED;
	else
		str[0]=SGRADIOBUTTON_NORMAL;
	str[1]=' ';
	strcpy(&str[2], _(rdlg[objnum].txt));

	SDLGui_Text(x, y, str);
}


/*-----------------------------------------------------------------------*/
/**
 * Draw a dialog check box object.
 */
static void SDLGui_DrawCheckBox(const SGOBJ *cdlg, int objnum)
{
	char str[80];
	int x, y;

	x = (cdlg[0].x + cdlg[objnum].x) * sdlgui_fontwidth;
	y = (cdlg[0].y + cdlg[objnum].y) * sdlgui_fontheight;

	if ( cdlg[objnum].state&SG_SELECTED )
		str[0]=SGCHECKBOX_SELECTED;
	else
		str[0]=SGCHECKBOX_NORMAL;
	str[1]=' ';
	strcpy(&str[2], _(cdlg[objnum].txt));

	SDLGui_Text(x, y, str);
}


/*-----------------------------------------------------------------------*/
/**
 * Draw a scrollbar button.
 */
static void SDLGui_DrawScrollbar(const SGOBJ *bdlg, int objnum)
{
	SDL_Rect rect;
	int x, y, w, h;
	Uint32 grey0 = SDL_MapRGB(pSdlGuiScrn->format,128,128,128);
	Uint32 grey1 = SDL_MapRGB(pSdlGuiScrn->format,196,196,196);
	Uint32 grey2 = SDL_MapRGB(pSdlGuiScrn->format, 64, 64, 64);

	x = bdlg[objnum].x * sdlgui_fontwidth;
	y = bdlg[objnum].y * sdlgui_fontheight + bdlg[objnum].h;

	x += bdlg[0].x*sdlgui_fontwidth;   /* add mainbox absolute coordinates */
	y += bdlg[0].y*sdlgui_fontheight;  /* add mainbox absolute coordinates */
	
	w = 1 * sdlgui_fontwidth;
	h = bdlg[objnum].w;

	/* Draw background: */
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
	SDL_FillRect(pSdlGuiScrn, &rect, grey0);

	/* Draw upper border: */
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = 1;
	SDL_FillRect(pSdlGuiScrn, &rect, grey1);

	/* Draw bottom border: */
	rect.x = x;
	rect.y = y + h - 1;
	rect.w = w;
	rect.h = 1;
	SDL_FillRect(pSdlGuiScrn, &rect, grey2);
	
}

/*-----------------------------------------------------------------------*/
/**
 *  Draw a dialog popup button object.
 */
static void SDLGui_DrawPopupButton(const SGOBJ *pdlg, int objnum)
{
	int x, y, w;
	const char *downstr = "\x02";

	SDLGui_DrawBox(pdlg, objnum);

	x = (pdlg[0].x + pdlg[objnum].x) * sdlgui_fontwidth;
	y = (pdlg[0].y + pdlg[objnum].y) * sdlgui_fontheight;
	w = pdlg[objnum].w*sdlgui_fontwidth;
	//h = pdlg[objnum].h*sdlgui_fontheight;

	SDLGui_Text(x, y, pdlg[objnum].txt);
	SDLGui_Text(x+w-sdlgui_fontwidth, y, downstr);
}


/*-----------------------------------------------------------------------*/
/**
 * Let the user insert text into an edit field object.
 * NOTE: The dlg[objnum].txt must point to an an array that is big enough
 * for dlg[objnum].w characters!
 */
static void SDLGui_EditField(SGOBJ *dlg, int objnum)
{
	size_t cursorPos;                   /* Position of the cursor in the edit field */
	int blinkState = 0;                 /* Used for cursor blinking */
	int bStopEditing = false;           /* true if user wants to exit the edit field */
	char *txt;                          /* Shortcut for dlg[objnum].txt */
	SDL_Rect rect;
	Uint32 grey, cursorCol;
	SDL_Event event;
#if !WITH_SDL2
	int nOldUnicodeMode;
#endif

	grey = SDL_MapRGB(pSdlGuiScrn->format, 192, 192, 192);
	cursorCol = SDL_MapRGB(pSdlGuiScrn->format, 128, 128, 128);

	rect.x = (dlg[0].x + dlg[objnum].x) * sdlgui_fontwidth;
	rect.y = (dlg[0].y + dlg[objnum].y) * sdlgui_fontheight;
	rect.w = (dlg[objnum].w + 1) * sdlgui_fontwidth - 1;
	rect.h = dlg[objnum].h * sdlgui_fontheight;

#if WITH_SDL2
	SDL_SetTextInputRect(&rect);
	SDL_StartTextInput();
#else
	/* Enable unicode translation to get proper characters with SDL_PollEvent */
	nOldUnicodeMode = SDL_EnableUNICODE(true);
#endif

	txt = dlg[objnum].txt;
	cursorPos = strlen(txt);

	do
	{
		/* Look for events */
		if (SDL_PollEvent(&event) == 0)
		{
			/* No event: Wait some time for cursor blinking */
			SDL_Delay(250);
			blinkState ^= 1;
		}
		else
		{
			/* Handle events */
			do
			{
				switch (event.type)
				{
				 case SDL_QUIT:                     /* User wants to quit */
					// bQuitProgram = true;
					bStopEditing = true;
					break;
				 case SDL_MOUSEBUTTONDOWN:          /* Mouse pressed -> stop editing */
					bStopEditing = true;
					break;
#if WITH_SDL2
				 case SDL_TEXTINPUT:
					if (strlen(txt) < (size_t)dlg[objnum].w)
					{
						memmove(&txt[cursorPos+1], &txt[cursorPos], strlen(&txt[cursorPos])+1);
							txt[cursorPos] = event.text.text[0];
						cursorPos += 1;
					}
					break;
#endif
				 case SDL_KEYDOWN:                  /* Key pressed */
					switch (event.key.keysym.sym)
					{
					 case SDLK_RETURN:
					 case SDLK_KP_ENTER:
						bStopEditing = true;
						break;
					 case SDLK_LEFT:
						if (cursorPos > 0)
							cursorPos -= 1;
						break;
					 case SDLK_RIGHT:
						if (cursorPos < strlen(txt))
							cursorPos += 1;
						break;
					 case SDLK_BACKSPACE:
						if (cursorPos > 0)
						{
							memmove(&txt[cursorPos-1], &txt[cursorPos], strlen(&txt[cursorPos])+1);
							cursorPos -= 1;
						}
						break;
					 case SDLK_DELETE:
						if (cursorPos < strlen(txt))
							memmove(&txt[cursorPos], &txt[cursorPos+1], strlen(&txt[cursorPos+1])+1);
						break;
					 default:
#if !WITH_SDL2
						/* If it is a "good" key then insert it into the text field */
						if (event.key.keysym.unicode >= 32 && event.key.keysym.unicode < 128
						        /* && event.key.keysym.unicode != PATHSEP*/)
						{
							if (strlen(txt) < (size_t)dlg[objnum].w)
							{
								memmove(&txt[cursorPos+1], &txt[cursorPos], strlen(&txt[cursorPos])+1);
								txt[cursorPos] = event.key.keysym.unicode;
								cursorPos += 1;
							}
						}
#endif
						break;
					}
					break;
				}
			}
			while (SDL_PollEvent(&event));

			blinkState = 1;
		}

		/* Redraw the text field: */
		SDL_FillRect(pSdlGuiScrn, &rect, grey);  /* Draw background */
		/* Draw the cursor: */
		if (blinkState && !bStopEditing)
		{
			SDL_Rect cursorrect;
			cursorrect.x = rect.x + cursorPos * sdlgui_fontwidth;
			cursorrect.y = rect.y;
			cursorrect.w = sdlgui_fontwidth;
			cursorrect.h = rect.h;
			SDL_FillRect(pSdlGuiScrn, &cursorrect, cursorCol);
		}
		SDLGui_Text(rect.x, rect.y, dlg[objnum].txt);  /* Draw text */
		SDL_UpdateRects(pSdlGuiScrn, 1, &rect);
	}
	while (!bStopEditing);

#if WITH_SDL2
	SDL_StopTextInput();
#else
	SDL_EnableUNICODE(nOldUnicodeMode);
#endif
}


/**
 * Draw a user object.
 */
static void SDLGui_DrawUserObj(const SGOBJ *bdlg, int objnum)
{
	int x, y, w, h;
	char (*userfun)(int x, int y, int w, int h);

	x = bdlg[objnum].x*sdlgui_fontwidth;
	y = bdlg[objnum].y*sdlgui_fontheight;

	/* add absolute coordinates */
	x += bdlg[0].x*sdlgui_fontwidth;
	y += bdlg[0].y*sdlgui_fontheight;

	w = bdlg[objnum].w*sdlgui_fontwidth;
	h = bdlg[objnum].h*sdlgui_fontheight;

	userfun = (void*)bdlg[objnum].txt;

	userfun(x, y, w, h);
}


/*-----------------------------------------------------------------------*/
/**
 * Draw a whole dialog.
 */
void SDLGui_DrawDialog(const SGOBJ *dlg)
{
	int i;

	for (i = 0; dlg[i].type != -1; i++)
	{
		switch (dlg[i].type)
		{
		 case SGBOX:
			SDLGui_DrawBox(dlg, i);
			break;
		 case SGTEXT:
			SDLGui_DrawText(dlg, i);
			break;
		 case SGEDITFIELD:
			SDLGui_DrawEditField(dlg, i);
			break;
		 case SGBUTTON:
			SDLGui_DrawButton(dlg, i);
			break;
		 case SGRADIOBUT:
			SDLGui_DrawRadioButton(dlg, i);
			break;
		 case SGCHECKBOX:
			SDLGui_DrawCheckBox(dlg, i);
			break;
		 case SGPOPUP:
			SDLGui_DrawPopupButton(dlg, i);
			break;
		 case SGSCROLLBAR:
			SDLGui_DrawScrollbar(dlg, i);
			break;
		 case SGUSER:
			SDLGui_DrawUserObj(dlg, i);
			break;
		}
	}
	SDL_UpdateRect(pSdlGuiScrn, 0,0,0,0);
}


/*-----------------------------------------------------------------------*/
/**
 * Search an object at a certain position.
 */
static int SDLGui_FindObj(const SGOBJ *dlg, int fx, int fy)
{
	int len, i;
	int ob = -1;
	int xpos, ypos;

	len = 0;
	while (dlg[len].type != -1)   len++;

	xpos = fx / sdlgui_fontwidth;
	ypos = fy / sdlgui_fontheight;
	/* Now search for the object: */
	for (i = len; i >= 0; i--)
	{
		/* clicked on a scrollbar ? */
		if (dlg[i].type == SGSCROLLBAR) {
			if (xpos >= dlg[0].x+dlg[i].x && xpos < dlg[0].x+dlg[i].x+1) {
				ypos = dlg[i].y * sdlgui_fontheight + dlg[i].h + dlg[0].y * sdlgui_fontheight;
				if (fy >= ypos && fy < ypos + dlg[i].w) {
					ob = i;
					break;
				}
			}
		}
		/* clicked on another object ? */
		else if (xpos >= dlg[0].x+dlg[i].x && ypos >= dlg[0].y+dlg[i].y
		    && xpos < dlg[0].x+dlg[i].x+dlg[i].w && ypos < dlg[0].y+dlg[i].y+dlg[i].h)
		{
			ob = i;
			break;
		}
	}

	return ob;
}


/*-----------------------------------------------------------------------*/
/**
 * Search a button with a special flag (e.g. SG_DEFAULT or SG_CANCEL).
 */
static int SDLGui_SearchFlaggedButton(const SGOBJ *dlg, int flag)
{
	int i = 0;

	while (dlg[i].type != -1)
	{
		if (dlg[i].flags & flag)
			return i;
		i++;
	}

	return 0;
}


/*-----------------------------------------------------------------------*/
/**
 * Show and process a dialog. Returns the button number that has been
 * pressed or SDLGUI_UNKNOWNEVENT if an unsupported event occured (will be
 * stored in parameter pEventOut).
 */
int SDLGui_DoDialog(SGOBJ *dlg, SDL_Event *pEventOut)
{
	int obj=0;
	int oldbutton=0;
	int retbutton=0;
	int i, j, b;
	SDL_Event sdlEvent;
	SDL_Rect rct;
	Uint32 grey;
	SDL_Surface *pBgSurface;
	SDL_Rect dlgrect, bgrect;

	if (pSdlGuiScrn->h / sdlgui_fontheight < dlg[0].h)
	{
		fprintf(stderr, "Screen size too small for dialog!\n");
		return SDLGUI_ERROR;
	}

	grey = SDL_MapRGB(pSdlGuiScrn->format,192,192,192);

	dlgrect.x = dlg[0].x * sdlgui_fontwidth;
	dlgrect.y = dlg[0].y * sdlgui_fontheight;
	dlgrect.w = dlg[0].w * sdlgui_fontwidth;
	dlgrect.h = dlg[0].h * sdlgui_fontheight;

	bgrect.x = bgrect.y = 0;
	bgrect.w = dlgrect.w;
	bgrect.h = dlgrect.h;

	/* Save background */
	pBgSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, dlgrect.w, dlgrect.h, pSdlGuiScrn->format->BitsPerPixel,
	                                  pSdlGuiScrn->format->Rmask, pSdlGuiScrn->format->Gmask, pSdlGuiScrn->format->Bmask, pSdlGuiScrn->format->Amask);
	if (pSdlGuiScrn->format->palette != NULL)
	{
		SDL_SetColors(pBgSurface, pSdlGuiScrn->format->palette->colors, 0, pSdlGuiScrn->format->palette->ncolors-1);
	}

	if (pBgSurface != NULL)
	{
		SDL_BlitSurface(pSdlGuiScrn,  &dlgrect, pBgSurface, &bgrect);
	}
	else
	{
		fprintf(stderr, "SDLGUI_DoDialog: CreateRGBSurface failed: %s\n", SDL_GetError());
	}

	/* (Re-)draw the dialog */
	SDLGui_DrawDialog(dlg);

	/* Is the left mouse button still pressed? Yes -> Handle TOUCHEXIT objects here */
	SDL_PumpEvents();
	b = SDL_GetMouseState(&i, &j);

	/* If current object is the scrollbar, and mouse is still down, we can scroll it */
	/* also if the mouse pointer has left the scrollbar */
	if (dlg[current_object].type == SGSCROLLBAR) {
		if (b & SDL_BUTTON(1)) {
			obj = current_object;
			dlg[obj].state |= SG_MOUSEDOWN;
			oldbutton = obj;
			retbutton = obj;
		}
		else {
			obj = current_object;
			current_object = 0;
			dlg[obj].state &= SG_MOUSEUP;
			retbutton = obj;
			oldbutton = obj;
		}
	}
	else {
		obj = SDLGui_FindObj(dlg, i, j);
		current_object = obj;
		if (obj > 0 && (dlg[obj].flags&SG_TOUCHEXIT) )
		{
			oldbutton = obj;
			if (b & SDL_BUTTON(1))
			{
				dlg[obj].state |= SG_SELECTED;
				retbutton = obj;
			}
		}
	}


	/* The main loop */
	while (retbutton == 0 /*&& !bQuitProgram*/)
	{
		if (SDL_WaitEvent(&sdlEvent) == 1)  /* Wait for events */
		
			switch (sdlEvent.type)
			{
			 case SDL_QUIT:
				retbutton = SDLGUI_QUIT;
				break;

			 case SDL_MOUSEBUTTONDOWN:
				if (sdlEvent.button.button != SDL_BUTTON_LEFT)
				{
					/* Not left mouse button -> unsupported event */
					if (pEventOut)
						retbutton = SDLGUI_UNKNOWNEVENT;
					break;
				}
				/* It was the left button: Find the object under the mouse cursor */
				obj = SDLGui_FindObj(dlg, sdlEvent.button.x, sdlEvent.button.y);
				if (obj>0)
				{
					if (dlg[obj].type==SGBUTTON)
					{
						dlg[obj].state |= SG_SELECTED;
						SDLGui_DrawButton(dlg, obj);
						SDL_UpdateRect(pSdlGuiScrn, (dlg[0].x+dlg[obj].x)*sdlgui_fontwidth-2, (dlg[0].y+dlg[obj].y)*sdlgui_fontheight-2,
						               dlg[obj].w*sdlgui_fontwidth+4, dlg[obj].h*sdlgui_fontheight+4);
						oldbutton=obj;
					}
					if (dlg[obj].type==SGSCROLLBAR)
					{
						dlg[obj].state |= SG_MOUSEDOWN;
						oldbutton=obj;
					}
					if ( dlg[obj].flags&SG_TOUCHEXIT )
					{
						dlg[obj].state |= SG_SELECTED;
						retbutton = obj;
					}
				}
				break;

			 case SDL_MOUSEBUTTONUP:
				if (sdlEvent.button.button != SDL_BUTTON_LEFT)
				{
					/* Not left mouse button -> unsupported event */
					if (pEventOut)
						retbutton = SDLGUI_UNKNOWNEVENT;
					break;
				}
				/* It was the left button: Find the object under the mouse cursor */
				obj = SDLGui_FindObj(dlg, sdlEvent.button.x, sdlEvent.button.y);
				if (obj>0)
				{
					switch (dlg[obj].type)
					{
					 case SGBUTTON:
						if (oldbutton==obj)
							retbutton=obj;
						break;
					 case SGSCROLLBAR:
 						dlg[obj].state &= SG_MOUSEUP;

						if (oldbutton==obj)
							retbutton=obj;
						break;
					case SGEDITFIELD:
						SDLGui_EditField(dlg, obj);
						break;
					 case SGRADIOBUT:
						for (i = obj-1; i > 0 && dlg[i].type == SGRADIOBUT; i--)
						{
							dlg[i].state &= ~SG_SELECTED;  /* Deselect all radio buttons in this group */
							rct.x = (dlg[0].x+dlg[i].x)*sdlgui_fontwidth;
							rct.y = (dlg[0].y+dlg[i].y)*sdlgui_fontheight;
							rct.w = sdlgui_fontwidth;
							rct.h = sdlgui_fontheight;
							SDL_FillRect(pSdlGuiScrn, &rct, grey); /* Clear old */
							SDLGui_DrawRadioButton(dlg, i);
							SDL_UpdateRects(pSdlGuiScrn, 1, &rct);
						}
						for (i = obj+1; dlg[i].type == SGRADIOBUT; i++)
						{
							dlg[i].state &= ~SG_SELECTED;  /* Deselect all radio buttons in this group */
							rct.x = (dlg[0].x+dlg[i].x)*sdlgui_fontwidth;
							rct.y = (dlg[0].y+dlg[i].y)*sdlgui_fontheight;
							rct.w = sdlgui_fontwidth;
							rct.h = sdlgui_fontheight;
							SDL_FillRect(pSdlGuiScrn, &rct, grey); /* Clear old */
							SDLGui_DrawRadioButton(dlg, i);
							SDL_UpdateRects(pSdlGuiScrn, 1, &rct);
						}
						dlg[obj].state |= SG_SELECTED;  /* Select this radio button */
						rct.x = (dlg[0].x+dlg[obj].x)*sdlgui_fontwidth;
						rct.y = (dlg[0].y+dlg[obj].y)*sdlgui_fontheight;
						rct.w = sdlgui_fontwidth;
						rct.h = sdlgui_fontheight;
						SDL_FillRect(pSdlGuiScrn, &rct, grey); /* Clear old */
						SDLGui_DrawRadioButton(dlg, obj);
						SDL_UpdateRects(pSdlGuiScrn, 1, &rct);
						break;
					 case SGCHECKBOX:
						dlg[obj].state ^= SG_SELECTED;
						rct.x = (dlg[0].x+dlg[obj].x)*sdlgui_fontwidth;
						rct.y = (dlg[0].y+dlg[obj].y)*sdlgui_fontheight;
						rct.w = sdlgui_fontwidth;
						rct.h = sdlgui_fontheight;
						SDL_FillRect(pSdlGuiScrn, &rct, grey); /* Clear old */
						SDLGui_DrawCheckBox(dlg, obj);
						SDL_UpdateRects(pSdlGuiScrn, 1, &rct);
						break;
					 case SGPOPUP:
						dlg[obj].state |= SG_SELECTED;
						SDLGui_DrawPopupButton(dlg, obj);
						SDL_UpdateRect(pSdlGuiScrn, (dlg[0].x+dlg[obj].x)*sdlgui_fontwidth-2, (dlg[0].y+dlg[obj].y)*sdlgui_fontheight-2,
						               dlg[obj].w*sdlgui_fontwidth+4, dlg[obj].h*sdlgui_fontheight+4);
						retbutton=obj;
						break;
					}
				}
				if (oldbutton > 0 && dlg[oldbutton].type == SGBUTTON)
				{
					dlg[oldbutton].state &= ~SG_SELECTED;
					SDLGui_DrawButton(dlg, oldbutton);
					SDL_UpdateRect(pSdlGuiScrn, (dlg[0].x+dlg[oldbutton].x)*sdlgui_fontwidth-2, (dlg[0].y+dlg[oldbutton].y)*sdlgui_fontheight-2,
					               dlg[oldbutton].w*sdlgui_fontwidth+4, dlg[oldbutton].h*sdlgui_fontheight+4);
					oldbutton = 0;
				}
				if (obj >= 0 && (dlg[obj].flags&SG_EXIT))
				{
					retbutton = obj;
				}
				break;

			 case SDL_JOYAXISMOTION:
			 case SDL_JOYBALLMOTION:
			 case SDL_JOYHATMOTION:
			 case SDL_MOUSEMOTION:
				break;

			 case SDL_KEYDOWN:                     /* Key pressed */
				if (sdlEvent.key.keysym.sym == SDLK_RETURN
				    || sdlEvent.key.keysym.sym == SDLK_KP_ENTER)
				{
					retbutton = SDLGui_SearchFlaggedButton(dlg, SG_DEFAULT);
				}
				else if (sdlEvent.key.keysym.sym == SDLK_ESCAPE)
				{
					retbutton = SDLGui_SearchFlaggedButton(dlg, SG_CANCEL);
				}
				else if (pEventOut)
				{
					retbutton = SDLGUI_UNKNOWNEVENT;
				}
				break;

			 default:
				if (pEventOut)
					retbutton = SDLGUI_UNKNOWNEVENT;
				break;
			}
	}

	/* Restore background */
	if (pBgSurface)
	{
		SDL_BlitSurface(pBgSurface, &bgrect, pSdlGuiScrn,  &dlgrect);
		SDL_FreeSurface(pBgSurface);
	}

	/* Copy event data of unsupported events if caller wants to have it */
	if (retbutton == SDLGUI_UNKNOWNEVENT && pEventOut)
		memcpy(pEventOut, &sdlEvent, sizeof(SDL_Event));

	//if (retbutton == SDLGUI_QUIT)
	//	bQuitProgram = true;

	bt = 0;		// FIXME: Hack

	return retbutton;
}
