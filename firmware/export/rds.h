/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2011 by Bertrik Sikken
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
#ifndef RDS_H
#define RDS_H

#include <stdint.h>
#include <stdbool.h>
#include "time.h"

void rds_init(void);
void rds_reset(void);
void rds_sync(void);

#if (CONFIG_RDS & RDS_CFG_PROCESS)
/* RDS raw data processing */
void rds_process(const uint16_t data[4]);
#endif /* (CONFIG_RDS & RDS_CFG_PROCESS) */

enum rds_info_id
{
    RDS_INFO_NULL = 0,
    RDS_INFO_CODEABLE,  /* code table, right now only G0 */
    RDS_INFO_PI,        /* programme identifier */
    RDS_INFO_PS,        /* programme service name */
    RDS_INFO_RT,        /* radio text */
    RDS_INFO_CT,        /* clock time */
};

enum rds_code_table
{
    RDS_CT_G0,          /* default code table G0 */
    RDS_CT_G1,          /* alternate code table G1 */
    RDS_CT_G2,          /* alternate code table G2 */
};

#if (CONFIG_RDS & RDS_CFG_PUSH)
/* pushes preprocesed RDS information */
void rds_push_info(enum rds_info_id info_id, uintptr_t data, size_t size);
#endif /* (CONFIG_RDS & RDS_CFG_PUSH) */

/* read fully-processed RDS data */
size_t rds_pull_info(enum rds_info_id info_id, uintptr_t data, size_t size);

#endif /* RDS_H */
