/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardell
 *
 * Based on Rockbox iriver bootloader by Linus Nielsen Feltzing
 * and the ipodlinux bootloader by Daniel Palffy and Bernard Leach
 * 
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cpu.h"
#include "system.h"
#include "lcd.h"
#include "kernel.h"
#include "thread.h"
#include "ata.h"
#include "fat.h"
#include "disk.h"
#include "font.h"
#include "adc.h"
#include "backlight.h"
#include "button.h"
#include "panic.h"
#include "power.h"
#include "file.h"

/* Size of the buffer to store the loaded Rockbox/iriver image */
#define MAX_LOADSIZE (5*1024*1024)

/* This is identical to the function used in the iPod bootloader */
static void memmove16(void *dest, const void *src, unsigned count)
{
    struct bufstr {
        unsigned _buf[4];
    } *d, *s;

    if (src >= dest) {
        count = (count + 15) >> 4;
        d = (struct bufstr *) dest;
        s = (struct bufstr *) src;
        while (count--)
            *d++ = *s++;
    } else {
        count = (count + 15) >> 4;
        d = (struct bufstr *)(dest + (count <<4));
        s = (struct bufstr *)(src + (count <<4));
        while (count--)
            *--d = *--s;
    }
}


/* Load original iriver firmware. This function expects a file called "H10_20GC.mi4" in
   the root directory of the player. It should be decrypted and have the header stripped
   using mi4code. It reads the file in to a memory buffer called buf. The rest of the 
   loading is done in main()
*/
int load_iriver(unsigned char* buf)
{
    int fd;
    int rc;
    int len;
    
    fd = open("/H10_20GC.mi4", O_RDONLY);

    len = filesize(fd);
    rc = read(fd, buf, len);
    if(rc < len)
        return -4;

    close(fd);
    return len;
}

/* A buffer to load the iriver firmware or Rockbox into */
unsigned char loadbuffer[MAX_LOADSIZE];

void main(void)
{
	/* Attempt to load original iriver firmware. Successfully starts loading the iriver
       firmware but then locks up once the "System Initialising" screen is displayed.
	   
	   The iriver firmware was decrypted and the header removed. It was then appended to 
	   the end of bootloader.bin and an mi4 file was created from the resulting file. 
	   
	   The original firmware starts at 0xd800 in the file and is of length 0x47da00.
	   
	   The whole file (bootloader.bin + decrypted mi4) are loaded to memory by the 
	   original iriver bootloader on startup. This copies the mi4 part to the start
	   of DRAM and passes execution to there.
	
	memmove16((void*)DRAMORIG, (void*)(DRAMORIG + 0xd800), 0x47da00);
	asm volatile(
		"mov   r0, #" SC(DRAMORIG) "\n"
		"mov   pc, r0                 \n"
	);
	*/

    int i;
    int rc;
	int btn;

    i=ata_init();
    disk_init();
    rc = disk_mount_all();

	/* Load original iriver firmware. Uses load_iriver(buf) to load the decrypted mi4 file from
       disk to DRAM. This then copies that part of DRAM to the start of DRAM and passes 
	   execution to there.
	
	rc=load_iriver(loadbuffer);
	memcpy((void*)DRAMORIG,loadbuffer,rc);
	asm volatile(
		"mov   r0, #" SC(DRAMORIG) "\n"
		"mov   pc, r0                 \n"
	);*/
	
	
	/* This assumes that /test.txt exists */
	int fd;
	char buffer[24];
	fd=open("/test.txt",O_RDWR);
	
	
	/* WARNING: Running this code on the H10 caused permanent damage to my H10's hdd
	            I strongly recommend against trying it.
				
	for(i=0;i<100;i++){
		btn = button_read_device();
		switch(btn){
		case BUTTON_LEFT:
			snprintf(buffer, sizeof(buffer), "Left");
			write(fd,buffer,sizeof(buffer));
			break;
		case BUTTON_RIGHT:
		break;
		case BUTTON_REW:
		break;
		case BUTTON_FF:
		break;
		case BUTTON_PLAY:
		break;
		default:
		break;
		}
		

	}
	*/
	
	
	
    /* Investigate gpio
	
	unsigned int gpio_a, gpio_b, gpio_c, gpio_d;
    unsigned int gpio_e, gpio_f, gpio_g, gpio_h;
    unsigned int gpio_i, gpio_j, gpio_k, gpio_l;

	gpio_a = GPIOA_INPUT_VAL;
	gpio_b = GPIOB_INPUT_VAL;
	gpio_c = GPIOC_INPUT_VAL;

	gpio_g = GPIOG_INPUT_VAL;
	gpio_h = GPIOH_INPUT_VAL;
	gpio_i = GPIOI_INPUT_VAL;

	snprintf(buffer, sizeof(buffer), "GPIO_A: %02x  GPIO_G: %02x\n", gpio_a, gpio_g);
	write(fd,buffer,sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "GPIO_B: %02x  GPIO_H: %02x\n", gpio_b, gpio_h);
	write(fd,buffer,sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "GPIO_C: %02x  GPIO_I: %02x\n", gpio_c, gpio_i);
	write(fd,buffer,sizeof(buffer));
	
	gpio_d = GPIOD_INPUT_VAL;
	gpio_e = GPIOE_INPUT_VAL;
	gpio_f = GPIOF_INPUT_VAL;

	gpio_j = GPIOJ_INPUT_VAL;
	gpio_k = GPIOK_INPUT_VAL;
	gpio_l = GPIOL_INPUT_VAL;

	snprintf(buffer, sizeof(buffer), "GPIO_D: %02x  GPIO_J: %02x\n", gpio_d, gpio_j);
	write(fd,buffer,sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "GPIO_E: %02x  GPIO_K: %02x\n", gpio_e, gpio_k);
	write(fd,buffer,sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "GPIO_F: %02x  GPIO_L: %02x\n", gpio_f, gpio_l);
	write(fd,buffer,sizeof(buffer));
	*/
	
	
	
	/* Detect the scroller being touched
	
	int j = 0;
	for(j=0;j<1000;j++){
		if(gpio_c!=0xF7){
			snprintf(buffer, sizeof(buffer), "GPIO_C: %02x\n", gpio_c);
			write(fd,buffer,sizeof(buffer));
		}
		if(gpio_d!=0xDD){
			snprintf(buffer, sizeof(buffer), "GPIO_D: %02x\n", gpio_d);
			write(fd,buffer,sizeof(buffer));
		}
	}*/
	
	
	/* Apparently necessary for the data to be actually written to file */
	fsync(fd);
	udelay(1000000);
	
	/* This causes the device to shut off instantly
	
	GPIOF_OUTPUT_VAL = 0;
	*/
	
	close(fd);
	udelay(1000000);
}

/* These functions are present in the firmware library, but we reimplement
   them here because the originals do a lot more than we want */

void reset_poweroff_timer(void)
{
}

int dbg_ports(void)
{
   return 0;
}

void mpeg_stop(void)
{
}

void usb_acknowledge(void)
{
}

void usb_wait_for_disconnect(void)
{
}

void sys_poweroff(void)
{
}
