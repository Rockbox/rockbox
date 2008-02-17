/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: debug-tcc780x.c 16316 2008-02-15 12:37:36Z christian $
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
#include "system.h"
#include "string.h"
#include <stdbool.h>
#include "button.h"
#include "lcd.h"
#include "sprintf.h"
#include "font.h"
#include "debug-target.h"

#ifdef IPOD_ARCH
#include "hwcompat.h"       /* needed for IPOD_HW_REVISION */
#endif

static int perfcheck(void)
{
    int result;

    asm (
        "mrs     r2, CPSR            \n"
        "orr     r0, r2, #0xc0       \n" /* disable IRQ and FIQ */
        "msr     CPSR_c, r0          \n"
        "mov     %[res], #0          \n"
        "ldr     r0, [%[timr]]       \n"
        "add     r0, r0, %[tmo]      \n"
    "1:                              \n"
        "add     %[res], %[res], #1  \n"
        "ldr     r1, [%[timr]]       \n"
        "cmp     r1, r0              \n"
        "bmi     1b                  \n"
        "msr     CPSR_c, r2          \n" /* reset IRQ and FIQ state */
        :
        [res]"=&r"(result)
        :
        [timr]"r"(&USEC_TIMER),
        [tmo]"r"(
#if CONFIG_CPU == PP5002
        16000
#else /* PP5020/5022/5024 */
        10226
#endif
        )
        :
        "r0", "r1", "r2"
    );
    return result;
}

bool __dbg_hw_info(void)
{
    int line = 0;
    char buf[32];

#if defined(CPU_PP502x)
    char pp_version[] = { (PP_VER2 >> 24) & 0xff, (PP_VER2 >> 16) & 0xff,
                          (PP_VER2 >> 8) & 0xff, (PP_VER2) & 0xff,
                          (PP_VER1 >> 24) & 0xff, (PP_VER1 >> 16) & 0xff,
                          (PP_VER1 >> 8) & 0xff, (PP_VER1) & 0xff, '\0' };

#elif CONFIG_CPU == PP5002
    char pp_version[] = { (PP_VER4 >> 8) & 0xff, PP_VER4 & 0xff,
                          (PP_VER3 >> 8) & 0xff, PP_VER3 & 0xff,
                          (PP_VER2 >> 8) & 0xff, PP_VER2 & 0xff,
                          (PP_VER1 >> 8) & 0xff, PP_VER1 & 0xff, '\0' };
#endif

    lcd_setmargins(0, 0);
    lcd_setfont(FONT_SYSFIXED);
    lcd_clear_display();

    lcd_puts(0, line++, "[Hardware info]");

#ifdef IPOD_ARCH
    snprintf(buf, sizeof(buf), "HW rev: 0x%08lx", IPOD_HW_REVISION);
    lcd_puts(0, line++, buf);
#endif

#ifdef IPOD_COLOR
    extern int lcd_type; /* Defined in lcd-colornano.c */

    snprintf(buf, sizeof(buf), "LCD type: %d", lcd_type);
    lcd_puts(0, line++, buf);
#endif

    snprintf(buf, sizeof(buf), "PP version: %s", pp_version);
    lcd_puts(0, line++, buf);

    snprintf(buf, sizeof(buf), "Est. clock (kHz): %d", perfcheck());
    lcd_puts(0, line++, buf);

    lcd_update();

    while (1) {
        if (button_get_w_tmo(HZ/10) == (DEBUG_CANCEL|BUTTON_REL)) {
            return false;
        }
    }
}
