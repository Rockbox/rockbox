/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#ifdef ROCKBOX
#include "plugin.h"
#include "../../pdbox.h"
#endif

/*  scheduling stuff  */

#include "m_pd.h"
#include "m_imp.h"
#include "s_stuff.h"

    /* LATER consider making this variable.  It's now the LCM of all sample
    rates we expect to see: 32000, 44100, 48000, 88200, 96000. */
#define TIMEUNITPERSEC (32.*441000.)


/* T.Grill - enable PD global thread locking - sys_lock, sys_unlock, sys_trylock functions */
#ifndef ROCKBOX
#define THREAD_LOCKING
#include "pthread.h"
#endif


static int sys_quit;
#ifndef ROCKBOX
static
#endif
t_time sys_time;
#ifndef ROCKBOX
static
#endif
t_time sys_time_per_msec = TIMEUNITPERSEC / 1000.;

int sys_schedblocksize = DEFDACBLKSIZE;
int sys_usecsincelastsleep(void);
int sys_sleepgrain;

typedef void (*t_clockmethod)(void *client);

struct _clock
{
    t_time c_settime;
    void *c_owner;
    t_clockmethod c_fn;
    struct _clock *c_next;
};

t_clock *clock_setlist;

#ifdef UNIX
#include <unistd.h>
#endif

t_clock *clock_new(void *owner, t_method fn)
{
    t_clock *x = (t_clock *)getbytes(sizeof *x);
    x->c_settime = -1;
    x->c_owner = owner;
    x->c_fn = (t_clockmethod)fn;
    x->c_next = 0;
    return (x);
}

void clock_unset(t_clock *x)
{
    if (x->c_settime >= 0)
    {
    	if (x == clock_setlist) clock_setlist = x->c_next;
    	else
    	{
    	    t_clock *x2 = clock_setlist;
    	    while (x2->c_next != x) x2 = x2->c_next;
    	    x2->c_next = x->c_next;
    	}
    	x->c_settime = -1;
    }
}

    /* set the clock to call back at an absolute system time */
void clock_set(t_clock *x, t_time setticks)
{
    if (setticks < sys_time) setticks = sys_time;
    clock_unset(x);
    x->c_settime = setticks;
    if (clock_setlist && clock_setlist->c_settime <= setticks)
    {
    	t_clock *cbefore, *cafter;
    	for (cbefore = clock_setlist, cafter = clock_setlist->c_next;
    	    cbefore; cbefore = cafter, cafter = cbefore->c_next)
    	{
    	    if (!cafter || cafter->c_settime > setticks)
    	    {
    	    	cbefore->c_next = x;
    	    	x->c_next = cafter;
    	    	return;
    	    }
    	}
    }
    else x->c_next = clock_setlist, clock_setlist = x;
}

    /* set the clock to call back after a delay in msec */
void clock_delay(t_clock *x, t_time delaytime)
{
    clock_set(x, sys_time + sys_time_per_msec * delaytime);
}

    /* get current logical time.  We don't specify what units this is in;
    use clock_gettimesince() to measure intervals from time of this call. 
    This was previously, incorrectly named "clock_getsystime"; the old
    name is aliased to the new one in m_pd.h. */
t_time clock_getlogicaltime( void)
{
    return (sys_time);
}
    /* OBSOLETE NAME */
t_time clock_getsystime( void) { return (sys_time); }

    /* elapsed time in milliseconds since the given system time */
t_time clock_gettimesince(t_time prevsystime)
{
    return ((sys_time - prevsystime)/sys_time_per_msec);
}

    /* what value the system clock will have after a delay */
t_time clock_getsystimeafter(t_time delaytime)
{
    return (sys_time + sys_time_per_msec * delaytime);
}

void clock_free(t_clock *x)
{
    clock_unset(x);
    freebytes(x, sizeof *x);
}


/* the following routines maintain a real-execution-time histogram of the
various phases of real-time execution. */

static int sys_bin[] = {0, 2, 5, 10, 20, 30, 50, 100, 1000};
#define NBIN (sizeof(sys_bin)/sizeof(*sys_bin))
#define NHIST 10
static int sys_histogram[NHIST][NBIN];
#ifndef ROCKBOX
static t_time sys_histtime;
#endif
static int sched_diddsp, sched_didpoll, sched_didnothing;

#ifndef ROCKBOX
static void sys_clearhist( void)
{
    unsigned int i, j;
    for (i = 0; i < NHIST; i++)
    	for (j = 0; j < NBIN; j++) sys_histogram[i][j] = 0;
    sys_histtime = sys_getrealtime();
    sched_diddsp = sched_didpoll = sched_didnothing = 0;
}
#endif

