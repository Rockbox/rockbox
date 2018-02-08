/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *
 * Copyright (C) 2016 by Amaury Pouly
 *               2018 by Marcin Bukat
 *
 * Based on Rockbox iriver bootloader by Linus Nielsen Feltzing
 * and the ipodlinux bootloader by Daniel Palffy and Bernard Leach
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

#include "system.h"
#include "lcd.h"
#include "backlight.h"
#include "button-target.h"
#include "button.h"
#include "../kernel/kernel-internal.h"
#include "core_alloc.h"
#include "filesystem-app.h"
#include "lcd.h"
#include "font.h"
#include "power.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/wait.h>
#include <stdarg.h>
#include "version.h"

/* all images must have the following size */
#define ICON_WIDTH  70
#define ICON_HEIGHT 70

/* images */
#include "bitmaps/rockboxicon.h"
#include "bitmaps/hibyicon.h"
#include "bitmaps/toolsicon.h"

/* don't issue an error when parsing the file for dependencies */
#if defined(BMPWIDTH_rockboxicon) && (BMPWIDTH_rockboxicon != ICON_WIDTH || \
    BMPHEIGHT_rockboxicon != ICON_HEIGHT)
#error rockboxicon has the wrong resolution
#endif
#if defined(BMPWIDTH_hibyicon) && (BMPWIDTH_hibyicon != ICON_WIDTH || \
    BMPHEIGHT_hibyicon != ICON_HEIGHT)
#error hibyicon has the wrong resolution
#endif
#if defined(BMPWIDTH_toolsicon) && (BMPWIDTH_toolsicon != ICON_WIDTH || \
    BMPHEIGHT_toolsicon != ICON_HEIGHT)
#error toolsicon has the wrong resolution
#endif

#ifndef BUTTON_REW
#define BUTTON_REW  BUTTON_LEFT
#endif
#ifndef BUTTON_FF
#define BUTTON_FF   BUTTON_RIGHT
#endif
#ifndef BUTTON_PLAY
#define BUTTON_PLAY BUTTON_SELECT
#endif

/* return icon y position (x is always centered) */
int get_icon_y(void)
{
    /* adjust so that this contains the Sony logo and produces a nice logo
     * when used with rockbox */
    return (LCD_HEIGHT - ICON_HEIGHT)/4;
}

/* Important Note: this bootloader is carefully written so that in case of
 * error, the OF is run. This seems like the safest option since the OF is
 * always there and might do magic things. */

enum boot_mode
{
    BOOT_ROCKBOX,
    BOOT_TOOLS,
    BOOT_OF,
    BOOT_COUNT,
    BOOT_USB, /* special */
    BOOT_STOP, /* power down/suspend */
};

static void display_text_center(int y, const char *text)
{
    int width;
    lcd_getstringsize(text, &width, NULL);
    lcd_putsxy(LCD_WIDTH / 2 - width / 2, y, text);
}

static void display_text_centerf(int y, const char *format, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, format);

    vsnprintf(buf, sizeof(buf), format, ap);
    display_text_center(y, buf);
}

/* get timeout before taking action if the user doesn't touch the device */
int get_inactivity_tmo(void)
{
    if(button_hold())
        return 5 * HZ; /* Inactivity timeout when on hold */
    else
        return 10 * HZ; /* Inactivity timeout when not on hold */
}

/* return action on idle timeout */
enum boot_mode inactivity_action(enum boot_mode cur_selection)
{
    if(button_hold())
        return BOOT_STOP; /* power down/suspend */
    else
        return cur_selection; /* return last choice */
}

/* we store the boot mode in a file in /tmp so we can reload it between 'boots'
 * (since the mostly suspends instead of powering down) */
enum boot_mode load_boot_mode(enum boot_mode mode)
{
    int fd = open("/data/rb_bl_mode.txt", O_RDONLY);
    if(fd >= 0)
    {
        read(fd, &mode, sizeof(mode));
        close(fd);
    }
    return mode;
}

