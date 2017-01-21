/*
 * dlgAlert.c - AES-like AlertBox 
 *
 * Copyright (c) 2004 Petr Stehlik
 * Copyright (c) 2004, 2010 Thomas Huth
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
const char DlgAlert_fileid[] = "dlgAlert.c : " __DATE__ " " __TIME__;

#include "sdlgui.h"
#include "screen.h"

#define MAX_LINES 4

static char dlglines[MAX_LINES][50+1];


#define DLGALERT_OK       5
#define DLGALERT_CANCEL   6

/* The "Alert"-dialog: */
static SGOBJ alertdlg[] =
{
	{ SGBOX, 0, 0, 0,0, 52,8, NULL },
	{ SGTEXT, 0, 0, 1,1, 50,1, dlglines[0] },
	{ SGTEXT, 0, 0, 1,2, 50,1, dlglines[1] },
	{ SGTEXT, 0, 0, 1,3, 50,1, dlglines[2] },
	{ SGTEXT, 0, 0, 1,4, 50,1, dlglines[3] },
	{ SGBUTTON, SG_DEFAULT, 0, 5,6, 8,1, "OK" },
	{ SGBUTTON, SG_CANCEL, 0, 24,6, 8,1, "Cancel" },
	{ -1, 0, 0, 0,0, 0,0, NULL }
};


/*-----------------------------------------------------------------------*/
/**
 * Breaks long string to several strings of max_width, divided by '\0',
 * sets text_width to the longest line width and returns the number of lines
 * you need to display the strings.
 */
static int DlgAlert_FormatTextToBox(char *text, int max_width, int *text_width)
{
	int columns = 0;
	int lines = 1;
	int txtlen;
	char *p;            /* pointer to begin of actual line */
	char *q;            /* pointer to start of next search */
	char *llb;          /* pointer to last place suitable for breaking the line */
	char *txtend;       /* pointer to end of the text */

	txtlen = strlen(text);

	q = p = text;
	llb = text-1;       /* pointer to last line break */
	txtend = text + txtlen;

	if (txtlen <= max_width && strchr(text, '\n') == NULL)
	{
		*text_width = txtlen;
		return lines;
	}

	while(q < txtend)                             /* q was last place suitable for breaking */
	{
		char *r = strpbrk(q, " \t/\\\n");     /* find next suitable place for the break */
		if (r == NULL)
			r = txtend;                   /* if there's no place then point to the end */

		if ((r-p) <= max_width && *r != '\n') /* '\n' is always used for breaking */
		{
			llb = r;                      /* remember new place suitable for breaking */
			q++;
			if ((r-p) > columns)
				columns = r - p;
			continue;                     /* search again */
		}

		if ((r-p) > max_width)                /* too long line already? */
		{
			if (p > llb)                  /* bad luck - no place for the delimiter. Let's do it the strong way */
				llb = p + max_width;  /* we loose one character */
		}
		else
			llb = r;                /* break from previous delimiter */

		*llb = '\0';                    /* BREAK */
		if ((llb-p) > columns)
			columns = llb - p;      /* longest line so far */
		p = q = llb + 1;                /* next line begins here */
		lines++;                        /* increment line counter */
	}

	*text_width = columns;

	return lines;                           /* return line counter */
}



/*-----------------------------------------------------------------------*/
/**
 * Show the "alert" dialog. Return true if user pressed "OK".
 */
static int DlgAlert_ShowDlg(const char *text)
{
	static int maxlen = sizeof(dlglines[0])-1;
	char *t = (char *)malloc(strlen(text)+1);
	char *orig_t = t;
	int lines, i, len, offset;
	bool bOldMouseVisibility;
	int nOldMouseX, nOldMouseY;

	strcpy(t, text);
	lines = DlgAlert_FormatTextToBox(t, maxlen, &len);
	offset = (maxlen-len)/2;

	for(i=0; i<MAX_LINES; i++)
	{
		if (i < lines)
		{
			/* center text to current dlgline */
			memset(dlglines[i], ' ', offset);
			strcpy(dlglines[i] + offset, t);
			t += strlen(t)+1;
		}
		else
		{
			dlglines[i][0] = '\0';
		}
	}

	free(orig_t);

	if (SDLGui_SetScreen(surf))
		return false;
	SDLGui_CenterDlg(alertdlg);

	SDL_GetMouseState(&nOldMouseX, &nOldMouseY);
	bOldMouseVisibility = SDL_ShowCursor(SDL_QUERY);
	SDL_ShowCursor(SDL_ENABLE);

	i = SDLGui_DoDialog(alertdlg, NULL);

	SDL_UpdateRect(surf, 0,0, 0,0);
	SDL_ShowCursor(bOldMouseVisibility);

	return (i == DLGALERT_OK);
}


/**
 * Show a "notice" dialog: (only one button)
 */
int DlgAlert_Notice(const char *text, const char *button)
{
	/* First "OK" button: */
	alertdlg[DLGALERT_OK].txt = (char *)button;
	alertdlg[DLGALERT_OK].w = strlen(button) + 2;

	/* Hide "cancel" button: */
	alertdlg[DLGALERT_CANCEL].type = SGTEXT;
	alertdlg[DLGALERT_CANCEL].txt = "";
	alertdlg[DLGALERT_CANCEL].w = 0;
	alertdlg[DLGALERT_CANCEL].h = 0;

	/* Adjust button position: */
	alertdlg[DLGALERT_OK].x = (alertdlg[0].w - alertdlg[DLGALERT_OK].w) / 2;

	return DlgAlert_ShowDlg(text);
}


/**
 * Show a "query" dialog: (two buttons), return true for OK
 */
int DlgAlert_Query(const char *text, const char *button1, 
		   const char *button2)
{
	/* First "OK" button: */
	alertdlg[DLGALERT_OK].txt = (char *)button1;
	alertdlg[DLGALERT_OK].w = strlen(button1) + 2;

	/* Show "cancel" button: */
	alertdlg[DLGALERT_CANCEL].type = SGBUTTON;
	alertdlg[DLGALERT_CANCEL].txt = (char *)button2;
	alertdlg[DLGALERT_CANCEL].w = strlen(button2) + 2;
	alertdlg[DLGALERT_CANCEL].h = 1;

	/* Adjust buttons positions: */
	alertdlg[DLGALERT_OK].x = (alertdlg[0].w - alertdlg[DLGALERT_OK].w - alertdlg[DLGALERT_CANCEL].w) / 3;
	alertdlg[DLGALERT_CANCEL].x = alertdlg[DLGALERT_OK].x * 2 + alertdlg[DLGALERT_OK].w;

	return DlgAlert_ShowDlg(text);
}