void sys_printhist( void)
{
    unsigned int i, j;
    for (i = 0; i < NHIST; i++)
    {
    	int doit = 0;
    	for (j = 0; j < NBIN; j++) if (sys_histogram[i][j]) doit = 1;
    	if (doit)
    	{
    	    post("%2d %8d %8d %8d %8d %8d %8d %8d %8d", i,
    	    	sys_histogram[i][0],
    	    	sys_histogram[i][1],
    	    	sys_histogram[i][2],
    	    	sys_histogram[i][3],
    	    	sys_histogram[i][4],
    	    	sys_histogram[i][5],
    	    	sys_histogram[i][6],
    	    	sys_histogram[i][7]);
    	}
    }
    post("dsp %d, pollgui %d, nothing %d",
    	sched_diddsp, sched_didpoll, sched_didnothing);
}

#ifndef ROCKBOX
static int sys_histphase;
#endif

int sys_addhist(int phase)
{
#ifdef ROCKBOX
    (void) phase;
#endif
#ifndef FIXEDPOINT
    int i, j, phasewas = sys_histphase;
    t_time newtime = sys_getrealtime();
    int msec = (newtime - sys_histtime) * 1000.;
    for (j = NBIN-1; j >= 0; j--)
    {
    	if (msec >= sys_bin[j]) 
    	{
    	    sys_histogram[phasewas][j]++;
    	    break;
    	}
    }
    sys_histtime = newtime;
    sys_histphase = phase;
    return (phasewas);
#else
    return 0;
#endif
}

#define NRESYNC 20

typedef struct _resync
{
    int r_ntick;
    int r_error;
} t_resync;

static int oss_resyncphase = 0;
static int oss_nresync = 0;
static t_resync oss_resync[NRESYNC];


static char *(oss_errornames[]) = {
"unknown",
"ADC blocked",
"DAC blocked",
"A/D/A sync",
"data late"
};

void glob_audiostatus(void)
{
#ifdef ROCKBOX
    int nresync, nresyncphase, i;
#else
    int dev, nresync, nresyncphase, i;
#endif
    nresync = (oss_nresync >= NRESYNC ? NRESYNC : oss_nresync);
    nresyncphase = oss_resyncphase - 1;
    post("audio I/O error history:");
    post("seconds ago\terror type");
    for (i = 0; i < nresync; i++)
    {
	int errtype;
	if (nresyncphase < 0)
	    nresyncphase += NRESYNC;
    	errtype = oss_resync[nresyncphase].r_error;
    	if (errtype < 0 || errtype > 4)
	    errtype = 0;
	
	post("%9.2f\t%s",
	    (sched_diddsp - oss_resync[nresyncphase].r_ntick)
	    	* ((double)sys_schedblocksize) / sys_dacsr,
	    oss_errornames[errtype]);
    	nresyncphase--;
    }
}

static int sched_diored;
static int sched_dioredtime;
static int sched_meterson;

void sys_log_error(int type)
{
    oss_resync[oss_resyncphase].r_ntick = sched_diddsp;
    oss_resync[oss_resyncphase].r_error = type;
    oss_nresync++;
    if (++oss_resyncphase == NRESYNC) oss_resyncphase = 0;
    if (type != ERR_NOTHING && !sched_diored &&
    	(sched_diddsp >= sched_dioredtime))
    {
#ifndef ROCKBOX
    	sys_vgui("pdtk_pd_dio 1\n");
#endif
	sched_diored = 1;
    }
    sched_dioredtime =
    	sched_diddsp + (int)(sys_dacsr /(double)sys_schedblocksize);
}

static int sched_lastinclip, sched_lastoutclip,
    sched_lastindb, sched_lastoutdb;

void glob_ping(t_pd *dummy);

