/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Itai Shaked
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdlib.h>
#include "lcd.h"
#include "config.h"
#include "kernel.h"
#include "menu.h"
#include "button.h"

#define NUM_PARTICLES 100

static short particles[NUM_PARTICLES][2];

static bool particle_exists(int particle)
{
    if (particles[particle][0]>=0 && particles[particle][1]>=0 && 
        particles[particle][0]<112 && particles[particle][1]<64) 
        return true;
    else
        return false;
}

static int create_particle(void)
{
    int i;

    for (i=0; i<NUM_PARTICLES; i++) {
        if (!particle_exists(i)) {
            particles[i][0]=(rand()%112);
            particles[i][1]=0;
            return i;
        }
    }
    return -1;
}

static void snow_move(void)
{
    int i;

    if (!(rand()%2))
        create_particle();

    for (i=0; i<NUM_PARTICLES; i++) {
        if (particle_exists(i)) {
            lcd_clearpixel(particles[i][0],particles[i][1]);
            switch ((rand()%7)) {
                case 0:
                    particles[i][0]++;
                    break;

                case 1:
                    particles[i][0]--;
                    break;

                case 2:
                    break;

                default:
                    particles[i][1]++;
                    break;
            }
            if (particle_exists(i))
                lcd_drawpixel(particles[i][0],particles[i][1]);
        }
    }
}

static void snow_init(void)
{
    int i;

    for (i=0; i<NUM_PARTICLES; i++) {
        particles[i][0]=-1;
        particles[i][1]=-1;
    }        
    lcd_clear_display();
}

bool snow(void)
{
    snow_init();

    while (1) {
        snow_move();
        lcd_update();
        sleep(HZ/20);
        
        if (button_get(false) == BUTTON_OFF)
            return false;
    }
}









