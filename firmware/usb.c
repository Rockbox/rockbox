/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "cpu.h"
#include "kernel.h"
#include "thread.h"
#include "system.h"
#include "debug.h"
#include "ata.h"
#include "fat.h"
#include "disk.h"
#include "panic.h"
#include "lcd.h"
#include "adc.h"
#include "usb.h"
#include "button.h"
#include "sprintf.h"
#include "hwcompat.h"
#ifdef HAVE_MMC
#include "ata_mmc.h"
#endif

extern void dbg_ports(void); /* NASTY! defined in apps/ */

#ifdef HAVE_LCD_BITMAP
bool do_screendump_instead_of_usb = false;
void screen_dump(void);   /* Nasty again. Defined in apps/ too */
#endif

#define USB_REALLY_BRAVE

#if !defined(SIMULATOR) && !defined(USB_NONE)

/* Messages from usb_tick */
#define USB_INSERTED    1
#define USB_EXTRACTED   2
#ifdef HAVE_MMC
#define USB_REENABLE    3
#endif

/* Thread states */
#define EXTRACTING      1
#define EXTRACTED       2
#define INSERTED        3
#define INSERTING       4

/* The ADC tick reads one channel per tick, and we want to check 3 successive
   readings on the USB voltage channel. This doesn't apply to the Player, but
   debouncing the USB detection port won't hurt us either. */
#define NUM_POLL_READINGS (NUM_ADC_CHANNELS * 3)
static int countdown;

static int usb_state;

#ifdef HAVE_MMC
static int usb_mmc_countdown = 0;
#endif

/* FIXME: The extra 0x400 is consumed by fat_mount() when the fsinfo
   needs updating */
static char usb_stack[DEFAULT_STACK_SIZE + 0x400];
static const char usb_thread_name[] = "usb";
static struct event_queue usb_queue;
static bool last_usb_status;
static bool usb_monitor_enabled;

static void usb_enable(bool on)
{
#ifdef USB_ENABLE_ONDIOSTYLE
    PACR2 &= ~0x04C0; /* use PA3, PA5 as GPIO */
    if(on)
    {
#ifdef HAVE_MMC
        mmc_select_clock(mmc_detect() ? 1 : 0);
#endif
        if (!(read_hw_mask() & MMC_CLOCK_POLARITY))
            and_b(~0x20, &PBDRH); /* old circuit needs SCK1 = low while on USB */
        or_b(0x20, &PADRL); /* enable USB */
        and_b(~0x08, &PADRL); /* assert card detect */
    }
    else
    {
        if (!(read_hw_mask() & MMC_CLOCK_POLARITY))
            or_b(0x20, &PBDRH); /* reset SCK1 = high for old circuit */
        and_b(~0x20, &PADRL); /* disable USB */
        or_b(0x08, &PADRL); /* deassert card detect */
    }
    or_b(0x28, &PAIORL); /* output for USB enable and card detect */
#else /* standard HD Jukebox */
#ifdef HAVE_LCD_BITMAP
    if(read_hw_mask() & USB_ACTIVE_HIGH)
        on = !on;
#endif
    if(on)
    {
        and_b(~0x04, &PADRH); /* enable USB */
    }
    else
    {
        or_b(0x04, &PADRH);
    }
    or_b(0x04, &PAIORH);
#endif
}