#ifndef ROCKBOX
static void sched_pollformeters( void)
{
    int inclip, outclip, indb, outdb;
    static int sched_nextmeterpolltime, sched_nextpingtime;

    	/* if there's no GUI but we're running in "realtime", here is
	where we arrange to ping the watchdog every 2 seconds. */
#ifdef __linux__
    if (sys_nogui && sys_hipriority && (sched_diddsp - sched_nextpingtime > 0))
    {
    	glob_ping(0);
	    /* ping every 2 seconds */
	sched_nextpingtime = sched_diddsp +
	    (2* sys_dacsr) /(int)sys_schedblocksize;
    }
#endif

    if (sched_diddsp - sched_nextmeterpolltime < 0)
    	return;
    if (sched_diored && (sched_diddsp - sched_dioredtime > 0))
    {
    	sys_vgui("pdtk_pd_dio 0\n");
	sched_diored = 0;
    }
    if (sched_meterson)
    {
    	float inmax, outmax;
    	sys_getmeters(&inmax, &outmax);
	indb = 0.5 + rmstodb(inmax);
	outdb = 0.5 + rmstodb(outmax);
    	inclip = (inmax > 0.999);
	outclip = (outmax >= 1.0);
    }
    else
    {
    	indb = outdb = 0;
	inclip = outclip = 0;
    }
    if (inclip != sched_lastinclip || outclip != sched_lastoutclip
    	|| indb != sched_lastindb || outdb != sched_lastoutdb)
    {
    	sys_vgui("pdtk_pd_meters %d %d %d %d\n", indb, outdb, inclip, outclip);
	sched_lastinclip = inclip;
	sched_lastoutclip = outclip;
	sched_lastindb = indb;
	sched_lastoutdb = outdb;
    }
    sched_nextmeterpolltime =
    	sched_diddsp + (int)(sys_dacsr /(double)sys_schedblocksize);
}
#endif /* ROCKBOX */

void glob_meters(void *dummy, float f)
{
#ifdef ROCKBOX
    (void) dummy;
#endif
    if (f == 0)
    	sys_getmeters(0, 0);
    sched_meterson = (f != 0);
    sched_lastinclip = sched_lastoutclip = sched_lastindb = sched_lastoutdb =
	-1;
}

#if 0
void glob_foo(void *dummy, t_symbol *s, int argc, t_atom *argv)
{
    if (argc) sys_clearhist();
    else sys_printhist();
}
#endif

void dsp_tick(void);

static int sched_usedacs = 1;
static t_time sched_referencerealtime, sched_referencelogicaltime;
#ifndef ROCKBOX
static
#endif
t_time sys_time_per_dsp_tick;

void sched_set_using_dacs(int flag)
{
    sched_usedacs = flag;
    if (!flag)
    {
    	sched_referencerealtime = sys_getrealtime();
	sched_referencelogicaltime = clock_getlogicaltime();
#ifndef ROCKBOX
        post("schedsetuding");
#endif
    }
    sys_time_per_dsp_tick = (TIMEUNITPERSEC) *
    	((double)sys_schedblocksize) / sys_dacsr;
/*
#ifdef SIMULATOR
printf("%f\n%f\n%f\n%f\n", (double)sys_time_per_dsp_tick, (double)TIMEUNITPERSEC, (double) sys_schedblocksize, (double)sys_dacsr);
#endif
*/
}

    /* take the scheduler forward one DSP tick, also handling clock timeouts */
#ifndef ROCKBOX
static
#endif
void sched_tick(t_time next_sys_time)
{
    int countdown = 5000;
    while (clock_setlist && clock_setlist->c_settime < next_sys_time)
    {
    	t_clock *c = clock_setlist;
    	sys_time = c->c_settime;
    	clock_unset(clock_setlist);
	outlet_setstacklim();
    	(*c->c_fn)(c->c_owner);
	if (!countdown--)
	{
	    countdown = 5000;
#ifndef ROCKBOX
	    sys_pollgui();
#endif
	}
	if (sys_quit)
	    return;
    }
    sys_time = next_sys_time;
    dsp_tick();
    sched_diddsp++;
}

/*
Here is Pd's "main loop."  This routine dispatches clock timeouts and DSP
"ticks" deterministically, and polls for input from MIDI and the GUI.  If
we're left idle we also poll for graphics updates; but these are considered
lower priority than the rest.

The time source is normally the audio I/O subsystem via the "sys_send_dacs()"
call.  This call returns true if samples were transferred; false means that
the audio I/O system is still busy with previous transfers.
*/

void sys_pollmidiqueue( void);
void sys_initmidiqueue( void);

#ifndef ROCKBOX
int m_scheduler_pda( void)
{
    int idlecount = 0;

    sys_time_per_dsp_tick = (TIMEUNITPERSEC) *
    	((double)sys_schedblocksize) / sys_dacsr;


    sys_clearhist();
    if (sys_sleepgrain < 1000)
    	sys_sleepgrain = sys_schedadvance/4;
    if (sys_sleepgrain < 100)
    	sys_sleepgrain = 100;
    else if (sys_sleepgrain > 5000)
    	sys_sleepgrain = 5000;

    sys_initmidiqueue();

    while (!sys_quit)
    {

    	int didsomething = 0;

    	int timeforward;

      	sys_addhist(0);

    waitfortick:
    	if (sched_usedacs)
    	{
    	    timeforward = sys_send_dacs();
            sys_pollgui();
        }
        else {
    	    if ((sys_getrealtime() - sched_referencerealtime)
	    	> (t_time)clock_gettimesince(sched_referencelogicaltime)*1000)
            	    timeforward = SENDDACS_YES;
            else  timeforward = SENDDACS_NO;
            if (timeforward == SENDDACS_YES)
                sys_microsleep(sys_sleepgrain);
        }
        if (timeforward != SENDDACS_NO) {
            sched_tick(sys_time + sys_time_per_dsp_tick);
        }
    }
    return 0;
}


