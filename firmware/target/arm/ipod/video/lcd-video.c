/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * LCD driver for iPod Video
 * 
 * Based on code from the ipodlinux project - http://ipodlinux.org/
 * Adapted for Rockbox in December 2005
 *
 * Original file: linux/arch/armnommu/mach-ipod/fb.c
 *
 * Copyright (c) 2003-2005 Bernard Leach (leachbj@bouncycastle.org)
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
#include "config.h"
#include "cpu.h"
#include "lcd.h"
#include "kernel.h"
#include "system.h"
#ifdef HAVE_LCD_SLEEP
/* Included only for lcd_awake() prototype */
#include "backlight-target.h"
#endif

/* The BCM bus width is 16 bits. But since the low address bits aren't decoded
 * by the chip (the 3 BCM address bits are mapped to address bits 16..18 of the
 * PP5022), writing 32 bits (and even more, using 'stmia') at once works. */
#define BCM_DATA      (*(volatile unsigned short*)(0x30000000))
#define BCM_DATA32    (*(volatile unsigned long *)(0x30000000))
#define BCM_WR_ADDR   (*(volatile unsigned short*)(0x30010000))
#define BCM_WR_ADDR32 (*(volatile unsigned long *)(0x30010000))
#define BCM_RD_ADDR   (*(volatile unsigned short*)(0x30020000))
#define BCM_RD_ADDR32 (*(volatile unsigned long *)(0x30020000))
#define BCM_CONTROL   (*(volatile unsigned short*)(0x30030000))

#define BCM_ALT_DATA      (*(volatile unsigned short*)(0x30040000))
#define BCM_ALT_DATA32    (*(volatile unsigned long *)(0x30040000))
#define BCM_ALT_WR_ADDR   (*(volatile unsigned short*)(0x30050000))
#define BCM_ALT_WR_ADDR32 (*(volatile unsigned long *)(0x30050000))
#define BCM_ALT_RD_ADDR   (*(volatile unsigned short*)(0x30060000))
#define BCM_ALT_RD_ADDR32 (*(volatile unsigned long *)(0x30060000))
#define BCM_ALT_CONTROL   (*(volatile unsigned short*)(0x30070000))

/* Time until the BCM is considered stalled and will be re-kicked.
 * Must be guaranteed to be >~ 20ms. */
#define BCM_UPDATE_TIMEOUT (HZ/20)  

/* Addresses within BCM */
#define BCMA_SRAM_BASE   0
#define BCMA_COMMAND     0x1F8
#define BCMA_STATUS      0x1FC
#define BCMA_CMDPARAM    0xE0000    /* Parameters/data for commands */
#define BCMA_SDRAM_BASE  0xC0000000
#define BCMA_TV_FB       0xC0000000 /* TV out framebuffer */
#define BCMA_TV_BMPDATA  0xC0200000 /* BMP data for TV out functions */

/* BCM commands.  Write them to BCMA_COMMAND.  Note BCM_CMD encoding. */
#define BCM_CMD(x) ((~((unsigned long)x) << 16) | ((unsigned long)x))
#define BCMCMD_LCD_UPDATE     BCM_CMD(0)
/* Execute "M25 Diagnostics".  Status displayed on LCD.  Takes <40s */
#define BCMCMD_SELFTEST       BCM_CMD(1)
#define BCMCMD_TV_PALBMP      BCM_CMD(2)
#define BCMCMD_TV_NTSCBMP     BCM_CMD(3)
/* BCM_CMD(4) may be another TV-related command */
/* The following might do more depending on word at 0xE00000 */
#define BCMCMD_LCD_UPDATERECT BCM_CMD(5)
#define BCMCMD_LCD_SLEEP      BCM_CMD(8)
/* BCM_CMD(12) involved in shutdown */
/* Macrovision analog copy prevention is on by default on TV output.  
   Execute this command after enabling TV out to turn it off. 
 */
#define BCMCMD_TV_MVOFF       BCM_CMD(14) 

enum lcd_status {
    LCD_IDLE,
    LCD_INITIAL,
    LCD_NEED_UPDATE,
    LCD_UPDATING
};

