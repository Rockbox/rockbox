

#include "rockmacros.h"

#include "rc.h"

extern rcvar_t emu_exports[], loader_exports[],
	lcd_exports[], rtc_exports[], sound_exports[],
	vid_exports[], joy_exports[], pcm_exports[];


rcvar_t *sources[] =
{
	emu_exports,
	loader_exports,
	lcd_exports,
	rtc_exports,
	sound_exports,
	vid_exports,
	joy_exports,
	pcm_exports,
	NULL
};


void init_exports(void)
{
	rcvar_t **s = sources;
	
	while (*s)
		rc_exportvars(*(s++));
}


void show_exports(void)
{
	// TODO
	/*int i, j;
	for (i = 0; sources[i]; i++)
		for (j = 0; sources[i][j].name; j++)
			printf("%s\n", sources[i][j].name);*/
}
