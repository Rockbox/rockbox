/* Copyright (c) 1997-1999 Miller Puckette and others.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* IOhannes :
 * hacked the code to add advanced multidevice-support
 * 1311:forum::für::umläute:2001
 */

char pd_version[] = "Pd version 0.37.4\n";
char pd_compiletime[] = __TIME__;
char pd_compiledate[] = __DATE__;

#include "m_pd.h"
#include "m_imp.h"
#include "s_stuff.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

#ifdef UNIX
#include <unistd.h>
#endif
#ifdef MSW
#include <io.h>
#include <windows.h>
#include <winbase.h>
#endif

void pd_init(void);
int sys_argparse(int argc, char **argv);
void sys_findprogdir(char *progname);
int sys_startgui(const char *guipath);
int sys_rcfile(void);
int m_scheduler(void);
void sys_addhelppath(char *p);
void alsa_adddev(char *name);

int sys_debuglevel;
int sys_verbose;
int sys_noloadbang;
int sys_nogui;
int sys_stdin = 0;
char *sys_guicmd;
t_symbol *sys_libdir;
static t_symbol *sys_guidir;
static t_namelist *sys_externlist;
static t_namelist *sys_openlist;
static t_namelist *sys_messagelist;
static int sys_version;
int sys_oldtclversion;	    /* hack to warn g_rtext.c about old text sel */

int sys_nmidiout = 1;
#ifdef MSW
int sys_nmidiin = 0;
#else
int sys_nmidiin = 1;
#endif
int sys_midiindevlist[MAXMIDIINDEV] = {1};
int sys_midioutdevlist[MAXMIDIOUTDEV] = {1};

static int sys_main_srate = DEFAULTSRATE;
static int sys_main_advance = DEFAULTADVANCE;

/* IOhannes { */

    /* here the "-1" counts signify that the corresponding vector hasn't been
    specified in command line arguments; sys_open_audio will detect this
    and fill things in. */
int sys_nsoundin = -1;
int sys_nsoundout = -1;
int sys_soundindevlist[MAXAUDIOINDEV];
int sys_soundoutdevlist[MAXAUDIOOUTDEV];

int sys_nchin = -1;
int sys_nchout = -1;
int sys_chinlist[MAXAUDIOINDEV];
int sys_choutlist[MAXAUDIOOUTDEV];
/* } IOhannes */


typedef struct _fontinfo
{
    int fi_fontsize;
    int fi_maxwidth;
    int fi_maxheight;
    int fi_hostfontsize;
    int fi_width;
    int fi_height;
} t_fontinfo;

    /* these give the nominal point size and maximum height of the characters
    in the six fonts.  */

static t_fontinfo sys_fontlist[] = {
    {8, 5, 9, 0, 0, 0}, {10, 7, 13, 0, 0, 0}, {12, 9, 16, 0, 0, 0},
    {16, 10, 20, 0, 0, 0}, {24, 15, 25, 0, 0, 0}, {36, 25, 45, 0, 0, 0}};
#define NFONT (sizeof(sys_fontlist)/sizeof(*sys_fontlist))

/* here are the actual font size structs on msp's systems:
MSW:
font 8 5 9 8 5 11
font 10 7 13 10 6 13
font 12 9 16 14 8 16
font 16 10 20 16 10 18
font 24 15 25 16 10 18
font 36 25 42 36 22 41

linux:
font 8 5 9 8 5 9
font 10 7 13 12 7 13
font 12 9 16 14 9 15
font 16 10 20 16 10 19
font 24 15 25 24 15 24
font 36 25 42 36 22 41
*/

static t_fontinfo *sys_findfont(int fontsize)
{
    unsigned int i;
    t_fontinfo *fi;
    for (i = 0, fi = sys_fontlist; i < (NFONT-1); i++, fi++)
    	if (fontsize < fi[1].fi_fontsize) return (fi);
    return (sys_fontlist + (NFONT-1));
}

int sys_nearestfontsize(int fontsize)
{
    return (sys_findfont(fontsize)->fi_fontsize);
}

int sys_hostfontsize(int fontsize)
{
    return (sys_findfont(fontsize)->fi_hostfontsize);
}

int sys_fontwidth(int fontsize)
{
    return (sys_findfont(fontsize)->fi_width);
}

int sys_fontheight(int fontsize)
{
    return (sys_findfont(fontsize)->fi_height);
}