static void usb_slave_mode(bool on)
{
    int rc;
    struct partinfo* pinfo;
    
    if(on)
    {
        DEBUGF("Entering USB slave mode\n");
        ata_soft_reset();
        ata_init();
        ata_standby(15);
        ata_enable(false);
        usb_enable(true);
    }
    else
    {
        int i;
        DEBUGF("Leaving USB slave mode\n");
        
        /* Let the ISDx00 settle */
        sleep(HZ*1);
        
        usb_enable(false);

        rc = ata_init();
        if(rc)
        {
            char str[32];
            lcd_clear_display();
            snprintf(str, 31, "ATA error: %d", rc);
            lcd_puts(0, 0, str);
            lcd_puts(0, 1, "Press ON to debug");
            lcd_update();
            while(!(button_get(true) & BUTTON_REL)) {};
            dbg_ports();
            panicf("ata: %d",rc);
        }
    
        pinfo = disk_init(IF_MV(0));
        if (!pinfo)
            panicf("disk: NULL");
    
        fat_init();
        for ( i=0; i<4; i++ ) {
            rc = fat_mount(IF_MV2(0,) IF_MV2(0,) pinfo[i].start);
            if (!rc)
                break; /* only one partition gets mounted as of now */
        }
        if (i==4)
            panicf("mount: %d",rc);
#ifdef HAVE_MULTIVOLUME
        /* mount partition on the optional volume */
#ifdef HAVE_MMC
        if (mmc_detect()) /* for Ondio, only if card detected */
#endif
        {
            pinfo = disk_init(1);
            if (pinfo)
            {
                for ( i=0; i<4; i++ ) {
                    if (!fat_mount(1, 1, pinfo[i].start))
                        break; /* only one partition gets mounted as of now */
                }

                if ( i==4 ) {
                    rc = fat_mount(1, 1, 0);
                }
            }
        }
#endif /* #ifdef HAVE_MULTIVOLUME */
    }
}

static void usb_thread(void)
{
    int num_acks_to_expect = -1;
    bool waiting_for_ack;
    struct event ev;

    waiting_for_ack = false;

    while(1)
    {
        queue_wait(&usb_queue, &ev);
        switch(ev.id)
        {
            case USB_INSERTED:
#ifdef HAVE_LCD_BITMAP
                if(do_screendump_instead_of_usb)
                {
                    screen_dump();
                }
                else
                {
#endif
                    /* Tell all threads that they have to back off the ATA.
                       We subtract one for our own thread. */
                    num_acks_to_expect =
                        queue_broadcast(SYS_USB_CONNECTED, NULL) - 1;
                    waiting_for_ack = true;
                    DEBUGF("USB inserted. Waiting for ack from %d threads...\n",
                           num_acks_to_expect);
#ifdef HAVE_LCD_BITMAP
                }
#endif
                break;
       
            case SYS_USB_CONNECTED_ACK:
                if(waiting_for_ack)
                {
                    num_acks_to_expect--;
                    if(num_acks_to_expect == 0)
                    {
                        DEBUGF("All threads have acknowledged the connect.\n");
#ifdef USB_REALLY_BRAVE
                        usb_slave_mode(true);
                        usb_state = USB_INSERTED;
#else
                        system_reboot();
#endif
                    }
                    else
                    {
                        DEBUGF("usb: got ack, %d to go...\n",
                               num_acks_to_expect);
                    }
                }
                break;

            case USB_EXTRACTED:
#ifdef HAVE_LCD_BITMAP
                if(!do_screendump_instead_of_usb)
                {
#endif
                    if(usb_state == USB_INSERTED)
                    {
                        /* Only disable the USB mode if we really have enabled it
                           some threads might not have acknowledged the
                           insertion */
                        usb_slave_mode(false);
                    }

                    usb_state = USB_EXTRACTED;
                    
                    /* Tell all threads that we are back in business */
                    num_acks_to_expect =
                        queue_broadcast(SYS_USB_DISCONNECTED, NULL) - 1;
                    waiting_for_ack = true;
                    DEBUGF("USB extracted. Waiting for ack from %d threads...\n",
                           num_acks_to_expect);
#ifdef HAVE_LCD_CHARCELLS
                    lcd_icon(ICON_USB, false);
#endif
#ifdef HAVE_LCD_BITMAP
                }
#endif
                break;
        
            case SYS_USB_DISCONNECTED_ACK:
                if(waiting_for_ack)
                {
                    num_acks_to_expect--;
                    if(num_acks_to_expect == 0)
                    {
                        DEBUGF("All threads have acknowledged. "
                               "We're in business.\n");
                    }
                    else
                    {
                        DEBUGF("usb: got ack, %d to go...\n",
                               num_acks_to_expect);
                    }
                }
                break;

#ifdef HAVE_MMC
            case SYS_MMC_INSERTED:
            case SYS_MMC_EXTRACTED:
                if(usb_state == USB_INSERTED)
                {
                    usb_enable(false);
                    usb_mmc_countdown = HZ/2; /* re-enable after 0.5 sec */
                }
                break;

            case USB_REENABLE:
                if(usb_state == USB_INSERTED)
                    usb_enable(true);  /* reenable only if still inserted */
                break;
#endif
        }
    }
}

