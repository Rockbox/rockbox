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
#include "system.h"

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
#define CE_LO  __clear_bit_constant(3, PBDRL_ADDR)
#define CE_HI  __set_bit_constant(3, PBDRL_ADDR)
#define CL_LO  __clear_bit_constant(1, PBDRL_ADDR)
#define CL_HI  __set_bit_constant(1, PBDRL_ADDR)
#define DO     (PBDR & PB4)
#define DI_LO  __clear_bit_constant(0, PBDRL_ADDR)
#define DI_HI  __set_bit_constant(0, PBDRL_ADDR)

#define START __set_mask_constant((PB3 | PB1), PBDRL_ADDR)

/* delay loop */
#define DELAY   do { int _x; for(_x=0;_x<10;_x++);} while (0)

static int fmstatus = 0;

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

void fmradio_set_status(int status)
{
    fmstatus = status;
}

int fmradio_get_status(void)
{
    return fmstatus;
}

#endif
