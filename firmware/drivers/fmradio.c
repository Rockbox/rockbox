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
#include "lcd.h"
#include "sh7034.h"
#include "kernel.h"
#include "thread.h"
#include "debug.h"

#ifdef HAVE_FMRADIO

/* Signals:
   DI (Data In)     - PB0 (doubles as data pin for the LCD)
   CL (Clock)       - PB1 (doubles as clock for the LCD)
   CE (Chip Enable) - PB3 (also chip select for the LCD, but active low)
   DO (Data Out)    - PB4
*/

#define PB0  0x0001
#define PB1  0x0002
#define PB3  0x0008
#define PB4  0x0010

/* cute little functions */
#define CE_LO  (PBDR &= ~PB3)
#define CE_HI  (PBDR |= PB3)
#define CL_LO  (PBDR &= ~PB1)
#define CL_HI  (PBDR |= PB1)
#define DO     (PBDR & PB4)
#define DI_LO  (PBDR &= ~PB0)
#define DI_HI  (PBDR |= PB0)

#define START (PBDR |= (PB3 | PB1))

/* delay loop */
#define DELAY   do { int _x; for(_x=0;_x<10;_x++);} while (0)

static struct mutex fmradio_mtx;

void fmradio_begin(void)
{
    mutex_lock(&fmradio_mtx);
}

void fmradio_end(void)
{
    mutex_unlock(&fmradio_mtx);
}

int fmradio_read(int addr)
{
    int i;
    int data = 0;

    START;
    
    /* First address bit */
    CL_LO;
    if(addr & 2)
        DI_HI;
    else
        DI_LO;
    DELAY;
    CL_HI;
    DELAY;

    /* Second address bit */
    CL_LO;
    if(addr & 1)
        DI_HI;
    else
        DI_LO;
    DELAY;
    CL_HI;
    DELAY;

    for(i = 0; i < 21;i++)
    {
        CL_LO;
        DELAY;
        data <<= 1;
        data |= (DO)?1:0;
        CL_HI;
        DELAY;
    }

    CE_LO;
    
    return data;
}

void fmradio_set(int addr, int data)
{
    int i;
    
    /* Include the address in the data */
    data |= addr << 21;

    START;

    for(i = 0; i < 23;i++)
    {
        CL_LO;
        DELAY;
        if(data & (1 << 22))
            DI_HI;
        else
            DI_LO;

        data <<= 1;
        CL_HI;
        DELAY;
    }

    CE_LO;
}

#endif
