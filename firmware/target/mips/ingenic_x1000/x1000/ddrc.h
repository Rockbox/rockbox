/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * This file was automatically generated by headergen, DO NOT EDIT it.
 * headergen version: 3.0.0
 * x1000 version: 1.0
 * x1000 authors: Aidan MacDonald
 *
 * Copyright (C) 2015 by the authors
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
#ifndef __HEADERGEN_DDRC_H__
#define __HEADERGEN_DDRC_H__

#include "macro.h"

#define REG_DDRC_STATUS jz_reg(DDRC_STATUS)
#define JA_DDRC_STATUS  (0xb34f0000 + 0x0)
#define JT_DDRC_STATUS  JIO_32_RW
#define JN_DDRC_STATUS  DDRC_STATUS
#define JI_DDRC_STATUS  

#define REG_DDRC_CFG    jz_reg(DDRC_CFG)
#define JA_DDRC_CFG     (0xb34f0000 + 0x4)
#define JT_DDRC_CFG     JIO_32_RW
#define JN_DDRC_CFG     DDRC_CFG
#define JI_DDRC_CFG     

#define REG_DDRC_CTRL   jz_reg(DDRC_CTRL)
#define JA_DDRC_CTRL    (0xb34f0000 + 0x8)
#define JT_DDRC_CTRL    JIO_32_RW
#define JN_DDRC_CTRL    DDRC_CTRL
#define JI_DDRC_CTRL    

#define REG_DDRC_TIMING1    jz_reg(DDRC_TIMING1)
#define JA_DDRC_TIMING1     (0xb34f0000 + 0x60)
#define JT_DDRC_TIMING1     JIO_32_RW
#define JN_DDRC_TIMING1     DDRC_TIMING1
#define JI_DDRC_TIMING1     

#define REG_DDRC_TIMING2    jz_reg(DDRC_TIMING2)
#define JA_DDRC_TIMING2     (0xb34f0000 + 0x64)
#define JT_DDRC_TIMING2     JIO_32_RW
#define JN_DDRC_TIMING2     DDRC_TIMING2
#define JI_DDRC_TIMING2     

#define REG_DDRC_TIMING3    jz_reg(DDRC_TIMING3)
#define JA_DDRC_TIMING3     (0xb34f0000 + 0x68)
#define JT_DDRC_TIMING3     JIO_32_RW
#define JN_DDRC_TIMING3     DDRC_TIMING3
#define JI_DDRC_TIMING3     

#define REG_DDRC_TIMING4    jz_reg(DDRC_TIMING4)
#define JA_DDRC_TIMING4     (0xb34f0000 + 0x6c)
#define JT_DDRC_TIMING4     JIO_32_RW
#define JN_DDRC_TIMING4     DDRC_TIMING4
#define JI_DDRC_TIMING4     

#define REG_DDRC_TIMING5    jz_reg(DDRC_TIMING5)
#define JA_DDRC_TIMING5     (0xb34f0000 + 0x70)
#define JT_DDRC_TIMING5     JIO_32_RW
#define JN_DDRC_TIMING5     DDRC_TIMING5
#define JI_DDRC_TIMING5     

#define REG_DDRC_TIMING6    jz_reg(DDRC_TIMING6)
#define JA_DDRC_TIMING6     (0xb34f0000 + 0x74)
#define JT_DDRC_TIMING6     JIO_32_RW
#define JN_DDRC_TIMING6     DDRC_TIMING6
#define JI_DDRC_TIMING6     

#define REG_DDRC_REFCNT jz_reg(DDRC_REFCNT)
#define JA_DDRC_REFCNT  (0xb34f0000 + 0x18)
#define JT_DDRC_REFCNT  JIO_32_RW
#define JN_DDRC_REFCNT  DDRC_REFCNT
#define JI_DDRC_REFCNT  

#define REG_DDRC_MMAP0  jz_reg(DDRC_MMAP0)
#define JA_DDRC_MMAP0   (0xb34f0000 + 0x24)
#define JT_DDRC_MMAP0   JIO_32_RW
#define JN_DDRC_MMAP0   DDRC_MMAP0
#define JI_DDRC_MMAP0   

#define REG_DDRC_MMAP1  jz_reg(DDRC_MMAP1)
#define JA_DDRC_MMAP1   (0xb34f0000 + 0x28)
#define JT_DDRC_MMAP1   JIO_32_RW
#define JN_DDRC_MMAP1   DDRC_MMAP1
#define JI_DDRC_MMAP1   

#define REG_DDRC_DLP    jz_reg(DDRC_DLP)
#define JA_DDRC_DLP     (0xb34f0000 + 0xbc)
#define JT_DDRC_DLP     JIO_32_RW
#define JN_DDRC_DLP     DDRC_DLP
#define JI_DDRC_DLP     

#define REG_DDRC_REMAP1 jz_reg(DDRC_REMAP1)
#define JA_DDRC_REMAP1  (0xb34f0000 + 0x9c)
#define JT_DDRC_REMAP1  JIO_32_RW
#define JN_DDRC_REMAP1  DDRC_REMAP1
#define JI_DDRC_REMAP1  

#define REG_DDRC_REMAP2 jz_reg(DDRC_REMAP2)
#define JA_DDRC_REMAP2  (0xb34f0000 + 0xa0)
#define JT_DDRC_REMAP2  JIO_32_RW
#define JN_DDRC_REMAP2  DDRC_REMAP2
#define JI_DDRC_REMAP2  

#define REG_DDRC_REMAP3 jz_reg(DDRC_REMAP3)
#define JA_DDRC_REMAP3  (0xb34f0000 + 0xa4)
#define JT_DDRC_REMAP3  JIO_32_RW
#define JN_DDRC_REMAP3  DDRC_REMAP3
#define JI_DDRC_REMAP3  

#define REG_DDRC_REMAP4 jz_reg(DDRC_REMAP4)
#define JA_DDRC_REMAP4  (0xb34f0000 + 0xa8)
#define JT_DDRC_REMAP4  JIO_32_RW
#define JN_DDRC_REMAP4  DDRC_REMAP4
#define JI_DDRC_REMAP4  

#define REG_DDRC_REMAP5 jz_reg(DDRC_REMAP5)
#define JA_DDRC_REMAP5  (0xb34f0000 + 0xac)
#define JT_DDRC_REMAP5  JIO_32_RW
#define JN_DDRC_REMAP5  DDRC_REMAP5
#define JI_DDRC_REMAP5  

#define REG_DDRC_AUTOSR_CNT jz_reg(DDRC_AUTOSR_CNT)
#define JA_DDRC_AUTOSR_CNT  (0xb34f0000 + 0x308)
#define JT_DDRC_AUTOSR_CNT  JIO_32_RW
#define JN_DDRC_AUTOSR_CNT  DDRC_AUTOSR_CNT
#define JI_DDRC_AUTOSR_CNT  

#define REG_DDRC_AUTOSR_EN  jz_reg(DDRC_AUTOSR_EN)
#define JA_DDRC_AUTOSR_EN   (0xb34f0000 + 0x304)
#define JT_DDRC_AUTOSR_EN   JIO_32_RW
#define JN_DDRC_AUTOSR_EN   DDRC_AUTOSR_EN
#define JI_DDRC_AUTOSR_EN   

#endif /* __HEADERGEN_DDRC_H__*/
