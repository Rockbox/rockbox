/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Heikki Hannikainen
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#ifndef SIMULATOR
#include <stdio.h>
#include <stdbool.h>
#include "lcd.h"
#include "menu.h"
#include "debug_menu.h"
#include "kernel.h"
#include "sprintf.h"
#include "button.h"
#include "adc.h"
#include "mas.h"
#include "power.h"
#include "rtc.h"
#include "debug.h"
#include "thread.h"

/*---------------------------------------------------*/
/*    SPECIAL DEBUG STUFF                            */
/*---------------------------------------------------*/
extern int ata_device;
extern int ata_io_address;
extern int num_threads;
extern char *thread_name[];

#ifdef ARCHOS_RECORDER
/* Test code!!! */
void dbg_os(void)
{
    char buf[32];
    int button;
    int i;
    int usage;

    lcd_clear_display();

    while(1)
    {
	lcd_puts(0, 0, "Stack usage:");
        for(i = 0; i < num_threads;i++)
        {
            usage = thread_stack_usage(i);
	    snprintf(buf, 32, "%s: %d%%", thread_name[i], usage);
	    lcd_puts(0, 1+i, buf);
        }

        lcd_update();
        sleep(HZ/10);

        button = button_get(false);

        switch(button)
        {
            case BUTTON_OFF:
                return;
        }
    }
}
#else
void dbg_os(void)
{
    char buf[32];
    int button;
    int usage;
    int currval = 0;

    lcd_clear_display();

    while(1)
    {
	lcd_puts(0, 0, "Stack usage");

	usage = thread_stack_usage(currval);
	snprintf(buf, 32, "%d: %d%%  ", currval, usage);
	lcd_puts(0, 1, buf);
	
        sleep(HZ/10);

        button = button_get(false);

        switch(button)
        {
            case BUTTON_STOP:
                return;

	    case BUTTON_LEFT:
                currval--;
                if(currval < 0)
                    currval = num_threads-1;
                break;

            case BUTTON_RIGHT:
                currval++;
                if(currval > num_threads-1)
                    currval = 0;
                break;
        }
    }
}
#endif

#ifdef ARCHOS_RECORDER
/* Test code!!! */
void dbg_ports(void)
{
    unsigned short porta;
    unsigned short portb;
    unsigned char portc;
    char buf[32];
    int button;
    int battery_voltage;
    int batt_int, batt_frac;
    bool charge_status = false;
    bool ide_status = true;

    lcd_clear_display();

    while(1)
    {
        porta = PADR;
        portb = PBDR;
        portc = PCDR;

        snprintf(buf, 32, "PADR: %04x", porta);
        lcd_puts(0, 0, buf);
        snprintf(buf, 32, "PBDR: %04x", portb);
        lcd_puts(0, 1, buf);

        snprintf(buf, 32, "AN0: %03x AN4: %03x", adc_read(0), adc_read(4));
        lcd_puts(0, 2, buf);
        snprintf(buf, 32, "AN1: %03x AN5: %03x", adc_read(1), adc_read(5));
        lcd_puts(0, 3, buf);
        snprintf(buf, 32, "AN2: %03x AN6: %03x", adc_read(2), adc_read(6));
        lcd_puts(0, 4, buf);
        snprintf(buf, 32, "AN3: %03x AN7: %03x", adc_read(3), adc_read(7));
        lcd_puts(0, 5, buf);

        battery_voltage = (adc_read(6) * BATTERY_SCALE_FACTOR) / 10000;
        batt_int = battery_voltage / 100;
        batt_frac = battery_voltage % 100;
    
        snprintf(buf, 32, "Batt: %d.%02dV %d%%  ", batt_int, batt_frac,
                 battery_level());
        lcd_puts(0, 6, buf);

        snprintf(buf, 32, "ATA: %s, 0x%x",
                 ata_device?"slave":"master", ata_io_address);
        lcd_puts(0, 7, buf);
	
        lcd_update();
        sleep(HZ/10);

        button = button_get(false);

        switch(button)
        {
            case BUTTON_ON:
                charge_status = charge_status?false:true;
                charger_enable(charge_status);
                break;
                
            case BUTTON_UP:
                ide_status = ide_status?false:true;
                ide_power_enable(ide_status);
                break;

            case BUTTON_OFF:
                charger_enable(false);
                ide_power_enable(true);
                return;
        }
    }
}
#else
void dbg_ports(void)
{
    unsigned short porta;
    unsigned short portb;
    unsigned char portc;
    char buf[32];
    unsigned long crc_count;
    int button;
    int battery_voltage;
    int batt_int, batt_frac;
    int currval = 0;

    lcd_clear_display();

    while(1)
    {
        porta = PADR;
        portb = PBDR;
        portc = PCDR;

        switch(currval)
        {
            case 0:
                snprintf(buf, 32, "PADR: %04x  ", porta);
                break;
            case 1:
                snprintf(buf, 32, "PBDR: %04x  ", portb);
                break;
            case 2:
                snprintf(buf, 32, "AN0: %03x  ", adc_read(0));
                break;
            case 3:
                snprintf(buf, 32, "AN1: %03x  ", adc_read(1));
                break;
            case 4:
                snprintf(buf, 32, "AN2: %03x  ", adc_read(2));
                break;
            case 5:
                snprintf(buf, 32, "AN3: %03x  ", adc_read(3));
                break;
            case 6:
                snprintf(buf, 32, "AN4: %03x  ", adc_read(4));
                break;
            case 7:
                snprintf(buf, 32, "AN5: %03x  ", adc_read(5));
                break;
            case 8:
                snprintf(buf, 32, "AN6: %03x  ", adc_read(6));
                break;
            case 9:
                snprintf(buf, 32, "AN7: %03x  ", adc_read(7));
                break;
            case 10:
                snprintf(buf, 32, "%s, 0x%x ",
                         ata_device?"slv":"mst", ata_io_address);
                break;
            case 11:
                mas_readmem(MAS_BANK_D0, 0x303, &crc_count, 1);
                
                snprintf(buf, 32, "CRC: %d    ", crc_count);
                break;
        }
        lcd_puts(0, 0, buf);
        
        battery_voltage = (adc_read(6) * BATTERY_SCALE_FACTOR) / 10000;
        batt_int = battery_voltage / 100;
        batt_frac = battery_voltage % 100;
    
        snprintf(buf, 32, "Batt: %d.%02dV", batt_int, batt_frac);
        lcd_puts(0, 1, buf);
        
        sleep(HZ/5);

        button = button_get(false);

        switch(button)
        {
            case BUTTON_STOP:
                return;

            case BUTTON_LEFT:
                currval--;
                if(currval < 0)
                    currval = 11;
                break;

            case BUTTON_RIGHT:
                currval++;
                if(currval > 11)
                    currval = 0;
                break;
        }
    }
}
#endif