struct {
    long update_timeout;
    enum lcd_status state;
    bool blocked;
#if NUM_CORES > 1
    struct corelock cl;   /* inter-core sync */
#endif
#ifdef HAVE_LCD_SLEEP
    bool display_on;
#endif
} lcd_state IBSS_ATTR;

#ifdef HAVE_LCD_SLEEP
static long bcm_off_wait;
const fb_data *flash_vmcs_offset;
unsigned flash_vmcs_length;
/* This mutex exists because enabling the backlight by changing a setting
   will cause multiple concurrent lcd_wake() calls.
 */
static struct mutex lcdstate_lock SHAREDBSS_ATTR;

#define ROM_BASE        0x20000000
#define ROM_ID(a,b,c,d) (unsigned int)(  ((unsigned int)(d))        | \
                                        (((unsigned int)(c)) << 8)  | \
                                        (((unsigned int)(b)) << 16) | \
                                        (((unsigned int)(a)) << 24) )

/* Get address and length of iPod flash section.
   Based on part of FS#6721.  This may belong elsewhere.
   (BCM initialization uploads the vmcs section to the BCM.)
 */
bool flash_get_section(const unsigned int imageid, 
                       void **offset,
                       unsigned int *length)
{
    unsigned long *p = (unsigned long*)(ROM_BASE + 0xffe00);
    unsigned char *csp, *csend;
    unsigned long checksum;

    /* Find the image in the directory */
    while (1) {
        if (p[0] != ROM_ID('f','l','s','h'))
            return false;
        if (p[1] == imageid)
            break;
        p += 10;
    }

    *offset = (void *)(ROM_BASE + p[3]);
    *length = p[4];

    /* Verify checksum.  Probably unnecessary, but it's fast. */
    checksum = 0;
    csend = (unsigned char *)(ROM_BASE + p[3] + p[4]);
    for(csp = (unsigned char *)(ROM_BASE + p[3]); csp < csend; csp++) {
        checksum += *csp;
    }

    return checksum == p[7];
}
#endif /* HAVE_LCD_SLEEP */

static inline void bcm_write_addr(unsigned address)
{
    BCM_WR_ADDR32 = address;       /* write destination address */

    while (!(BCM_CONTROL & 0x2));  /* wait for it to be write ready */
}

static inline void bcm_write32(unsigned address, unsigned value)
{

    bcm_write_addr(address);       /* set destination address */

    BCM_DATA32 = value;            /* write value */
}

static inline unsigned bcm_read32(unsigned address)
{
    while (!(BCM_RD_ADDR & 1));

    BCM_RD_ADDR32 = address;       /* write source address */

    while (!(BCM_CONTROL & 0x10)); /* wait for it to be read ready */

    return BCM_DATA32;             /* read value */
}

#ifndef BOOTLOADER
static void lcd_tick(void)
{
    /* No core level interrupt mask - already in interrupt context */
#if NUM_CORES > 1
    corelock_lock(&lcd_state.cl);
#endif

    if (!lcd_state.blocked && lcd_state.state >= LCD_NEED_UPDATE)
    {
        unsigned data = bcm_read32(BCMA_COMMAND);
        bool bcm_is_busy = (data == BCMCMD_LCD_UPDATE || data == 0xFFFF);
        
        if (((lcd_state.state == LCD_NEED_UPDATE) && !bcm_is_busy) 
            /* Update requested and BCM is no longer busy. */
         || (TIME_AFTER(current_tick, lcd_state.update_timeout) && bcm_is_busy))
            /* BCM still busy after timeout, i.e. stalled. */
        {
            bcm_write32(BCMA_COMMAND, BCMCMD_LCD_UPDATE);  /* Kick off update */
            BCM_CONTROL = 0x31;
            lcd_state.update_timeout = current_tick + BCM_UPDATE_TIMEOUT;
            lcd_state.state = LCD_UPDATING;
        }
        else if ((lcd_state.state == LCD_UPDATING) && !bcm_is_busy)
        {
            /* Update finished properly and no new update pending. */
            lcd_state.state = LCD_IDLE;
        }
    }
#if NUM_CORES > 1
    corelock_unlock(&lcd_state.cl);
#endif
}

