



#include "rockmacros.h"

#include "defs.h"
#include "rc.h"
#include "hw.h"
#include "emu.h"
#include "save.h"
#include "split.h"

/*
 * the set command is used to set rc-exported variables.
 */

static int cmd_set(int argc, char **argv)
{
	if (argc < 3)
		return -1;
	return rc_setvar(argv[1], argc-2, argv+2);
}



/*
 * the following commands allow keys to be bound to perform rc commands.
 */

static int cmd_reset(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	emu_reset();
	return 0;
}

static int cmd_savestate(int argc, char **argv)
{
	state_save(argc > 1 ? atoi(argv[1]) : -1);
	return 0;
}

static int cmd_loadstate(int argc, char **argv)
{
	state_load(argc > 1 ? atoi(argv[1]) : -1);
	return 0;
}



/*
 * table of command names and the corresponding functions to be called
 */

rccmd_t rccmds[] =
{
	RCC("set", cmd_set),
	RCC("reset", cmd_reset),
	RCC("savestate", cmd_savestate),
	RCC("loadstate", cmd_loadstate),
	RCC_END
};





int rc_command(char *line)
{
	int i, argc, ret;
	char *argv[128], linecopy[500];

//	linecopy = malloc(strlen(line)+1);
	strcpy(linecopy, line);
	
	argc = splitline(argv, (sizeof argv)/(sizeof argv[0]), linecopy);
	if (!argc)
	{
//		free(linecopy);
		return -1;
	}
	
	for (i = 0; rccmds[i].name; i++)
	{
		if (!strcmp(argv[0], rccmds[i].name))
		{
			ret = rccmds[i].func(argc, argv);
//			free(linecopy);
			return ret;
		}
	}

	/* printf("unknown command: %s\n", argv[0]); */
//	free(linecopy);
	
	return -1;
}