int m_scheduler( void)
{
    int idlecount = 0;
    sys_time_per_dsp_tick = (TIMEUNITPERSEC) *
    	((double)sys_schedblocksize) / sys_dacsr;

#ifdef THREAD_LOCKING
    /* T.Grill - lock mutex */
	sys_lock();
#endif

    sys_clearhist();
    if (sys_sleepgrain < 1000)
    	sys_sleepgrain = sys_schedadvance/4;
    if (sys_sleepgrain < 100)
    	sys_sleepgrain = 100;
    else if (sys_sleepgrain > 5000)
    	sys_sleepgrain = 5000;
    sys_initmidiqueue();
    while (!sys_quit)
    {
    	int didsomething = 0;
    	int timeforward;

    	sys_addhist(0);
    waitfortick:
    	if (sched_usedacs)
    	{
    	    timeforward = sys_send_dacs();

    		/* if dacs remain "idle" for 1 sec, they're hung up. */
    	    if (timeforward != 0)
	    	idlecount = 0;
    	    else
    	    {
    		idlecount++;
    		if (!(idlecount & 31))
    		{
    	    	    static t_time idletime;
    	    	    	/* on 32nd idle, start a clock watch;  every
    	    	    	32 ensuing idles, check it */
    	    	    if (idlecount == 32)
    	    	    	idletime = sys_getrealtime();
    	    	    else if (sys_getrealtime() - idletime > 1.)
    	    	    {
    	    		post("audio I/O stuck... closing audio\n");
    	    		sys_close_audio();
			sched_set_using_dacs(0);
			goto waitfortick;
    	    	    }
    	    	}
    	    }
    	}
    	else
    	{
    	    if (1000. * (sys_getrealtime() - sched_referencerealtime)
	    	> clock_gettimesince(sched_referencelogicaltime))
            	    timeforward = SENDDACS_YES;
    	    else timeforward = SENDDACS_NO;
    	}
    	sys_setmiditimediff(0, 1e-6 * sys_schedadvance);
    	sys_addhist(1);
    	if (timeforward != SENDDACS_NO)
	    sched_tick(sys_time + sys_time_per_dsp_tick);
    	if (timeforward == SENDDACS_YES)
	    didsomething = 1;

    	sys_addhist(2);
	//	sys_pollmidiqueue();
    	if (sys_pollgui())
	{
	    if (!didsomething)
	    	sched_didpoll++;
	    didsomething = 1;
	}
    	sys_addhist(3);
	    /* test for idle; if so, do graphics updates. */
    	if (!didsomething)
    	{
    	    sched_pollformeters();
    	    sys_reportidle();

#ifdef THREAD_LOCKING
            /* T.Grill - enter idle phase -> unlock thread lock */
            sys_unlock();
#endif
    	    if (timeforward != SENDDACS_SLEPT)
    	    	sys_microsleep(sys_sleepgrain);
#ifdef THREAD_LOCKING
            /* T.Grill - leave idle phase -> lock thread lock */
	    sys_lock();
#endif

    	    sys_addhist(5);
    	    sched_didnothing++;

    	}
    }

#ifdef THREAD_LOCKING
    /* T.Grill - done */
    sys_unlock();
#endif

    return (0);
}


/* ------------ thread locking ------------------- */
/* added by Thomas Grill */

#ifdef THREAD_LOCKING
static pthread_mutex_t sys_mutex = PTHREAD_MUTEX_INITIALIZER;

void sys_lock(void)
{
    pthread_mutex_lock(&sys_mutex);
}

void sys_unlock(void)
{
    pthread_mutex_unlock(&sys_mutex);
}

int sys_trylock(void)
{
    return pthread_mutex_trylock(&sys_mutex);
}

#else

void sys_lock(void) {}
void sys_unlock(void) {}
int sys_trylock(void) { return 0; }

#endif


/* ------------ soft quit ------------------- */
/* added by Thomas Grill - 
	just set the quit flag for the scheduler loop
	this is useful for applications using the PD shared library to signal the scheduler to terminate
*/

void sys_exit(void)
{
	sys_quit = 1;
}

#endif /* ROCKBOX */
