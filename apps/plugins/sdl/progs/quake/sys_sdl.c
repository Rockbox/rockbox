#include <limits.h>
#include <sys/types.h>
#include <stdarg.h>
#include <ctype.h>

#include "quakedef.h"

qboolean			isDedicated;

int noconinput = 0;

char *basedir = "/.rockbox/quake";
char *cachedir = NULL;

cvar_t  sys_linerefresh = {"sys_linerefresh","0"};// set for entity display
cvar_t  sys_nostdout = {"sys_nostdout","0"};

// =======================================================================
// General routines
// =======================================================================

void Sys_DebugNumber(int y, int val)
{
}

int enable_printf = 1;

void Sys_Printf (char *fmt, ...)
{
	va_list		argptr;
	char		text[1024];
	
	va_start (argptr,fmt);
	vsprintf (text,fmt,argptr);
	va_end (argptr);
        if(enable_printf)
        {
            printf("%s", text);
        }
        LOGF("%s", text);
	
	//Con_Print (text);
}

void Sys_Quit (void)
{
	Host_Shutdown();
	exit(0);
}

void Sys_Init(void)
{
#if id386
	Sys_SetFPCW();
#endif
}

#if !id386

/*
================
Sys_LowFPPrecision
================
*/
void Sys_LowFPPrecision (void)
{
// causes weird problems on Nextstep
}


/*
================
Sys_HighFPPrecision
================
*/
void Sys_HighFPPrecision (void)
{
// causes weird problems on Nextstep
}

#endif	// !id386


void Sys_Error (char *error, ...)
{ 
    va_list     argptr;
    char        string[1024];

    va_start (argptr,error);
    vsprintf (string,error,argptr);
    va_end (argptr);
    rb->splashf(HZ*5, "Error: %s\n", string);

	Host_Shutdown ();
	exit (1);

} 

void Sys_Warn (char *warning, ...)
{ 
    va_list     argptr;
    char        string[1024];
    
    va_start (argptr,warning);
    vsprintf (string,warning,argptr);
    va_end (argptr);
	rb->splashf(HZ*2, "Warning: %s", string);
} 

/*
===============================================================================

FILE IO

===============================================================================
*/

#define	MAX_HANDLES		10
//static FILE	*sys_handles[MAX_HANDLES];

/* a file fully or partially cached in memory */
/* We use the FILE handle to track the current position */
static struct memfile {
    FILE *f; /* NULL: unused slot */
    char *buf;
    size_t resident_len; /* bytes 0-(resident_len-1) are in memory */
    size_t full_len;
} sys_handles[MAX_HANDLES];

void Sys_Shutdown(void)
{
    for(int i = 0; i < MAX_HANDLES; i++)
    {
        FILE *f = sys_handles[i].f;
        if(f)
            fclose(f);
        sys_handles[i].f = NULL;
    }
}

int		findhandle (void)
{
	int		i;
	
	for (i=1 ; i<MAX_HANDLES ; i++)
		if (!sys_handles[i].f)
			return i;
	Sys_Error ("out of handles");
	return -1;
}

/*
================
Qfilelength
================
*/
static int Qfilelength (FILE *f)
{
	int		pos;
	int		end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}

#define CACHE_THRESHOLD (1024*1024)
#define CACHE_ENABLE

/* really rough guesses */

#ifdef CACHE_ENABLE
#if MEMORYSIZE >= 64
#define MAX_CACHE (32*1024*1024)
#elif MEMORYSIZE >= 32
#define MAX_CACHE (20*1024*1024)
#endif
#endif

#ifndef MAX_CACHE
#define MAX_CACHE 0
#endif

static int cache_left = MAX_CACHE;

int Sys_FileOpenRead (char *path, int *hndl)
{
    FILE	*f;
    int		i;
	
    i = findhandle ();

    f = fopen(path, "rb");
    if (!f)
    {
        *hndl = -1;
        return -1;
    }
    sys_handles[i].f = f;
    *hndl = i;

    //rb->splashf(HZ*2, "Allocating handle %d to %s", i, path);

    int len = sys_handles[i].full_len = Qfilelength(f);

    /* cache in memory? */
    if(len > CACHE_THRESHOLD && cache_left > 0)
    {
        /* cache all or part of it */
        int cachelen = (cache_left > len) ? len : cache_left;

        if((sys_handles[i].buf = malloc(cachelen)))
        {
            cache_left -= cachelen;

            /* fill in cache */
            printf("Please wait... caching %d KB of large file", cachelen / 1024);
            if(fread(sys_handles[i].buf, 1, cachelen, f) == cachelen)
            {
                sys_handles[i].resident_len = cachelen;
                fseek(f, 0, SEEK_SET);

                printf("Success!");
            }
        }
    }

    return len;
}

int Sys_FileOpenWrite (char *path)
{
	FILE	*f;
	int		i;
	
	i = findhandle ();

	f = fopen(path, "wb");
	if (!f)
		Sys_Error ("Error opening %s: %s", path,strerror(errno));
	sys_handles[i].f = f;
	
	return i;
}

void Sys_FileClose (int handle)
{
	if ( handle >= 0 ) {
            //rb->splashf(HZ, "Close handle %d", handle);
		fclose (sys_handles[handle].f);

                cache_left += sys_handles[handle].resident_len;

                // clear all fields so we can safely reuse
                memset(sys_handles + handle, 0, sizeof(sys_handles[0]));
	}
}

