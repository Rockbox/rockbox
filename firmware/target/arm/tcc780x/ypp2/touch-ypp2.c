/***************************************************************************
 * Samsung YP-P2 Synaptics ClearPad input.
 *
 * Transport follows Samsung 5.10 (0x2010670c..0x20107120) and the service
 * schematic, sections 7-4 and 7-7.
 *
 * Seven-bit address 0x20 uses wire bytes 0x40 (write) and 0x41 (read).
 * A packet contains a MEP header, (header & 7) payload bytes, and an
 * additive eight-bit checksum including the wire address. The startup
 * reply is 19 10 6a; absolute reports use header 0x0e.
 *
 * GPIO waits are bounded. Read-address NACKs use a delayed, bounded retry.
 * The coordinate baseline comes from one player's four target taps.
 * Rockbox's Calibrate screen supplies the saved user mapping.
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
#include "config.h"
#include "cpu.h"
#include "kernel.h"
#include "system.h"
#include "thread.h"
#include "touch-target.h"

/* CPD0..2 map to C24..26. ATT is active low; DAT/CLK have external pull-ups. */
#define TOUCH_ATT (1ul << 24)
#define TOUCH_DAT (1ul << 25)
#define TOUCH_CLK (1ul << 26)
#define TOUCH_PADCFG (*(volatile unsigned long *)0xf005a0d8)
#define TOUCH_PAD_BITS 0x00040040ul /* exact mask in Samsung's touch setup */
#define TOUCH_HALF_US 5
#define TOUCH_STRETCH_US 1000
#define TOUCH_NACK_DELAY MAX(1, HZ/20) /* Space read-address retries by 50 ms. */

enum ypp2_touch_error {
    YPP2_TOUCH_OK, YPP2_TOUCH_BUS_BUSY, YPP2_TOUCH_CLOCK_TIMEOUT,
    YPP2_TOUCH_NACK, YPP2_TOUCH_CHECKSUM
};

#define YPP2_TOUCH_READ_NACK_LIMIT 16

static struct {
    bool active, down;
    enum ypp2_touch_error error;
    unsigned int x, y, consecutive_nacks;
} touch;
static bool configured;
static unsigned long old_pad_bits;
static long next_read_tick;
static bool service_initialized;
static long touch_stack[DEFAULT_STACK_SIZE / sizeof(long)];
static volatile bool cached_down;
static volatile int cached_x, cached_y;
static volatile long cached_tick;

/* Baseline taps at portrait targets (16,18), (246,18), (246,463), (16,463):
 * TL=1036/5211, TR=4816/5196, BR=4800/672, BL=1044/672.
 * Average opposing sides, subtract the hidden top strip, and clip to the
 * usable viewport. Panel coordinates are 272 by 480 pixels. */
static void touch_to_pixels(unsigned int raw_x, unsigned int raw_y, int *x, int *y)
{
    int native_x = 16 + ((int)raw_x - 1040) * 230 / 3768;
    int native_y = 18 + (5204 - (int)raw_y) * 445 / 4532;
    *x = MAX(0, MIN(LCD_WIDTH - 1, native_x));
    *y = MAX(0, MIN(LCD_HEIGHT - 1, native_y - LCD_TOP_MARGIN));
}

static void touch_publish(void)
{
    int x = 0, y = 0;
    bool down = touch.active && touch.error == YPP2_TOUCH_OK && touch.down;
    if (down)
        touch_to_pixels(touch.x, touch.y, &x, &y);
    int level = disable_irq_save();
    cached_x = x;
    cached_y = y;
    cached_down = down;
    cached_tick = current_tick;
    restore_irq(level);
}

/* Open drain: preload low before output; release high to the pull-up.
 * Mask IRQs for each GPIO read-modify-write, then restore them while waiting
 * for the bus. GPIOC writes preserve the other board functions. */
static void touch_low(unsigned long mask)
{
    int level = disable_irq_save();
    GPIOC &= ~mask;
    GPIOC_DIR |= mask;
    restore_irq(level);
}

