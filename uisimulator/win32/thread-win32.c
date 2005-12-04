/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Felix Arends
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#include <time.h>
#include "thread-win32.h"
#include "kernel.h"
#include "debug.h"

HANDLE              lpThreads[256];
int                 nThreads = 0,
                    nPos = 0;
long                current_tick = 0;
CRITICAL_SECTION    CriticalSection;


void yield(void)
{
    LeaveCriticalSection(&CriticalSection);
    Sleep(1);
    EnterCriticalSection(&CriticalSection);
}

void sim_sleep(int ticks)
{
    LeaveCriticalSection(&CriticalSection);
    Sleep((1000/HZ) * ticks);
    EnterCriticalSection(&CriticalSection);
}

DWORD WINAPI runthread (LPVOID lpParameter)
{
    EnterCriticalSection(&CriticalSection);
    ((void(*)())lpParameter) ();
    LeaveCriticalSection(&CriticalSection);
    return 0;
}

int create_thread(void (*fp)(void), void* sp, int stk_size)
{
    DWORD dwThreadID;

    (void)sp;
    (void)stk_size;

    if (nThreads == 256)
        return -1;

    lpThreads[nThreads++] = CreateThread (NULL,
        0,
        runthread,
        fp,
        0,
        &dwThreadID);

    return 0;
}

void init_threads(void)
{
    InitializeCriticalSection(&CriticalSection);
    EnterCriticalSection(&CriticalSection);
}
