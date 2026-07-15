/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 by Hemant Kumar
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

/* "Mikey" is the internal controller for the headphone jack microphone and
 * the inline earphone remote (I2C bus 0, address 0x72).
 *
 * Register protocol (reverse engineered on-device, 2026-07):
 *   reg0 = 0x2f     remote-reporting mode. The low 3 bits are the mic bias
 *                   level (7, same as the recording path uses); 0x28 arms
 *                   the button detection engine. The chip resets on jack
 *                   removal (all registers read zero afterwards), so the
 *                   mode is re-armed on insertion.
 *   reg4 & 0x40     identified-remote bit, set after the chip hears an
 *                   Apple remote's ID chirp. UNIT-DEPENDENT: reliably
 *                   latched on one 6G, never observed on two other
 *                   120GB units, so nothing is gated on it; it is only
 *                   shown in the hardware debug screen.
 *   reg4 & 0x05     center (play/pause) held; a level signal. Can
 *                   flicker while the mode is (re)armed, hence the
 *                   "armed" guard below.
 *   reg5            volume edge events, each readable for ~100ms:
 *                   0x04 vol+ press, 0x08 vol+ release,
 *                   0x01 vol- press, 0x02 vol- release.
 *                   0x30 is the accessory-ID event (no button meaning).
 * The remote itself signals volume as static DC loads on the mic line and
 * identifies via a one-time ultrasonic chirp (the same scheme David Carne
 * documented for the shuffle 3G remote); Mikey does that level and chirp
 * detection in hardware and reports the results as above. */

#include "system.h"
#include "cpu.h"
#include "kernel.h"
#include "thread.h"
#include "button.h"
#include "i2c-s5l8702.h"
#include "mikey-target.h"

#define MIKEY_ADDR      0x72
#define MIKEY_REG_MODE  0       /* mic bias + detection-engine mode */
#define MIKEY_REG_BTN   4       /* center-button level */
#define MIKEY_REG_EVT   5       /* volume press/release edge events */

#define MIKEY_MODE_OFF      0x05    /* reset default, mic line unpowered */
#define MIKEY_MODE_MIC      0x07    /* mic bias raised, for recording */
#define MIKEY_MODE_REMOTE   0x2f    /* mic bias + button detection */

#define MIKEY_ID_FLAG       0x40    /* identified-remote latch in reg4 */
#define MIKEY_BTN_CENTER    0x05
#define MIKEY_EVT_VOLUP_DN  0x04
#define MIKEY_EVT_VOLUP_UP  0x08
#define MIKEY_EVT_VOLDN_DN  0x01
#define MIKEY_EVT_VOLDN_UP  0x02

/* A press+release pair landing in a single poll is stretched over this
 * many polls, so the button driver's debounce (two identical consecutive
 * reads at the 10ms button tick) still accepts the click. */
#define MIKEY_CLICK_POLLS   2

/* Center-button handling: click-only. reg4's rising edge is always
 * immediate, but after ~5s of button inactivity the chip's event engine
 * naps and reports the release (reg4 fall, reg6 events) ~1.8s late, so
 * the press duration is unreliable. Every press is therefore reported
 * as a fixed-length click at the rise; there is no hold/long-press. A
 * release must be seen for several consecutive polls before a new rise
 * counts, so bounce or an I2C NAK can't double-fire a click. */
#define MIKEY_CENTER_PULSE_POLLS  3     /* reported click length, 60ms */
#define MIKEY_CENTER_OFF_POLLS    3     /* release debounce, 60ms */

unsigned char mikey_read(int address)
{
    /* default to "no press, no events" if the transfer fails: the chip
     * NAKs while it resets itself around jack removal/insertion */
    unsigned char val = 0;
    i2c_read(0, MIKEY_ADDR, address, 1, &val);
    return val;
}

int mikey_write(int address, unsigned char val)
{
    return i2c_write(0, MIKEY_ADDR, address, 1, &val);
}

void mikey_reset(void)
{
    mikey_write(MIKEY_REG_MODE, MIKEY_MODE_OFF);
    mikey_write(1, 0x80);
}

static int mikey_btn = BUTTON_NONE;
static volatile bool mic_active = false;
static bool mikey_detected = false;
static long mikey_stack[DEFAULT_STACK_SIZE/2/sizeof(long)];

bool mikey_present(void)
{
    return mikey_detected;
}

