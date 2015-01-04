/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \  
 *                     \/            \/     \/    \/            \/ 
 *
 * Copyright (C) 2011-2013 by Lorenzo Miori
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

#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/mount.h>
#include <errno.h>

#include "system.h"
#include "kernel.h"
#include "thread.h"

#include "string-extra.h"
#include "panic.h"
#include "debug.h"
#include "storage.h"
#include "mv.h"
#include "ascodec.h"
#include "gpio-ypr.h"
#include "ascodec.h"
#include "backlight.h"
#include "rbunicode.h"
#include "logdiskf.h"

void power_off(void)
{
    /* Something that we need to do before exit on our platform YPR0 */
    ascodec_close();
    gpio_close();
    exit(EXIT_SUCCESS);
}

uintptr_t *stackbegin;
uintptr_t *stackend;
void system_init(void)
{
    int *s;
    /* fake stack, OS manages size (and growth) */
    stackbegin = stackend = (uintptr_t*)&s;
    /* Here begins our platform specific initilization for various things */
    ascodec_init();
    gpio_init();
}


void system_reboot(void)
{
    power_off();
}

void system_exception_wait(void)
{
    system_reboot();
}

#ifdef HAVE_MULTIDRIVE
/* MicroSD card removal / insertion management */

bool hostfs_removable(IF_MD_NONVOID(int drive))
{
#ifdef HAVE_MULTIDRIVE
    if (drive > 0) /* Active LOW */
        return true;
    else
#endif
        return false; /* internal: always present */
}

bool hostfs_present(IF_MD_NONVOID(int drive))
{
#ifdef HAVE_MULTIDRIVE
    if (drive > 0) /* Active LOW */
        return (!gpio_get(GPIO_SD_SENSE));
    else
#endif
        return true; /* internal: always present */
}

#ifdef HAVE_MULTIDRIVE
int volume_drive(int drive)
{
    return drive;
}
#endif /* HAVE_MULTIDRIVE */

#ifdef CONFIG_STORAGE_MULTI
int hostfs_driver_type(int drive)
{
    return drive > 0 ? STORAGE_SD_NUM : STORAGE_HOSTFS_NUM;
}
#endif /* CONFIG_STORAGE_MULTI */

#ifdef HAVE_HOTSWAP
bool volume_removable(int volume)
{
    /* don't support more than one partition yet, so volume == drive */
    return hostfs_removable(volume);
}

bool volume_present(int volume)
{
    /* don't support more than one partition yet, so volume == drive */
    return hostfs_present(volume);
}
#endif

static int unmount_sd(void)
{
    int ret;
    do
    {
        ret = umount("/mnt/mmc");
    } while (ret && errno != EBUSY && errno != EINVAL);

    return ret;
}

static int mount_sd(void)
{
    int ret;
    /* kludge to make sure we get our wanted mount flags. This is needed
     * when the sd was already mounted before we booted */
    unmount_sd();
    char iocharset[64] = "iocharset=";
    strlcat(iocharset, get_current_codepage_name_linux(), sizeof(iocharset));
    ret = mount("/dev/mmcblk0p1", "/mnt/mmc", "vfat",
                MS_MGC_VAL | MS_SYNCHRONOUS | MS_RELATIME,
                iocharset);
    /* failure probably means the kernel does not support the iocharset.
     * retry without to load the default */
    if (ret == -1)
        ret = mount("/dev/mmcblk0p1", "/mnt/mmc", "vfat",
                     MS_MGC_VAL | MS_SYNCHRONOUS | MS_RELATIME, NULL);
    return ret;
}

#ifdef HAVE_HOTSWAP

static int sd_thread_stack[DEFAULT_STACK_SIZE];

enum {
    STATE_POLL,
    STATE_DEBOUNCE,
    STATE_MOUNT,
};

static void NORETURN_ATTR sd_thread(void)
{
    int ret, state = STATE_POLL;
    bool last_present, present;
    int attempts = 0;

    last_present = present = storage_present(1); /* shut up gcc */

    while (1)
    {
        switch (state)
        {
            case STATE_POLL:
                sleep(HZ/3);
                attempts = 0;
                present = storage_present(1);
                if (last_present != present)
                    state = STATE_DEBOUNCE;
                break;

            case STATE_DEBOUNCE:
                sleep(HZ/5);
                present = storage_present(1);
                if (last_present == present)
                {
                    if (present)
                        queue_broadcast(SYS_HOTSWAP_INSERTED, 0);
                    else
                        queue_broadcast(SYS_HOTSWAP_EXTRACTED, 0);
                    state = STATE_MOUNT;
                }
                else
                    state = STATE_POLL;
                break;

            case STATE_MOUNT:
                sleep(HZ/10);
                if (present)
                    ret = mount_sd();
                else
                    ret = unmount_sd();
                if (ret == 0)
                {
                    NOTEF("Successfully %smounted SD card\n", present ? "":"un");
                    queue_broadcast(SYS_FS_CHANGED, 0);
                    state = STATE_POLL;
                }
                else if (++attempts > 20) /* stop retrying after 2s */
                {
                    ERRORF("Failed to %smount SD card. Giving up.", present ? "":"un");
                    state = STATE_POLL;
                }
                /* else: need to retry a few times because the kernel is
                 * busy setting up the SD (=> do not change state) */
                break;
        }
        last_present = present;
    }
}

#endif

#endif /* HAVE_MULTIDRIVE */

int hostfs_init(void)
{
    /* Setup GPIO pin for microSD sense, copied from OF */
    gpio_set_iomux(GPIO_SD_SENSE, CONFIG_DEFAULT);
    gpio_set_pad(GPIO_SD_SENSE, PAD_CTL_SRE_SLOW);
    gpio_direction_input(GPIO_SD_SENSE);
#ifdef HAVE_MULTIDRIVE
    if (storage_present(IF_MD(1)))
        mount_sd();
#ifdef HAVE_HOTSWAP
    create_thread(sd_thread, sd_thread_stack, sizeof(sd_thread_stack), 0,
        "sd thread" IF_PRIO(, PRIORITY_BACKGROUND) IF_COP(, CPU));
#endif
#endif
    return 0;
}

int hostfs_flush(void)
{
    sync();
    return 0;
}

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
#include <stdio.h>
#include "file.h"
/* This is the Linux Kernel CPU governor... */
static void set_cpu_freq(int speed)
{
    char temp[10];
    int cpu_dev;
    cpu_dev = open("/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed", O_WRONLY);
    if (cpu_dev < 0)
        return;
    write(cpu_dev, temp, sprintf(temp, "%d", speed) + 1);
    close(cpu_dev);
}

void set_cpu_frequency(long frequency)
{
    switch (frequency)
    {
        case CPUFREQ_MAX:
            set_cpu_freq(532000);
            cpu_frequency = CPUFREQ_MAX;
            break;
        case CPUFREQ_NORMAL:
            set_cpu_freq(400000);
            cpu_frequency = CPUFREQ_NORMAL;
            break;
        default:
            set_cpu_freq(200000);
            cpu_frequency = CPUFREQ_DEFAULT;
            break;
    }
}
#endif