int sys_defaultfont;
#ifdef MSW
#define DEFAULTFONT 12
#else
#define DEFAULTFONT 10
#endif

static void openit(const char *dirname, const char *filename)
{
    char dirbuf[MAXPDSTRING], *nameptr;
    int fd = open_via_path(dirname, filename, "", dirbuf, &nameptr,
    	MAXPDSTRING, 0);
    if (fd)
    {
    	close (fd);
    	glob_evalfile(0, gensym(nameptr), gensym(dirbuf));
    }
    else
    	error("%s: can't open", filename);
}

#define NHOSTFONT 7

/* this is called from the gui process.  The first argument is the cwd, and
succeeding args give the widths and heights of known fonts.  We wait until 
these are known to open files and send messages specified on the command line.
We ask the GUI to specify the "cwd" in case we don't have a local OS to get it
from; for instance we could be some kind of RT embedded system.  However, to
really make this make sense we would have to implement
open(), read(), etc, calls to be served somehow from the GUI too. */

void glob_initfromgui(void *dummy, t_symbol *s, int argc, t_atom *argv)
{
    char *cwd = atom_getsymbolarg(0, argc, argv)->s_name;
    t_namelist *nl;
    unsigned int i, j;
    if (argc != 2 + 3 * NHOSTFONT) bug("glob_initfromgui");
    for (i = 0; i < NFONT; i++)
    {
    	int wantheight = sys_fontlist[i].fi_maxheight;
	for (j = 0; j < NHOSTFONT-1; j++)
	{
	    if (atom_getintarg(3 * (j + 1) + 3, argc, argv) > wantheight)
		    break;
	}
	    /* j is now the "real" font index for the desired font index i. */
	sys_fontlist[i].fi_hostfontsize = atom_getintarg(3 * j + 1, argc, argv);
	sys_fontlist[i].fi_width = atom_getintarg(3 * j + 2, argc, argv);
	sys_fontlist[i].fi_height = atom_getintarg(3 * j + 3, argc, argv);
    }
#if 0
    for (i = 0; i < 6; i++)
    	fprintf(stderr, "font %d %d %d %d %d\n",
	    sys_fontlist[i].fi_fontsize,
	    sys_fontlist[i].fi_maxheight,
	    sys_fontlist[i].fi_hostfontsize,
	    sys_fontlist[i].fi_width,
	    sys_fontlist[i].fi_height);
#endif
    	/* load dynamic libraries specified with "-lib" args */
    for  (nl = sys_externlist; nl; nl = nl->nl_next)
    	if (!sys_load_lib(cwd, nl->nl_string))
	    post("%s: can't load library", nl->nl_string);
    namelist_free(sys_externlist);
    sys_externlist = 0;
    	/* open patches specifies with "-open" args */
    for  (nl = sys_openlist; nl; nl = nl->nl_next)
    	openit(cwd, nl->nl_string);
    namelist_free(sys_openlist);
    sys_openlist = 0;
    	/* send messages specified with "-send" args */
    for  (nl = sys_messagelist; nl; nl = nl->nl_next)
    {
    	t_binbuf *b = binbuf_new();
	binbuf_text(b, nl->nl_string, strlen(nl->nl_string));
	binbuf_eval(b, 0, 0, 0);
	binbuf_free(b);
    }
    namelist_free(sys_messagelist);
    sys_messagelist = 0;
    sys_oldtclversion = atom_getfloatarg(1 + 3 * NHOSTFONT, argc, argv);
}

static void sys_afterargparse(void);

/* this is called from main() in s_entry.c */
int sys_main(int argc, char **argv)
{
#ifdef PD_DEBUG
    fprintf(stderr, "Pd: COMPILED FOR DEBUGGING\n");
#endif
    pd_init();
    sys_findprogdir(argv[0]);	    	    	/* set sys_progname, guipath */
#ifdef UNIX
    sys_rcfile();                               /* parse the startup file */
#endif
    if (sys_argparse(argc, argv))   	    /* parse cmd line */
    	return (1);
    sys_afterargparse();    	    	    /* post-argparse settings */
    if (sys_verbose || sys_version) fprintf(stderr, "%scompiled %s %s\n",
    	pd_version, pd_compiletime, pd_compiledate);
    if (sys_version)	/* if we were just asked our version, exit here. */
    	return (0);
    if (sys_startgui(sys_guidir->s_name))	/* start the gui */
    	return(1);
    	    /* open audio and MIDI */
    sys_open_midi(sys_nmidiin, sys_midiindevlist,
    	sys_nmidiout, sys_midioutdevlist);
    sys_open_audio(sys_nsoundin, sys_soundindevlist, sys_nchin, sys_chinlist,
    	sys_nsoundout, sys_soundoutdevlist, sys_nchout, sys_choutlist,
	    sys_main_srate, sys_main_advance, 1);
    	    /* run scheduler until it quits */
    
   return (m_scheduler_pda());
    return (m_scheduler());
}

