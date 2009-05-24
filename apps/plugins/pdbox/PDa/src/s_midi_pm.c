/* Copyright (c) 1997-2003 Guenter Geiger, Miller Puckette, Larry Troxler,
* Winfried Ritsch, Karl MacMillan, and others.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.

   this file calls portmidi to do MIDI I/O for MSW and Mac OSX. 

*/

#include "m_pd.h"
#include "s_stuff.h"
#include <stdio.h>
#ifdef UNIX
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "portaudio.h"
#include "portmidi.h"
#include "porttime.h"
#include "pminternal.h"

static PmStream *mac_midiindevlist[MAXMIDIINDEV];
static PmStream *mac_midioutdevlist[MAXMIDIOUTDEV];
static int mac_nmidiindev;
static int mac_nmidioutdev;

void sys_open_midi(int nmidiin, int *midiinvec,
    int nmidiout, int *midioutvec)
{
    int i = 0;
    int n = 0;
    PmError err;
    
    Pt_Start(1, 0, 0); /* start a timer with millisecond accuracy */
    mac_nmidiindev = 0;

    	/* protect the unwary from having MIDI inputs open; they're
	bad news if you close Pd's terminal window.  see sys_nmidiin
	in s_main.c too. */
#ifdef MSW
    if (nmidiin)
    {
    	post(
	 "midi input enabled; warning, don't close the DOS window directly!");
    }
    else post("not using MIDI input (use 'pd -midiindev 1' to override)");
#endif

    for (i = 0; i < nmidiin; i++)
    {
    	if (midiinvec[i] == DEFMIDIDEV)
	    midiinvec[i] = Pm_GetDefaultInputDeviceID();
    	err = Pm_OpenInput(&mac_midiindevlist[mac_nmidiindev], midiinvec[i],
	    NULL, 100, NULL, NULL, NULL);
    	if (err)
            post("could not open midi input device number %d: %s",
	    	midiinvec[i], Pm_GetErrorText(err));
	else
	{
	    if (sys_verbose)
    	    	post("Midi Input opened.\n");
	    mac_nmidiindev++;
    	}
    }

    mac_nmidioutdev = 0;
    for (i = 0; i < nmidiout; i++)
    {
    	if (midioutvec[i] == DEFMIDIDEV)
	    midioutvec[i] = Pm_GetDefaultOutputDeviceID();
    	err = Pm_OpenOutput(&mac_midioutdevlist[mac_nmidioutdev], midioutvec[i],
    	     NULL, 0, NULL, NULL, 0);
    	if (err)
            post("could not open midi output device number %d: %s",
	    	midioutvec[i], Pm_GetErrorText(err));
	else
	{
	    if (sys_verbose)
    	    	post("Midi Output opened.\n");
	    mac_nmidioutdev++;
    	}
    }
}

void sys_close_midi( void)
{
    int i;
    for (i = 0; i < mac_nmidiindev; i++)
    	Pm_Close(mac_midiindevlist[mac_nmidiindev]);
    mac_nmidiindev = 0;
    for (i = 0; i < mac_nmidioutdev; i++)
    	Pm_Close(mac_midioutdevlist[mac_nmidioutdev]);
    mac_nmidioutdev = 0; 
}

void sys_putmidimess(int portno, int a, int b, int c)
{
    PmEvent buffer;
        fprintf(stderr, "put 1 msg %d %d\n", portno, mac_nmidioutdev);
    if (portno >= 0 && portno < mac_nmidioutdev)
    {
    	buffer.message = Pm_Message(a, b, c);
    	buffer.timestamp = 0;
        fprintf(stderr, "put msg\n");
        Pm_Write(mac_midioutdevlist[portno], &buffer, 1);
    }
}

void sys_putmidibyte(int portno, int byte)
{
    post("sorry, no byte-by-byte MIDI output implemented in MAC OSX");
}

void sys_poll_midi(void)
{
    int i, nmess;
    PmEvent buffer;
    for (i = 0; i < mac_nmidiindev; i++)
    {
    	int nmess = Pm_Read(mac_midiindevlist[i], &buffer, 1);
    	if (nmess > 0)
	{
            int status = Pm_MessageStatus(buffer.message);
            int data1  = Pm_MessageData1(buffer.message);
            int data2  = Pm_MessageData2(buffer.message);
            int msgtype = (status >> 4) - 8;
	    switch (msgtype)
    	    {
    	    case 0: 
    	    case 1: 
    	    case 2:
    	    case 3:
    	    case 6:
		sys_midibytein(i, status);
		sys_midibytein(i, data1);
		sys_midibytein(i, data2);
		break; 
    	    case 4:
    	    case 5:
		sys_midibytein(i, status);
		sys_midibytein(i, data1);
		break;
	    case 7:
		sys_midibytein(i, status);
		break; 
	    }
    	}
    }
}

void sys_listmididevs(void)     /* lifted from pa_devs.c in portaudio */
{
    int      i,j;
    for (i = 0; i < Pm_CountDevices(); i++)
    {
        const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
        printf("%d: %s, %s", i, info->interf, info->name);
        if (info->input) printf(" (input)");
        if (info->output) printf(" (output)");
        printf("\n");
    }
}

