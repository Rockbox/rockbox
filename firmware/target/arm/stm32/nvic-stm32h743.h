/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 by Aidan MacDonald
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
#ifndef __NVIC_STM32H743_H__
#define __NVIC_STM32H743_H__

#define NVIC_IRQN_WWDG1                  0
#define NVIC_IRQN_PVD_PVM                1
#define NVIC_IRQN_RTC_TAMP_STAMP_CSS_LSE 2
#define NVIC_IRQN_RTC_WKUP               3
#define NVIC_IRQN_FLASH                  4
#define NVIC_IRQN_RCC                    5
#define NVIC_IRQN_EXTI0                  6
#define NVIC_IRQN_EXTI1                  7
#define NVIC_IRQN_EXTI2                  8
#define NVIC_IRQN_EXTI3                  9
#define NVIC_IRQN_EXTI4                  10
#define NVIC_IRQN_DMA1_STR0              11
#define NVIC_IRQN_DMA1_STR1              12
#define NVIC_IRQN_DMA1_STR2              13
#define NVIC_IRQN_DMA1_STR3              14
#define NVIC_IRQN_DMA1_STR4              15
#define NVIC_IRQN_DMA1_STR5              16
#define NVIC_IRQN_DMA1_STR6              17
#define NVIC_IRQN_ADC1_2                 18
#define NVIC_IRQN_FDCAN1_IT0             19
#define NVIC_IRQN_FDCAN2_IT0             20
#define NVIC_IRQN_FDCAN1_IT1             21
#define NVIC_IRQN_FDCAN2_IT1             22
#define NVIC_IRQN_EXTI9_5                23
#define NVIC_IRQN_TIM1_BRK               24
#define NVIC_IRQN_TIM1_UP                25
#define NVIC_IRQN_TIM1_TRG_COM           26
#define NVIC_IRQN_TIM1_CC                27
#define NVIC_IRQN_TIM2                   28
#define NVIC_IRQN_TIM3                   29
#define NVIC_IRQN_TIM4                   30
#define NVIC_IRQN_I2C1_EV                31
#define NVIC_IRQN_I2C1_ER                32
#define NVIC_IRQN_I2C2_EV                33
#define NVIC_IRQN_I2C2_ER                34
#define NVIC_IRQN_SPI1                   35
#define NVIC_IRQN_SPI2                   36
#define NVIC_IRQN_USART1                 37
#define NVIC_IRQN_USART2                 38
#define NVIC_IRQN_USART3                 39
#define NVIC_IRQN_EXTI15_10              40
#define NVIC_IRQN_RTC_ALARM              41
#define NVIC_IRQN_TIM8_BRK_TIM12         43
#define NVIC_IRQN_TIM8_UP_TIM13          44
#define NVIC_IRQN_TIM8_TRG_COM_TIM14     45
#define NVIC_IRQN_TIM8_CC                46
#define NVIC_IRQN_DMA1_STR7              47
#define NVIC_IRQN_FMC                    48
#define NVIC_IRQN_SDMMC1                 49
#define NVIC_IRQN_TIM5                   50
#define NVIC_IRQN_SPI3                   51
#define NVIC_IRQN_UART4                  52
#define NVIC_IRQN_UART5                  53
#define NVIC_IRQN_TIM6_DAC               54
#define NVIC_IRQN_TIM7                   55
#define NVIC_IRQN_DMA2_STR0              56
#define NVIC_IRQN_DMA2_STR1              57
#define NVIC_IRQN_DMA2_STR2              58
#define NVIC_IRQN_DMA2_STR3              59
#define NVIC_IRQN_DMA2_STR4              60
#define NVIC_IRQN_ETH                    61
#define NVIC_IRQN_ETH_WKUP               62
#define NVIC_IRQN_FDCAN_CAL              63
#define NVIC_IRQN_CM7_SEV                64
#define NVIC_IRQN_DMA2_STR5              68
#define NVIC_IRQN_DMA2_STR6              69
#define NVIC_IRQN_DMA2_STR7              70
#define NVIC_IRQN_USART6                 71
#define NVIC_IRQN_I2C3_EV                72
#define NVIC_IRQN_I2C3_ER                73
#define NVIC_IRQN_OTG_HS_EP1_OUT         74
#define NVIC_IRQN_OTG_HS_EP1_IN          75
#define NVIC_IRQN_OTG_HS_WKUP            76
#define NVIC_IRQN_OTG_HS                 77
#define NVIC_IRQN_DCMI                   78
#define NVIC_IRQN_CRYP                   79
#define NVIC_IRQN_HASH_RNG               80
#define NVIC_IRQN_FPU                    81
#define NVIC_IRQN_UART7                  82
#define NVIC_IRQN_UART8                  83
#define NVIC_IRQN_SPI4                   84
#define NVIC_IRQN_SPI5                   85
#define NVIC_IRQN_SPI6                   86
#define NVIC_IRQN_SAI1                   87
#define NVIC_IRQN_LTDC                   88
#define NVIC_IRQN_LTDC_ER                89
#define NVIC_IRQN_DMA2D                  90
#define NVIC_IRQN_SAI2                   91
#define NVIC_IRQN_QUADSPI                92
#define NVIC_IRQN_LPTIM1                 93
#define NVIC_IRQN_CEC                    94
#define NVIC_IRQN_I2C4_EV                95
#define NVIC_IRQN_I2C4_ER                96
#define NVIC_IRQN_SPDIF                  97
#define NVIC_IRQN_OTG_FS_EP1_OUT         98
#define NVIC_IRQN_OTG_FS_EP1_IN          99
#define NVIC_IRQN_OTG_FS_WKUP            100
#define NVIC_IRQN_OTG_FS                 101
#define NVIC_IRQN_DMAMUX1_OV             102
#define NVIC_IRQN_HRTIM1_MST             103
#define NVIC_IRQN_HRTIM1_TIMA            104
#define NVIC_IRQN_HRTIM1_TIMB            105
#define NVIC_IRQN_HRTIM1_TIMC            106
#define NVIC_IRQN_HRTIM1_TIMD            107
#define NVIC_IRQN_HRTIM1_TIME            108
#define NVIC_IRQN_HRTIM1_FLT             109
#define NVIC_IRQN_DFSDM1_FLT0            110
#define NVIC_IRQN_DFSDM1_FLT1            111
#define NVIC_IRQN_DFSDM1_FLT2            112
#define NVIC_IRQN_DFSDM1_FLT3            113
#define NVIC_IRQN_SAI3                   114
#define NVIC_IRQN_SWPMI1                 115
#define NVIC_IRQN_TIM15                  116
#define NVIC_IRQN_TIM16                  117
#define NVIC_IRQN_TIM17                  118
#define NVIC_IRQN_MDIOS_WKUP             119
#define NVIC_IRQN_MDIOS                  120
#define NVIC_IRQN_JPEG                   121
#define NVIC_IRQN_MDMA                   122
#define NVIC_IRQN_SDMMC2                 124
#define NVIC_IRQN_HSEM0                  125
#define NVIC_IRQN_ADC3                   127
#define NVIC_IRQN_DMAMUX2_OVR            128
#define NVIC_IRQN_BDMA_CH0               129
#define NVIC_IRQN_BDMA_CH1               130
#define NVIC_IRQN_BDMA_CH2               131
#define NVIC_IRQN_BDMA_CH3               132
#define NVIC_IRQN_BDMA_CH4               133
#define NVIC_IRQN_BDMA_CH5               134
#define NVIC_IRQN_BDMA_CH6               135
#define NVIC_IRQN_BDMA_CH7               136
#define NVIC_IRQN_COMP                   137
#define NVIC_IRQN_LPTIM2                 138
#define NVIC_IRQN_LPTIM3                 139
#define NVIC_IRQN_LPTIM4                 140
#define NVIC_IRQN_LPTIM5                 141
#define NVIC_IRQN_LPUART                 142
#define NVIC_IRQN_WWDG1_RST              143
#define NVIC_IRQN_CRS                    144
#define NVIC_IRQN_RAMECC                 145
#define NVIC_IRQN_SAI4                   146
#define NVIC_IRQN_WKUP                   149

#endif /* __NVIC_STM32H743_H__ */