static char *(usagemessage[]) = {
"usage: pd [-flags] [file]...\n",
"\naudio configuration flags:\n",
"-r <n>           -- specify sample rate\n",
"-audioindev ...  -- audio in devices; e.g., \"1,3\" for first and third\n",
"-audiooutdev ... -- audio out devices (same)\n",
"-audiodev ...    -- specify input and output together\n",
"-inchannels ...  -- audio input channels (by device, like \"2\" or \"16,8\")\n",
"-outchannels ... -- number of audio out channels (same)\n",
"-channels ...    -- specify both input and output channels\n",
"-audiobuf <n>    -- specify size of audio buffer in msec\n",
"-blocksize <n>   -- specify audio I/O block size in sample frames\n",
"-sleepgrain <n>  -- specify number of milliseconds to sleep when idle\n",
"-nodac           -- suppress audio output\n",
"-noadc           -- suppress audio input\n",
"-noaudio         -- suppress audio input and output (-nosound is synonym) \n",
"-listdev         -- list audio and MIDI devices\n",

#ifdef USEAPI_OSS
"-oss     	  -- use OSS audio API\n",
"-32bit     	  ----- allow 32 bit OSS audio (for RME Hammerfall)\n",
#endif

#ifdef USEAPI_ALSA
"-alsa            -- use ALSA audio API\n",
"-alsaadd <name>  -- add an ALSA device name to list\n",
"-alsadev <n>     ----- obsolete: use -audiodev\n",
#endif

#ifdef USEAPI_JACK
"-jack            -- use JACK audio API\n",
#endif

#ifdef USEAPI_PORTAUDIO
#ifdef MSW
"-asio            -- use ASIO audio driver (via Portaudio)\n",
"-pa              -- synonym for -asio\n",
#else
"-pa              -- use Portaudio API\n",
#endif
#endif

#ifdef USEAPI_MMIO
"-mmio     	  -- use MMIO audio API (default for Windows)\n",
#endif
"      (default audio API for this platform:  ", API_DEFSTRING, ")\n\n",

"\nMIDI configuration flags:\n",
"-midiindev ...   -- midi in device list; e.g., \"1,3\" for first and third\n",
"-midioutdev ...  -- midi out device list, same format\n",
"-mididev ...     -- specify -midioutdev and -midiindev together\n",
"-nomidiin        -- suppress MIDI input\n",
"-nomidiout       -- suppress MIDI output\n",
"-nomidi          -- suppress MIDI input and output\n",

"\nother flags:\n",
"-path <path>     -- add to file search path\n",
"-helppath <path> -- add to help file search path\n",
"-open <file>     -- open file(s) on startup\n",
"-lib <file>      -- load object library(s)\n",
"-font <n>        -- specify default font size in points\n",
"-verbose         -- extra printout on startup and when searching for files\n",
"-version         -- don't run Pd; just print out which version it is \n",
"-d <n>           -- specify debug level\n",
"-noloadbang      -- suppress all loadbangs\n",
"-nogui           -- suppress starting the GUI\n",
"-stdin           -- scan stdin for keypresses\n",
"-guicmd \"cmd...\" -- substitute another GUI program (e.g., rsh)\n",
"-send \"msg...\"   -- send a message at startup (after patches are loaded)\n",
#ifdef UNIX
"-rt or -realtime -- use real-time priority\n",
"-nrt             -- don't use real-time priority\n",
#endif
};

static void sys_parsedevlist(int *np, int *vecp, int max, char *str)
{
    int n = 0;
    while (n < max)
    {
    	if (!*str) break;
	else
	{
    	    char *endp;
	    vecp[n] = strtol(str, &endp, 10);
	    if (endp == str)
	    	break;
	    n++;
	    if (!endp)
	    	break;
	    str = endp + 1;
	}
    }
    *np = n;
}