bool usb_detect(void)
{
    bool current_status;

#ifdef USB_RECORDERSTYLE
    current_status = (adc_read(ADC_USB_POWER) > 500)?true:false;
#endif
#ifdef USB_FMRECORDERSTYLE
    current_status = (adc_read(ADC_USB_POWER) <= 512)?true:false;
#endif
#ifdef USB_PLAYERSTYLE
    current_status = (PADR & 0x8000)?false:true;
#endif
#ifdef IRIVER_H100
    current_status = (GPIO1_READ & 0x80)?true:false;
#endif
    return current_status;
}


static void usb_tick(void)
{
    bool current_status;

    if(usb_monitor_enabled)
    {
        current_status = usb_detect();
    
        /* Only report when the status has changed */
        if(current_status != last_usb_status)
        {
            last_usb_status = current_status;
            countdown = NUM_POLL_READINGS;
        }
        else
        {
            /* Count down until it gets negative */
            if(countdown >= 0)
                countdown--;

            /* Report to the thread if we have had 3 identical status
               readings in a row */
            if(countdown == 0)
            {
                if(current_status)
                    queue_post(&usb_queue, USB_INSERTED, NULL);
                else
                    queue_post(&usb_queue, USB_EXTRACTED, NULL);
            }
        }
    }
#ifdef HAVE_MMC
    if(usb_mmc_countdown > 0)
    {
        usb_mmc_countdown--;
        if (usb_mmc_countdown == 0)
            queue_post(&usb_queue, USB_REENABLE, NULL);
    }
#endif
}

void usb_acknowledge(int id)
{
    queue_post(&usb_queue, id, NULL);
}

void usb_init(void)
{
    usb_state = USB_EXTRACTED;
    usb_monitor_enabled = false;
    countdown = -1;

#ifdef IRIVER_H100
    GPIO1_FUNCTION |= 0x80; /* GPIO39 is the USB detect input */
#endif

    usb_enable(false);

    /* We assume that the USB cable is extracted */
    last_usb_status = false;

    queue_init(&usb_queue);
    create_thread(usb_thread, usb_stack, sizeof(usb_stack), usb_thread_name);

    tick_add_task(usb_tick);
}

void usb_wait_for_disconnect(struct event_queue *q)
{
    struct event ev;

    /* Don't return until we get SYS_USB_DISCONNECTED */
    while(1)
    {
        queue_wait(q, &ev);
        if(ev.id == SYS_USB_DISCONNECTED)
        {
            usb_acknowledge(SYS_USB_DISCONNECTED_ACK);
            return;
        }
    }
}

int usb_wait_for_disconnect_w_tmo(struct event_queue *q, int ticks)
{
    struct event ev;

    /* Don't return until we get SYS_USB_DISCONNECTED or SYS_TIMEOUT */
    while(1)
    {
        queue_wait_w_tmo(q, &ev, ticks);
        switch(ev.id)
        {
            case SYS_USB_DISCONNECTED:
                usb_acknowledge(SYS_USB_DISCONNECTED_ACK);
                return 0;
                break;
            case SYS_TIMEOUT:
                return 1;
                break;
        }
    }
}

void usb_start_monitoring(void)
{
    usb_monitor_enabled = true;
}

bool usb_inserted(void)
{
    return usb_state == USB_INSERTED;
}

#else

#ifdef USB_NONE
bool usb_inserted(void)
{
    return false;
}
#endif

/* Dummy simulator functions */
void usb_acknowledge(int id)
{
    id = id;
}

void usb_init(void)
{
}

void usb_start_monitoring(void)
{
}

bool usb_detect(void)
{
    return false;
}

#endif /* USB_NONE or SIMULATOR */