static inline void lcd_block_tick(void)
{
    int oldlevel = disable_irq_save();

#if NUM_CORES > 1
    corelock_lock(&lcd_state.cl);
    lcd_state.blocked = true;
    corelock_unlock(&lcd_state.cl);
#else
    lcd_state.blocked = true;
#endif
    restore_irq(oldlevel);
}

static void lcd_unblock_and_update(void)
{
    unsigned data;
    bool bcm_is_busy;
    int oldlevel = disable_irq_save();

#if NUM_CORES > 1
    corelock_lock(&lcd_state.cl);
#endif
    data = bcm_read32(BCMA_COMMAND);
    bcm_is_busy = (data == BCMCMD_LCD_UPDATE || data == 0xFFFF);
    
    if (!bcm_is_busy || (lcd_state.state == LCD_INITIAL) ||
        TIME_AFTER(current_tick, lcd_state.update_timeout))
    {
        bcm_write32(BCMA_COMMAND, BCMCMD_LCD_UPDATE);  /* Kick off update */
        BCM_CONTROL = 0x31;
        lcd_state.update_timeout = current_tick + BCM_UPDATE_TIMEOUT;
        lcd_state.state = LCD_UPDATING;
    }
    else
    {
         lcd_state.state = LCD_NEED_UPDATE; /* Post update request */
    }
    lcd_state.blocked = false;
    
#if NUM_CORES > 1
    corelock_unlock(&lcd_state.cl);
#endif
    restore_irq(oldlevel);
}

#else /* BOOTLOADER */

#define lcd_block_tick()

static void lcd_unblock_and_update(void)
{
    unsigned data;
    
    if (lcd_state.state != LCD_INITIAL)
    {
        data = bcm_read32(BCMA_COMMAND);
        while (data == BCMCMD_LCD_UPDATE || data == 0xFFFF)
        {
            yield();
            data = bcm_read32(BCMA_COMMAND);
        }
    }
    bcm_write32(BCMA_COMMAND, BCMCMD_LCD_UPDATE);  /* Kick off update */
    BCM_CONTROL = 0x31;
    lcd_state.state = LCD_IDLE;
}
#endif /* BOOTLOADER */

/*** hardware configuration ***/

void lcd_set_contrast(int val)
{
  /* TODO: Implement lcd_set_contrast() */
  (void)val;
}

void lcd_set_invert_display(bool yesno)
{
  /* TODO: Implement lcd_set_invert_display() */
  (void)yesno;
}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
  /* TODO: Implement lcd_set_flip() */
  (void)yesno;
}

/* LCD init */
void lcd_init_device(void)
{
    /* These port initializations are supposed to be done when initializing
       the BCM.  None of it is changed when shutting down the BCM.
     */
    GPO32_ENABLE |= 0x4000;
    /* GPO32_VAL & 0x8000 may supply power for BCM sleep state */
    GPO32_ENABLE |= 0x8000;
    GPO32_VAL &= ~0x8000;
    GPIO_CLEAR_BITWISE(GPIOC_ENABLE, 0x80);  
    /* This pin is used for BCM interrupts */
    GPIOC_ENABLE |= 0x40;
    GPIOC_OUTPUT_EN &= ~0x40;
    GPO32_ENABLE &= ~1;

    lcd_state.blocked = false;
    lcd_state.state = LCD_INITIAL;
#ifndef BOOTLOADER
#if NUM_CORES > 1
    corelock_init(&lcd_state.cl);
#endif
#ifdef HAVE_LCD_SLEEP
    lcd_state.display_on = true;    /* Code in flash turned it on */
    if (!flash_get_section(ROM_ID('v', 'm', 'c', 's'), 
                           (void **)(&flash_vmcs_offset), &flash_vmcs_length))
        /* BCM cannot be shut down because firmware wasn't found */
        flash_vmcs_length = 0;
    else {
        /* lcd_write_data needs an even number of 16 bit values */
        flash_vmcs_length = ((flash_vmcs_length + 3) >> 1) & ~1;
    }
    mutex_init(&lcdstate_lock);
#endif
    tick_add_task(&lcd_tick);
#endif /* !BOOTLOADER */
}

