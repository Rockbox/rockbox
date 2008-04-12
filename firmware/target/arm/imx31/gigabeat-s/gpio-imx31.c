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
 * IMX31 GPIO event manager
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
#include "avic-imx31.h"
#include "gpio-imx31.h"

/* UIE vector found in avic-imx31.c */
extern void UIE_VECTOR(void);

/* Event lists are allocated for the specific target */
#if (GPIO_EVENT_MASK & USE_GPIO1_EVENTS)
static __attribute__((interrupt("IRQ"))) void GPIO1_HANDLER(void);
extern const struct gpio_event_list gpio1_event_list;
#endif

#if (GPIO_EVENT_MASK & USE_GPIO2_EVENTS)
static __attribute__((interrupt("IRQ"))) void GPIO2_HANDLER(void);
extern const struct gpio_event_list gpio2_event_list;
#endif

#if (GPIO_EVENT_MASK & USE_GPIO3_EVENTS)
static __attribute__((interrupt("IRQ"))) void GPIO3_HANDLER(void);
extern const struct gpio_event_list gpio3_event_list;
#endif

static struct gpio_module_descriptor
{
    volatile unsigned long *base;       /* Module base address */
    enum IMX31_INT_LIST ints;           /* AVIC int number */
    void (*handler)(void);              /* Interrupt function */
    const struct gpio_event_list *list; /* Event handler list */
} gpio_descs[GPIO_NUM_GPIO] =
{
#if (GPIO_EVENT_MASK & USE_GPIO1_EVENTS)
    {
        .base    = (unsigned long *)GPIO1_BASE_ADDR,
        .ints    = GPIO1,
        .handler = GPIO1_HANDLER,
    },
#endif
#if (GPIO_EVENT_MASK & USE_GPIO2_EVENTS)
    {
        .base    = (unsigned long *)GPIO2_BASE_ADDR,
        .ints    = GPIO2,
        .handler = GPIO2_HANDLER,
    },
#endif
#if (GPIO_EVENT_MASK & USE_GPIO3_EVENTS)
    {
        .base    = (unsigned long *)GPIO3_BASE_ADDR,
        .ints    = GPIO3,
        .handler = GPIO3_HANDLER,
    },
#endif
};

static void gpio_call_events(enum gpio_module_number gpio)
{
    const struct gpio_module_descriptor * const desc = &gpio_descs[gpio];
    const struct gpio_event_list * const list = desc->list;
    volatile unsigned long * const base = desc->base;
    unsigned i;

    /* Intersect pending and unmasked bits */
    unsigned long pending = base[GPIO_ISR_I] & base[GPIO_IMR_I];

    /* Call each event handler in order */
    for (i = 0; i < list->count; i++)
    {
        const struct gpio_event * const event = &list->events[i];
        unsigned long bit = 1ul << event->line;

        if ((pending & bit) && event->callback())
            pending &= ~bit;
    }

    if (pending != 0)
    {
        /* Wasn't handled */
        UIE_VECTOR();
    }
}

#if (GPIO_EVENT_MASK & USE_GPIO1_EVENTS)
static __attribute__((interrupt("IRQ"))) void GPIO1_HANDLER(void)
{
    gpio_call_events(GPIO1_NUM);
}
#endif

#if (GPIO_EVENT_MASK & USE_GPIO2_EVENTS)
static __attribute__((interrupt("IRQ"))) void GPIO2_HANDLER(void)
{
    gpio_call_events(GPIO2_NUM);
}
#endif

#if (GPIO_EVENT_MASK & USE_GPIO3_EVENTS)
static __attribute__((interrupt("IRQ"))) void GPIO3_HANDLER(void)
{
    gpio_call_events(GPIO3_NUM);
}
#endif

void gpio_init(void)
{
    /* Mask-out GPIO interrupts - enable what's wanted later */
    GPIO1_IMR = 0;
    GPIO2_IMR = 0;
    GPIO3_IMR = 0;

    /* Init the externally-defined event lists for each port */
#if (GPIO_EVENT_MASK & USE_GPIO1_EVENTS)
    gpio_descs[GPIO1_NUM].list = &gpio1_event_list;
#endif
#if (GPIO_EVENT_MASK & USE_GPIO2_EVENTS)
    gpio_descs[GPIO2_NUM].list = &gpio2_event_list;
#endif
#if (GPIO_EVENT_MASK & USE_GPIO3_EVENTS)
    gpio_descs[GPIO3_NUM].list = &gpio3_event_list;
#endif
}

bool gpio_enable_event(enum gpio_module_number gpio, unsigned id)
{
    const struct gpio_module_descriptor * const desc = &gpio_descs[gpio];
    const struct gpio_event * const event = &desc->list->events[id];
    volatile unsigned long * const base = desc->base;
    volatile unsigned long * icr;
    unsigned long mask;
    unsigned long imr;
    int shift;

    if (id >= desc->list->count)
        return false;

    int oldlevel = disable_irq_save();

    imr =  base[GPIO_IMR_I];

    if (imr == 0)
    {
        /* First enabled interrupt for this GPIO */
        avic_enable_int(desc->ints, IRQ, desc->list->priority,
                        desc->handler);
    }

    /* Set the line sense */
    icr = &base[GPIO_ICR1_I] + event->line / 16;
    shift = 2*(event->line % 16);
    mask = GPIO_SENSE_CONFIG_MASK << shift;

    *icr = (*icr & ~mask) | ((event->sense << shift) & mask);

    /* Unmask the line */
    base[GPIO_IMR_I] = imr | (1ul << event->line);

    restore_irq(oldlevel);

    return true;
}

void gpio_disable_event(enum gpio_module_number gpio, unsigned id)
{
    const struct gpio_module_descriptor * const desc = &gpio_descs[gpio];
    const struct gpio_event * const event = &desc->list->events[id];
    volatile unsigned long * const base = desc->base;
    unsigned long imr;

    if (id >= desc->list->count)
        return;

    int oldlevel = disable_irq_save();

    /* Remove bit from mask */
    imr = base[GPIO_IMR_I] & ~(1ul << event->line);

    /* Mask the line */
    base[GPIO_IMR_I] = imr;

    if (imr == 0)
    {
        /* No events remain enabled */
        avic_disable_int(desc->ints);
    }

    restore_irq(oldlevel);
}
