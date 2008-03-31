/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2007 by Greg White
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/
#include "config.h"
#include "cpu.h"
#include <stdbool.h>
#include "kernel.h"
#include "system.h"
#include "logf.h"
#include "debug.h"
#include "string.h"

#define SC606_REG_A      0
#define SC606_REG_B      1
#define SC606_REG_C      2
#define SC606_REG_CONF   3

#define SC606_LED_A1     (1 << 0)
#define SC606_LED_A2     (1 << 1)
#define SC606_LED_B1     (1 << 2)
#define SC606_LED_B2     (1 << 3)
#define SC606_LED_C1     (1 << 4)
#define SC606_LED_C2     (1 << 5)

#define SC606_LOW_FREQ   (1 << 6)

int sc606_write(unsigned char reg, unsigned char data);

int sc606_read(unsigned char reg, unsigned char* data);

void sc606_init(void);