/*** update functions ***/

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    const fb_data *addr;
    unsigned bcmaddr;

#ifdef HAVE_LCD_SLEEP
    if (!lcd_state.display_on)
        return;
#endif

    if (x + width >= LCD_WIDTH)
        width = LCD_WIDTH - x;
    if (y + height >= LCD_HEIGHT)
        height = LCD_HEIGHT - y;
        
    if ((width <= 0) || (height <= 0))
        return; /* Nothing left to do. */

    /* Ensure x and width are both even. The BCM doesn't like small unaligned
     * writes and would just ignore them. */
    width = (width + (x & 1) + 1) & ~1;
    x &= ~1;
    
    /* Prevent the tick from triggering BCM updates while we're writing. */
    lcd_block_tick();

    addr = &lcd_framebuffer[y][x];
    bcmaddr = BCMA_CMDPARAM + (LCD_WIDTH*2) * y + (x << 1);

    if (width == LCD_WIDTH)
    {
        bcm_write_addr(bcmaddr);
        lcd_write_data(addr, width * height);
    }
    else
    {
        do
        {
            bcm_write_addr(bcmaddr);
            bcmaddr += (LCD_WIDTH*2);
            lcd_write_data(addr, width);
            addr += LCD_WIDTH;
        }
        while (--height > 0);
    }
    lcd_unblock_and_update();
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

/* Line write helper function for lcd_yuv_blit. Writes two lines of yuv420. */
extern void lcd_write_yuv420_lines(unsigned char const * const src[3],
                                   unsigned bcmaddr,
                                   int width,
                                   int stride);
                  
/* Performance function to blit a YUV bitmap directly to the LCD */
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    unsigned bcmaddr;
    off_t z;
    unsigned char const * yuv_src[3];

#ifdef HAVE_LCD_SLEEP
    if (!lcd_state.display_on)
        return;
#endif

    /* Sorry, but width and height must be >= 2 or else */
    width &= ~1;

    z = stride * src_y;
    yuv_src[0] = src[0] + z + src_x;
    yuv_src[1] = src[1] + (z >> 2) + (src_x >> 1);
    yuv_src[2] = src[2] + (yuv_src[1] - src[1]);

    /* Prevent the tick from triggering BCM updates while we're writing. */
    lcd_block_tick();

    bcmaddr = BCMA_CMDPARAM + (LCD_WIDTH*2) * y + (x << 1);
    height >>= 1;

    do
    {
        lcd_write_yuv420_lines(yuv_src, bcmaddr, width, stride);
        bcmaddr += (LCD_WIDTH*4);  /* Skip up two lines */
        yuv_src[0] += stride << 1;
        yuv_src[1] += stride >> 1; /* Skip down one chroma line */
        yuv_src[2] += stride >> 1;
    } 
    while (--height > 0);
    
    lcd_unblock_and_update();
}

#ifdef HAVE_LCD_SLEEP
/* Executes a BCM command immediately and waits for it to complete. 
   Other BCM commands (eg. LCD updates or lcd_tick) must not interfere.
 */
static void bcm_command(unsigned cmd) {
    unsigned status;
    
    bcm_write32(BCMA_COMMAND,  cmd);

    BCM_CONTROL = 0x31;

    while (1) {
        status = bcm_read32(BCMA_COMMAND);
        if (status != cmd && status != 0xFFFF)
            break;
        yield();
    }
}

void bcm_powerdown(void)
{
    bcm_write32(0x10001400, bcm_read32(0x10001400) & ~0xF0);

    /* Blanks the LCD and decreases power consumption 
       below what clearing the LCD would achieve.
       Executing an LCD update command wakes it.
     */
    bcm_command(BCMCMD_LCD_SLEEP);

    /* Not sure if this does anything */
    bcm_command(BCM_CMD(0xC));

    /* Further cuts power use, probably by powering down BCM.
       After this point, BCM needs to be bootstrapped
     */
    GPO32_VAL &= ~0x4000;
}

