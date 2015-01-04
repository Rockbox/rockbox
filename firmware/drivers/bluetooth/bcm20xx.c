/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2015 Lorenzo Miori
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

// this has to receive transport information (i.e. read/write perhaps)

struct my_fancy_transport {
    read = **,
    write = **,
    init
    etc
}

// ... so that a driver remains cross-platform!

// => the goal is to avoid as many IFDEFS as possible :)

int bcm20xx_init(void)
{
    
}

int bcm20xx_close(void)
{
    
}

int bcm20xx_reset(void)
{
    
}

int bcm20xx_set_bdaddr(char[6] bdaddr)
{
    
}

int bcm20xx_get_bdaddr(char[6]* bdaddr)
{
    
}

//#ifdef SAMSUNGYPR1
// yep we did it, let's see if there is a better way ...
// the target might define something like nice_pointer_to_be_freed bcm20xx_patchram_provide(void)
// which every target can define. The goal is to read the patchram directly from the OF, patch it
// and free.

//#endif

#ifdef SAMSUNGYPR1
void bcm20xx_patchram_provide(char* buffer, char* len)
{
    
}
#else
void bcm20xx_patchram_provide(char* buffer, char* len)
{
    (void)buffer;
    (void)len;
}
#endif

static void bcm20xx_patchram_upload(void)
{
    char buf[50*1024];
    bcm20xx_patchram_provide(&buf, sizeof(buf));
    
    // do the magic
}

static void bcm20xx_speed(int speed)
{
    
}

static void bcm20xx_pcm(int speed)
{
    
}

static void bcm20xx_speed(int speed)
{
    
}
