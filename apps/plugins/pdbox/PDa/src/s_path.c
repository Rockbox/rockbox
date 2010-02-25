/* Copyright (c) 1999 Guenter Geiger and others.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/*
 * This file implements the loader for linux, which includes
 * a little bit of path handling.
 *
 * Generalized by MSP to provide an open_via_path function
 * and lists of files for all purposes.
 */ 

/* #define DEBUG(x) x */
#define DEBUG(x)
void readsf_banana( void);    /* debugging */

#ifdef ROCKBOX

#include "plugin.h"
#include "../../pdbox.h"

#include "m_pd.h"
#include "m_imp.h"
#include "s_stuff.h"

#else /* ROCKBOX */
#include <stdlib.h>
#ifdef UNIX
#include <unistd.h>
#include <sys/stat.h>
#endif
#ifdef MSW
#include <io.h>
#endif

#include <string.h>
#include "m_pd.h"
#include "m_imp.h"
#include "s_stuff.h"
#include <stdio.h>
#include <fcntl.h>
#endif /* ROCKBOX */

static t_namelist *pd_path, *pd_helppath;

/* Utility functions */

/* copy until delimiter and return position after delimiter in string */
/* if it was the last substring, return NULL */

static const char* strtokcpy(char *to, const char *from, int delim)
{
    int size = 0;

    while (from[size] != (char)delim && from[size] != '\0')
    	size++;

    strncpy(to,from,size);
    to[size] = '\0';
    if (from[size] == '\0') return NULL;
    if (size) return from+size+1;
    else return NULL;
}

/* add a colon-separated list of names to a namelist */

#ifdef MSW
#define SEPARATOR ';'
#else
#define SEPARATOR ':'
#endif

static t_namelist *namelist_doappend(t_namelist *listwas, const char *s)
{
    t_namelist *nl = listwas, *rtn = listwas, *nl2;
    nl2 = (t_namelist *)(getbytes(sizeof(*nl)));
    nl2->nl_next = 0;
    nl2->nl_string = (char *)getbytes(strlen(s) + 1);
    strcpy(nl2->nl_string, s);
    sys_unbashfilename(nl2->nl_string, nl2->nl_string);
    if (!nl)
        nl = rtn = nl2;
    else
    {
        while (nl->nl_next)
    	    nl = nl->nl_next;
        nl->nl_next = nl2;
    }
    return (rtn);

}

t_namelist *namelist_append(t_namelist *listwas, const char *s)
{
    const char *npos;
    char temp[MAXPDSTRING];
    t_namelist *nl = listwas, *rtn = listwas;
    
#ifdef ROCKBOX
    (void) rtn;
#endif

    npos = s;
    do
    {
	npos = strtokcpy(temp, npos, SEPARATOR);
	if (! *temp) continue;
	nl = namelist_doappend(nl, temp);
    }
	while (npos);
    return (nl);
}

void namelist_free(t_namelist *listwas)
{
    t_namelist *nl, *nl2;
    for (nl = listwas; nl; nl = nl2)
    {
    	nl2 = nl->nl_next;
	t_freebytes(nl->nl_string, strlen(nl->nl_string) + 1);
	t_freebytes(nl, sizeof(*nl));
    }
}

void sys_addpath(const char *p)
{
     pd_path = namelist_append(pd_path, p);
}

void sys_addhelppath(const char *p)
{
     pd_helppath = namelist_append(pd_helppath, p);
}

#ifdef MSW
#define MSWOPENFLAG(bin) (bin ? _O_BINARY : _O_TEXT)
#else
#define MSWOPENFLAG(bin) 0
#endif

/* search for a file in a specified directory, then along the globally
defined search path, using ext as filename extension.  Exception:
if the 'name' starts with a slash or a letter, colon, and slash in MSW,
there is no search and instead we just try to open the file literally.  The
fd is returned, the directory ends up in the "dirresult" which must be at
least "size" bytes.  "nameresult" is set to point to the filename, which
ends up in the same buffer as dirresult. */

