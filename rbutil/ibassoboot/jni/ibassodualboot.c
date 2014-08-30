/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Ilia Sergachev
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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#include <linux/fb.h>
#include <linux/input.h>
#include <linux/reboot.h>

#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <time.h>

#include <sys/limits.h>
#include <sys/mman.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <unistd.h>

#include "qdbmp.h"


#define MIN_TIME 1395606821
#define TIME_FILE "/data/time_store"
#define TIME_CHECK_PERIOD 60 /* seconds */
#define VOLD_LINK "/data/vold"
#define PLAYER_FILE "/data/chosen_player"
#define NOASK_FLAG "/data/no_ask_once"
#define POLL_MS 10


#define KEYCODE_HEADPHONES 114
#define KEYCODE_HOLD 115
#define KEYCODE_PWR 116
#define KEYCODE_PWR_LONG 117
#define KEYCODE_SD 143
#define KEYCODE_VOLPLUS 158
#define KEYCODE_VOLMINUS 159
#define KEYCODE_PREV 160
#define KEYCODE_NEXT 162
#define KEYCODE_PLAY 161

#define KEY_HOLD_OFF 16

void checktime()
{
    time_t t_stored=0, t_current=time(NULL);

    FILE *f = fopen(TIME_FILE, "r");
    if(f!=NULL)
    {
        fscanf(f, "%ld", &t_stored);
        fclose(f);
    }

    printf("stored time: %ld, current time: %ld\n", t_stored, t_current);

    if(t_stored<MIN_TIME)
        t_stored=MIN_TIME;

    if(t_stored<t_current)
    {
        f = fopen(TIME_FILE, "w");
        fprintf(f, "%ld", t_current);
        fclose(f);
    }
    else
    {
        t_stored += TIME_CHECK_PERIOD;
        struct tm *t = localtime(&t_stored);
        struct timeval tv = {mktime(t), 0};
        settimeofday(&tv, 0);
    }
}


static struct pollfd *ufds;
static char **device_names;
static int nfds;



static int open_device(const char *device, int print_flags)
{
    int fd;
    struct pollfd *new_ufds;
    char **new_device_names;

    fd = open(device, O_RDWR);
    if(fd < 0)
    {
        fprintf(stderr, "could not open %s, %s\n", device, strerror(errno));
        return -1;
    }

    new_ufds = realloc(ufds, sizeof(ufds[0]) * (nfds + 1));
    if(new_ufds == NULL)
    {
        fprintf(stderr, "out of memory\n");
        return -1;
    }
    ufds = new_ufds;
    new_device_names = realloc(device_names, sizeof(device_names[0]) * (nfds + 1));
    if(new_device_names == NULL)
    {
        fprintf(stderr, "out of memory\n");
        return -1;
    }
    device_names = new_device_names;

    ufds[nfds].fd = fd;
    ufds[nfds].events = POLLIN;
    device_names[nfds] = strdup(device);
    nfds++;

    return 0;
}



static int scan_dir(const char *dirname, int print_flags)
{
    char devname[PATH_MAX];
    char *filename;
    DIR *dir;
    struct dirent *de;
    dir = opendir(dirname);
    if(dir == NULL)
        return -1;
    strcpy(devname, dirname);
    filename = devname + strlen(devname);
    *filename++ = '/';
    while((de = readdir(dir)))
    {
        if(de->d_name[0] == '.' &&
                (de->d_name[1] == '\0' ||
                        (de->d_name[1] == '.' && de->d_name[2] == '\0')))
            continue;
        strcpy(filename, de->d_name);
        open_device(devname, print_flags);
    }
    closedir(dir);
    return 0;
}



void button_init_device(void)
{
    int res;
    int print_flags = 0;
    const char *device = NULL;
    const char *device_path = "/dev/input";

    nfds = 1;
    ufds = calloc(1, sizeof(ufds[0]));
    ufds[0].fd = inotify_init();
    ufds[0].events = POLLIN;
    if(device)
    {
        res = open_device(device, print_flags);
        if(res < 0) {
             fprintf(stderr, "open device failed\n");
        }
    }
    else
    {
        res = inotify_add_watch(ufds[0].fd, device_path, IN_DELETE | IN_CREATE);
        if(res < 0)
        {
            fprintf(stderr, "could not add watch for %s, %s\n", device_path, strerror(errno));
        }
        res = scan_dir(device_path, print_flags);
        if(res < 0)
        {
            fprintf(stderr, "scan dir failed for %s\n", device_path);
        }
    }
}