static void touch_release(unsigned long mask)
{
    int level = disable_irq_save();
    GPIOC_DIR &= ~mask;
    restore_irq(level);
}

static bool touch_high(unsigned long mask)
{
    return (GPIOC & mask) != 0;
}

static bool touch_clock_high(void)
{
    touch_release(TOUCH_CLK);
    for (unsigned int i = 0; i < TOUCH_STRETCH_US; ++i)
    {
        if (touch_high(TOUCH_CLK))
            return true;
        udelay(1);
    }
    touch.error = YPP2_TOUCH_CLOCK_TIMEOUT;
    return false;
}

static bool touch_start(void)
{
    touch_release(TOUCH_DAT);
    if (!touch_clock_high())
        return false;
    udelay(TOUCH_HALF_US);
    if (!touch_high(TOUCH_DAT))
    {
        touch.error = YPP2_TOUCH_BUS_BUSY;
        return false;
    }
    touch_low(TOUCH_DAT);
    udelay(TOUCH_HALF_US);
    touch_low(TOUCH_CLK);
    return true;
}

static bool touch_stop(void)
{
    touch_low(TOUCH_CLK);
    touch_low(TOUCH_DAT);
    udelay(TOUCH_HALF_US);
    unsigned int saved_error = touch.error;
    bool ok = touch_clock_high();
    udelay(TOUCH_HALF_US);
    touch_release(TOUCH_DAT); /* also release both wires on every failure */
    udelay(TOUCH_HALF_US);
    if (ok && saved_error != YPP2_TOUCH_OK)
        touch.error = saved_error;
    return ok;
}

static bool touch_write_byte(unsigned char value)
{
    for (unsigned int mask = 0x80; mask; mask >>= 1)
    {
        if (value & mask)
            touch_release(TOUCH_DAT);
        else
            touch_low(TOUCH_DAT);
        udelay(TOUCH_HALF_US);
        if (!touch_clock_high())
            return false;
        udelay(TOUCH_HALF_US);
        touch_low(TOUCH_CLK);
    }
    touch_release(TOUCH_DAT);
    udelay(TOUCH_HALF_US);
    if (!touch_clock_high())
        return false;
    bool ack = !touch_high(TOUCH_DAT);
    udelay(TOUCH_HALF_US);
    touch_low(TOUCH_CLK);
    if (!ack)
        touch.error = YPP2_TOUCH_NACK;
    return ack;
}

static bool touch_read_byte(unsigned char *value, bool ack)
{
    unsigned int byte = 0;
    touch_release(TOUCH_DAT);
    for (unsigned int i = 0; i < 8; ++i)
    {
        udelay(TOUCH_HALF_US);
        if (!touch_clock_high())
            return false;
        byte = (byte << 1) | touch_high(TOUCH_DAT);
        udelay(TOUCH_HALF_US);
        touch_low(TOUCH_CLK);
    }
    if (ack)
        touch_low(TOUCH_DAT);
    udelay(TOUCH_HALF_US);
    if (!touch_clock_high())
        return false;
    udelay(TOUCH_HALF_US);
    touch_low(TOUCH_CLK);
    touch_release(TOUCH_DAT);
    *value = (unsigned char)byte;
    return true;
}

static void touch_error_count(bool read_nack)
{
    touch.down = false;
    /* Retry read-address NACKs with a delay and a bounded consecutive count.
     * A write NACK or a failed STOP/clock wait ends the session. */
    if (read_nack && touch.error == YPP2_TOUCH_NACK)
    {
        next_read_tick = current_tick + TOUCH_NACK_DELAY;
        if (++touch.consecutive_nacks >= YPP2_TOUCH_READ_NACK_LIMIT)
            touch.active = false;
    }
    else if (touch.error != YPP2_TOUCH_CHECKSUM)
        touch.active = false;
}