static int sys_getmultidevchannels(int n, int *devlist)
{
    int sum = 0;
    if (n<0)return(-1);
    if (n==0)return 0;
    while(n--)sum+=*devlist++;
    return sum;
}


    /* this routine tries to figure out where to find the auxilliary files
    Pd will need to run.  This is either done by looking at the command line
    invokation for Pd, or if that fails, by consulting the variable
    INSTALL_PREFIX.  In MSW, we don't try to use INSTALL_PREFIX. */
void sys_findprogdir(char *progname)
{
    char sbuf[MAXPDSTRING], sbuf2[MAXPDSTRING], *sp;
    char *lastslash; 
#ifdef UNIX
    struct stat statbuf;
#endif

    /* find out by what string Pd was invoked; put answer in "sbuf". */
#ifdef MSW
    GetModuleFileName(NULL, sbuf2, sizeof(sbuf2));
    sbuf2[MAXPDSTRING-1] = 0;
    sys_unbashfilename(sbuf2, sbuf);
#endif /* MSW */
#ifdef UNIX
    strncpy(sbuf, progname, MAXPDSTRING);
    sbuf[MAXPDSTRING-1] = 0;
#endif
    lastslash = strrchr(sbuf, '/');
    if (lastslash)
    {
    	    /* bash last slash to zero so that sbuf is directory pd was in,
	    	e.g., ~/pd/bin */
    	*lastslash = 0; 
	    /* go back to the parent from there, e.g., ~/pd */
    	lastslash = strrchr(sbuf, '/');
    	if (lastslash)
    	{
    	    strncpy(sbuf2, sbuf, lastslash-sbuf);
    	    sbuf2[lastslash-sbuf] = 0;
    	}
    	else strcpy(sbuf2, "..");
    }
    else
    {
    	    /* no slashes found.  Try INSTALL_PREFIX. */
#ifdef INSTALL_PREFIX
    	strcpy(sbuf2, INSTALL_PREFIX);
#else
    	strcpy(sbuf2, ".");
#endif
    }
    	/* now we believe sbuf2 holds the parent directory of the directory
	pd was found in.  We now want to infer the "lib" directory and the
	"gui" directory.  In "simple" UNIX installations, the layout is
	    .../bin/pd
	    .../bin/pd-gui
	    .../doc
	and in "complicated" UNIX installations, it's:
	    .../bin/pd
	    .../lib/pd/bin/pd-gui
	    .../lib/pd/doc
    	To decide which, we stat .../lib/pd; if that exists, we assume it's
	the complicated layout.  In MSW, it's the "simple" layout, but
	the gui program is straight wish80:
	    .../bin/pd
	    .../bin/wish80.exe
	    .../doc
	*/
#ifdef UNIX
    strncpy(sbuf, sbuf2, MAXPDSTRING-30);
    sbuf[MAXPDSTRING-30] = 0;
    strcat(sbuf, "/lib/pd");
    if (stat(sbuf, &statbuf) >= 0)
    {
    	    /* complicated layout: lib dir is the one we just stat-ed above */
    	sys_libdir = gensym(sbuf);
	    /* gui lives in .../lib/pd/bin */
    	strncpy(sbuf, sbuf2, MAXPDSTRING-30);
    	sbuf[MAXPDSTRING-30] = 0;
    	strcat(sbuf, "/lib/pd/bin");
    	sys_guidir = gensym(sbuf);
    }
    else
    {
    	    /* simple layout: lib dir is the parent */
    	sys_libdir = gensym(sbuf2);
	    /* gui lives in .../bin */
    	strncpy(sbuf, sbuf2, MAXPDSTRING-30);
    	sbuf[MAXPDSTRING-30] = 0;
    	strcat(sbuf, "/bin");
    	sys_guidir = gensym(sbuf);
    }
#endif
#ifdef MSW
    sys_libdir = gensym(sbuf2);
    sys_guidir = &s_;	/* in MSW the guipath just depends on the libdir */
#endif
}

#ifdef MSW
static int sys_mmio = 1;
#else
static int sys_mmio = 0;
#endif