void Sys_FileSeek (int handle, int position)
{
	if ( handle >= 0 ) {
		fseek (sys_handles[handle].f, position, SEEK_SET);
	}
}

int Sys_FileRead (int handle, void *dst, int count)
{
	char *data;
	int size, done;

	size = 0;
	if ( handle >= 0 ) {
            FILE *f = sys_handles[handle].f;

            data = dst;
            
            /* partially or fully in memory */
            int pos = ftell(f);
            if(pos < sys_handles[handle].resident_len)
            {
                int memleft = sys_handles[handle].resident_len - pos;
                int memlen = MIN(count, memleft);

                memcpy(data, sys_handles[handle].buf + pos, memlen);
                data += memlen;
                count -= memlen;
                size += memlen;

                fseek(f, memlen, SEEK_CUR);
            }

            while ( count > 0 ) {
                done = fread (data, 1, count, sys_handles[handle].f);
                if ( done == 0 ) {
                    break;
                }
                else if(done < 0)
                {
                    Sys_Error("stream error %d, file is %d = %d", done, handle, sys_handles[handle]);
                }
                data += done;
                count -= done;
                size += done;
            }
	}
	return size;
		
}

int Sys_FileWrite (int handle, void *src, int count)
{
	char *data;
	int size, done;

	size = 0;
	if ( handle >= 0 ) {
		data = src;
		while ( count > 0 ) {
			done = fread (data, 1, count, sys_handles[handle].f);
			if ( done == 0 ) {
				break;
			}
			data += done;
			count -= done;
			size += done;
		}
	}
	return size;
}

int	Sys_FileTime (char *path)
{
	FILE	*f;
	
	f = fopen(path, "rb");
	if (f)
	{
		fclose(f);
		return 1;
	}
	
	return -1;
}

void Sys_mkdir (char *path)
{
#ifdef __WIN32__
    mkdir (path);
#else
    mkdir (path);
#endif
}

void Sys_DebugLog(char *file, char *fmt, ...)
{
    va_list argptr; 
    static char data[1024];
    FILE *fp;
    
    va_start(argptr, fmt);
    vsprintf(data, fmt, argptr);
    va_end(argptr);
    fp = fopen(file, "a");
    fwrite(data, strlen(data), 1, fp);
    fclose(fp);
}

double Sys_FloatTime (void)
{
	static int starttime = 0;

	if ( ! starttime )
		starttime = *rb->current_tick;

	return (*rb->current_tick - starttime) / ((double)HZ);
}

// =======================================================================
// Sleeps for microseconds
// =======================================================================

static volatile int oktogo;

void alarm_handler(int x)
{
	oktogo=1;
}

byte *Sys_ZoneBase (int *size)
{

	char *QUAKEOPT = getenv("QUAKEOPT");

	*size = 0xc00000;
	if (QUAKEOPT)
	{
		while (*QUAKEOPT)
			if (tolower(*QUAKEOPT++) == 'm')
			{
				*size = atof(QUAKEOPT) * 1024*1024;
				break;
			}
	}
	return malloc (*size);

}

void Sys_LineRefresh(void)
{
}

void Sys_Sleep(void)
{
	SDL_Delay(1);
}

void floating_point_exception_handler(int whatever)
{
//	Sys_Warn("floating point exception\n");
}

void moncontrol(int x)
{
}

int my_main (int c, char **v)
{
	double		time, oldtime, newtime;
	quakeparms_t parms;
	extern int vcrFile;
	extern int recording;
	static int frame;

	moncontrol(0);

//	signal(SIGFPE, floating_point_exception_handler);

        //rb->splash(0, "quake 1");
        
	parms.memsize = 8*1024*1024;
	parms.membase = malloc (parms.memsize);
	parms.basedir = basedir;
	parms.cachedir = cachedir;

	COM_InitArgv(c, v);
	parms.argc = com_argc;
	parms.argv = com_argv;

	Sys_Init();
        //rb->splash(0, "quake 2");

    Host_Init(&parms);
    //rb->splash(0, "quake 3");
        
	//Cvar_RegisterVariable (&sys_nostdout);
    //rb->splash(0, "quake 4");

    oldtime = Sys_FloatTime () - 0.1;
    while (1)
    {
// find time spent rendering last frame
        newtime = Sys_FloatTime ();
        time = newtime - oldtime;

        if (cls.state == ca_dedicated)
        {   // play vcrfiles at max speed
            if (time < sys_ticrate.value && (vcrFile == -1 || recording) )
            {
                rb->yield();
                continue;       // not time to run a server only tic yet
            }
            time = sys_ticrate.value;
        }

        if (time > sys_ticrate.value*2)
            oldtime = newtime;
        else
            oldtime += time;

        if (++frame > 10)
            moncontrol(1);      // profile only while we do each Quake frame
        Host_Frame (time);
        moncontrol(0);

// graphic debugging aids
        if (sys_linerefresh.value)
            Sys_LineRefresh ();

        rb->yield();
    }

}


/*
================
Sys_MakeCodeWriteable
================
*/
void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length)
{

    Sys_Error("Protection change failed\n");

}