#ifdef HAVE_RTC
/* Read RTC RAM contents and display them */
void dbg_rtc(void)
{
    char buf[32];
    unsigned char addr = 0, r, c;
    int i;
    int button;

    lcd_clear_display();
    lcd_puts(0, 0, "RTC read:");

    while(1)
    {
        for (r = 0; r < 4; r++) {
            snprintf(buf, 10, "0x%02x: ", addr + r*4);
            for (c = 0; c <= 3; c++) {
                i = rtc_read(addr + r*4 + c);
                snprintf(buf + 6 + c*2, 3, "%02x", i);
            }
            lcd_puts(1, r+1, buf);
        }
        
        lcd_update();
        sleep(HZ/2);

        button = button_get(false);

        switch(button)
        {
            case BUTTON_DOWN:
                if (addr < 63-16) { addr += 16; }
                break;
            case BUTTON_UP:
                if (addr) { addr -= 16; }
                break;
            case BUTTON_F2:
                /* clear the user RAM space */
                for (c = 0; c <= 43; c++)
                    rtc_write(0x18 + c, 0);
                break;
            case BUTTON_OFF:
            case BUTTON_LEFT:
                return;
        }
    }
}
#else
void dbg_rtc(void)
{
    return;
}
#endif

/* Read MAS registers and display them */
void dbg_mas(void)
{
    char buf[32];
    unsigned int addr = 0, r, i;
    int button;

    lcd_clear_display();
    lcd_puts(0, 0, "MAS register read:");

    while(1)
    {
        for (r = 0; r < 4; r++) {
            i = mas_readreg(addr + r);
            snprintf(buf, 30, "0x%02x: %08x", addr + r, i);
            lcd_puts(1, r+1, buf);
        }
        
        lcd_update();
        sleep(HZ/16);

        button = button_get(false);

        switch(button)
        {
            case BUTTON_DOWN:
                addr += 4;
                break;
            case BUTTON_UP:
                if (addr) { addr -= 4; }
                break;
            case BUTTON_LEFT:
                return;
        }
    }
}

#ifdef ARCHOS_RECORDER
void dbg_mas_codec(void)
{
    char buf[32];
    unsigned int addr = 0, r, i;
    int button;

    lcd_clear_display();
    lcd_puts(0, 0, "MAS codec reg read:");

    while(1)
    {
        for (r = 0; r < 4; r++) {
            i = mas_codec_readreg(addr + r);
            snprintf(buf, 30, "0x%02x: %08x", addr + r, i);
            lcd_puts(1, r+1, buf);
        }
        
        lcd_update();
        sleep(HZ/16);

        button = button_get(false);

        switch(button)
        {
            case BUTTON_DOWN:
                addr += 4;
                break;
            case BUTTON_UP:
                if (addr) { addr -= 4; }
                break;
            case BUTTON_LEFT:
                return;
        }
    }
}
#endif

void debug_menu(void)
{
    int m;

    struct menu_items items[] = {
        { "View I/O ports", dbg_ports },
#ifdef HAVE_LCD_BITMAP
#ifdef HAVE_RTC
        { "View/clr RTC RAM", dbg_rtc },
#endif /* HAVE_RTC */
#endif /* HAVE_LCD_BITMAP */
        { "View OS stacks", dbg_os },
        { "View MAS regs", dbg_mas },
#ifdef ARCHOS_RECORDER
        { "View MAS codec", dbg_mas_codec },
#endif
    };

    m=menu_init( items, sizeof items / sizeof(struct menu_items) );
    menu_run(m);
    menu_exit(m);
}

#endif /* SIMULATOR */

