/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Randy D. Wood
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
#include "lcd.h"
#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif
#include "scroll_engine.h"
#include "thread.h"
#include "kernel.h"
#include "button.h"
#include "file.h"
#include "audio.h"
#include "system.h"
#include "i2c.h"
#include "adc.h"
#include "string.h"
#include "core_alloc.h"
#include "storage.h"
#include "rolo.h"

#include "loader_strerror.h"
#if defined(MI4_FORMAT)
#include "mi4-loader.h"
#define LOAD_FIRMWARE(a,b,c) load_mi4(a,b,c)
#elif defined(RKW_FORMAT)
#include "rkw-loader.h"
#define LOAD_FIRMWARE(a,b,c) load_rkw(a,b,c)
#else
#include "rb-loader.h"
#define LOAD_FIRMWARE(a,b,c) load_firmware(a,b,c)
#endif

#if defined(HAVE_BOOTDATA) && !defined(SIMULATOR)
#include "multiboot.h"
#include "bootdata.h"
#include "crc32.h"
#endif

#if CONFIG_CPU == AS3525v2
#include "ascodec.h"
#endif

#if defined(FIIO_M3K)
#include "backlight-target.h"
#endif

#define IRQ0_EDGE_TRIGGER 0x80

static int rolo_handle;
#ifdef CPU_PP
/* Handle the COP properly - it needs to jump to a function outside SDRAM while
 * the new firmware is being loaded, and then jump to the start of SDRAM
 * TODO: Use the mailboxes built into the PP processor for this
 */

#if NUM_CORES > 1
volatile unsigned char IDATA_ATTR cpu_message = 0;
volatile unsigned char IDATA_ATTR cpu_reply = 0;
extern int cop_idlestackbegin[];

void rolo_restart_cop(void) ICODE_ATTR;
void rolo_restart_cop(void)
{
    if (CURRENT_CORE == CPU)
    {
        /* There should be free thread slots aplenty */
        create_thread(rolo_restart_cop, cop_idlestackbegin, IDLE_STACK_SIZE,
                      0, "rolo COP" IF_PRIO(, PRIORITY_REALTIME)
                      IF_COP(, COP));
        return;
    }

    COP_INT_DIS = -1;

    /* Invalidate cache */
    commit_discard_idcache();

    /* Disable cache */
    CACHE_CTL = CACHE_CTL_DISABLE;

    /* Tell the main core that we're ready to reload */
    cpu_reply = 1;

    /* Wait while RoLo loads the image into SDRAM */
    /* TODO: Accept checksum failure gracefully */
    while(cpu_message != 1);

    /* Acknowledge the CPU and then reload */
    cpu_reply = 2;

    asm volatile(
        "bx    %0   \n"
        : : "r"(DRAM_START)
    );
}
#endif /* NUM_CORES > 1 */
#endif /* CPU_PP */

static void rolo_error(const char *text)
{
    rolo_handle = core_free(rolo_handle);
    lcd_clear_display();
    lcd_puts(0, 0, "ROLO error:");
    lcd_puts_scroll(0, 1, text);
    lcd_update();
    button_get(true);
    button_get(true);
    button_get(true);
    lcd_scroll_stop();
}

#if CONFIG_CPU == IMX31L || CONFIG_CPU == RK27XX || CONFIG_CPU == X1000
/* this is in firmware/target/arm/imx31/rolo_restart.c for IMX31 */
/* this is in firmware/target/arm/rk27xx/rolo_restart.c for rk27xx */
/* this is in firmware/target/mips/ingenic_x1000/boot-x1000.c for X1000 */
extern void rolo_restart(const unsigned char* source, unsigned char* dest,
                         int length);
#else

/* explicitly put this code in iram, ICODE_ATTR is defined to be null for some
   targets that are low on iram, like the gigabeat F/X */
void rolo_restart(const unsigned char* source, unsigned char* dest,
                  long length) __attribute__ ((section(".icode")));