/* Data written to BCM_CONTROL and BCM_ALT_CONTROL */
const unsigned char bcm_bootstrapdata[] = {
    0xA1, 0x81, 0x91, 0x02, 0x12, 0x22, 0x72, 0x62
};

void bcm_init(void) 
{
    int i;

    /* Power up BCM */
    GPO32_VAL |= 0x4000;

    /* Changed from HZ/2 to speed up this function */
    sleep(HZ/8); 

    /* Bootstrap stage 1 */

    STRAP_OPT_A &= ~0xF00;
    outl(0x1313, 0x70000040);

    /* Interrupt-related code for future use
       GPIOC_INT_LEV |= 0x40;
       GPIOC_INT_EN |= 0x40;
       CPU_HI_INT_EN |= 0x40000; 
    */

    /* Bootstrap stage 2 */
        
    for (i = 0; i < 8; i++) {
        BCM_CONTROL = bcm_bootstrapdata[i];
    }

    for (i = 3; i < 8; i++) {
        BCM_ALT_CONTROL = bcm_bootstrapdata[i];
    }
     
    while ((BCM_RD_ADDR & 1) == 0 || (BCM_ALT_RD_ADDR & 1) == 0)
        yield();
    
    (void)BCM_WR_ADDR;
    (void)BCM_ALT_WR_ADDR;
    
    /* Bootstrap stage 3: upload firmware */
    
    while (BCM_ALT_CONTROL & 0x80)
        yield();

    while (!(BCM_ALT_CONTROL & 0x40))
        yield();

    /* Upload firmware to BCM SRAM */
    bcm_write_addr(BCMA_SRAM_BASE);
    lcd_write_data(flash_vmcs_offset, flash_vmcs_length);

    bcm_write32(BCMA_COMMAND,  0);
    bcm_write32(0x10000C00, 0xC0000000);
    
    while (!(bcm_read32(0x10000C00) & 1))
        yield();
    
    bcm_write32(0x10000C00, 0);
    bcm_write32(0x10000400, 0xA5A50002);
    
    while (bcm_read32(BCMA_COMMAND) == 0)
        yield();

    /* sleep(HZ/2) apparently unneeded */
}

void lcd_awake(void)
{
    mutex_lock(&lcdstate_lock);
    if (!lcd_state.display_on && flash_vmcs_length != 0)
    {
        /* Ensure BCM has been off for 1/2 s at least */
        while (!TIME_AFTER(current_tick, lcd_state.update_timeout))
            yield();

        bcm_init();

        /* Update LCD, but don't use lcd_update().  Instead, wait here
           until the command completes so LCD isn't white when the 
           backlight turns on
         */
        bcm_write_addr(BCMA_CMDPARAM);
        lcd_write_data(&(lcd_framebuffer[0][0]), LCD_WIDTH * LCD_HEIGHT);
        bcm_command(BCMCMD_LCD_UPDATE);

        lcd_state.state = LCD_INITIAL;
        tick_add_task(&lcd_tick);
        lcd_state.display_on = true;
        /* Note that only the RGB data from lcd_framebuffer has been 
           displayed.  If YUV data was displayed, it needs to be updated
           now.  (eg. see lcd_call_enable_hook())
         */
    }
    mutex_unlock(&lcdstate_lock);
}

void lcd_sleep(void) 
{
    mutex_lock(&lcdstate_lock);
    if (lcd_state.display_on && flash_vmcs_length != 0) {
        lcd_state.display_on = false;
        
        /* Wait for BCM to finish work */
        while (lcd_state.state != LCD_INITIAL && lcd_state.state != LCD_IDLE)
            yield();
        
        tick_remove_task(&lcd_tick);
        bcm_powerdown();
        bcm_off_wait = current_tick + HZ/2;
    }
    mutex_unlock(&lcdstate_lock);
}

#ifdef HAVE_LCD_SHUTDOWN
void lcd_shutdown(void) 
{
    lcd_sleep();
}
#endif /* HAVE_LCD_SHUTDOWN */
#endif /* HAVE_LCD_SLEEP */
