/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Christian Gmeiner
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <errno.h>
#include <string.h>
#include <ctype.h>
#include "usbstack.h"

#include "config.h"

#include "usbstack/core.h"
#include "usbstack/config.h"
#include "usbstack/controller.h"
#include "usbstack/drivers/device/usb_serial.h"
#include "usbstack/drivers/device/usb_storage.h"

struct usb_core usbcore;

/* private used functions */
static void update_driver_names(unsigned char* result);
static void bind_device_driver(struct usb_device_driver* driver);

/**
 * Initialize usb stack.
 */
void usb_stack_init(void)
{
    int i;
    logf("usb_stack_init");

    /* init datastructures */
    usbcore.controller[0] = NULL;
    usbcore.controller[1] = NULL;
    usbcore.active_controller = NULL;
    usbcore.device_driver = NULL;
    usbcore.running = false;

    memset(&device_driver_names, 0, USB_STACK_MAX_SETTINGS_NAME);

    /* init arrays */
    for (i = 0; i < NUM_DRIVERS; i++) {
        usbcore.device_drivers[i] = NULL;
        usbcore.host_drivers[i] = NULL;
    }

    /* init controllers */
#if (USBSTACK_CAPS & CONTROLLER_DEVICE)
    usb_dcd_init();
#endif

#if (USBSTACK_CAPS & CONTROLLER_HOST)
    usb_hcd_init();
#endif

    /* init drivers */
    usb_serial_driver_init();
    usb_storage_driver_init();
}

/**
 * Start processing of usb stack. This function init
 * active usb controller.
 */
void usb_stack_start(void)
{
    /* are we allready running? */
    if (usbcore.running) {
        logf("allready running!");
        return;
    }

    if (usbcore.active_controller == NULL) {
        logf("no active controller!");
        return;
    }

    /* forward to controller */
    logf("starting controller");
    usbcore.active_controller->start();
    usbcore.running = true;

    /* look if started controller is a device controller
     * and if it has a device driver bind to it */
    logf("check for auto bind");
    if (usbcore.active_controller->type == DEVICE) {
        if (usbcore.active_controller->device_driver == NULL && usbcore.device_driver != NULL) {
            /* bind driver */
            logf("binding...");
            bind_device_driver(usbcore.device_driver);
        }
    }
}

/**
 * Stop processing of usb stack. This function shutsdown
 * active usb controller.
 */
void usb_stack_stop(void)
{
    /* are we allready stopped? */
    if (usbcore.running == false) {
        return;
    }

    /* forward to controller */
    usbcore.active_controller->stop();
    usbcore.running = false;
}

/**
 * Gets called by upper layers to indicate that there is
 * an interrupt waiting for the controller.
 */
void usb_stack_irq(void)
{
    /* simply notify usb controller */
    if (usbcore.active_controller != NULL && usbcore.active_controller->irq != NULL) {
        usbcore.active_controller->irq();
    }
}

/**
 * If a host device controller is loaded, we need to have a function
 * to call for maintanence. We need to check if a new device has connected,
 * find suitable drivers for new devices.
 */
void usb_stack_work(void)
{
    /* TODO will be used with host device controllers 
     * and needs to be called in a loop (thread) */
}

/**
 * Register an usb controller in the stack. The stack can
 * only have two controllers registered at one time.
 * One device host controller and one host device controller.
 * 
 * @param ctrl pointer to controller to register.
 * @return 0 on success else a defined error code. 
 */
int usb_controller_register(struct usb_controller* ctrl)
{
    if (ctrl == NULL) {
        return EINVAL;
    }

    logf("usb_stack: register usb ctrl");
    logf("  -> name: %s", ctrl->name);
    logf("  -> type: %d", ctrl->type);

    switch (ctrl->type) {
    case DEVICE:
        if (usbcore.controller[0] == NULL) {
            usbcore.controller[0] = ctrl;
            return 0;
        }
        break;
    case HOST:
        if (usbcore.controller[1] == NULL) {
            usbcore.controller[1] = ctrl;
            return 0;
        }
        break;
    default:
        return EINVAL;
    }

    return ENOFREESLOT;
}

/**
 * Unregister an usb controller from the stack.
 * 
 * @param ctrl pointer to controller to unregister.
 * @return 0 on success else a defined error code. 
 */