void save_boot_mode(enum boot_mode mode)
{
    int fd = open("/data/rb_bl_mode.txt", O_RDWR | O_CREAT | O_TRUNC);
    if(fd >= 0)
    {
        write(fd, &mode, sizeof(mode));
        close(fd);
    }
}

enum boot_mode get_boot_mode(void)
{
    /* load previous mode, or start with rockbox if none */
    enum boot_mode init_mode = load_boot_mode(BOOT_ROCKBOX);
    /* wait for user action */
    enum boot_mode mode = init_mode;
    int last_activity = current_tick;
    bool hold_status = button_hold();
    while(true)
    {
        /* on usb detect, return to usb
         * FIXME this is a hack, we need proper usb detection */
        if(power_input_status() & POWER_INPUT_USB_CHARGER)
        {
            /* save last choice */
            save_boot_mode(mode);
            return BOOT_USB;
        }
        /* inactivity detection */
        int timeout = last_activity + get_inactivity_tmo();
        if(TIME_AFTER(current_tick, timeout))
        {
            /* save last choice */
            save_boot_mode(mode);
            return inactivity_action(mode);
        }
        /* redraw */
        lcd_clear_display();
        /* display top text */
        if(button_hold())
        {
            lcd_set_foreground(LCD_RGBPACK(255, 0, 0));
            display_text_center(0, "ON HOLD!");
        }
        else
        {
            lcd_set_foreground(LCD_RGBPACK(255, 201, 0));
            display_text_center(0, "SELECT PLAYER");
        }
        lcd_set_foreground(LCD_RGBPACK(255, 201, 0));
        /* display icon */
        const struct bitmap *icon = (mode == BOOT_OF) ? &bm_hibyicon :
            (mode == BOOT_ROCKBOX) ? &bm_rockboxicon : &bm_toolsicon;
        lcd_bmp(icon, (LCD_WIDTH - ICON_WIDTH) / 2, get_icon_y());
        /* display bottom description */
        const char *desc = (mode == BOOT_OF) ? "HIBY PLAYER" :
            (mode == BOOT_ROCKBOX) ? "ROCKBOX" : "TOOLS";
        display_text_center(3 * get_icon_y() + ICON_HEIGHT, desc);
        /* display arrows */
        int arrow_width, arrow_height;
        lcd_getstringsize("<", &arrow_width, &arrow_height);
        int arrow_y = get_icon_y() + ICON_HEIGHT / 2 - arrow_height / 2;
        lcd_putsxy(arrow_width / 2, arrow_y, "<");
        lcd_putsxy(LCD_WIDTH - 3 * arrow_width / 2, arrow_y, ">");

        lcd_set_foreground(LCD_RGBPACK(0, 255, 0));
        display_text_centerf(LCD_HEIGHT - arrow_height * 3 / 2, "timeout in %d sec",
            (timeout - current_tick + HZ - 1) / HZ);

        lcd_update();

        /* wait for a key  */
        int btn = button_get_w_tmo(HZ / 10);
        /* record action, changing HOLD counts as action */
        if(btn & BUTTON_MAIN || hold_status != button_hold())
            last_activity = current_tick;
        hold_status = button_hold();
        /* ignore release, allow repeat */
        if(btn & BUTTON_REL)
            continue;
        if(btn & BUTTON_REPEAT)
            btn &= ~BUTTON_REPEAT;
        /* play -> stop loop and return mode */
        if(btn == BUTTON_PLAY)
            break;
        /* left/right/up/down: change mode */
        if(btn == BUTTON_LEFT || btn == BUTTON_DOWN || btn == BUTTON_REW)
            mode = (mode + BOOT_COUNT - 1) % BOOT_COUNT;
        if(btn == BUTTON_RIGHT || btn == BUTTON_UP || btn == BUTTON_FF)
            mode = (mode + 1) % BOOT_COUNT;
    }

    /* save mode */
    save_boot_mode(mode);
    return mode;
}