static bool touch_read_packet(void)
{
    touch.error = YPP2_TOUCH_OK;
    unsigned char raw[9] = { 0 }; /* header, up to seven payload bytes, checksum */
    /* 0x40/0x41 are wire addresses; seven-bit address is0x20. */
    bool ok = touch_start() && touch_write_byte(0x41);
    if (ok)
        ok = touch_read_byte(&raw[0], true);
    unsigned int length = ok ? (raw[0] & 7u) + 2u : 0u;
    for (unsigned int i = 1; ok && i < length; ++i)
    {
        ok = touch_read_byte(&raw[i], i + 1 < length);
    }
    bool stopped = touch_stop();
    if (!stopped) ok = false;
    if (!ok)
    {
        touch_error_count(stopped && touch.error == YPP2_TOUCH_NACK);
        return false;
    }
    touch.consecutive_nacks = 0;
    unsigned int sum = 0x41;
    for (unsigned int i = 0; i + 1 < length; ++i)
        sum += raw[i];
    if ((sum & 255) != raw[length - 1])
    {
        touch.error = YPP2_TOUCH_CHECKSUM;
        touch_error_count(false);
        return false;
    }
    /* Absolute reports have header 0x0e and a six-byte payload. */
    if (length != 8 || raw[0] != 0x0e)
    {
        touch.down = false;
        return true;
    }
    bool down = (raw[1] & 1) != 0;
    touch.down = down;
    touch.x = ((raw[2] & 31u) << 8) | raw[3];
    touch.y = ((raw[4] & 31u) << 8) | raw[5];
    return true;
}

static void touch_stop_session(void)
{
    if (configured)
    {
        touch_release(TOUCH_DAT | TOUCH_CLK);
        int level = disable_irq_save();
        TOUCH_PADCFG = (TOUCH_PADCFG & ~TOUCH_PAD_BITS) | old_pad_bits;
        restore_irq(level);
        configured = false;
    }
    touch.active = touch.down = false;
}

static void touch_start_session(void)
{
    touch_stop_session();
    touch.down = false;
    touch.error = YPP2_TOUCH_OK;
    touch.x = touch.y = touch.consecutive_nacks = 0;
    int level = disable_irq_save();
    old_pad_bits = TOUCH_PADCFG & TOUCH_PAD_BITS;
    /* Preserve the other fields in the stock setup's pad register. */
    TOUCH_PADCFG |= TOUCH_PAD_BITS;
    GPIOC_DIR &= ~(TOUCH_ATT | TOUCH_DAT | TOUCH_CLK);
    restore_irq(level);
    configured = touch.active = true;
    next_read_tick = current_tick;

    /* Drain up to two startup replies, then send stock command 0x18. */
    for (unsigned int i = 0; i < 2 && touch.active && !touch_high(TOUCH_ATT); ++i)
        if (!touch_read_packet())
            break;
    if (touch.active)
    {
        touch.error = YPP2_TOUCH_OK;
        bool ok = touch_start() && touch_write_byte(0x40) &&
                  touch_write_byte(0x18) && touch_write_byte(0x58);
        if (!touch_stop()) ok = false;
        if (!ok) touch_error_count(false);
    }
}

static void touch_poll_session(void)
{
    if (!touch.active)
        return;
    /* Read one packet per call to expose each press and release to the UI. */
    if (!TIME_BEFORE(current_tick, next_read_tick) && !touch_high(TOUCH_ATT))
        touch_read_packet();
}

static void touch_thread(void)
{
    touch_start_session();
    while (true)
    {
        touch_poll_session();
        touch_publish();
        sleep(MAX(1, HZ/100));
    }
}

void ypp2_touch_init(void)
{
    if (service_initialized)
        return;
    service_initialized = true;
    create_thread(touch_thread, touch_stack, sizeof(touch_stack), 0, "touch"
                  IF_PRIO(, PRIORITY_USER_INTERFACE) IF_COP(, CPU));
}

bool ypp2_touch_read(int *x, int *y)
{
    int level = disable_irq_save();
    *x = cached_x;
    *y = cached_y;
    /* An unmoving finger can hold contact between packets. Expire the cached
     * contact when the worker heartbeat is older than half a second. */
    bool down = cached_down &&
                !TIME_BEFORE(cached_tick + HZ/2, current_tick);
    restore_irq(level);
    return down;
}