/* The recording path owns the chip while the jack mic is enabled; the
 * polling thread backs off and re-arms the remote mode afterwards. */
void mikey_set_mic_capture(bool enable)
{
    mic_active = enable;
    if (enable)
        mikey_write(MIKEY_REG_MODE, MIKEY_MODE_MIC);
    else
        mikey_reset();
}

/* Decode one volume button's press/release edge events into a held state.
 * A real hold only produces the two edges, so between them *held carries
 * the state; *click stretches a same-poll press+release (see above). */
static void mikey_decode_vol(unsigned char evt, unsigned char dn,
                             unsigned char up, bool *held, int *click)
{
    if (*click > 0 && --*click == 0)
        *held = false;

    if (evt & dn)
    {
        *held = true;
        *click = (evt & up) ? MIKEY_CLICK_POLLS : 0;
    }
    else if ((evt & up) && *click == 0)
        *held = false;
}

/* Phantom-load failsafe: some passive cables present a DC load on the
 * mic line that Mikey decodes as a volume button, producing a press
 * edge the moment the detection engine arms and never a release (seen
 * in the wild as volume ramping to zero on its own). A press edge this
 * early cannot be a human, so it suppresses that button until a
 * release edge proves a real remote is attached. This is the
 * edge-event analog of the center button's "armed" guard, which
 * already keeps mic-less TRS plugs (a permanent mic-line short) from
 * reading as a stuck center button. */
#define MIKEY_ARM_WINDOW_POLLS  10      /* 200ms after (re)arming */

/* Per-poll decode state, kept apart from the thread's power/mode
 * bookkeeping so the register-to-button logic is a pure function of
 * (state, reg4, reg5). */
struct mikey_decode {
    bool armed;          /* need a released reading before first press */
    bool vol_up, vol_dn;
    int up_click, dn_click;
    int arm_window;      /* polls left of phantom-press suppression window */
    bool up_suppressed, dn_suppressed;
    bool center_down;    /* debounced "press already reported" */
    int center_off;      /* consecutive released polls */
    int center_pulse;    /* click pulse countdown */
};

static void mikey_decode_reset(struct mikey_decode *d)
{
    d->armed = false;
    d->vol_up = d->vol_dn = false;
    d->up_click = d->dn_click = 0;
    d->arm_window = MIKEY_ARM_WINDOW_POLLS;
    d->up_suppressed = d->dn_suppressed = false;
    d->center_down = false;
    d->center_off = MIKEY_CENTER_OFF_POLLS;
    d->center_pulse = 0;
}

/* Apply the phantom-load failsafe to one volume button's edge bits:
 * a press edge (without a same-poll release) inside the arm window
 * marks the button suppressed; suppressed press edges are stripped
 * until a release edge lifts the suppression. */
static unsigned char mikey_vol_gate(struct mikey_decode *d, unsigned char evt,
                                    unsigned char dn, unsigned char up,
                                    bool *suppressed)
{
    if (evt & up)
        *suppressed = false;
    else if ((evt & dn) && d->arm_window > 0)
        *suppressed = true;

    if (*suppressed)
        evt &= ~dn;
    return evt;
}

/* Decode one poll's register pair into a button mask. Volume is NOT
 * gated on the reg4 identified-remote bit: that bit proved to be
 * unit-dependent (reliably latched on one 6G, never seen on two other
 * 120GB units where gating on it broke volume entirely), so it is
 * only surfaced in the hardware debug screen for now. */
