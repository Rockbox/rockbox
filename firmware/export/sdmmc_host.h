/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 Aidan MacDonald
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
#ifndef __SDMMC_HOST_H__
#define __SDMMC_HOST_H__

/*
 * Generic SD/MMC host driver which implements the storage
 * API for targets with SD/MMC based storage. Targets only
 * have to implement a simplified controller interface to
 * process SD/MMC commands, and manage clocks/power.
 *
 * sdmmc_host supports hotswappable storage, but the target
 * is responsible for detecting insertion and removals, and
 * informing sdmmc_host.
 *
 * To enable this driver add the following defines in your
 * target config.h to set the number of SD/MMC controllers
 * on the system:
 *
 * - SDMMC_HOST_NUM_SD_CONTROLLERS
 * - SDMMC_HOST_NUM_MMC_CONTROLLERS
 *
 * You also need to implement `struct sdmmc_controller_ops`
 * for your hardware, and initialize the host drivers for
 * each storage device from the target-specific function
 * `sdmmc_host_target_init()`.
 */

#include "storage.h"
#include "sdmmc.h"
#include "mutex.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* Response length flags */
#define SDMMC_RESP_NONE             0x00
#define SDMMC_RESP_SHORT            0x01
#define SDMMC_RESP_LONG             0x02
#define SDMMC_RESP_LENGTH(flags)    ((flags) & 0x03)

/* Response requires busy polling */
#define SDMMC_RESP_BUSY             0x04

/* Response has no CRC */
#define SDMMC_RESP_NOCRC            0x08

/* Data present & direction of data transfer */
#define SDMMC_DATA_READ             0x10
#define SDMMC_DATA_WRITE            0x30
#define SDMMC_DATA_PRESENT(flags)   ((flags) & 0x10)
#define SDMMC_DATA_DIR(flags)       ((flags) & 0x30)

/* Status codes returned by submit_command() callback */
#define SDMMC_STATUS_OK             0
#define SDMMC_STATUS_ERROR          (-1)
#define SDMMC_STATUS_TIMEOUT        (-2)
#define SDMMC_STATUS_INVALID_CRC    (-3)

struct sdmmc_host_command
{
    uint16_t flags;
    uint16_t command;
    uint32_t argument;
    uint16_t nr_blocks;
    uint16_t block_len;
    void *buffer;
};

struct sdmmc_host_response
{
    uint32_t data[4];
};

/* Possible bus voltages */
#define SDMMC_BUS_VOLTAGE_2V7_2V8   (1 << 15)
#define SDMMC_BUS_VOLTAGE_2V8_2V9   (1 << 16)
#define SDMMC_BUS_VOLTAGE_2V9_3V0   (1 << 17)
#define SDMMC_BUS_VOLTAGE_3V0_3V1   (1 << 18)
#define SDMMC_BUS_VOLTAGE_3V1_3V2   (1 << 19)
#define SDMMC_BUS_VOLTAGE_3V2_3V3   (1 << 20)
#define SDMMC_BUS_VOLTAGE_3V3_3V4   (1 << 21)
#define SDMMC_BUS_VOLTAGE_3V4_3V5   (1 << 22)
#define SDMMC_BUS_VOLTAGE_3V5_3V6   (1 << 23)

/* Possible bus widths */
#define SDMMC_BUS_WIDTH_1BIT        (1 << 0)
#define SDMMC_BUS_WIDTH_4BIT        (1 << 1)
#define SDMMC_BUS_WIDTH_8BIT        (1 << 2)

/* Possible bus clock speeds */
#define SDMMC_BUS_CLOCK_400KHZ      (1 << 0)
#define SDMMC_BUS_CLOCK_25MHZ       (1 << 1)
#define SDMMC_BUS_CLOCK_50MHZ       (1 << 2)

/**
 * Callbacks for implementing an SD/MMC host controller.
 *
 * Most functions here are called with the sdmmc_host lock
 * held which prevents multiple threads from accessing the
 * same controller.
 */
struct sdmmc_controller_ops
{
    /**
     * \brief Enable/disable power supply of the SD/MMC device
     * \param controller    Controller pointer
     * \param enable        Whether power should be enabled or not
     *
     * sdmmc_host will try to power cycle the device after
     * non-recoverable errors. Removable devices will also
     * have bus power enabled/disabled when the device is
     * inserted or removed.
     *
     * Even if the target can't physically power cycle the
     * card, a reset of the SDMMC controller is often good
     * enough.
     *
     * Optional. Not implementing this function does not
     * change any behavior of sdmmc_host -- it will still
     * try to recover from errors by reinitializing the
     * SDMMC device.
     */
    void (*set_power_enabled) (void *controller, bool enable);

