/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Robert Kukla 
 * 
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "rtc.h" 
#include "logf.h"
#include "sw_i2c.h"
#include "i2c-pp.h"

/* The RTC chip is unknown, the information about it was gathered by 
 * reverse engineering the bootloader.
 */

#define RTC_ADDR   0x60

#define RTC_CMD_CTRL 0 /* OF uses it with single byte 1 or 2 */
#define RTC_CMD_UNKN 1 /* OF uses it with single byte 8 */
#define RTC_CMD_DATA 2
#define RTC_CMD_TEST 7 /* OF uses it with single byte 0xAA */

/* private */

static void reverse_bits(unsigned char* v, int size) {

    int i,j,in,out=0;

    for(j=0; j<size; j++) {   
        in = v[j];
        for(i=0; i<8; i++) {
           out |= (in & 1);
           in  = in >>1;
           out = out<<1;
        }   
        v[j] = out>>1;
    }   
}    

static int sw_i2c(int access, unsigned char chip, unsigned char cmd, 
                              unsigned char* buf, int count) {
    int i;

    i2c_lock();
    GPIOC_ENABLE |=  0x00000030;

    chip|=cmd<<1;

    if(access == SW_I2C_READ) {
        i = sw_i2c_read(chip, 0, buf, count);
        reverse_bits(buf, count);
    } else {
        reverse_bits(buf, count);
        i = sw_i2c_write(chip, 0, buf, count);
    }    

    GPIOC_ENABLE &= ~0x00000030;
    i2c_unlock();
    
    return i;
}

/* public */

void rtc_init(void)
{
    sw_i2c_init();

    /* to set a time while buttons are stil not working
    unsigned char v[7] = {0x00,0x47,0x17,0x06,0x03,0x02,0x08};
    rtc_write_datetime(v);
    */
}

int rtc_read_datetime(unsigned char* buf)
{
    int i;
    unsigned char v[7];

    i = sw_i2c(SW_I2C_READ, RTC_ADDR, RTC_CMD_DATA, v, 7);

    v[4] &= 0x3f; /* mask out p.m. flag */
    
    for(i=0; i<7; i++) 
        buf[i] = v[6-i];
    
    return i;
}

int rtc_write_datetime(unsigned char* buf)
{
    int i;
    unsigned char v[7];

    for(i=0; i<7; i++) 
        v[i]=buf[6-i];
        
    i = sw_i2c(SW_I2C_WRITE, RTC_ADDR, RTC_CMD_DATA, v, 7);
    
    return i;
}