void error_screen(const char *msg)
{
    lcd_clear_display();
    lcd_putsf(0, 0, msg);
    lcd_update();
}

int choice_screen(const char *title, bool center, int nr_choices, const char *choices[])
{
    int choice = 0;
    int max_len = 0;
    int h;
    lcd_getstringsize("x", NULL, &h);
    for(int i = 0; i < nr_choices; i++)
    {
        int len = strlen(choices[i]);
        if(len > max_len)
            max_len = len;
    }
    char *buf = malloc(max_len + 10);
    int top_y = 2 * h;
    int nr_lines = (LCD_HEIGHT - top_y) / h;
    while(true)
    {
        /* make sure choice is visible */
        int offset = choice - nr_lines / 2;
        if(offset < 0)
            offset = 0;
        lcd_clear_display();
        /* display top text */
        lcd_set_foreground(LCD_RGBPACK(255, 201, 0));
        display_text_center(0, title);
        int line = 0;
        for(int i = 0; i < nr_choices && line < nr_lines; i++)
        {
            if(i < offset)
                continue;
            if(i == choice)
                lcd_set_foreground(LCD_RGBPACK(255, 0, 0));
            else
                lcd_set_foreground(LCD_RGBPACK(255, 201, 0));
            sprintf(buf, "%s", choices[i]);
            if(center)
                display_text_center(top_y + h * line, buf);
            else
                lcd_putsxy(0, top_y + h * line, buf);
            line++;
        }

        lcd_update();

        /* wait for a key  */
        int btn = button_get_w_tmo(HZ / 10);
        /* ignore release, allow repeat */
        if(btn & BUTTON_REL)
            continue;
        if(btn & BUTTON_REPEAT)
            btn &= ~BUTTON_REPEAT;
        /* play -> stop loop and return mode */
        if(btn == BUTTON_PLAY || btn == BUTTON_LEFT)
        {
            free(buf);
            return btn == BUTTON_PLAY ? choice : -1;
        }
        /* left/right/up/down: change mode */
        if(btn == BUTTON_UP)
            choice = (choice + nr_choices - 1) % nr_choices;
        if(btn == BUTTON_DOWN)
            choice = (choice + 1) % nr_choices;
    }
}

void run_file(const char *name)
{
    char *dirname = "/mnt/sd_0/";
    char *buf = malloc(strlen(dirname) + strlen(name) + 1);
    sprintf(buf, "%s%s", dirname, name);

    lcd_clear_display();
    lcd_set_foreground(LCD_RGBPACK(255, 201, 0));
    lcd_putsf(0, 0, "Running %s", name);
    lcd_update();

    pid_t pid = fork();
    if(pid == 0)
    {
        execlp("sh", "sh", buf, NULL);
        _exit(42);
    }
    int status;
    waitpid(pid, &status, 0);
    if(WIFEXITED(status))
    {
        lcd_set_foreground(LCD_RGBPACK(255, 201, 0));
        lcd_putsf(0, 1, "program returned %d", WEXITSTATUS(status));
    }
    else
    {
        lcd_set_foreground(LCD_RGBPACK(255, 0, 0));
        lcd_putsf(0, 1, "an error occured: %x", status);
    }
    lcd_set_foreground(LCD_RGBPACK(255, 0, 0));
    lcd_putsf(0, 3, "Press any key or wait");
    lcd_update();
    /* wait a small time */
    sleep(HZ);
    /* ignore event */
    while(button_get(false) != 0) {}
    /* wait for any key or timeout */
    button_get_w_tmo(4 * HZ);
}

void run_script_menu(void)
{
    const char **entries = NULL;
    int nr_entries = 0;
    DIR *dir = opendir("/mnt/sd_0");
    struct dirent *ent;
    while((ent = readdir(dir)))
    {
        if(ent->d_type != DT_REG)
            continue;
        entries = realloc(entries, (nr_entries + 1) * sizeof(const char *));
        entries[nr_entries++] = strdup(ent->d_name);
    }
    closedir(dir);
    int idx = choice_screen("RUN SCRIPT", false, nr_entries, entries);
    if(idx >= 0)
        run_file(entries[idx]);
    for(int i = 0; i < nr_entries; i++)
        free((char *)entries[i]);
    free(entries);
}

