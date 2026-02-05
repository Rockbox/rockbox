/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2026 by Aidan MacDonald
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
#ifndef __DMAMUX_STM32H743_H__
#define __DMAMUX_STM32H743_H__

/* DMAMUX1 request multiplexer inputs (DMAMUX_CR.DMAREQ_ID) */
#define DMAMUX1_REQ_GEN0                1
#define DMAMUX1_REQ_GEN1                2
#define DMAMUX1_REQ_GEN2                3
#define DMAMUX1_REQ_GEN3                4
#define DMAMUX1_REQ_GEN4                5
#define DMAMUX1_REQ_GEN5                6
#define DMAMUX1_REQ_GEN6                7
#define DMAMUX1_REQ_GEN7                8
#define DMAMUX1_REQ_ADC1_DMA            9
#define DMAMUX1_REQ_ADC2_DMA            10
#define DMAMUX1_REQ_TIM1_CH1            11
#define DMAMUX1_REQ_TIM1_CH2            12
#define DMAMUX1_REQ_TIM1_CH3            13
#define DMAMUX1_REQ_TIM1_CH4            14
#define DMAMUX1_REQ_TIM1_UP             15
#define DMAMUX1_REQ_TIM1_TRIG           16
#define DMAMUX1_REQ_TIM1_COM            16
#define DMAMUX1_REQ_TIM2_CH1            18
#define DMAMUX1_REQ_TIM2_CH2            19
#define DMAMUX1_REQ_TIM2_CH3            20
#define DMAMUX1_REQ_TIM2_CH4            21
#define DMAMUX1_REQ_TIM2_UP             22
#define DMAMUX1_REQ_TIM3_CH1            23
#define DMAMUX1_REQ_TIM3_CH2            24
#define DMAMUX1_REQ_TIM3_CH3            25
#define DMAMUX1_REQ_TIM3_CH4            26
#define DMAMUX1_REQ_TIM3_UP             27
#define DMAMUX1_REQ_TIM3_TRIG           28
#define DMAMUX1_REQ_TIM4_CH1            29
#define DMAMUX1_REQ_TIM4_CH2            30
#define DMAMUX1_REQ_TIM4_CH3            31
#define DMAMUX1_REQ_TIM4_UP             32
#define DMAMUX1_REQ_I2C1_RX_DMA         33
#define DMAMUX1_REQ_I2C1_TX_DMA         34
#define DMAMUX1_REQ_I2C2_RX_DMA         35
#define DMAMUX1_REQ_I2C2_TX_DMA         36
#define DMAMUX1_REQ_SPI1_RX_DMA         37
#define DMAMUX1_REQ_SPI1_TX_DMA         38
#define DMAMUX1_REQ_SPI2_RX_DMA         39
#define DMAMUX1_REQ_SPI2_TX_DMA         40
#define DMAMUX1_REQ_USART1_RX_DMA       41
#define DMAMUX1_REQ_USART1_TX_DMA       42
#define DMAMUX1_REQ_USART2_RX_DMA       43
#define DMAMUX1_REQ_USART2_TX_DMA       44
#define DMAMUX1_REQ_USART3_RX_DMA       45
#define DMAMUX1_REQ_USART3_TX_DMA       46
#define DMAMUX1_REQ_TIM8_CH1            47
#define DMAMUX1_REQ_TIM8_CH2            48
#define DMAMUX1_REQ_TIM8_CH3            49
#define DMAMUX1_REQ_TIM8_CH4            50
#define DMAMUX1_REQ_TIM8_UP             51
#define DMAMUX1_REQ_TIM8_TRIG           52
#define DMAMUX1_REQ_TIM8_COM            53
#define DMAMUX1_REQ_TIM5_CH1            55
#define DMAMUX1_REQ_TIM5_CH2            56
#define DMAMUX1_REQ_TIM5_CH3            57
#define DMAMUX1_REQ_TIM5_CH4            58
#define DMAMUX1_REQ_TIM5_UP             59
#define DMAMUX1_REQ_TIM5_TRIG           60
#define DMAMUX1_REQ_SPI3_RX_DMA         61
#define DMAMUX1_REQ_SPI3_TX_DMA         62
#define DMAMUX1_REQ_UART4_RX_DMA        63
#define DMAMUX1_REQ_UART4_TX_DMA        64
#define DMAMUX1_REQ_UART5_RX_DMA        65
#define DMAMUX1_REQ_UART5_TX_DMA        66
#define DMAMUX1_REQ_DAC_CH1_DMA         67
#define DMAMUX1_REQ_DAC_CH2_DMA         68
#define DMAMUX1_REQ_TIM6_UP             69
#define DMAMUX1_REQ_TIM7_UP             70
#define DMAMUX1_REQ_USART6_RX_DMA       71
#define DMAMUX1_REQ_USART6_TX_DMA       72
#define DMAMUX1_REQ_I2C3_RX_DMA         73
#define DMAMUX1_REQ_I2C3_TX_DMA         74
#define DMAMUX1_REQ_DCMI_DMA            75
#define DMAMUX1_REQ_CRYP_IN_DMA         76
#define DMAMUX1_REQ_CRYP_OUT_DMA        77
#define DMAMUX1_REQ_HASH_IN_DMA         78
#define DMAMUX1_REQ_UART7_RX_DMA        79
#define DMAMUX1_REQ_UART7_TX_DMA        80
#define DMAMUX1_REQ_UART8_RX_DMA        81
#define DMAMUX1_REQ_UART8_TX_DMA        82
#define DMAMUX1_REQ_SPI4_RX_DMA         83
#define DMAMUX1_REQ_SPI4_TX_DMA         84
#define DMAMUX1_REQ_SPI5_RX_DMA         85
#define DMAMUX1_REQ_SPI5_TX_DMA         86
#define DMAMUX1_REQ_SAI1A_DMA           87
#define DMAMUX1_REQ_SAI1B_DMA           88
#define DMAMUX1_REQ_SAI2A_DMA           89
#define DMAMUX1_REQ_SAI2B_DMA           90
#define DMAMUX1_REQ_SWPMI_RX_DMA        91
#define DMAMUX1_REQ_SWPMI_TX_DMA        92
#define DMAMUX1_REQ_SPDIFRX_DAT_DMA     93
#define DMAMUX1_REQ_SPDIFRX_CTRL_DMA    94
#define DMAMUX1_REQ_HR_REQ1             95
#define DMAMUX1_REQ_HR_REQ2             96
#define DMAMUX1_REQ_HR_REQ3             97
#define DMAMUX1_REQ_HR_REQ4             98
#define DMAMUX1_REQ_HR_REQ5             99
#define DMAMUX1_REQ_HR_REQ6             100
#define DMAMUX1_REQ_DFSDM1_DMA0         101
#define DMAMUX1_REQ_DFSDM1_DMA1         102
#define DMAMUX1_REQ_DFSDM1_DMA2         103
#define DMAMUX1_REQ_DFSDM1_DMA3         104
#define DMAMUX1_REQ_TIM15_CH1           105
#define DMAMUX1_REQ_TIM15_UP            106
#define DMAMUX1_REQ_TIM15_TRIG          107
#define DMAMUX1_REQ_TIM15_COM           108
#define DMAMUX1_REQ_TIM16_CH1           109
#define DMAMUX1_REQ_TIM16_UP            110
#define DMAMUX1_REQ_TIM17_CH1           111
#define DMAMUX1_REQ_TIM17_UP            112
#define DMAMUX1_REQ_SAI3A_DMA           113
#define DMAMUX1_REQ_SAI3B_DMA           114
#define DMAMUX1_REQ_ADC3_DMA            115

/* DMAMUX2 request multiplexer inputs (DMAMUX_CR.DMAREQ_ID) */
#define DMAMUX2_REQ_GEN0                1
#define DMAMUX2_REQ_GEN1                2
#define DMAMUX2_REQ_GEN2                3
#define DMAMUX2_REQ_GEN3                4
#define DMAMUX2_REQ_GEN4                5
#define DMAMUX2_REQ_GEN5                6
#define DMAMUX2_REQ_GEN6                7
#define DMAMUX2_REQ_GEN7                8
#define DMAMUX2_REQ_LPUART1_RX_DMA      9
#define DMAMUX2_REQ_LPUART1_TX_DMA      10
#define DMAMUX2_REQ_SPI6_RX_DMA         11
#define DMAMUX2_REQ_SPI6_TX_DMA         12
#define DMAMUX2_REQ_I2C4_RX_DMA         13
#define DMAMUX2_REQ_I2C4_TX_DMA         14
#define DMAMUX2_REQ_SAI4A_DMA           15
#define DMAMUX2_REQ_SAI4B_DMA           16
#define DMAMUX2_REQ_ADC3_DMA            17

#endif /* __DMAMUX_STM32H743_H__ */