int usb_controller_unregister(struct usb_controller* ctrl) {

    if (ctrl == NULL) {
        return EINVAL;
    }

    switch (ctrl->type) {
    case DEVICE:
        if (usbcore.controller[0] == ctrl) {
            usbcore.controller[0] = NULL;
            return 0;
        }
        break;
    case HOST:
        if (usbcore.controller[1] == ctrl) {
            usbcore.controller[1] = NULL;
            return 0;
        }
        break;
    default:
        return EINVAL;
    }

    return 0; /* never reached */
}

/**
 * Select an usb controller and active it.
 * 
 * @param type of controller to activate.
 */
void usb_controller_select(int type)
{
    struct usb_controller* new = NULL;

    /* check if a controller of the wanted type is already loaded */
    if (usbcore.active_controller != NULL && (int)usbcore.active_controller->type == type) {
        logf("controller already set");
        return;
    }

    logf("usb_controller_select");
    logf("  -> type: %d", type);

    usbcore.mode = type;

    switch (type) {
    case DEVICE:
        new = usbcore.controller[0];
        break;
    case HOST:
        new = usbcore.controller[1];
        break;
    }

    /* if there is only one controller, stop here */
    if (new == NULL) {
        logf("no suitable cntrl found");
        return;
    }

    /* shutdown current used controller */
    if (usbcore.active_controller != NULL) {
        logf("shuting down old one");
        usbcore.active_controller->shutdown();
    }

    /* set and init new controller */
    usbcore.active_controller = new;
    logf("init controller");
    usbcore.active_controller->init();
}

int usb_stack_get_mode(void) {
    return usbcore.mode;
}

/**
 * Register an usb device driver.
 * 
 * @param driver pointer to an usb_device_driver struct.
 * @return 0 on success, else a defined error code.
 */
int usb_device_driver_register(struct usb_device_driver* driver)
{
    int i;

    if (driver == NULL) {
        return EINVAL;
    }

    /* add to linked list */
    logf("usb_stack: register usb driver");
    for (i = 0; i < NUM_DRIVERS; i++) {
        if (usbcore.device_drivers[i] == NULL) {
            usbcore.device_drivers[i] = driver;
            update_driver_names(device_driver_names);
            return 0;
        }
    }

    update_driver_names(device_driver_names);

    return 0;
}

int usb_device_driver_bind(const char* name) {

    int i;
    struct usb_device_driver *tmp = NULL;
    struct usb_device_driver *driver = NULL;

    if (name == NULL) {
        return EINVAL;
    }

    /* look for driver */
    logf("looking for driver %s", name);
    for (i = 0; i < NUM_DRIVERS; i++) {
        tmp = usbcore.device_drivers[i];
        if (tmp != NULL && strcmp(name, tmp->name) == 0) {
            driver = tmp;
        }
    }
 
    if (driver == NULL) {
         logf("no driver found");
         return ENODRIVERFOUND;
    } 

    /* look if there is an usb controller loaded */
    if (usbcore.active_controller == NULL) {
        /* safe choosen driver and set it when controller starts */
        usbcore.device_driver = driver;

    } else {

        /* we need to have an active dcd controller */
        if (usbcore.active_controller->type != DEVICE) {
            logf("wrong type");
            return EWRONGCONTROLLERTYPE;
        }

        /* bind driver to controller */
        bind_device_driver(driver);
    }

    return 0;
}

void usb_device_driver_unbind(void) {

    logf("usb_device_driver_unbind");
    if (usbcore.active_controller->device_driver != NULL) {
        usbcore.active_controller->device_driver->unbind();
        usbcore.active_controller->device_driver = NULL;
    }

    usbcore.device_driver = NULL;
}

static void update_driver_names(unsigned char* result) {

    int i;
    int pos = 0;
    unsigned char terminator = ',';
    struct usb_device_driver* dd = NULL;

    /* reset buffer, iterate through drivers and add to char array */
    memset(result, 0, USB_STACK_MAX_SETTINGS_NAME);
    for (i = 0; i < NUM_DRIVERS; i++) {
        int len;
        dd = usbcore.device_drivers[i];

        if (dd !=  NULL) {
            len = strlen(dd->name);
            if (pos > 0) {
                memcpy(result + pos, &terminator, 1);
                pos++;
            }
            memcpy(result + pos, dd->name, len);
            pos += len;
        }
    }
}

static void bind_device_driver(struct usb_device_driver* driver) {

    /* look if there is an old driver */
    if (usbcore.active_controller->device_driver != NULL) {
        usbcore.active_controller->device_driver->unbind();
    }

    /* bind driver to controller */
    usbcore.active_controller->device_driver = driver;

    /* init dirver */
    driver->bind(usbcore.active_controller->controller_ops);
}


