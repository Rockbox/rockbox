/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Module wrapper for SI4709 FM Radio Chip, using /dev/si470x (si4709.ko) 
 *      Samsung YP-R0 & Samsung YP-R1
 *
 * Copyright (c) 2012 Lorenzo Miori
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

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "stdint.h"
#include "string.h"
#include "kernel.h"

#include "radio-ypr.h"
#include "si4700.h"
#include "power.h"

static int radio_dev = -1;

void radiodev_open(void) {
    radio_dev = open("/dev/si470x", O_RDWR);
}

void radiodev_close(void) {
    close(radio_dev);
}

/* High-level registers access */
void si4709_write_reg(int addr, uint16_t value) {
    sSi4709_t r = { .addr = addr, .value = value };
    ioctl(radio_dev, IOCTL_SI4709_WRITE_BYTE, &r);
}

uint16_t si4709_read_reg(int addr) {
    sSi4709_t r = { .addr = addr, .value = 0 };
    ioctl(radio_dev, IOCTL_SI4709_READ_BYTE, &r);
    return r.value;
}

/* Low-level i2c channel access */
int fmradio_i2c_write(unsigned char address, unsigned char* buf, int count)
{
    (void)address;
    sSi4709_i2c_t r = { .size = count, .buf = buf };
    return ioctl(radio_dev, IOCTL_SI4709_I2C_WRITE, &r);
}

int fmradio_i2c_read(unsigned char address, unsigned char* buf, int count)
{
    (void)address;
    sSi4709_i2c_t r = { .size = count, .buf = buf };
    return ioctl(radio_dev, IOCTL_SI4709_I2C_READ, &r);
}

#ifdef HAVE_RDS_CAP

/* Register we are going to poll */
#define STATUSRSSI  0xA
#define STATUSRSSI_RDSR     (0x1 << 15)

/* Low-level RDS Support */
static struct event_queue rds_queue;
static uint32_t rds_stack[DEFAULT_STACK_SIZE / sizeof(uint32_t)];

enum {
    Q_POWERUP,
};

static void NORETURN_ATTR rds_thread(void)
{
    /* start up frozen */
    int timeout = TIMEOUT_BLOCK;
    struct queue_event ev;

    while (true) {
        queue_wait_w_tmo(&rds_queue, &ev, timeout);
        switch (ev.id) {
            case Q_POWERUP:
                /* power up: timeout after 1 tick, else block indefinitely */
                timeout = ev.data ? 1 : TIMEOUT_BLOCK;
                break;
            case SYS_TIMEOUT:
                /* Captures RDS data and processes it */
                if ((si4709_read_reg(STATUSRSSI) & STATUSRSSI_RDSR) >> 8) {
                    si4700_rds_process();
                }
                break;
        }
    }
}

/* true after full radio power up, and false before powering down */
void si4700_rds_powerup(bool on)
{
    queue_post(&rds_queue, Q_POWERUP, on);
}

/* One-time RDS init at startup */
void si4700_rds_init(void)
{
    queue_init(&rds_queue, false);
    create_thread(rds_thread, rds_stack, sizeof(rds_stack), 0, "rds"
        IF_PRIO(, PRIORITY_PLAYBACK) IF_COP(, CPU));
}
#endif /* HAVE_RDS_CAP */