void tools_screen(void)
{
    const char *choices[] = {"ADB", "Run script", "Restart", "Shutdown"};
    int choice = choice_screen("TOOLS MENU", true, 4, choices);
    if(choice == 0)
    {
        /* run service menu */
        fflush(stdout);
//        execl("/usr/local/bin/mptapp", "mptapp", NULL);
//        error_screen("Cannot boot service menu");
        sleep(5 * HZ);
    }
    else if(choice == 1)
        run_script_menu();
//    else if(choice == 2)
//        nwz_power_restart();
    else if(choice == 3)
        power_off();
}

/* open log file */
int open_log(void)
{
    /* open regular log file */
    int fd = open("/mnt/sd_0/rockbox.log", O_RDWR | O_CREAT | O_APPEND);
    /* get its size */
    struct stat stat;
    if(fstat(fd, &stat) != 0)
        return fd; /* on error, don't do anything */
    /* if file is too large, rename it and start a new log file */
    if(stat.st_size < 1000000)
        return fd;
    close(fd);
    /* move file */
    rename("/mnt/sd_0/rockbox.log", "/mnt_sd0/rockbox.log.1");
    /* re-open the file, truncate in case the move was unsuccessful */
    return open("/mnt/sd_0/rockbox.log", O_RDWR | O_CREAT | O_APPEND | O_TRUNC);
}

int main(int argc, char **argv)
{
    (void) argc;
    (void) argv;
    /* redirect stdout and stderr to have error messages logged somewhere on the
     * user partition */
    int fd = open_log();
    if(fd >= 0)
    {
        dup2(fd, fileno(stdout));
        dup2(fd, fileno(stderr));
        close(fd);
    }
    /* print version */
    printf("Rockbox boot loader\n");
    printf("Version: %s\n", rbversion);
    printf("%s\n", MODEL_NAME);

    system_init();
    core_allocator_init();
    kernel_init();
    paths_init();
    lcd_init();
    font_init();
    button_init();
    backlight_init();
    backlight_set_brightness(DEFAULT_BRIGHTNESS_SETTING);
    /* try to load the extra font we install on the device */
    int font_id = font_load("/usr/local/share/rockbox/bootloader.fnt");
    if(font_id >= 0)
        lcd_setfont(font_id);

    /* run all tools menu */
    while(true)
    {
        enum boot_mode mode = get_boot_mode();
        if(mode == BOOT_USB || mode == BOOT_OF)
        {
            fflush(stdout);
            fflush(stderr);
            close(fileno(stdout));
            close(fileno(stderr));
            /* for now the only way we have to trigger USB mode it to run the OF */
            /* boot OF */
            execvp("/usr/bin/hibyplayer", argv);
            error_screen("Cannot boot OF");
            sleep(5 * HZ);
        }
        else if(mode == BOOT_TOOLS)
        {
            tools_screen();
        }
        else if(mode == BOOT_ROCKBOX)
        {
            /* Rockbox expects /.rockbox to contain themes, rocks, etc, but we
            * cannot easily create this symlink because the root filesystem is
            * mounted read-only. Although we could remount it read-write temporarily,
            * this is neededlessly complicated and we defer this job to the dualboot
            * install script */
            fflush(stdout);
            execl("/mnt/sd_0/.rockbox/rockbox.agptek", "rockbox.agptek", NULL);
            printf("execvp failed: %s\n", strerror(errno));
            /* fallback to OF in case of failure */
            error_screen("Cannot boot Rockbox");
            sleep(5 * HZ);
        }
        else
        {
            printf("suspend\n");
//            nwz_power_suspend();
        }
    }
    /* if we reach this point, everything failed, so return an error so that
     * sysmgrd knows something is wrong */
    return 1;
}
