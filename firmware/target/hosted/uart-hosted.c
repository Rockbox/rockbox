/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/ 
 *
 * Copyright (C) 2015 Thomas Martitz
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
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>

#include "uart-hosted.h"

struct termios tio;
struct termios stdio;
int uart_dev;

void uart_init(char* port)
{
    int err = 0;

    memset(&tio, 0, sizeof(tio));
    tio.c_iflag = 0;  //  RAW
    tio.c_oflag = 0;  //  RAW
    tio.c_cflag = CS8|CREAD|CLOCAL;           // 8n1, see termios.h for more information
    tio.c_lflag = 0;
    tio.c_cc[VMIN] = 1;   // 1 byte is sufficient to return from read call
    tio.c_cc[VTIME] = 5;

    uart_dev = open(port, O_RDWR | O_NONBLOCK);
    if(uart_dev < 0)
    {
        printf( "failed to open port\n");
    }

    err = cfsetospeed(&tio,B115200);
    if(err < 0)
    {
        printf( "failed to set output speed\n");
    }

    err = cfsetispeed(&tio,B115200);
    if(err < 0)
    {
        printf( "failed to set input speed\n");
    }

    err = tcsetattr(uart_dev, TCSANOW, &tio);
    if (err < 0)
    {
        printf("failed to set termios options\n");
    }

}

/* Flush uart */
void uart_reset(void)
{
    tcdrain(uart_dev);
    // nothing in particular to do
}

/* Write data */
int uart_write(char* data, int len)
{
    return write(uart_dev, data, len);
}

/* Read data */
int uart_read(char* data, int len)
{
    return read(uart_dev, data, len);
}

/* Get current speed */
int uart_get_speed(void)
{
    return 115200; // only testing ;=)
}

void uart_close(void)
{
    close(uart_dev);
}
