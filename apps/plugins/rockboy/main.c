#include <stdio.h>
#include <string.h>

#include "rockmacros.h"
#include "input.h"
#include "rc.h"
#include "exports.h"
#include "emu.h"
#include "loader.h"
#include "hw.h"

//#include "Version"


static char *defaultconfig[] =
{
	"bind up +up",
	"bind down +down",
	"bind left +left",
	"bind right +right",
	"bind joy0 +b",
	"bind joy1 +a",
	"bind joy2 +select",
	"bind joy3 +start",
	"bind ins savestate",
	"bind del loadstate",
	NULL
};


void doevents()
{
	event_t ev;
	int st;

	ev_poll();
	while (ev_getevent(&ev))
	{
		if (ev.type != EV_PRESS && ev.type != EV_RELEASE)
			continue;
		st = (ev.type != EV_RELEASE);
		pad_set(ev.code, st);
	}
}



/* convenience macro for printing loading state */
#define PUTS(str) do { \
  rb->lcd_putsxy(1, y, str); \
  rb->lcd_getstringsize(str, &w, &h); \
  y += h + 1; \
} while (0)

int gnuboy_main(char *rom)
{
	int i, w, h, y;

  y = 1;
	// Avoid initializing video if we don't have to 
	// If we have special perms, drop them ASAP! 
	PUTS("Init exports");
	init_exports();

	PUTS("Loading default config");
	for (i = 0; defaultconfig[i]; i++)
		rc_command(defaultconfig[i]);

//	sprintf(cmd, "source %s", rom);
//	s = strchr(cmd, '.');
//	if (s) *s = 0;
//	strcat(cmd, ".rc");
//	rc_command(cmd);

	// FIXME - make interface modules responsible for atexit() 
	PUTS("Init video");
	vid_init();
	PUTS("Init sound");
	pcm_init();
	PUTS("Loading rom");
	loader_init(rom);
	if(shut)
		return PLUGIN_ERROR;
	PUTS("Emu reset");
	emu_reset();
	PUTS("Emu run");
	emu_run();

	// never reached 
	return PLUGIN_OK;
}
#undef PUTS
