/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "thread.h"
#include "kernel.h"
#include "sh7034.h"
#include "debug.h"

void tick_start(unsigned int interval);

unsigned int s1[256];
unsigned int s2[256];

void t1(void);
void t2(void);

int main(void)
{
    char buf[40];
    char str[32];
    int i=0;

    /* Clear it all! */
    SSR1 &= ~(SCI_RDRF | SCI_ORER | SCI_PER | SCI_FER);

    /* This enables the serial Rx interrupt, to be able to exit into the
       debugger when you hit CTRL-C */
    SCR1 |= 0x40;
    SCR1 &= ~0x80;
    asm ("ldc\t%0,sr" : : "r"(0<<4));

    debugf("OK. Let's go\n");

    tick_start(40);

    create_thread(t1, s1, 1024);
    create_thread(t2, s2, 1024);

    while(1)
    {
	debugf("t0\n");
	sleep(100);
    }
}

void t1(void)
{
    while(1)
    {
	debugf("t1\n");
	sleep(200);
    }
}

void t2(void)
{
    while(1)
    {
	debugf("t2\n");
	sleep(300);
    }
}
