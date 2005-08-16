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

#include <windows.h>
#include <stdio.h>

char debugmembuf[200];

void debug( const char *message )
{
    static int debugger = -1;

    if (debugger == -1)
    {
        HINSTANCE hInst = LoadLibrary("kernel32.dll");
        debugger = 0;

        if (hInst != NULL)
        {
            FARPROC pIsDebuggerPresent = GetProcAddress(hInst, "IsDebuggerPresent");
            if (pIsDebuggerPresent != NULL)
                debugger = pIsDebuggerPresent();
        }
    }

    if (debugger)
        OutputDebugString (message);
    else
        printf("%s", message);
}

void debugf(const char *fmt, ...)
{
    va_list ap;

    va_start( ap, fmt );
    wvsprintf( debugmembuf, fmt, ap );
    va_end( ap );
    debug ( debugmembuf );
}

void Logf(char *fmt, ...)
{
    va_list ap;

    va_start( ap, fmt );
    wvsprintf( debugmembuf, fmt, ap );
    va_end( ap );
    debug ( debugmembuf );
}
