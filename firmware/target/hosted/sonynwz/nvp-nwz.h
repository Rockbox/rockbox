/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Amaury Pouly
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
#ifndef __NVP_NWZ_H__
#define __NVP_NWZ_H__

#include "system.h"
#include "nwz-db.h"

/* get model ID */
unsigned long nwz_get_model_id(void);
/* get model NAME (ie NWZ-E463) */
const char *nwz_get_model_name(void);
/* return series (index into nwz_db) */
int nwz_get_series(void);

/* read a nvp node and return its size, if the data pointer is null, then simply
 * return the size, return -1 on error */
int nwz_nvp_read(enum nwz_nvp_node_t node, void *data);
/* write a nvp node, return 0 on success and -1 on error, the size of the buffer
 * must be the one returned by nwz_nvp_read */
int nwz_nvp_write(enum nwz_nvp_node_t node, void *data);

#endif /* __NVP_NWZ_H__ */