static int mikey_decode_poll(struct mikey_decode *d,
                             unsigned char btn, unsigned char evt)
{
    evt = mikey_vol_gate(d, evt, MIKEY_EVT_VOLUP_DN, MIKEY_EVT_VOLUP_UP,
                         &d->up_suppressed);
    evt = mikey_vol_gate(d, evt, MIKEY_EVT_VOLDN_DN, MIKEY_EVT_VOLDN_UP,
                         &d->dn_suppressed);
    if (d->arm_window > 0)
        d->arm_window--;

    mikey_decode_vol(evt, MIKEY_EVT_VOLUP_DN, MIKEY_EVT_VOLUP_UP,
                     &d->vol_up, &d->up_click);
    mikey_decode_vol(evt, MIKEY_EVT_VOLDN_DN, MIKEY_EVT_VOLDN_UP,
                     &d->vol_dn, &d->dn_click);

    /* reg4 can read pressed while the mode settles; don't report the
     * center button until it has been seen released once */
    bool raw = (btn & MIKEY_BTN_CENTER) != 0;
    if (!raw)
        d->armed = true;

    /* click-only: emit a fixed pulse at each debounced rise */
    if (raw && d->armed)
    {
        if (!d->center_down && d->center_off >= MIKEY_CENTER_OFF_POLLS)
            d->center_pulse = MIKEY_CENTER_PULSE_POLLS;
        d->center_down = true;
        d->center_off = 0;
    }
    else
    {
        if (d->center_off < MIKEY_CENTER_OFF_POLLS)
            d->center_off++;
        if (d->center_off >= MIKEY_CENTER_OFF_POLLS)
            d->center_down = false;
    }

    bool center = false;
    if (d->center_pulse > 0)
    {
        center = true;
        d->center_pulse--;
    }

    /* All buttons report as multimedia keys: handled globally by
     * default_event_handler on every screen, so volume and play/pause
     * work in menus, WPS and plugins alike, matching the OF. */
    return (center    ? BUTTON_MULTIMEDIA_PLAYPAUSE   : 0)
         | (d->vol_up ? BUTTON_MULTIMEDIA_VOLUME_UP   : 0)
         | (d->vol_dn ? BUTTON_MULTIMEDIA_VOLUME_DOWN : 0);
}

/* The chip only ACKs the bus while something occupies the jack, so
 * presence can't be probed at boot; it is probed here whenever the
 * jack is occupied. A few insertion transients may NAK before the
 * first ACK; a model without the chip (2007) NAKs forever, so after
 * the retry budget the poll backs way off to stay out of the way. */
#define MIKEY_PROBE_TRIES   25          /* ~2.5s of HZ/10 retries */

static void mikey_thread(void)
{
    bool powered = false;
    int naks = 0;
    struct mikey_decode decode;

    mikey_decode_reset(&decode);

    while (1)
    {
        if (!headphones_inserted() || mic_active)
        {
            if (powered && !mic_active)
                mikey_reset();
            powered = false;
            naks = 0;
            mikey_decode_reset(&decode);
            mikey_btn = BUTTON_NONE;
            sleep(HZ/2);   /* nothing to poll, check back at leisure */
            continue;
        }

        /* Re-arm when the mode readback disagrees (first pass, chip reset
         * on jack removal, or the recording path rewrote it). Arming can
         * flicker reg4 and emits a spurious accessory-ID event, so drop
         * the state and settle for one poll. */
        unsigned char mode;
        if (i2c_read(0, MIKEY_ADDR, MIKEY_REG_MODE, 1, &mode) != 0)
        {
            /* no ACK: chip resetting around insertion, or not present */
            mikey_decode_reset(&decode);
            mikey_btn = BUTTON_NONE;
            if (naks < MIKEY_PROBE_TRIES)
            {
                naks++;
                sleep(HZ/10);
            }
            else
                sleep(HZ*2);
            continue;
        }
        naks = 0;
        powered = true;
        mikey_detected = true;   /* latched for the hw debug screen */

        if (mode != MIKEY_MODE_REMOTE)
        {
            mikey_write(MIKEY_REG_MODE, MIKEY_MODE_REMOTE);
            mikey_decode_reset(&decode);
            mikey_btn = BUTTON_NONE;
            sleep(HZ/50);
            continue;
        }

        unsigned char evt = mikey_read(MIKEY_REG_EVT);
        unsigned char btn = mikey_read(MIKEY_REG_BTN);
        mikey_btn = mikey_decode_poll(&decode, btn, evt);

        /* volume edge events stay readable for ~100ms; 20ms leaves a
         * comfortable margin against scheduling jitter */
        sleep(HZ/50);
    }
}

int mikey_button_read(void)
{
    return mikey_btn;
}

void mikey_init(void)
{
    /* Presence can't be probed here: the chip only ACKs while the
     * jack is occupied (booting without headphones must not disable
     * the remote). The thread probes on insertion instead and backs
     * off on models without the chip (2007). */
    mikey_reset();
    create_thread(mikey_thread, mikey_stack, sizeof(mikey_stack), 0,
                  "mikey" IF_PRIO(, PRIORITY_SYSTEM) IF_COP(, CPU));
}
