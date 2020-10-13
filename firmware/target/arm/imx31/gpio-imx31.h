/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2008 by Michael Sevakis
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
#ifndef GPIO_IMX31_H
#define GPIO_IMX31_H

/* Static registration mechanism for imx31 GPIO interrupts */
#define USE_GPIO1_EVENTS (1 << 0)
#define USE_GPIO2_EVENTS (1 << 1)
#define USE_GPIO3_EVENTS (1 << 2)

/* Module logical indexes */
enum gpio_module_number
{
    GPIO1_NUM,      /* ID  0..31 */
    GPIO2_NUM,      /* ID 32..63 */
    GPIO3_NUM,      /* ID 64..95 */
    GPIO_NUM_GPIO,
};

enum gpio_id
{
    /* GPIO1 */
    GPIO1_0_ID = 0,
    GPIO1_1_ID,
    GPIO1_2_ID,
    GPIO1_3_ID,
    GPIO1_4_ID,
    GPIO1_5_ID,
    GPIO1_6_ID,
    GPIO1_7_ID,
    GPIO1_8_ID,
    GPIO1_9_ID,
    GPIO1_10_ID,
    GPIO1_11_ID,
    GPIO1_12_ID,
    GPIO1_13_ID,
    GPIO1_14_ID,
    GPIO1_15_ID,
    GPIO1_16_ID,
    GPIO1_17_ID,
    GPIO1_18_ID,
    GPIO1_19_ID,
    GPIO1_20_ID,
    GPIO1_21_ID,
    GPIO1_22_ID,
    GPIO1_23_ID,
    GPIO1_24_ID,
    GPIO1_25_ID,
    GPIO1_26_ID,
    GPIO1_27_ID,
    GPIO1_28_ID,
    GPIO1_29_ID,
    GPIO1_30_ID,
    GPIO1_31_ID,
    /* GPIO2 */
    GPIO2_0_ID = 32,
    GPIO2_1_ID,
    GPIO2_2_ID,
    GPIO2_3_ID,
    GPIO2_4_ID,
    GPIO2_5_ID,
    GPIO2_6_ID,
    GPIO2_7_ID,
    GPIO2_8_ID,
    GPIO2_9_ID,
    GPIO2_10_ID,
    GPIO2_11_ID,
    GPIO2_12_ID,
    GPIO2_13_ID,
    GPIO2_14_ID,
    GPIO2_15_ID,
    GPIO2_16_ID,
    GPIO2_17_ID,
    GPIO2_18_ID,
    GPIO2_19_ID,
    GPIO2_20_ID,
    GPIO2_21_ID,
    GPIO2_22_ID,
    GPIO2_23_ID,
    GPIO2_24_ID,
    GPIO2_25_ID,
    GPIO2_26_ID,
    GPIO2_27_ID,
    GPIO2_28_ID,
    GPIO2_29_ID,
    GPIO2_30_ID,
    GPIO2_31_ID,
    /* GPIO3 */
    GPIO3_0_ID = 64,
    GPIO3_1_ID,
    GPIO3_2_ID,
    GPIO3_3_ID,
    GPIO3_4_ID,
    GPIO3_5_ID,
    GPIO3_6_ID,
    GPIO3_7_ID,
    GPIO3_8_ID,
    GPIO3_9_ID,
    GPIO3_10_ID,
    GPIO3_11_ID,
    GPIO3_12_ID,
    GPIO3_13_ID,
    GPIO3_14_ID,
    GPIO3_15_ID,
    GPIO3_16_ID,
    GPIO3_17_ID,
    GPIO3_18_ID,
    GPIO3_19_ID,
    GPIO3_20_ID,
    GPIO3_21_ID,
    GPIO3_22_ID,
    GPIO3_23_ID,
    GPIO3_24_ID,
    GPIO3_25_ID,
    GPIO3_26_ID,
    GPIO3_27_ID,
    GPIO3_28_ID,
    GPIO3_29_ID,
    GPIO3_30_ID,
    GPIO3_31_ID,
};

