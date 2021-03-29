/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 Boris Gjenero
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"

/* This attempts to trigger the PP502x cache bug.
 * It creates a memory dump file if memory corruption is found. */

/* Amount of memory to dump */
#define REALMEMSIZE (32 * 0x100000)

/* Dump and quit after this many mismatches.
 * The user can quit after less.
 * Setting this too high increases probability of 
 * corrupting something important and not getting output. */
#define MAX_MISMATCHES 1000

/* Define to use a loop instead of code repeated inline.
 * Larger code size leads to a lot more trashing. */
/* #define USE_TRASHING_LOOP */

/* Mask for addresses used for buffer activity */
#define ADDRMASK 0xFFFFFF

/* Fill pattern for allocated memory */
#define FILLPATTERN 0x12345678

/* This tries to preserve corrupted areas for later examination in the dump */
#define NOT_TRASHED() \
    (*(volatile long *)UNCACHED_ADDR((long)p + (rand & (ADDRMASK & ~15))) == FILLPATTERN && \
    *(volatile long *)UNCACHED_ADDR((long)p + (rand & (ADDRMASK & ~15)) + 4) == FILLPATTERN && \
    *(volatile long *)UNCACHED_ADDR((long)p + (rand & (ADDRMASK & ~15)) + 8) == FILLPATTERN && \
    *(volatile long *)UNCACHED_ADDR((long)p + (rand & (ADDRMASK & ~15)) + 12) == FILLPATTERN)

#define TRASH() \
    /* Commenting out reading caused a lot more mismatches */ \
    /* rand = rand*0x0019660dL + 0x3c6ef35fL; */ \
    /* (void)*(volatile long*)((long)p + (rand & (ADDRMASK & ~3))); */ \
    if (NOT_TRASHED()) \
        *(volatile long*)((long)p + (rand & (ADDRMASK & ~3))) = FILLPATTERN; \
    rand = rand*0x0019660dL + 0x3c6ef35fL

#ifdef USE_TRASHING_LOOP
#define TRASH256() { \
                       int j; \
                       for (j = 0; j < 256; j++) { \
                           TRASH(); \
                       } \
                   }
#else
#define TRASH16() \
    TRASH(); TRASH(); TRASH(); TRASH(); \
    TRASH(); TRASH(); TRASH(); TRASH(); \
    TRASH(); TRASH(); TRASH(); TRASH(); \
    TRASH(); TRASH(); TRASH(); TRASH()

/* This much code noticeably slows compilation */
#define TRASH256() \
    TRASH16(); TRASH16(); TRASH16(); TRASH16(); \
    TRASH16(); TRASH16(); TRASH16(); TRASH16(); \
    TRASH16(); TRASH16(); TRASH16(); TRASH16(); \
    TRASH16(); TRASH16(); TRASH16(); TRASH16()
#endif

/* Stored in IRAM to protect against cache bug */
static int mismatches IDATA_ATTR = 0;
static unsigned long *p IDATA_ATTR;
static size_t size IDATA_ATTR;

enum plugin_status plugin_start(const void *parameter)
{
    unsigned long rand;
    unsigned long pass = 0;
    unsigned long *p2;

    rand = *rb->current_tick;

    rb->lcd_setfont(FONT_SYSFIXED);
    
    p = (unsigned long *)(rb->plugin_get_audio_buffer(&size));
    rb->lcd_putsf(0, 0, "Testing %d bytes", size);
    rb->lcd_putsf(0, 1, "at 0x%08x", (intptr_t)p);
    rb->lcd_puts(0, 2, "Press SELECT to quit.");

    rb->lcd_update();

    /* Fill allocated memory with pattern */
    for (p2 = p; p2 < p + size/4; p2++) {
        if (((long)p2 & 0xFFFFF) == 0) rb->yield();
        *p2 = FILLPATTERN;
    }

    while (!(rb->button_get(false) > BUTTON_NONE)) {
        /* long t; only used when reading */
        int i;

        rb->lcd_putsf(0, 3, "Pass: %d", pass++);
        if (mismatches > 0) 
            rb->lcd_putsf(0, 4, "Mismatches: %d", mismatches);
        rb->lcd_update();

        /* Trigger the bug */
        for (i = 0; i < 1000; i++) {
            rb->commit_discard_dcache();
            TRASH256();
            rb->commit_discard_dcache();
            TRASH256();
            rb->commit_discard_dcache();
            TRASH256();
            rb->commit_discard_dcache();
            rb->yield();
        }

        /* Count corrupted words */
        mismatches = 0;
        for (p2 = p; p2 < p + size/4; p2++) {
            if (((long)p2 & 0xFFFFF) == 0) rb->yield();

            if (*p2 != FILLPATTERN) {
                mismatches++;
            }
        }

        if (mismatches > MAX_MISMATCHES) {
            break;
        }
    } /* while */

    /* If any words got corrupted, write the memory dump */
    if (mismatches > 0) {
        int fd = rb->open("/ram.dump", O_WRONLY | O_TRUNC | O_CREAT);
        if (fd >= 0) {
            rb->write(fd, (void *)0x10000000, REALMEMSIZE);

            /* Append buffer offset, size and number of mismatches */
            rb->write(fd, &p, sizeof(p));
            rb->write(fd, &size, sizeof(size));
            rb->write(fd, &mismatches, sizeof(mismatches));

            rb->close(fd);
        }

        rb->lcd_putsf(0, 4, "RAM dumped, %d mismatches", mismatches);
        rb->lcd_puts(0, 5, "Press SELECT to quit.");
        rb->lcd_update();

        /* The first button press is ignored, and the second quits
         * the main loop and then passes the first wait here, 
         * so there are two waits here */
        while (!(rb->button_get(false) > BUTTON_NONE));
        while (!(rb->button_get(false) > BUTTON_NONE));
    }

    return PLUGIN_OK;
    (void)parameter;
}