int sys_argparse(int argc, char **argv)
{
    char sbuf[MAXPDSTRING];
    int i;
    argc--; argv++;
    while ((argc > 0) && **argv == '-')
    {
    	if (!strcmp(*argv, "-r") && argc > 1 &&
    	    sscanf(argv[1], "%d", &sys_main_srate) >= 1)
    	{
    	    argc -= 2;
    	    argv += 2;
    	}
    	else if (!strcmp(*argv, "-inchannels"))
	{ /* IOhannes */
	    sys_parsedevlist(&sys_nchin,
	    	sys_chinlist, MAXAUDIOINDEV, argv[1]);

	  if (!sys_nchin)
	      goto usage;

	  argc -= 2; argv += 2;
    	}
    	else if (!strcmp(*argv, "-outchannels"))
	{ /* IOhannes */
	    sys_parsedevlist(&sys_nchout, sys_choutlist,
	    	MAXAUDIOOUTDEV, argv[1]);

	  if (!sys_nchout)
	    goto usage;

	  argc -= 2; argv += 2;
    	}
    	else if (!strcmp(*argv, "-channels"))
	{
	    sys_parsedevlist(&sys_nchin, sys_chinlist,MAXAUDIOINDEV,
	    	argv[1]);
	    sys_parsedevlist(&sys_nchout, sys_choutlist,MAXAUDIOOUTDEV,
	    	argv[1]);

	    if (!sys_nchout)
	      goto usage;

	    argc -= 2; argv += 2;
    	}
    	else if (!strcmp(*argv, "-soundbuf") || !strcmp(*argv, "-audiobuf"))
    	{
    	    sys_main_advance = atoi(argv[1]);
    	    argc -= 2; argv += 2;
    	}
    	else if (!strcmp(*argv, "-blocksize"))
    	{
    	    sys_setblocksize(atoi(argv[1]));
    	    argc -= 2; argv += 2;
    	}
    	else if (!strcmp(*argv, "-sleepgrain"))
    	{
    	    sys_sleepgrain = 1000 * atoi(argv[1]);
    	    argc -= 2; argv += 2;
    	}
    	else if (!strcmp(*argv, "-nodac"))
	{ /* IOhannes */
	    sys_nsoundout=0;
	    sys_nchout = 0;
	    argc--; argv++;
    	}
    	else if (!strcmp(*argv, "-noadc"))
	{ /* IOhannes */
	    sys_nsoundin=0;
	    sys_nchin = 0;
	    argc--; argv++;
    	}
    	else if (!strcmp(*argv, "-nosound") || !strcmp(*argv, "-noaudio"))
	{ /* IOhannes */
	    sys_nsoundin=sys_nsoundout = 0;
	    sys_nchin = sys_nchout = 0;
	    argc--; argv++;
    	}
#ifdef USEAPI_OSS
    	else if (!strcmp(*argv, "-oss"))
    	{
    	    sys_set_audio_api(API_OSS);
    	    argc--; argv++;
    	}
    	else if (!strcmp(*argv, "-32bit"))
    	{
    	    sys_set_audio_api(API_OSS);
    	    oss_set32bit();
    	    argc--; argv++;
    	}
#endif
#ifdef USEAPI_ALSA
    	else if (!strcmp(*argv, "-alsa"))
    	{
    	    sys_set_audio_api(API_ALSA);
    	    argc--; argv++;
    	}
    	else if (!strcmp(*argv, "-alsaadd"))
    	{
	    if (argc > 1)
	    	alsa_adddev(argv[1]);
	    else goto usage;
	    argc -= 2; argv +=2;
    	}
	    /* obsolete flag for setting ALSA device number or name */
	else if (!strcmp(*argv, "-alsadev"))
	{
	    int devno = 0;
	    if (argv[1][0] >= '1' && argv[1][0] <= '9')
	    	devno = 1 + 2 * (atoi(argv[1]) - 1);
	    else if (!strncmp(argv[1], "hw:", 3))
    	    	devno = 1 + 2 * atoi(argv[1]+3);
	    else if (!strncmp(argv[1], "plughw:", 7))
    	    	devno = 2 + 2 * atoi(argv[1]+7);
	    else goto usage;
	    sys_nsoundin = sys_nsoundout = 1;
	    sys_soundindevlist[0] = sys_soundoutdevlist[0] = devno;
    	    sys_set_audio_api(API_ALSA);
	    argc -= 2; argv +=2;
	}
#endif
#ifdef USEAPI_JACK
    	else if (!strcmp(*argv, "-jack"))
    	{
    	    sys_set_audio_api(API_JACK);
    	    argc--; argv++;
    	}
#endif
#ifdef USEAPI_PORTAUDIO
    	else if (!strcmp(*argv, "-pa") || !strcmp(*argv, "-portaudio")
#ifdef MSW
    	    || !strcmp(*argv, "-asio")
#endif
	    )
    	{
    	    sys_set_audio_api(API_PORTAUDIO);
	    sys_mmio = 0;
    	    argc--; argv++;
    	}
#endif
#ifdef USEAPI_MMIO
    	else if (!strcmp(*argv, "-mmio"))
    	{
    	    sys_set_audio_api(API_MMIO);
	    sys_mmio = 1;
    	    argc--; argv++;
    	}
#endif
    	else if (!strcmp(*argv, "-nomidiin"))
    	{
    	    sys_nmidiin = 0;
    	    argc--; argv++;
    	}
    	else if (!strcmp(*argv, "-nomidiout"))
    	{
    	    sys_nmidiout = 0;
    	    argc--; argv++;
    	}
    	else if (!strcmp(*argv, "-nomidi"))
    	{
    	    sys_nmidiin = sys_nmidiout = 0;
    	    argc--; argv++;
    	}
    	else if (!strcmp(*argv, "-midiindev"))
    	{
    	    sys_parsedevlist(&sys_nmidiin, sys_midiindevlist, MAXMIDIINDEV,
	    	argv[1]);
    	    if (!sys_nmidiin)
	    	goto usage;
    	    argc -= 2; argv += 2;
    	}
    	else if (!strcmp(*argv, "-midioutdev"))
    	{
    	    sys_parsedevlist(&sys_nmidiout, sys_midioutdevlist, MAXMIDIOUTDEV,
	    	argv[1]);
    	    if (!sys_nmidiout)
	    	goto usage;
    	    argc -= 2; argv += 2;
    	}
    	else if (!strcmp(*argv, "-mididev"))
    	{
    	    sys_parsedevlist(&sys_nmidiin, sys_midiindevlist, MAXMIDIINDEV,
	    	argv[1]);
    	    sys_parsedevlist(&sys_nmidiout, sys_midioutdevlist, MAXMIDIOUTDEV,
	    	argv[1]);
    	    if (!sys_nmidiout)
	    	goto usage;
    	    argc -= 2; argv += 2;
    	}
    	else if (!strcmp(*argv, "-path"))
    	{
    	    sys_addpath(argv[1]);
    	    argc -= 2; argv += 2;
    	}
    	else if (!strcmp(*argv, "-helppath"))
    	{
    	    sys_addhelppath(argv[1]);
    	    argc -= 2; argv += 2;
    	}
    	else if (!strcmp(*argv, "-open") && argc > 1)
    	{
    	    sys_openlist = namelist_append(sys_openlist, argv[1]);
    	    argc -= 2; argv += 2;
    	}
        else if (!strcmp(*argv, "-lib") && argc > 1)
        {
    	    sys_externlist = namelist_append(sys_externlist, argv[1]);
    	    argc -= 2; argv += 2;
        }
    	else if (!strcmp(*argv, "-font") && argc > 1)
    	{
    	    sys_defaultfont = sys_nearestfontsize(atoi(argv[1]));
    	    argc -= 2;
    	    argv += 2;
    	}
    	else if (!strcmp(*argv, "-verbose"))
    	{
    	    sys_verbose = 1;
    	    argc--; argv++;
    	}
    	else if (!strcmp(*argv, "-version"))
    	{
    	    sys_version = 1;
    	    argc--; argv++;
    	}
    	else if (!strcmp(*argv, "-d") && argc > 1 &&
    	    sscanf(argv[1], "%d", &sys_debuglevel) >= 1)
    	{
    	    argc -= 2;
    	    argv += 2;
    	}
    	else if (!strcmp(*argv, "-noloadbang"))
    	{
    	    sys_noloadbang = 1;
    	    argc--; argv++;
    	}
    	else if (!strcmp(*argv, "-nogui"))
    	{
    	    sys_nogui = 1;
    	    argc--; argv++;
    	}
    	else if (!strcmp(*argv, "-stdin"))
    	{
    	    sys_stdin = 1;
    	    argc--; argv++;
    	}
    	else if (!strcmp(*argv, "-guicmd") && argc > 1)
    	{
    	    sys_guicmd = argv[1];
    	    argc -= 2; argv += 2;
    	}
    	else if (!strcmp(*argv, "-send") && argc > 1)
    	{
    	    sys_messagelist = namelist_append(sys_messagelist, argv[1]);
    	    argc -= 2; argv += 2;
    	}
    	else if (!strcmp(*argv, "-listdev"))
    	{
	    sys_listdevs();
    	    argc--; argv++;
    	}
#ifdef UNIX
    	else if (!strcmp(*argv, "-rt") || !strcmp(*argv, "-realtime"))
    	{
    	    sys_hipriority = 1;
    	    argc--; argv++;
    	}
    	else if (!strcmp(*argv, "-nrt"))
    	{
    	    sys_hipriority = 0;
    	    argc--; argv++;
    	}
#endif
    	else if (!strcmp(*argv, "-soundindev") ||
	    !strcmp(*argv, "-audioindev"))
	  { /* IOhannes */
	  sys_parsedevlist(&sys_nsoundin, sys_soundindevlist,
	      MAXAUDIOINDEV, argv[1]);
	  if (!sys_nsoundin)
	    goto usage;
	  argc -= 2; argv += 2;
    	}
    	else if (!strcmp(*argv, "-soundoutdev") ||
	    !strcmp(*argv, "-audiooutdev"))
	  { /* IOhannes */
	  sys_parsedevlist(&sys_nsoundout, sys_soundoutdevlist,
	      MAXAUDIOOUTDEV, argv[1]);
	  if (!sys_nsoundout)
	    goto usage;
    	    argc -= 2; argv += 2;
    	}
    	else if (!strcmp(*argv, "-sounddev") || !strcmp(*argv, "-audiodev"))
	{
	  sys_parsedevlist(&sys_nsoundin, sys_soundindevlist,
	      MAXAUDIOINDEV, argv[1]);
	  sys_parsedevlist(&sys_nsoundout, sys_soundoutdevlist,
	      MAXAUDIOOUTDEV, argv[1]);
	  if (!sys_nsoundout)
	    goto usage;
    	    argc -= 2; argv += 2;
    	}
    	else
    	{
	    unsigned int i;
    	usage:
	    for (i = 0; i < sizeof(usagemessage)/sizeof(*usagemessage); i++)
	    	fprintf(stderr, "%s", usagemessage[i]);
    	    return (1);
    	}
    }
    if (!sys_defaultfont)
    	sys_defaultfont = DEFAULTFONT;
    for (; argc > 0; argc--, argv++) 
    	sys_openlist = namelist_append(sys_openlist, *argv);


    return (0);
}