/* Possible values for gpio interrupt line config */
enum gpio_int_sense
{
    GPIO_SENSE_LOW_LEVEL,  /* High-level sensitive */
    GPIO_SENSE_HIGH_LEVEL, /* Low-level sensitive */
    GPIO_SENSE_RISING,     /* Rising-edge sensitive */
    GPIO_SENSE_FALLING,    /* Falling-edge sensitive */
    GPIO_SENSE_EDGE_SEL,   /* Detect any edge */
};

/* Handlers will be called in declared order for a given module
   Handlers of same module should be grouped together; module order
   doesn't matter */
#ifdef DEFINE_GPIO_VECTOR_TABLE

/* Describes a single event for a pin */
struct gpio_event
{
    uint8_t id;             /* GPIOx_y_ID */
    uint8_t sense;          /* GPIO_SENSE_x */
    void (*callback)(void); /* GPIOx_y_EVENT_CB */
};

#define GPIO_VECTOR_TBL_START() \
    static FORCE_INLINE uintptr_t __gpio_event_vector_tbl(int __what) \
    {                                                                 \
        static const struct gpio_event __tbl[] = {

#define GPIO_EVENT_VECTOR_CB(__name)     void __name##_EVENT_CB(void)

#define GPIO_EVENT_VECTOR(__name, __sense) \
        { .id       = (__name##_ID),       \
          .sense    = (__sense),           \
          .callback = (__name##_EVENT_CB) },

#define GPIO_VECTOR_TBL_END() \
        };                                              \
        switch (__what)                                 \
        {                                               \
            default: return (uintptr_t)__tbl;           \
            case  1: return (uintptr_t)ARRAYLEN(__tbl); \
        }                                               \
    }

#define gpio_event_vector_tbl \
    ((const struct gpio_event *)__gpio_event_vector_tbl(0))

#define gpio_event_vector_tbl_len \
    ((unsigned int)__gpio_event_vector_tbl(1))

#endif /* DEFINE_GPIO_VECTOR_TABLE */

#define GPIO_BASE_ADDR \
    (volatile unsigned long * const [GPIO_NUM_GPIO]) { \
        (volatile unsigned long *)GPIO1_BASE_ADDR,     \
        (volatile unsigned long *)GPIO2_BASE_ADDR,     \
        (volatile unsigned long *)GPIO3_BASE_ADDR }

#define GPIO_DR       (0x00 / sizeof (unsigned long)) /* 00h        */
#define GPIO_GDIR     (0x04 / sizeof (unsigned long)) /* 04h        */
#define GPIO_PSR      (0x08 / sizeof (unsigned long)) /* 08h        */
#define GPIO_ICR      (0x0C / sizeof (unsigned long)) /* 0Ch ICR1,2 */
#define GPIO_IMR      (0x14 / sizeof (unsigned long)) /* 14h        */
#define GPIO_ISR      (0x18 / sizeof (unsigned long)) /* 18h        */
#define GPIO_EDGE_SEL (0x1C / sizeof (unsigned long)) /* 1Ch        */

void gpio_init(void);
bool gpio_enable_event(enum gpio_id id, bool enable);

static FORCE_INLINE void gpio_int_clear(enum gpio_id id)
    { GPIO_BASE_ADDR[id / 32][GPIO_ISR] = 1ul << (id % 32); }

static FORCE_INLINE void gpio_int_enable(enum gpio_id id)
    { bitset32(&GPIO_BASE_ADDR[id / 32][GPIO_IMR], 1ul << (id % 32)); }

static FORCE_INLINE void gpio_int_disable(enum gpio_id id)
    { bitclr32(&GPIO_BASE_ADDR[id / 32][GPIO_IMR], 1ul << (id % 32)); }

#endif /* GPIO_IMX31_H */