int open_via_path(const char *dir, const char *name, const char* ext,
    char *dirresult, char **nameresult, unsigned int size, int bin)
{
    t_namelist *nl, thislist;
    int fd = -1;
    char listbuf[MAXPDSTRING];

#ifdef ROCKBOX
    (void) bin;
#endif

    if (name[0] == '/' 
#ifdef MSW
    	|| (name[1] == ':' && name[2] == '/')
#endif
    	    )
    {
    	thislist.nl_next = 0;
    	thislist.nl_string = listbuf;
    	listbuf[0] = 0;
    }
    else
    {
    	thislist.nl_string = listbuf;
	thislist.nl_next = pd_path;
	strncpy(listbuf, dir, MAXPDSTRING);
	listbuf[MAXPDSTRING-1] = 0;
	sys_unbashfilename(listbuf, listbuf);
    }

    for (nl = &thislist; nl; nl = nl->nl_next)
    {
    	if (strlen(nl->nl_string) + strlen(name) + strlen(ext) + 4 >
	    size)
	    	continue;
	strcpy(dirresult, nl->nl_string);
	if (*dirresult && dirresult[strlen(dirresult)-1] != '/')
	       strcat(dirresult, "/");
	strcat(dirresult, name);
	strcat(dirresult, ext);
	sys_bashfilename(dirresult, dirresult);

	DEBUG(post("looking for %s",dirresult));
	    /* see if we can open the file for reading */
	if ((fd=open(dirresult,O_RDONLY | MSWOPENFLAG(bin))) >= 0)
	{
	    	/* in UNIX, further check that it's not a directory */
#ifdef UNIX
    	    struct stat statbuf;
	    int ok =  ((fstat(fd, &statbuf) >= 0) &&
	    	!S_ISDIR(statbuf.st_mode));
	    if (!ok)
	    {
	    	if (sys_verbose) post("tried %s; stat failed or directory",
		    dirresult);
	    	close (fd);
		fd = -1;
    	    }
	    else
#endif
    	    {
	    	char *slash;
		if (sys_verbose) post("tried %s and succeeded", dirresult);
		sys_unbashfilename(dirresult, dirresult);

		slash = strrchr(dirresult, '/');
		if (slash)
		{
		    *slash = 0;
		    *nameresult = slash + 1;
		}
		else *nameresult = dirresult;
		
	    	return (fd);
	    }
	}
	else
	{
	    if (sys_verbose) post("tried %s and failed", dirresult);
	}
    }
    *dirresult = 0;
    *nameresult = dirresult;
    return (-1);
}

static int do_open_via_helppath(const char *realname, t_namelist *listp)
{
    t_namelist *nl;
    int fd = -1;
    char dirresult[MAXPDSTRING], realdir[MAXPDSTRING];
    for (nl = listp; nl; nl = nl->nl_next)
    {
	strcpy(dirresult, nl->nl_string);
	strcpy(realdir, dirresult);
	if (*dirresult && dirresult[strlen(dirresult)-1] != '/')
	       strcat(dirresult, "/");
	strcat(dirresult, realname);
	sys_bashfilename(dirresult, dirresult);

	DEBUG(post("looking for %s",dirresult));
	    /* see if we can open the file for reading */
	if ((fd=open(dirresult,O_RDONLY | MSWOPENFLAG(0))) >= 0)
	{
	    	/* in UNIX, further check that it's not a directory */
#ifdef UNIX
    	    struct stat statbuf;
	    int ok =  ((fstat(fd, &statbuf) >= 0) &&
	    	!S_ISDIR(statbuf.st_mode));
	    if (!ok)
	    {
	    	if (sys_verbose) post("tried %s; stat failed or directory",
		    dirresult);
	    	close (fd);
		fd = -1;
    	    }
	    else
#endif
    	    {
#ifndef ROCKBOX
	    	char *slash;
#endif
		if (sys_verbose) post("tried %s and succeeded", dirresult);
		sys_unbashfilename(dirresult, dirresult);
		close (fd);
		glob_evalfile(0, gensym((char*)realname), gensym(realdir));
		return (1);
	    }
	}
	else
	{
	    if (sys_verbose) post("tried %s and failed", dirresult);
	}
    }
    return (0);
}

    /* LATER make this use open_via_path above.  We expect the ".pd"
    suffix here, even though we have to tear it back off for one of the
    search attempts. */
