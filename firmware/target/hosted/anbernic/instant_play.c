/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2025 Hairo R. Carela
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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

static int get_value(const char *cmd)
{
    /* Read values with the brightness/volume tools, stored in
       bootloader variables */
    char line[4];
    int value;

    char full_cmd[18];
    snprintf(full_cmd, 18, "%s get", cmd);

    FILE* fp = popen(full_cmd, "r");

    if (fp == NULL) {
        value = -1;
    }

    if (fgets(line, sizeof(line), fp) != NULL)
    {
       value = strtol(line, NULL, 10);
    }
    else
    {
        value = -1;
    }

    pclose(fp);
    return value;
}

static void set_value(const char *cmd, const int value)
{
    /* Set values with the brightness/volume tools, stored in
       bootloader variables. */
    char full_cmd[22];
    snprintf(full_cmd, 22, "%s set %d", cmd, value);
    system(full_cmd);
}

void ip_reset_values(void)
{
    /* Use the brightness/volume tools to reset to their previous
       values before launching rockbox */
    int volume_level = get_value("volume");
    if (volume_level > -1)
    {
        set_value("volume", volume_level);
    }

    int brightness_level = get_value("brightness");
    if (brightness_level > -1)
    {
        set_value("brightness", brightness_level);
    }
}

void ip_power_off(void)
{
    /* Stop the powerdown countdown */
    system("powerdown handle");

    /* Write the instant_play file */
    char buf[60];
    size_t nbytes;
    int fd;

    fd = open("/mnt/instant_play", O_WRONLY | O_CREAT, 0640);

    strcpy(buf, "'/opk/rockbox' &\npid record $!\nwait $!\npid erase\n");
    nbytes = strlen(buf);
    write(fd, buf, nbytes);

    close(fd);

    /* Powerdown the handheld after writting the file */
    system("powerdown now");
}

void ip_handle_sigusr1(int sig)
{
    if (sig == SIGUSR1)
    {
        ip_power_off();
    }
}
