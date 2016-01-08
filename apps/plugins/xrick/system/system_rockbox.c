 /***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Port of xrick, a Rick Dangerous clone, to Rockbox.
 * See http://www.bigorno.net/xrick/
 *
 * Copyright (C) 2008-2014 Pierluigi Vicinanza
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "xrick/system/system.h"

#include "xrick/config.h"
#ifdef ENABLE_SOUND
#include "xrick/system/syssnd_rockbox.h"
#endif

#include "plugin.h"

enum { LINE_LENGTH = 80 };

/*
* Error
*/
void sys_error(const char *err, ...)
{
    va_list argptr;
    char s[LINE_LENGTH];

    /* prepare message */
    va_start(argptr, err);
    rb->vsnprintf(s, sizeof(s), err, argptr);
    va_end(argptr);

    /* print error message */
    rb->splashf(HZ*3, "%s\nError!\n", s);
    DEBUGF("%s\nError!\n", s);
}

/*
* Print a message to standard output
*/
void sys_printf(const char *msg, ...)
{
    va_list argptr;
    char s[LINE_LENGTH];

    /* prepare message */
    va_start(argptr, msg);
    rb->vsnprintf(s, sizeof(s), msg, argptr);
    va_end(argptr);

    /* print message */
    DEBUGF("%s",s);

#ifdef ENABLE_SYSPRINTF_TO_SCREEN
    {
        static int currentYPos = 0;
        size_t i;

        /* Device LCDs display newlines funny. */
        for(i = 0; s[i] != '\0'; ++i)
        {
            if(s[i] == '\n')
            {
                s[i] = ' ';
            }
        }

        rb->lcd_putsxy(1, currentYPos, (unsigned char *)s);
        rb->lcd_update();

        currentYPos += 12;
        if(currentYPos > LCD_HEIGHT-12)
        {
            currentYPos = 0;
            rb->lcd_clear_display();
        }
    }
#endif /* ENABLE_SYSPRINTF_TO_SCREEN */
}

/*
* Print a message to string buffer
*/
void sys_snprintf(char *buf, size_t size, const char *msg, ...)
{
    va_list argptr;

    va_start(argptr, msg);
    rb->vsnprintf(buf, size, msg, argptr);
    va_end(argptr);
}

/*
* Returns string length
*/
size_t sys_strlen(const char * str)
{
    return rb->strlen(str);
}

/*
* Return number of milliseconds elapsed since first call
*/
U32 sys_gettime(void)
{
    long ticks = *(rb->current_tick);
    return (U32)((ticks * 1000) / HZ);
}

/*
* Yield execution to another thread
*/
void sys_yield(void)
{
    rb->yield();
}

/*
* Initialize system
*/
bool sys_init(int argc, char **argv)
{
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif

    if (!sysarg_init(argc, argv))
    {
        return false;
    }
    if (!sysmem_init())
    {
        return false;
    }
    if (!sysvid_init())
    {
        return false;
    }
#ifdef ENABLE_SOUND
    if (!sysarg_args_nosound && !syssnd_init())
    {
        return false;
    }
#endif
    if (!sysfile_setRootPath(sysarg_args_data? sysarg_args_data : sysfile_defaultPath))
    {
        return false;
    }
    return true;
}

/*
* Shutdown system
*/
void sys_shutdown(void)
{
    sysfile_clearRootPath();
#ifdef ENABLE_SOUND
    syssnd_shutdown();
#endif
    sysvid_shutdown();
    sysmem_shutdown();

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
}

/*
* Preload data before entering main loop
*/
bool sys_cacheData(void)
{
#ifdef ENABLE_SOUND
    syssnd_load(soundGameover);
    syssnd_load(soundSbonus2);
    syssnd_load(soundBullet);
    syssnd_load(soundBombshht);
    syssnd_load(soundExplode);
    syssnd_load(soundStick);
    syssnd_load(soundWalk);
    syssnd_load(soundCrawl);
    syssnd_load(soundJump);
    syssnd_load(soundPad);
    syssnd_load(soundBox);
    syssnd_load(soundBonus);
    syssnd_load(soundSbonus1);
    syssnd_load(soundDie);
    syssnd_load(soundEntity[0]);
    syssnd_load(soundEntity[1]);
    syssnd_load(soundEntity[2]);
    syssnd_load(soundEntity[3]);
    syssnd_load(soundEntity[4]);
    syssnd_load(soundEntity[5]);
    syssnd_load(soundEntity[6]);
    syssnd_load(soundEntity[7]);
    syssnd_load(soundEntity[8]);
    syssnd_load(soundTune0);
    syssnd_load(soundTune1);
    syssnd_load(soundTune2);
    syssnd_load(soundTune3);
    syssnd_load(soundTune4);
    syssnd_load(soundTune5);
#endif /* ENABLE_SOUND */
    return true;
}

/*
* Clear preloaded data before shutdown
*/
void sys_uncacheData(void)
{
#ifdef ENABLE_SOUND
    syssnd_unload(soundTune5);
    syssnd_unload(soundTune4);
    syssnd_unload(soundTune3);
    syssnd_unload(soundTune2);
    syssnd_unload(soundTune1);
    syssnd_unload(soundTune0);
    syssnd_unload(soundEntity[8]);
    syssnd_unload(soundEntity[7]);
    syssnd_unload(soundEntity[6]);
    syssnd_unload(soundEntity[5]);
    syssnd_unload(soundEntity[4]);
    syssnd_unload(soundEntity[3]);
    syssnd_unload(soundEntity[2]);
    syssnd_unload(soundEntity[1]);
    syssnd_unload(soundEntity[0]);
    syssnd_unload(soundDie);
    syssnd_unload(soundSbonus1);
    syssnd_unload(soundBonus);
    syssnd_unload(soundBox);
    syssnd_unload(soundPad);
    syssnd_unload(soundJump);
    syssnd_unload(soundCrawl);
    syssnd_unload(soundWalk);
    syssnd_unload(soundStick);
    syssnd_unload(soundExplode);
    syssnd_unload(soundBombshht);
    syssnd_unload(soundBullet);
    syssnd_unload(soundSbonus2);
    syssnd_unload(soundGameover);
#endif /* ENABLE_SOUND */
}

/* eof */