int sys_getblksize(void)
{
    return (DEFDACBLKSIZE);
}

    /* stuff to do, once, after calling sys_argparse() -- which may itself
    be called twice because of the .pdrc hack. */
static void sys_afterargparse(void)
{
    char sbuf[MAXPDSTRING];
    int i;
	    /* add "extra" library to path */
    strncpy(sbuf, sys_libdir->s_name, MAXPDSTRING-30);
    sbuf[MAXPDSTRING-30] = 0;
    strcat(sbuf, "/extra");
    sys_addpath(sbuf);
    strncpy(sbuf, sys_libdir->s_name, MAXPDSTRING-30);
    sbuf[MAXPDSTRING-30] = 0;
    strcat(sbuf, "/intern");
    sys_addpath(sbuf);
	    /* add "doc/5.reference" library to helppath */
    strncpy(sbuf, sys_libdir->s_name, MAXPDSTRING-30);
    sbuf[MAXPDSTRING-30] = 0;
    strcat(sbuf, "/doc/5.reference");
    sys_addhelppath(sbuf);
    	/* correct to make audio and MIDI device lists zero based.  On
	MMIO, however, "1" really means the second device (the first one
	is "mapper" which is was not included when the command args were
	set up, so we leave it that way for compatibility. */
    if (!sys_mmio)
    {
    	for (i = 0; i < sys_nsoundin; i++)
    	    sys_soundindevlist[i]--;
    	for (i = 0; i < sys_nsoundout; i++)
    	    sys_soundoutdevlist[i]--;
    }
    for (i = 0; i < sys_nmidiin; i++)
    	sys_midiindevlist[i]--;
    for (i = 0; i < sys_nmidiout; i++)
    	sys_midioutdevlist[i]--;
}

static void sys_addreferencepath(void)
{
    char sbuf[MAXPDSTRING];
}

