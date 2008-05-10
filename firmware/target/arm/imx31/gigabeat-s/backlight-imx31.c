/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
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
#include "backlight.h"
#include "mc13783.h"
#include "backlight-target.h"

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
/* Table that uses combinations of current level and pwm fraction to get
 * as many uniquely-visible brightness levels as possible. The lowest current
 * level for any average current is used even though many combinations give
 * duplicate values. Current (I) values are in mA. */
static const struct
{
    unsigned char md;
    unsigned char pwm;
} led_md_pwm_table[] =
{
               /* I-level PWM(x/15) I-Avg */
    { 0,  0 }, /*    0        0      0.0  */
    { 1,  1 }, /*    3        1      0.2  */
    { 1,  2 }, /*    3        2      0.4  */
    { 1,  3 }, /*    3        3      0.6  */ 
    { 1,  4 }, /*    3        4      0.8  */ 
    { 1,  5 }, /*    3        5      1.0  */ 
    { 1,  6 }, /*    3        6      1.2  */ 
    { 1,  7 }, /*    3        7      1.4  */ 
    { 1,  8 }, /*    3        8      1.6  */ 
    { 1,  9 }, /*    3        9      1.8  */ 
    { 1, 10 }, /*    3       10      2.0  */ 
    { 1, 11 }, /*    3       11      2.2  */ 
    { 1, 12 }, /*    3       12      2.4  */ /* default */
    { 1, 13 }, /*    3       13      2.6  */ 
    { 1, 14 }, /*    3       14      2.8  */ 
    { 1, 15 }, /*    3       15      3.0  */ 
    { 2,  9 }, /*    6        9      3.6  */ 
    { 2, 10 }, /*    6       10      4.0  */ 
    { 2, 11 }, /*    6       11      4.4  */ 
    { 2, 12 }, /*    6       12      4.8  */ 
    { 2, 13 }, /*    6       13      5.2  */ 
    { 2, 14 }, /*    6       14      5.6  */ 
    { 2, 15 }, /*    6       15      6.0  */ 
    { 3, 11 }, /*    9       11      6.6  */ 
    { 3, 12 }, /*    9       12      7.2  */ 
    /* Anything higher is just too much   */
};
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */

bool _backlight_init(void)
{
    mc13783_write(MC13783_LED_CONTROL0,
                  MC13783_LEDEN |
                  MC13783_LEDMDRAMPUP |
                  MC13783_LEDMDRAMPDOWN |
                  MC13783_BOOSTEN |
                  MC13783_ABMODE_MONCH_LEDMD1234 |
                  MC13783_ABREF_400MV);
    return true;
}

void _backlight_on(void)
{
    /* LEDEN=1 */
    mc13783_set(MC13783_LED_CONTROL0, MC13783_LEDEN);
}

void _backlight_off(void)
{
    /* LEDEN=0 */
    mc13783_clear(MC13783_LED_CONTROL0, MC13783_LEDEN);
}

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
/* Assumes that the backlight has been initialized */
void _backlight_set_brightness(int brightness)
{
    uint32_t data, md, pwm;

    if ((unsigned)brightness >= ARRAYLEN(led_md_pwm_table))
        brightness = DEFAULT_BRIGHTNESS_SETTING;

    data = mc13783_read(MC13783_LED_CONTROL2);

    if (data == (uint32_t)-1)
        return;

    md = led_md_pwm_table[brightness].md;
    pwm = led_md_pwm_table[brightness].pwm;

    data &= ~(MC13783_LEDMD | MC13783_LEDMDDC);
    data |= MC13783_LEDMDw(md) | MC13783_LEDMDDCw(pwm);

    mc13783_write(MC13783_LED_CONTROL2, data);
}
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */
