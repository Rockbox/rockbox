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

#include "stdbool.h"
#include "uart-hosted.h"
//#include "bluetooth-transport.h"

void transport_init(void)
{
    uart_init("/dev/ttymxc1");
}

void transport_reset(void)
{
    uart_reset();
}

int transport_write(char* data, int len)
{
    return uart_write(data, len);
}

int transport_read(char* data, int len)
{
    return uart_read(data, len);
}

int transport_get_speed(void)
{
    return uart_get_speed();
}

void transport_close(void)
{
    uart_close();
}
