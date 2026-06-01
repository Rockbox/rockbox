/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Aidan MacDonald
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
#include "aic310x.h"
#include "system.h"

/* Call with mutex held */
static int aic310x_set_page(struct aic310x *codec, uint8_t page)
{
    if (codec->current_page == page)
        return 0;

    codec->current_page = page;

    int err = codec->write_multiple(AIC310X_PAGE_SELECT, &codec->current_page, 1);
    if (err)
    {
        /* Clear current page on error */
        codec->current_page = 0xFF;
    }

    return err;
}

static int aic310x_readwrite_multiple(struct aic310x *codec, bool write,
                                      uint8_t reg, uint8_t *data, size_t count)
{
    mutex_lock(&codec->lock);

    uint8_t reg_page = AIC310X_REG_PAGE(reg);
    uint8_t reg_addr = AIC310X_REG_ADDR(reg);

    int err = aic310x_set_page(codec, reg_page);
    if (err)
        goto out;

    if (write)
        err = codec->write_multiple(reg_addr, data, count);
    else
        err = codec->read_multiple(reg_addr, data, count);

out:
    mutex_unlock(&codec->lock);
    return err;
}

void aic310x_init(struct aic310x *codec)
{
    mutex_init(&codec->lock);

    /* ensure we issue a page select before any register access */
    codec->current_page = 0xFF;
}

int aic310x_read_multiple(struct aic310x *codec,
                                uint8_t reg, uint8_t *data, size_t count)
{
    return aic310x_readwrite_multiple(codec, false, reg, data, count);
}

int aic310x_write_multiple(struct aic310x *codec,
                           uint8_t reg, const uint8_t *data, size_t count)
{
    return aic310x_readwrite_multiple(codec, true, reg, (uint8_t *)data, count);
}

int aic310x_read(struct aic310x *codec, uint8_t reg, uint8_t *value)
{
    return aic310x_read_multiple(codec, reg, value, 1);
}

int aic310x_write(struct aic310x *codec, uint8_t reg, uint8_t value)
{
    return aic310x_write_multiple(codec, reg, &value, 1);
}

int aic310x_modify(struct aic310x *codec,
                   uint8_t reg, uint8_t mask, uint8_t newvalue)
{
    mutex_lock(&codec->lock);

    uint8_t tmp;
    int err = aic310x_read(codec, reg, &tmp);
    if (err)
        goto out;

    tmp &= ~mask;
    tmp |= newvalue;

    err = aic310x_write(codec, reg, tmp);

out:
    mutex_unlock(&codec->lock);
    return err;
}
