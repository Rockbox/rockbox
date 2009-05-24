/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

void post(char *fmt, ...)
{
    va_list ap;
    t_int arg[8];
    int i;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    putc('\n', stderr);
}

void startpost(char *fmt, ...)
{
    va_list ap;
    t_int arg[8];
    int i;
    va_start(ap, fmt);
    
    for (i = 0 ; i < 8; i++) arg[i] = va_arg(ap, t_int);
    va_end(ap);
    fprintf(stderr, fmt, arg[0], arg[1], arg[2], arg[3],
    	arg[4], arg[5], arg[6], arg[7]);
}

void poststring(char *s)
{
    fprintf(stderr, " %s", s);
}

void postatom(int argc, t_atom *argv)
{
    int i;
    for (i = 0; i < argc; i++)
    {
    	char buf[80];
    	atom_string(argv+i, buf, 80);
    	poststring(buf);
    }
}

void postfloat(float f)
{
    char buf[80];
    t_atom a;
    SETFLOAT(&a, f);
    postatom(1, &a);
}

void endpost(void)
{
    fprintf(stderr, "\n");
}

void error(char *fmt, ...)
{
    va_list ap;
    t_int arg[8];
    int i;
    va_start(ap, fmt);
    fprintf(stderr, "error: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    putc('\n', stderr);
}

    /* here's the good way to log errors -- keep a pointer to the
    offending or offended object around so the user can search for it
    later. */

static void *error_object;
static char error_string[256];
void canvas_finderror(void *object);

void pd_error(void *object, char *fmt, ...)
{
    va_list ap;
    t_int arg[8];
    int i;
    static int saidit = 0;
    va_start(ap, fmt);
    vsprintf(error_string, fmt, ap);
    va_end(ap);
    fprintf(stderr, "error: %s\n", error_string);
    error_object = object;
    if (!saidit)
    {
    	post("... you might be able to track this down from the Find menu.");
    	saidit = 1;
    }
}

void glob_finderror(t_pd *dummy)
{
    if (!error_object)
    	post("no findable error yet.");
    else
    {
    	post("last trackable error:");
    	post("%s", error_string);
    	canvas_finderror(error_object);
    }
}

void bug(char *fmt, ...)
{
    va_list ap;
    t_int arg[8];
    int i;
    va_start(ap, fmt);
    
    for (i = 0 ; i < 8; i++) arg[i] = va_arg(ap, t_int);
    va_end(ap);
    fprintf(stderr, "Consistency check failed: ");
    fprintf(stderr, fmt, arg[0], arg[1], arg[2], arg[3],
    	arg[4], arg[5], arg[6], arg[7]);
    putc('\n', stderr);
}

    /* this isn't worked out yet. */
static char *errobject;
static char *errstring;

void sys_logerror(char *object, char *s)
{
    errobject = object;
    errstring = s;
}

void sys_unixerror(char *object)
{
    errobject = object;
    errstring = strerror(errno);
}

void sys_ouch(void)
{
    if (*errobject) error("%s: %s", errobject, errstring);
    else error("%s", errstring);
}