void rolo_restart(const unsigned char* source, unsigned char* dest,
                  long length)
{
    long i;
    unsigned char* localdest = dest;

    /* This is the equivalent of a call to memcpy() but this must be done from
       iram to avoid overwriting itself and we don't want to depend on memcpy()
       always being in iram */
    for(i = 0;i < length;i++)
        *localdest++ = *source++;

#if defined(CPU_COLDFIRE)
    asm (
        "movec.l %0,%%vbr    \n"
        "move.l  (%0)+,%%sp  \n"
        "move.l  (%0),%0     \n"
        "jmp     (%0)        \n"
        : : "a"(dest)
    );
#elif defined(CPU_PP)
    CPU_INT_DIS = -1;

    /* Flush cache */
    commit_discard_idcache();

    /* Disable cache */
    CACHE_CTL = CACHE_CTL_DISABLE;

    /* Reset the memory mapping registers to zero */
    {
        volatile unsigned long *mmap_reg;
        for (mmap_reg = &MMAP_FIRST; mmap_reg <= &MMAP_LAST; mmap_reg++)
            *mmap_reg = 0;
    }

#if NUM_CORES > 1
    /* Tell the COP it's safe to continue rebooting */
    cpu_message = 1;

    /* Wait for the COP to tell us it is rebooting */
    while(cpu_reply != 2);
#endif

    asm volatile(
        "bx    %0   \n"
        : : "r"(DRAM_START)
    );

#elif defined(CPU_ARM)
    /* Flush and invalidate caches */
    commit_discard_idcache();
    asm volatile(
        "bx    %0   \n"
        : : "r"(dest)
    );
#elif defined(CPU_MIPS)
    commit_discard_idcache();
    asm volatile(
        "jr     %0               \n"
        "nop\n"
        : : "r"(dest)
    );
#endif
}
#endif

/* This is assigned in the linker control file */
extern unsigned long loadaddress;

/***************************************************************************
 *
 * Name: rolo_load(const char *filename)
 * Filename must be a fully defined filename including the path and extension
 *
 ***************************************************************************/
int rolo_load(const char* filename)
{
    unsigned char* ramstart = (void*)&loadaddress;
    unsigned char* filebuf;
    size_t filebuf_size;
    int err, length;

    lcd_clear_display();
    lcd_puts(0, 0, "ROLO...");
    lcd_puts(0, 1, "Loading");
    lcd_update();
#ifdef HAVE_REMOTE_LCD
    lcd_remote_clear_display();
    lcd_remote_puts(0, 0, "ROLO...");
    lcd_remote_puts(0, 1, "Loading");
    lcd_remote_update();
#endif

    audio_hard_stop();

    /* get the system buffer. release only in case of error, otherwise
     * we don't return anyway */
    rolo_handle = core_alloc_maximum("rolo", &filebuf_size, &buflib_ops_locked);
    if (rolo_handle < 0)
    {
        rolo_error("OOM");
        return -1;
    }

    filebuf = core_get_data(rolo_handle);

    err = LOAD_FIRMWARE(filebuf, filename, filebuf_size);
#if defined(HAVE_BOOTDATA) && !defined(SIMULATOR)
    /* write the bootdata as if rolo were the bootloader */
    unsigned int crc = 0;
    if (strcmp(filename, BOOTDIR "/" BOOTFILE) == 0)
        crc = crc_32(boot_data.payload, boot_data.length, 0xffffffff);

    if(crc > 0 && crc == boot_data.crc)
        write_bootdata(filebuf, filebuf_size, boot_data.boot_volume); /* rb-loader.c */
#endif

    if (err <= 0)
    {
        rolo_error(loader_strerror(err));
        return -1;
    }
    else
        length = err;

#if defined(CPU_PP) && NUM_CORES > 1
    lcd_puts(0, 2, "Waiting for coprocessor...");
    lcd_update();
    rolo_restart_cop();
    /* Wait for COP to be in safe code */
    while(cpu_reply != 1);
    lcd_puts(0, 2, "                          ");
    lcd_update();
#endif

#ifdef HAVE_STORAGE_FLUSH
    lcd_puts(0, 1, "Flushing storage buffers");
    lcd_update();
    storage_flush();
#endif

    lcd_puts(0, 1, "Executing");
    lcd_update();
#ifdef HAVE_REMOTE_LCD
    lcd_remote_puts(0, 1, "Executing");
    lcd_remote_update();
#endif

#if defined(FIIO_M3K)
    /* Avoids the LCD backlight ramping down & up weirdly */
    backlight_hw_off();
#endif

    adc_close();
#if CONFIG_CPU == AS3525v2
    /* Set CVDD1 power supply to default*/
    ascodec_write_pmu(0x17, 1, 0);
#endif
#if defined(SANSA_FUZEV2) || defined(SANSA_CLIPPLUS) || defined(SANSA_CLIPZIP)
    /* It is necessary for proper detection AMSv2 variant 1.
     * We should restore initial state of GPIOB_PIN(5) as it used for
     * variant detection, but can be changed if we switch SD card. */
    if (amsv2_variant == 1)
        GPIOB_PIN(5) = 1 << 5;
#endif

#if CONFIG_CPU != IMX31L /* We're not finished yet */
#ifdef CPU_ARM
    /* Should do these together since some ARM version should never have
     * FIQ disabled and not IRQ (imx31 errata). */
    disable_interrupt(IRQ_FIQ_STATUS);
#else
    /* Some targets have a higher disable level than HIGEST_IRQ_LEVEL */
    set_irq_level(DISABLE_INTERRUPTS);
#endif
#endif /* CONFIG_CPU == IMX31L */

    rolo_restart(filebuf, ramstart, length);

    /* never reached */
    return 0;
}