    /**
     * \brief Set the bus width (number of active data lines)
     * \param controller    Controller pointer
     * \param width         One of the SDMMC_BUS_WIDTH constants
     *
     * Optional. If not implemented then the bus width is
     * limited to 1 bit wide.
     */
    void (*set_bus_width) (void *controller, uint32_t width);

    /**
     * \brief Set the frequency of the clock provided to the SD/MMC device
     * \param controller    Controller pointer
     * \param clock         One of the SDMMC_BUS_CLOCK constants
     *
     * All controllers must implement this funtion and support
     * at least 400 KHz and 25 MHz rates.
     */
    void (*set_bus_clock) (void *controller, uint32_t clock);

    /**
     * \brief Submit a command to the controller and wait for the response
     * \param cmd       Command descriptor
     * \param resp      Response data buffer; may be NULL
     *
     * \retval SDMMC_STATUS_OK          if command completed successfully
     * \retval SDMMC_STATUS_TIMEOUT     if failed due to command timeout
     * \retval SDMMC_STATUS_INVALID_CRC if failed due to an invalid CRC
     * \retval SDMMC_STATUS_ERROR       if failed in any other way
     *
     * Note that if `resp` is NULL, the controller must still use
     * the response format specified in `cmd->flags`; the response
     * data, if any, should be discarded.
     */
    int (*submit_command) (void *controller,
                           const struct sdmmc_host_command *cmd,
                           struct sdmmc_host_response *resp);

    /**
     * Abort command processing. Any in-progress call to
     * submit_command() must be made to return as soon as
     * possible with a nonzero status code.
     *
     * If no command is in progress then this function
     * must do nothing.
     *
     * Optional. If not implemented then the controller is
     * responsible for ensuring that submit_command() will
     * eventually return even in case of device removal or
     * a command unexpectedly hanging.
     *
     * May be called without the sdmmc_host lock held.
     */
    void (*abort_command) (void *controller);
};

struct sdmmc_host_config
{
    /* Storage type: either STORAGE_SD or STORAGE_MMC */
    int type;

    /* Supported voltages, bus widths, and clock speeds */
    uint32_t bus_voltages;
    uint32_t bus_widths;
    uint32_t bus_clocks;

    /* Set to true if the device is removable at runtime */
    bool is_removable;
};

struct sdmmc_host
{
    /* Static configuration */
    struct sdmmc_host_config config;

    /* Physical drive number */
    int drive;

#ifdef HAVE_HOTSWAP
    /* Medium presence flag */
    volatile int present;
#endif

    /*
     * Lock which protects most controller operations
     * and mutable sdmmc_host state.
     */
    struct mutex lock;

    /* Bus & device state flags; must only be accessed with lock held */
    bool enabled     : 1;
    bool need_reset  : 1;
    bool powered     : 1;
    bool initialized : 1;
    bool is_hcs_card : 1;

    /* Controller implemented by the target */
    const struct sdmmc_controller_ops *ops;
    void *controller;

    /* Card info probed from card */
    tCardInfo cardinfo;
};

/**
 * Called during storage_init() to add SD/MMC host controllers.
 * The target should call sdmmc_host_init() for each controller
 * in the system.
 */
void sdmmc_host_target_init(void) INIT_ATTR;

/**
 * \brief Initialize an SD/MMC host controller
 * \param host              Host driver state, allocated by the target
 * \param config            Static configuration for host controller
 * \param ops               Controller callbacks
 * \param controller        Opaque pointer passed to ops callbacks
 *
 * Physical drive numbers will be assigned in the order of
 * initialization, eg. the first host will be drive 0, the
 * second will be drive 1, and so forth.
 */
void sdmmc_host_init(struct sdmmc_host *host,
                     const struct sdmmc_host_config *config,
                     const struct sdmmc_controller_ops *ops,
                     void *controller) INIT_ATTR;

/**
 * \brief Set initial SD/MMC medium presence
 * \param host              Host controller
 * \param present           True if medium is present
 * \note Only use this function from `sdmmc_host_target_init()` to set
 *       the initial medium presence state. Only needs to be called for
 *       drives that support removable media; non-removable drives will
 *       always be considered inserted.
 */
void sdmmc_host_init_medium_present(struct sdmmc_host *host, bool present) INIT_ATTR;

/**
 * \brief Set SD/MMC medium presence state
 * \param host              Host controller
 * \param present           True if medium is present
 * \note Do not use this function from `sdmmc_host_target_init()` since
 *       it relies on the storage queue being available. You must instead
 *       call `sdmmc_host_init_medium_present()` to set the initial state.
 */
void sdmmc_host_set_medium_present(struct sdmmc_host *host, bool present);

#endif /* __SDMMC_HOST_H__ */
