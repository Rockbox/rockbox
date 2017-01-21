/*
    ballergui.c - GUI related functions for Ballerburg

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

#include <SDL.h>

#include "i18n.h"
#include "baller1.h"
#include "ballergui.h"
#include "screen.h"
#include "settings.h"


static int gui_handle_keys(SDL_Event *ev, int allow_dlgs)
{
	static bool in_table = false;

	switch (ev->key.keysym.sym)
	{
	 case SDLK_ESCAPE:
		if (allow_dlgs)
			return settings();
		break;
	 case SDLK_f:
		scr_togglefullscreen();
		break;
	 case SDLK_t:
		if (in_table || !allow_dlgs)
			break;
		in_table = true;
		tabelle();
		in_table = false;
		break;
	 case SDLK_q:
		return DlgAlert_Query(_("Quit Ballerburg?"), _("Yes"), _("No"));
	 default:
		break;
	}

	return 0;
}


int event(int wait, int allow_dlgs)
{
	int ev_avail;
	SDL_Event ev;
	static bool quitflag = false;

	// printf("event(%i)\n", wait);

	if (quitflag)
		return 1;

	if (wait)
		ev_avail = SDL_WaitEvent(&ev);
	else
		ev_avail = SDL_PollEvent(&ev);

	while (ev_avail)
	{
		// printf("EVENT = %i\n", ev.type);
		switch(ev.type)
		{
		 case SDL_QUIT:
			quitflag = true;
			return 1;
		 case SDL_MOUSEMOTION:               /* Read/Update internal mouse position */
			mx = ev.motion.x;
			my = ev.motion.y;
			break;
		 case SDL_MOUSEBUTTONDOWN:
			if (ev.button.button == SDL_BUTTON_LEFT)
			{
				bt |= 1;
			}
			else if (ev.button.button == SDL_BUTTON_RIGHT)
			{
				bt |= 2;
			}
			break;
		 case SDL_MOUSEBUTTONUP:
			if (ev.button.button == SDL_BUTTON_LEFT)
			{
				bt &= ~1;
			}
			else if (ev.button.button == SDL_BUTTON_RIGHT)
			{
				bt &= ~2;
			}
			break;
		 case SDL_KEYUP:
			if (gui_handle_keys(&ev, allow_dlgs)) {
				quitflag = true;
				return 1;
			}
			break;
		}

		ev_avail = SDL_PollEvent(&ev);
	}

	return 0;
}