void open_via_helppath(const char *name, const char *dir)
{
#ifdef ROCKBOX
    t_namelist thislist, *listp;
#else /*ROCKBOX */
    t_namelist *nl, thislist, *listp;
    int fd = -1;
#endif /* ROCKBOX */
    char dirbuf2[MAXPDSTRING], realname[MAXPDSTRING];

    	/* if directory is supplied, put it at head of search list. */
    if (*dir)
    {
        thislist.nl_string = dirbuf2;
	thislist.nl_next = pd_helppath;
	strncpy(dirbuf2, dir, MAXPDSTRING);
	dirbuf2[MAXPDSTRING-1] = 0;
	sys_unbashfilename(dirbuf2, dirbuf2);
	listp = &thislist;
    }
    else listp = pd_helppath;
    	/* 1. "objectname-help.pd" */
    strncpy(realname, name, MAXPDSTRING-10);
    realname[MAXPDSTRING-10] = 0;
    if (strlen(realname) > 3 && !strcmp(realname+strlen(realname)-3, ".pd"))
    	realname[strlen(realname)-3] = 0;
    strcat(realname, "-help.pd");
    if (do_open_via_helppath(realname, listp))
    	return;
    	/* 2. "help-objectname.pd" */
    strcpy(realname, "help-");
    strncat(realname, name, MAXPDSTRING-10);
    realname[MAXPDSTRING-1] = 0;
    if (do_open_via_helppath(realname, listp))
    	return;
    	/* 3. "objectname.pd" */
    if (do_open_via_helppath(name, listp))
    	return;
    post("sorry, couldn't find help patch for \"%s\"", name);
    return;
}


/* Startup file reading for linux and MACOSX.  This should be replaced by
a better mechanism.  This should be integrated with the audio, MIDI, and
path dialog system. */

#ifdef UNIX

#define STARTUPNAME ".pdrc"
#define NUMARGS 1000

int sys_argparse(int argc, char **argv);

int sys_rcfile(void)
{
    FILE* file;
    int i;
    int k;
    int rcargc;
    char* rcargv[NUMARGS];
    char* buffer;
    char  fname[MAXPDSTRING], buf[1000], *home = getenv("HOME");

    /* parse a startup file */
    
    *fname = '\0'; 

    strncat(fname, home? home : ".", MAXPDSTRING-10);
    strcat(fname, "/");

    strcat(fname, STARTUPNAME);

    if (!(file = fopen(fname, "r")))
    	return 1;

    post("reading startup file: %s", fname);

    rcargv[0] = ".";	/* this no longer matters to sys_argparse() */

    for (i = 1; i < NUMARGS-1; i++)
    {
    	if (fscanf(file, "%999s", buf) < 0)
	    break;
	buf[1000] = 0;
	if (!(rcargv[i] = malloc(strlen(buf) + 1)))
	    return (1);
	strcpy(rcargv[i], buf);
    }
    if (i >= NUMARGS-1)
    	fprintf(stderr, "startup file too long; extra args dropped\n");
    rcargv[i] = 0;

    rcargc = i;

    /* parse the options */

    fclose(file);
    if (sys_verbose)
    {
    	if (rcargv)
	{
    	    post("startup args from RC file:");
	    for (i = 1; i < rcargc; i++)
	    	post("%s", rcargv[i]);
    	}
	else post("no RC file arguments found");
    }
    if (sys_argparse(rcargc, rcargv))
    {
    	post("error parsing RC arguments");
	return (1);
    }
    return (0);
}
#endif /* UNIX */

    /* start an audio settings dialog window */
void glob_start_path_dialog(t_pd *dummy, t_floatarg flongform)
{
#ifdef ROCKBOX
    (void) dummy;
    (void) flongform;
#else /* ROCKBOX */
    char buf[MAXPDSTRING];
    int i;
    t_namelist *nl;
    
    for (nl = pd_path, i = 0; nl && i < 10; nl = nl->nl_next, i++)
	sys_vgui("pd_set pd_path%d \"%s\"\n", i, nl->nl_string);
    for (; i < 10; i++)
	sys_vgui("pd_set pd_path%d \"\"\n", i);

    sprintf(buf, "pdtk_path_dialog %%s\n");
    gfxstub_new(&glob_pdobject, glob_start_path_dialog, buf);
#endif /* ROCKBOX */
}

    /* new values from dialog window */
void glob_path_dialog(t_pd *dummy, t_symbol *s, int argc, t_atom *argv)
{
    int i;

#ifdef ROCKBOX
    (void) dummy;
    (void) s;
#endif /* ROCKBOX */

    namelist_free(pd_path);
    pd_path = 0;
    for (i = 0; i < argc; i++)
    {
    	t_symbol *s = atom_getsymbolarg(i, argc, argv);
	if (*s->s_name)
    	    sys_addpath(s->s_name);
    }
}