int draw()
{
    int fbfd = 0;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    long int screensize = 0;
    char *fbp = 0;
    int x = 0, y = 0;
    long int location = 0;

    /* Open the file for reading and writing */
    fbfd = open("/dev/graphics/fb0", O_RDWR);
    if (fbfd == -1)
    {
        perror("Error: cannot open framebuffer device");
        exit(1);
    }
    /* Get fixed screen information */
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1)
    {
        perror("Error reading fixed information");
        exit(2);
    }

    /* Get variable screen information */
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1)
    {
        perror("Error reading variable information");
        exit(3);
    }

    /* Figure out the size of the screen in bytes */
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

    vinfo.xres = vinfo.xres_virtual = vinfo.width = 320;
    vinfo.yres = vinfo.yres_virtual = vinfo.height = 240;
    vinfo.xoffset = vinfo.yoffset = vinfo.sync = vinfo.vmode = 0;
    vinfo.pixclock = 104377;
    vinfo.left_margin = 20;
    vinfo.right_margin = 50;
    vinfo.upper_margin = 2;
    vinfo.lower_margin = 4;
    vinfo.hsync_len = 10;
    vinfo.vsync_len = 2;
    vinfo.red.offset = 11;
    vinfo.red.length = 5;
    vinfo.red.msb_right = 0;
    vinfo.green.offset = 5;
    vinfo.green.length = 6;
    vinfo.green.msb_right = 0;
    vinfo.blue.offset = 0;
    vinfo.blue.length = 5;
    vinfo.blue.msb_right = 0;
    vinfo.transp.offset = vinfo.transp.length = vinfo.transp.msb_right = 0;
    vinfo.nonstd = 4;

    if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &vinfo))
    {
        perror("fbset(ioctl)");
        exit(4);
    }


    /* Map the device to memory */
    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED,
            fbfd, 0);
    if ((int)fbp == -1)
    {
        perror("Error: failed to map framebuffer device to memory");
        exit(4);
    }

    BMP* bmp = BMP_ReadFile("/system/rockbox/chooser.bmp");
    BMP_CHECK_ERROR( stderr, -1 );

    UCHAR r, g, b;
    unsigned short int t;

    for (y = 0; y < 240; y++)
        for (x = 0; x < 320; x++)
        {
            location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) +
                    (y+vinfo.yoffset) * finfo.line_length;

            BMP_GetPixelRGB(bmp, x, y, &r, &g, &b);
            t = (r>>3)<<11 | (g>>2) << 5 | (b>>3);
            *((unsigned short int*)(fbp + location)) = t;
        }

    BMP_Free( bmp );

    munmap(fbp, screensize);
    close(fbfd);
    return 0;
}


int choose_player()
{
    int i;
    int res;
    struct input_event event;

    while(true)
    {
        poll(ufds, nfds, POLL_MS);
        for(i = 1; i < nfds; i++)
        {
            if(ufds[i].revents & POLLIN)
            {
                res = read(ufds[i].fd, &event, sizeof(event));
                if(res < (int)sizeof(event))
                {
                    fprintf(stderr, "could not get event\n");
                }
                if(event.type==1)
                {
                    if(event.code==KEYCODE_NEXT)
                    {
                        puts("rockbox!");
                        return 1;
                    }
                    else if(event.code==KEYCODE_PREV)
                    {
                        puts("mango!");
                        return 0;
                    }
                    else if(event.code==KEYCODE_PWR || event.code==KEYCODE_PWR_LONG)
                    {
                        reboot(LINUX_REBOOT_CMD_POWER_OFF);
                    }
                }
                else if(event.type==3)
                {
                    if(event.code==53) //x coord
                    {
                        if(event.value<160)
                        {
                            puts("mango!");
                            return 0;
                        }
                        else
                        {
                            puts("rockbox!");
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return true;
}

bool check_for_hold()
{
    FILE *f = fopen("/sys/class/axppower/holdkey", "r");
    char x;
    fscanf(f, "%c", &x);
    fclose(f);

    if(x & KEY_HOLD_OFF)
      return false;
    else
      return true;
}

int main(int argc, char **argv)
{
    FILE *f;
    int last_chosen_player = -1;

    f = fopen(PLAYER_FILE, "r");
    if(f!=NULL)
    {
        fscanf(f, "%d", &last_chosen_player);
        fclose(f);
    }
    bool ask = (access(VOLD_LINK, F_OK) == -1) || ((access(NOASK_FLAG, F_OK) == -1) && check_for_hold()) || (last_chosen_player==-1);

    if(ask)
    {
        draw();
        button_init_device();
        int player_chosen_now = choose_player();

        if(last_chosen_player!=player_chosen_now)
        {
            f = fopen(PLAYER_FILE, "w");
            fprintf(f, "%d", player_chosen_now);
            fclose(f);
        }

        if(last_chosen_player!=player_chosen_now || (access(VOLD_LINK, F_OK) == -1))
        {
            system("rm "VOLD_LINK);

            if(player_chosen_now)
                system("ln -s /system/bin/vold_rockbox "VOLD_LINK);
            else
                system("ln -s /system/bin/vold_original "VOLD_LINK);

            system("touch "NOASK_FLAG);
            system("reboot");
        }
        last_chosen_player = player_chosen_now;
    }

    system("rm "NOASK_FLAG);

    while(1)
    {
        if(last_chosen_player)
        {
//            system("/system/bin/openadb");
            system("/system/rockbox/lib/rockbox");
        }
        else
//            system("/system/bin/closeadb");
            system("/system/bin/MangoPlayer_original");

        sleep(1);
    }

    return 0;
}



