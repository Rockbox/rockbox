/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: S5L8700X.h 2008-03-24 A4 $
 *
 * Copyright (C) 2008 by Bart van Adrichem
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
 
/* Copied from imx31l.h */
#define REG8_PTR_T  volatile unsigned char *
#define REG16_PTR_T volatile unsigned short *
#define REG32_PTR_T volatile unsigned long *

/* Base Adresses                                chapter in datasheet
*/ 
#define ARM_BASE_ADDR           0x38000000
#define MIU_BASE_ADDR           0x38200000 //07
#define IODMA_BASE_ADDR         0x38400000 //08
#define USB_H11_BASE_ADDR       0x38600000 //22
#define USB_F20_BASE_ADDR       0x38800000 //21
#define USB_H20_BASE_ADDR       0x38A00000
#define ATA_BASE_ADDR           0x38E00000 //28
#define ADM_BASE_ADDR           0x39000000 //04
#define LCD_CTRL_BASE_ADDR      0x39200000 //27
#define ICU_BASE_ADDR           0x39C00000 //06
#define EEC_BASE_ADDR           0x39E00000 //16
#define APB_BRIDGE_BASE_ADDR    0x3C000000
#define LCD_IF_BASE_ADDR        0x3C100000 //26
#define FMC_BASE_ADDR           0x3C200000 //12
#define MMC_SD_BASE_ADDR        0x3C300000 //13
#define USB_PHY_BASE_ADDR       0x3C400000 //23
#define CLCK_GEN_BASE_ADDR      0x3C500000 //05
#define MS_BASE_ADDR            0x3C600000 //14
#define TIMER_BASE_ADDR         0x3C700000 //11
#define WDT_BASE_ADDR           0x3C800000 //09
#define IIC_BASE_ADDR           0x3C900000 //18
#define IIS_BASE_ADDR           0x3CA00000 //17
#define SPDIF_OUT_BASE_ADDR     0x3CB00000 //15
#define UART0_BASE_ADDR         0x3CC00000 //25
#define UART1_BASE_ADDR         0x3CC08000 //25
#define SPI_BASE_ADDR           0x3CD00000 //19
#define ADC_BASE_ADDR           0x3CE00000 //20
#define GPIO_BASE_ADDR          0x3CF00000 //24
#define CHIP_ID_BASE_ADDR       0x3D100000 //29
#define RTC_BASE_ADDR           0x3D200000 //10


/* 04. CALMADM2E */
//Following registers are mapped on IO Area in data memory area of Calm.
//TODO: not sure if the following list is correct concerning the 'h' added to the adresses in the datasheet
//#DEFINE 7BIT OFFSET OR IS REG16_PTR_T CORRECT??
#define CALM_BASE               0x3F0000 //7 BITS LONG
#define CALM_CONFIG0            (*(REG16_PTR_T)(ADM_BASE_+0x00))
#define CALM_CONFIG1            (*(REG16_PTR_T)(ADM_BASE_+0x02))
#define CALM_COMMUN             (*(REG16_PTR_T)(ADM_BASE_+0x04))
#define CALM_DDATA0             (*(REG16_PTR_T)(ADM_BASE_+0x06))
#define CALM_DDATA1             (*(REG16_PTR_T)(ADM_BASE_+0x08))
#define CALM_DDATA2             (*(REG16_PTR_T)(ADM_BASE_+0x0A))
#define CALM_DDATA3             (*(REG16_PTR_T)(ADM_BASE_+0x0C))
#define CALM_DDATA4             (*(REG16_PTR_T)(ADM_BASE_+0x0E))
#define CALM_DDATA5             (*(REG16_PTR_T)(ADM_BASE_+0x10))
#define CALM_DDATA6             (*(REG16_PTR_T)(ADM_BASE_+0x12))
#define CALM_DDATA7             (*(REG16_PTR_T)(ADM_BASE_+0x14))
#define CALM_UDATA0             (*(REG16_PTR_T)(ADM_BASE_+0x16))
#define CALM_UDATA1             (*(REG16_PTR_T)(ADM_BASE_+0x18))
#define CALM_UDATA2             (*(REG16_PTR_T)(ADM_BASE_+0x1A))
#define CALM_UDATA3             (*(REG16_PTR_T)(ADM_BASE_+0x1C))
#define CALM_UDATA4             (*(REG16_PTR_T)(ADM_BASE_+0x1E))
#define CALM_UDATA5             (*(REG16_PTR_T)(ADM_BASE_+0x20))
#define CALM_UDATA6             (*(REG16_PTR_T)(ADM_BASE_+0x22))
#define CALM_UDATA7             (*(REG16_PTR_T)(ADM_BASE_+0x24))
#define CALM_IBASE_H            (*(REG16_PTR_T)(ADM_BASE_+0x26))
#define CALM_IBASE_L            (*(REG16_PTR_T)(ADM_BASE_+0x28))
#define CALM_DBASE_H            (*(REG16_PTR_T)(ADM_BASE_+0x2A))
#define CALM_DBASE_L            (*(REG16_PTR_T)(ADM_BASE_+0x2C))
#define CALM_XBASE_H            (*(REG16_PTR_T)(ADM_BASE_+0x2E))
#define CALM_XBASE_L            (*(REG16_PTR_T)(ADM_BASE_+0x30))
#define CALM_YBASE_H            (*(REG16_PTR_T)(ADM_BASE_+0x32))
#define CALM_YBASE_L            (*(REG16_PTR_T)(ADM_BASE_+0x34))
#define CALM_S0BASE_H           (*(REG16_PTR_T)(ADM_BASE_+0x36))
#define CALM_SOBASE_L           (*(REG16_PTR_T)(ADM_BASE_+0x38))
#define CALM_S1BASE_H           (*(REG16_PTR_T)(ADM_BASE_+0x3A))
#define CALM_S1BASE_L           (*(REG16_PTR_T)(ADM_BASE_+0x3C))
#define CALM_CACHECON           (*(REG16_PTR_T)(ADM_BASE_+0x3E))
#define CALM_CACHESTAT          (*(REG16_PTR_T)(ADM_BASE_+0x40))
#define CALM_SBFCON             (*(REG16_PTR_T)(ADM_BASE_+0x42))
#define CALM_SBFSTAT            (*(REG16_PTR_T)(ADM_BASE_+0x44))
#define CALM_SBL0OFF_H          (*(REG16_PTR_T)(ADM_BASE_+0x46))
#define CALM_SBL0OFF_L          (*(REG16_PTR_T)(ADM_BASE_+0x48))
#define CALM_SBL1OFF_H          (*(REG16_PTR_T)(ADM_BASE_+0x4A))
#define CALM_SBL1OFF_H          (*(REG16_PTR_T)(ADM_BASE_+0x4C))
#define CALM_SBL0BEGIN_H        (*(REG16_PTR_T)(ADM_BASE_+0x4E))
#define CALM_SBL0BEGIN_L        (*(REG16_PTR_T)(ADM_BASE_+0x50))
#define CALM_SBL1BEGIN_H        (*(REG16_PTR_T)(ADM_BASE_+0x52))
#define CALM_SBL1BEGIN_L        (*(REG16_PTR_T)(ADM_BASE_+0x54))
#define CALM_SBL0END_H          (*(REG16_PTR_T)(ADM_BASE_+0x56))
#define CALM_SBL0END_L          (*(REG16_PTR_T)(ADM_BASE_+0x58))
#define CALM_SBL0END_H          (*(REG16_PTR_T)(ADM_BASE_+0x5A))
#define CALM_SBL0END_L          (*(REG16_PTR_T)(ADM_BASE_+0x5C))
//Following registers are components of SFRS of the target system
#define ADM_CONFIG              (*(REG32_PTR_T)(ADM_BASE_ADDR+0x00))
#define ADM_COMMUN              (*(REG32_PTR_T)(ADM_BASE_ADDR+0x04))
#define ADM_DDATA0              (*(REG32_PTR_T)(ADM_BASE_ADDR+0x10))
#define ADM_DDATA1              (*(REG32_PTR_T)(ADM_BASE_ADDR+0x14))
#define ADM_DDATA2              (*(REG32_PTR_T)(ADM_BASE_ADDR+0x18))
#define ADM_DDATA3              (*(REG32_PTR_T)(ADM_BASE_ADDR+0x2C))
#define ADM_DDATA4              (*(REG32_PTR_T)(ADM_BASE_ADDR+0x20))
#define ADM_DDATA5              (*(REG32_PTR_T)(ADM_BASE_ADDR+0x24))
#define ADM_DDATA6              (*(REG32_PTR_T)(ADM_BASE_ADDR+0x28))
#define ADM_DDATA7              (*(REG32_PTR_T)(ADM_BASE_ADDR+0x2C))
#define ADM_UDATA0              (*(REG32_PTR_T)(ADM_BASE_ADDR+0x30))
#define ADM_UDATA1              (*(REG32_PTR_T)(ADM_BASE_ADDR+0x34))
#define ADM_UDATA2              (*(REG32_PTR_T)(ADM_BASE_ADDR+0x38))
#define ADM_UDATA3              (*(REG32_PTR_T)(ADM_BASE_ADDR+0x3C))
#define ADM_UDATA4              (*(REG32_PTR_T)(ADM_BASE_ADDR+0x40))
#define ADM_UDATA5              (*(REG32_PTR_T)(ADM_BASE_ADDR+0x44))
#define ADM_UDATA6              (*(REG32_PTR_T)(ADM_BASE_ADDR+0x48))
#define ADM_UDATA7              (*(REG32_PTR_T)(ADM_BASE_ADDR+0x4C))
#define ADM_IBASE               (*(REG32_PTR_T)(ADM_BASE_ADDR+0x50))
#define ADM_DBASE               (*(REG32_PTR_T)(ADM_BASE_ADDR+0x54))
#define ADM_XBASE               (*(REG32_PTR_T)(ADM_BASE_ADDR+0x58))
#define ADM_YBASE               (*(REG32_PTR_T)(ADM_BASE_ADDR+0x5C))
#define ADM_S0BASE              (*(REG32_PTR_T)(ADM_BASE_ADDR+0x60))
#define ADM_S1BASE              (*(REG32_PTR_T)(ADM_BASE_ADDR+0x64))

/* 05. CLOCK & POWER MANAGEMENT */
#define CLK_CON                 (*(REG32_PTR_T)(CLCK_GEN_BASE_ADDR+0x00))
#define PLL_CON                 (*(REG32_PTR_T)(CLCK_GEN_BASE_ADDR+0x24))
#define PLL0_PMS                (*(REG32_PTR_T)(CLCK_GEN_BASE_ADDR+0x04))
#define PLL1_PMS                (*(REG32_PTR_T)(CLCK_GEN_BASE_ADDR+0x08))
#define PLL0_CNT                (*(REG32_PTR_T)(CLCK_GEN_BASE_ADDR+0x14))
#define PLL1_CNT                (*(REG32_PTR_T)(CLCK_GEN_BASE_ADDR+0x18))
#define PLL_LOCK                (*(REG32_PTR_T)(CLCK_GEN_BASE_ADDR+0x20))
#define PWR_CON                 (*(REG32_PTR_T)(CLCK_GEN_BASE_ADDR+0x28))
#define PWR_MODE                (*(REG32_PTR_T)(CLCK_GEN_BASE_ADDR+0x2C))
#define SWR_CON                 (*(REG32_PTR_T)(CLCK_GEN_BASE_ADDR+0x30))
#define RST_SR                  (*(REG32_PTR_T)(CLCK_GEN_BASE_ADDR+0x34))
#define DSP_CLK_MD              (*(REG32_PTR_T)(CLCK_GEN_BASE_ADDR+0x38))
#define CLK_CON                 (*(REG32_PTR_T)(CLCK_GEN_BASE_ADDR+0x3C))

/* 06. INTERRUPT CONTROLLER UNIT */
#define ICU_SRC_PND             (*(REG32_PTR_T)(ICU_BASE_ADDR+0x00))
#define ICU_INT_MOD             (*(REG32_PTR_T)(ICU_BASE_ADDR+0x04))
#define ICU_INT_MSK             (*(REG32_PTR_T)(ICU_BASE_ADDR+0x08))
#define ICU_PRIORITY            (*(REG32_PTR_T)(ICU_BASE_ADDR+0x0C))
#define ICU_INT_PND             (*(REG32_PTR_T)(ICU_BASE_ADDR+0x10))
#define ICU_INT_OFFSET          (*(REG32_PTR_T)(ICU_BASE_ADDR+0x14))
#define ICU_EINT_POL            (*(REG32_PTR_T)(ICU_BASE_ADDR+0x18))
#define ICU_EINT_PEND           (*(REG32_PTR_T)(ICU_BASE_ADDR+0x1C))
#define ICU_EINT_MSK            (*(REG32_PTR_T)(ICU_BASE_ADDR+0x20))

/* 07. MEMORY INTERFACE UNIT (MIU) */
#define MIU_CON                 (*(REG32_PTR_T)(MIU_BASE_ADDR+0x00))
#define MIU_COM                 (*(REG32_PTR_T)(MIU_BASE_ADDR+0x04))
#define MIU_AREF                (*(REG32_PTR_T)(MIU_BASE_ADDR+0x08))
#define MIU_MRS                 (*(REG32_PTR_T)(MIU_BASE_ADDR+0x0C))
#define MIU_SDPARA              (*(REG32_PTR_T)(MIU_BASE_ADDR+0x10))

#define MIU_MEM_CONF            (*(REG32_PTR_T)(MIU_BASE_ADDR+0x020H)) // 9 BIT ADRESS IN DATASHEET????????
#define MIU_USR_CMD             (*(REG32_PTR_T)(MIU_BASE_ADDR+0x024H))
#define MIU_AREF                (*(REG32_PTR_T)(MIU_BASE_ADDR+0x028H))
#define MIU_MRS                 (*(REG32_PTR_T)(MIU_BASE_ADDR+0x02CH))
#define MIU_DPARAM              (*(REG32_PTR_T)(MIU_BASE_ADDR+0x030H))
#define MIU_SMEM_CONF           (*(REG32_PTR_T)(MIU_BASE_ADDR+0x034H))
#define MIU_S01PARA             (*(REG32_PTR_T)(MIU_BASE_ADDR+0x038H))
#define MIU_S23PARA             (*(REG32_PTR_T)(MIU_BASE_ADDR+0x03CH))
/* TODO:
#define MIU_ORG
#define MIU_DLYDQS
#define MIU_DLYCLK
#define MIU_DSS_SEL_B
#define MIU_DSS_SEL_O
#define MIU_PAD_DSS_SEL_NOR
#define MIU_PAD_DSS_SEL_ATA
#define MIU_SSTL2_PAD_ON
*/

/* 08. IODMA CONTROLLER */
#define DMA_BASE0               (*(REG32_PTR_T)(IODMA_BASE_ADDR+0x00))
#define DMA_BASE1               (*(REG32_PTR_T)(IODMA_BASE_ADDR+0x20))
#define DMA_BASE2               (*(REG32_PTR_T)(IODMA_BASE_ADDR+0x40))
#define DMA_BASE3               (*(REG32_PTR_T)(IODMA_BASE_ADDR+0x60))
#define DMA_CNT0                (*(REG32_PTR_T)(IODMA_BASE_ADDR+0x08))
#define DMA_CNT1                (*(REG32_PTR_T)(IODMA_BASE_ADDR+0x28))
#define DMA_CNT2                (*(REG32_PTR_T)(IODMA_BASE_ADDR+0x48))
#define DMA_CNT3                (*(REG32_PTR_T)(IODMA_BASE_ADDR+0x68))
#define DMA_CADDR0              (*(REG32_PTR_T)(IODMA_BASE_ADDR+0x0C))
#define DMA_CADDR1              (*(REG32_PTR_T)(IODMA_BASE_ADDR+0x2C))
#define DMA_CADDR2              (*(REG32_PTR_T)(IODMA_BASE_ADDR+0x4C))
#define DMA_CADDR3              (*(REG32_PTR_T)(IODMA_BASE_ADDR+0x6C))
#define DMA_CON0                (*(REG32_PTR_T)(IODMA_BASE_ADDR+0x04))
#define DMA_CON1                (*(REG32_PTR_T)(IODMA_BASE_ADDR+0x24))
#define DMA_CON2                (*(REG32_PTR_T)(IODMA_BASE_ADDR+0x44))
#define DMA_CON3                (*(REG32_PTR_T)(IODMA_BASE_ADDR+0x64))
#define DMA_CTCNT0              (*(REG32_PTR_T)(IODMA_BASE_ADDR+0x10))
#define DMA_CTCNT1              (*(REG32_PTR_T)(IODMA_BASE_ADDR+0x30))
#define DMA_CTCNT2              (*(REG32_PTR_T)(IODMA_BASE_ADDR+0x50))
#define DMA_CTCNT3              (*(REG32_PTR_T)(IODMA_BASE_ADDR+0x70))
#define DMA_COM0                (*(REG32_PTR_T)(IODMA_BASE_ADDR+0x14))
#define DMA_COM1                (*(REG32_PTR_T)(IODMA_BASE_ADDR+0x34))
#define DMA_COM2                (*(REG32_PTR_T)(IODMA_BASE_ADDR+0x54))
#define DMA_COM3                (*(REG32_PTR_T)(IODMA_BASE_ADDR+0x74))
#define DMA_OFF1                (*(REG32_PTR_T)(IODMA_BASE_ADDR+0x18))
#define DMA_ALLST               (*(REG32_PTR_T)(IODMA_BASE_ADDR+0x0100))

/* 10. REAL TIMER CLOCK (RTC) */
#define RTC_CON                 (*(REG32_PTR_T)(RTC_BASE_ADDR+0x00))
#define RTC_RST                 (*(REG32_PTR_T)(RTC_BASE_ADDR+0x04))
#define RTC_ALM_CON             (*(REG32_PTR_T)(RTC_BASE_ADDR+0x08))
#define RTC_ALM_SEC             (*(REG32_PTR_T)(RTC_BASE_ADDR+0x0C))
#define RTC_ALM_MIN             (*(REG32_PTR_T)(RTC_BASE_ADDR+0x10))
#define RTC_ALM_HOUR            (*(REG32_PTR_T)(RTC_BASE_ADDR+0x14))
#define RTC_ALM_DATE            (*(REG32_PTR_T)(RTC_BASE_ADDR+0x18))
#define RTC_ALM_DAY             (*(REG32_PTR_T)(RTC_BASE_ADDR+0x1C))
#define RTC_ALM_MON             (*(REG32_PTR_T)(RTC_BASE_ADDR+0x20))
#define RTC_AML_YEAR            (*(REG32_PTR_T)(RTC_BASE_ADDR+0x24))
#define RTC_SEC                 (*(REG32_PTR_T)(RTC_BASE_ADDR+0x28))
#define RTC_MIN                 (*(REG32_PTR_T)(RTC_BASE_ADDR+0x2C))
#define RTC_HOUR                (*(REG32_PTR_T)(RTC_BASE_ADDR+0x30))
#define RTC_DATE                (*(REG32_PTR_T)(RTC_BASE_ADDR+0x34))
#define RTC_DAY                 (*(REG32_PTR_T)(RTC_BASE_ADDR+0x38))
#define RTC_MON                 (*(REG32_PTR_T)(RTC_BASE_ADDR+0x3C))
#define RTC_YEAR                (*(REG32_PTR_T)(RTC_BASE_ADDR+0x40))
#define RTC_IM                  (*(REG32_PTR_T)(RTC_BASE_ADDR+0x44))
#define RTC_PEND                (*(REG32_PTR_T)(RTC_BASE_ADDR+0x48))

/* 09. WATCHDOG TIMER*/
#define WDT_CON                 (*(REG32_PTR_T)(WDT_BASE_ADDR+0x00))
#define WDT_CNT                 (*(REG32_PTR_T)(WDT_BASE_ADDR+0x04))

/* 11. 16 BIT TIMER */
#define TA_CON                  (*(REG32_PTR_T)(TIMER_BASE_ADDR+0x00))
#define TA_CMD                  (*(REG32_PTR_T)(TIMER_BASE_ADDR+0x04))
#define TA_DATA0                (*(REG32_PTR_T)(TIMER_BASE_ADDR+0x08))
#define TA_DATA1                (*(REG32_PTR_T)(TIMER_BASE_ADDR+0x0C))
#define TA_PRE                  (*(REG32_PTR_T)(TIMER_BASE_ADDR+0x10))
#define TA_CNT                  (*(REG32_PTR_T)(TIMER_BASE_ADDR+0x14))
#define TB_CON                  (*(REG32_PTR_T)(TIMER_BASE_ADDR+0x20))
#define TB_CMD                  (*(REG32_PTR_T)(TIMER_BASE_ADDR+0x24))
#define TB_DATA0                (*(REG32_PTR_T)(TIMER_BASE_ADDR+0x28))
#define TB_DATA1                (*(REG32_PTR_T)(TIMER_BASE_ADDR+0x2C))
#define TB_PRE                  (*(REG32_PTR_T)(TIMER_BASE_ADDR+0x30))
#define TB_CNT                  (*(REG32_PTR_T)(TIMER_BASE_ADDR+0x34))
#define TC_CON                  (*(REG32_PTR_T)(TIMER_BASE_ADDR+0x40))
#define TC_CMD                  (*(REG32_PTR_T)(TIMER_BASE_ADDR+0x44))
#define TC_DATA0                (*(REG32_PTR_T)(TIMER_BASE_ADDR+0x48))
#define TC_DATA1                (*(REG32_PTR_T)(TIMER_BASE_ADDR+0x4C))
#define TC_PRE                  (*(REG32_PTR_T)(TIMER_BASE_ADDR+0x50))
#define TC_CNT                  (*(REG32_PTR_T)(TIMER_BASE_ADDR+0x54))
#define TD_CON                  (*(REG32_PTR_T)(TIMER_BASE_ADDR+0x60))
#define TD_CMD                  (*(REG32_PTR_T)(TIMER_BASE_ADDR+0x64))
#define TD_DATA0                (*(REG32_PTR_T)(TIMER_BASE_ADDR+0x68))
#define TD_DATA1                (*(REG32_PTR_T)(TIMER_BASE_ADDR+0x6C))
#define TD_PRE                  (*(REG32_PTR_T)(TIMER_BASE_ADDR+0x70))
#define TD_CNT                  (*(REG32_PTR_T)(TIMER_BASE_ADDR+0x74))

/* 12. NAND FLASH CONTROLER */
// TODO: FIFO
#define FM_CTRL0            (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0000))
#define FM_CTRL1            (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0004))
#define FM_CMD              (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0008))
#define FM_ADR0             (*(REG32_PTR_T)(FMC_BASE_ADDR+0x000C))
#define FM_ADR1             (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0010))
#define FM_ADR2             (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0014))
#define FM_ADR3             (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0018))
#define FM_ADR4             (*(REG32_PTR_T)(FMC_BASE_ADDR+0x001C))
#define FM_ADR5             (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0020))
#define FM_ADR6             (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0024))
#define FM_ADR7             (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0028))
#define FM_ANUM             (*(REG32_PTR_T)(FMC_BASE_ADDR+0x002C))
#define FM_DNUM             (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0030))
#define FM_DATAW0           (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0034))
#define FM_DATAW1           (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0038))
#define FM_DATAW2           (*(REG32_PTR_T)(FMC_BASE_ADDR+0x003C))
#define FM_DATAW3           (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0040))
#define FM_CSTAT            (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0048))
#define FM_SYND0            (*(REG32_PTR_T)(FMC_BASE_ADDR+0x004C))
#define FM_SYND1            (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0050))
#define FM_SYND2            (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0054))
#define FM_SYND3            (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0058))
#define FM_SYND4            (*(REG32_PTR_T)(FMC_BASE_ADDR+0x005C))
#define FM_SYND5            (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0060))
#define FM_SYND6            (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0064))
#define FM_SYND7            (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0068))
#define FM_FIFO             (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0080)) // UNTILL (INCLUDING) 0x00FC <--
#define RS_CTRL             (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0100))
#define RS_PAITY0-0         (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0110))
#define RS_PAITY0-1         (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0114))
#define RS_PAITY0-2         (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0118))
#define RS_PAITY1-0         (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0120))
#define RS_PAITY1-1         (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0124))
#define RS_PAITY1-2         (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0128))
#define RS_PAITY2-0         (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0130))
#define RS_PAITY2-1         (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0134))
#define RS_PAITY2-2         (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0138))
#define RS_PAITY3-0         (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0140))
#define RS_PAITY3-1         (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0144))
#define RS_PAITY3-2         (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0148))
#define RS_SYND0-0          (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0150))
#define RS_SYND0-1          (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0154))
#define RS_SYND0-2          (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0158))
#define RS_SYND1-0          (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0160))
#define RS_SYND1-1          (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0164))
#define RS_SYND1-2          (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0168))
#define RS_SYND2-0          (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0170))
#define RS_SYND2-1          (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0174))
#define RS_SYND2-2          (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0178))
#define RS_SYND3-0          (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0180))
#define RS_SYND3-1          (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0184))
#define RS_SYND3-2          (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0188))
#define FLAG_SYND           (*(REG32_PTR_T)(FMC_BASE_ADDR+0x0190))

/* 13. SECURE DIGITAL CARD INTERFACE (SDCI) */
// TODO

/* 14. MEMORY STICK HOST CONTROLLER */
//TODO

/* 15. SPDIF TRANSMITTER (SPDIFOUT) */
#define SPD_CLKCON          (*(REG32_PTR_T)(SPDIF_OUT_BASE_ADDR+0x00))
#define SPD_CON             (*(REG32_PTR_T)(SPDIF_OUT_BASE_ADDR+0x04))
#define SPD_BSTAS           (*(REG32_PTR_T)(SPDIF_OUT_BASE_ADDR+0x08))
#define SPD_CSTAS           (*(REG32_PTR_T)(SPDIF_OUT_BASE_ADDR+0x0C))
#define SPD_DAT             (*(REG32_PTR_T)(SPDIF_OUT_BASE_ADDR+0x10))
#define SPD_CNT             (*(REG32_PTR_T)(SPDIF_OUT_BASE_ADDR+0x14))

/* 16. REED-SOLOMON ECC CODEC */
//TODO

/* 17. IIS Tx/Rx INTERFACE */
#define I2S_CLK_CON             (*(REG32_PTR_T)(IIS_BASE_ADDR+0x00))
#define I2S_TX_CON              (*(REG32_PTR_T)(IIS_BASE_ADDR+0x04))
#define I2S_TX_COM              (*(REG32_PTR_T)(IIS_BASE_ADDR+0x08))
#define I2S_TX_DB               (*(REG32_PTR_T)(IIS_BASE_ADDR+0x10))
#define I2S_RX_CON              (*(REG32_PTR_T)(IIS_BASE_ADDR+0x30))
#define I2S_RX_COM              (*(REG32_PTR_T)(IIS_BASE_ADDR+0x34))
#define I2S_RX_DB               (*(REG32_PTR_T)(IIS_BASE_ADDR+0x38))
#define I2S_STATUS              (*(REG32_PTR_T)(IIS_BASE_ADDR+0x3C))

/* 18. IIC BUS INTERFACE */
#define IIC_CON                 (*(REG32_PTR_T)(IIC_BASE_ADDR+0x00))
#define IIC_STST                (*(REG32_PTR_T)(IIC_BASE_ADDR+0x04))
#define IIC_ADD                 (*(REG32_PTR_T)(IIC_BASE_ADDR+0x08))
#define IIC_DS                  (*(REG32_PTR_T)(IIC_BASE_ADDR+0x0C))

/* 19. SPI (SERIAL PERHIPERAL INTERFACE) */
#define SP_CLK_CON              (*(REG32_PTR_T)(SPI_BASE_ADDR+0x00))
#define SP_CON                  (*(REG32_PTR_T)(SPI_BASE_ADDR+0x04))
#define SP_STA                  (*(REG32_PTR_T)(SPI_BASE_ADDR+0x08))
#define SP_PIN                  (*(REG32_PTR_T)(SPI_BASE_ADDR+0x0C))
#define SP_TDAT                 (*(REG32_PTR_T)(SPI_BASE_ADDR+0x10))
#define SP_RDAT                 (*(REG32_PTR_T)(SPI_BASE_ADDR+0x14))
#define SP_PRE                  (*(REG32_PTR_T)(SPI_BASE_ADDR+0x18))

/* 20. ADC CONTROLLER */
#define ADC_CON                 (*(REG32_PTR_T)(ADC_BASE_ADDR+0x00))
#define ADC_TSC                 (*(REG32_PTR_T)(ADC_BASE_ADDR+0x04))
#define ADC_DLY                 (*(REG32_PTR_T)(ADC_BASE_ADDR+0x08))
#define ADC_DAT0                (*(REG32_PTR_T)(ADC_BASE_ADDR+0x0C))
#define ADC_DAT1                (*(REG32_PTR_T)(ADC_BASE_ADDR+0x10))
#define ADC_UPDN                (*(REG32_PTR_T)(ADC_BASE_ADDR+0x14))

/*  21. USB 2.0 FUNCTION CONTROLER SPECIAL REGISTER */
#define USB2_IN                 (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x00))
#define USB2_EIR                (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x04))
#define USB2_EIER               (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x08))
#define USB2_FAR                (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x0C))
#define USB2_FNR                (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x10))
#define USB2_EDR                (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x14))
#define USB2_TR                 (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x18))
#define USB2_SSR                (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x1C))
#define USB2_SCR                (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x20))
#define USB2_EP0SR              (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x24))
#define USB2_EP0CR              (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x28))
#define USB2_EP0BR              (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x60))
#define USB2_EP1BR              (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x64))
#define USB2_EP2BR              (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x68))
#define USB2_EP3BR              (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x6C))
#define USB2_EP4BR              (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x70))
#define USB2_EP5BR              (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x74))
#define USB2_EP6BR              (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x78))
#define USB2_ESR                (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x2C))
#define USB2_ECR                (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x30))
#define USB2_BRCR               (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x34))
#define USB2_BSCR               (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x38))
#define USB2_MPR                (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x3C))
#define USB2_MCR                (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x40))
#define USB2_MTCR               (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x44))
#define USB2_MFCR               (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x48))
#define USB2_MTTCR1             (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x4C))
#define USB2_MTTCR2             (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x50))
#define USB2_MICR               (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x84))
#define USB2_MBAR1              (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x88))
#define USB2_MBAR2              (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x8C))
#define USB2_MCAR1              (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x94))
#define USB2_MCAR2              (*(REG32_PTR_T)(USB_F20_BASE_ADDR+0x98))

/* 22. USB 1.1 HOST CONTROLER SPECIAL REGISTER */
#define HC_REV                  (*(REG32_PTR_T)(USB_H11_BASE_ADDR+0x00))
#define HC_CON                  (*(REG32_PTR_T)(USB_H11_BASE_ADDR+0x04))
#define HC_COMSTAT              (*(REG32_PTR_T)(USB_H11_BASE_ADDR+0x08))
#define HC_INTSTAT              (*(REG32_PTR_T)(USB_H11_BASE_ADDR+0x0C))
#define HC_INTEN                (*(REG32_PTR_T)(USB_H11_BASE_ADDR+0x10))
#define HC_INTDIS               (*(REG32_PTR_T)(USB_H11_BASE_ADDR+0x14))
#define HC_HCCA                 (*(REG32_PTR_T)(USB_H11_BASE_ADDR+0x18))
#define HC_PCED                 (*(REG32_PTR_T)(USB_H11_BASE_ADDR+0x1C))
#define HC_CHED                 (*(REG32_PTR_T)(USB_H11_BASE_ADDR+0x20))
#define HC_CCED                 (*(REG32_PTR_T)(USB_H11_BASE_ADDR+0x24))
#define HC_BHED                 (*(REG32_PTR_T)(USB_H11_BASE_ADDR+0x28))
#define HC_BCED                 (*(REG32_PTR_T)(USB_H11_BASE_ADDR+0x2C))
#define HC_DH                   (*(REG32_PTR_T)(USB_H11_BASE_ADDR+0x30))
#define HC_FMI                  (*(REG32_PTR_T)(USB_H11_BASE_ADDR+0x34))
#define HC_FMR                  (*(REG32_PTR_T)(USB_H11_BASE_ADDR+0x38))
#define HC_FMNR                 (*(REG32_PTR_T)(USB_H11_BASE_ADDR+0x3C))
#define HC_PS                   (*(REG32_PTR_T)(USB_H11_BASE_ADDR+0x40))
#define HC_LSTRESH              (*(REG32_PTR_T)(USB_H11_BASE_ADDR+0x44))
#define HC_RHSECA               (*(REG32_PTR_T)(USB_H11_BASE_ADDR+0x48))
#define HC_RHDESB               (*(REG32_PTR_T)(USB_H11_BASE_ADDR+0x4C))
#define HC_STAT                 (*(REG32_PTR_T)(USB_H11_BASE_ADDR+0x50))
#define HC_PSTAT                (*(REG32_PTR_T)(USB_H11_BASE_ADDR+0x54))

/* 23. USB 2.0 PHY CONTROL */
#define PHY_CTRL                (*(REG32_PTR_T)(USB_PHY_BASE_ADDR+0x00))
#define ULCK_CON                (*(REG32_PTR_T)(USB_PHY_BASE_ADDR+0x04))
#define URST_CON                (*(REG32_PTR_T)(USB_PHY_BASE_ADDR+0x08))

/* 24. GPIO PORT CONTROLL */
#define GPIO_PCON0              (*(REG32_PTR_T)(GPIO_BASE_ADDR+0x00))
#define GPIO_PDAT0              (*(REG32_PTR_T)(GPIO_BASE_ADDR+0x04))
#define GPIO_PCON1              (*(REG32_PTR_T)(GPIO_BASE_ADDR+0x10))
#define GPIO_PDAT1              (*(REG32_PTR_T)(GPIO_BASE_ADDR+0x14))
#define GPIO_PCON2              (*(REG32_PTR_T)(GPIO_BASE_ADDR+0x20))
#define GPIO_PDAT2              (*(REG32_PTR_T)(GPIO_BASE_ADDR+0x24))
#define GPIO_PCON3              (*(REG32_PTR_T)(GPIO_BASE_ADDR+0x30))
#define GPIO_PDAT3              (*(REG32_PTR_T)(GPIO_BASE_ADDR+0x34))
#define GPIO_PCON4              (*(REG32_PTR_T)(GPIO_BASE_ADDR+0x40))
#define GPIO_PDAT4              (*(REG32_PTR_T)(GPIO_BASE_ADDR+0x44))
#define GPIO_PCON5              (*(REG32_PTR_T)(GPIO_BASE_ADDR+0x50))
#define GPIO_PDAT5              (*(REG32_PTR_T)(GPIO_BASE_ADDR+0x54))
#define GPIO_PCON6              (*(REG32_PTR_T)(GPIO_BASE_ADDR+0x60))
#define GPIO_PDAT6              (*(REG32_PTR_T)(GPIO_BASE_ADDR+0x64))
#define GPIO_PCON7              (*(REG32_PTR_T)(GPIO_BASE_ADDR+0x70))
#define GPIO_PDAT7              (*(REG32_PTR_T)(GPIO_BASE_ADDR+0x74))
#define GPIO_PCON10             (*(REG32_PTR_T)(GPIO_BASE_ADDR+0xA0))
#define GPIO_PDAT10             (*(REG32_PTR_T)(GPIO_BASE_ADDR+0xA4))
#define GPIO_PCON11             (*(REG32_PTR_T)(GPIO_BASE_ADDR+0xF8))
#define GPIO_PDAT11             (*(REG32_PTR_T)(GPIO_BASE_ADDR+0xFC))
#define GPIO_PCON_ASRAM         (*(REG32_PTR_T)(GPIO_BASE_ADDR+0xF0))
#define GPIO_PCON_SDRAM         (*(REG32_PTR_T)(GPIO_BASE_ADDR+0xF4))

/* 25. UART */
#define UART0_LCON          (*(REG32_PTR_T)(UART0_BASE_ADDR+0x00))
#define UART0_CON           (*(REG32_PTR_T)(UART0_BASE_ADDR+0x04))
#define UART0_FCON          (*(REG32_PTR_T)(UART0_BASE_ADDR+0x08))
#define UART0_MCON          (*(REG32_PTR_T)(UART0_BASE_ADDR+0x0C))
#define UART0_TRSTAT        (*(REG32_PTR_T)(UART0_BASE_ADDR+0x10))
#define UART0_ERSTAT        (*(REG32_PTR_T)(UART0_BASE_ADDR+0x14))
#define UART0_FSTAT         (*(REG32_PTR_T)(UART0_BASE_ADDR+0x18))
#define UART0_MSTAT         (*(REG32_PTR_T)(UART0_BASE_ADDR+0x1C))
#define UART0_TXH           (*(REG32_PTR_T)(UART0_BASE_ADDR+0x10))
#define UART0_RXH           (*(REG32_PTR_T)(UART0_BASE_ADDR+0x24))
#define UART0_BRDIV         (*(REG32_PTR_T)(UART0_BASE_ADDR+0x28))

#define UART1_LCON          (*(REG32_PTR_T)(UART1_BASE_ADDR+0x00))
#define UART1_CON           (*(REG32_PTR_T)(UART1_BASE_ADDR+0x04))
#define UART1_FCON          (*(REG32_PTR_T)(UART1_BASE_ADDR+0x08))
#define UART1_MCON          (*(REG32_PTR_T)(UART1_BASE_ADDR+0x0C))
#define UART1_TRSTAT        (*(REG32_PTR_T)(UART1_BASE_ADDR+0x10))
#define UART1_ERSTAT        (*(REG32_PTR_T)(UART1_BASE_ADDR+0x14))
#define UART1_FSTAT         (*(REG32_PTR_T)(UART1_BASE_ADDR+0x18))
#define UART1_MSTAT         (*(REG32_PTR_T)(UART1_BASE_ADDR+0x1C))
#define UART1_TXH           (*(REG32_PTR_T)(UART1_BASE_ADDR+0x10))
#define UART1_RXH           (*(REG32_PTR_T)(UART1_BASE_ADDR+0x24))
#define UART1_BRDIV         (*(REG32_PTR_T)(UART1_BASE_ADDR+0x28))

/* 26. LCD INTERFACE CONTROLLER */
// TODO: WDATA
#define LCD_CON                 (*(REG32_PTR_T)(LCD_IF_BASE_ADDR+0x00))
#define LCD_WCMD                (*(REG32_PTR_T)(LCD_IF_BASE_ADDR+0x04))
#define LCD_RCMD                (*(REG32_PTR_T)(LCD_IF_BASE_ADDR+0x0C))
#define LCD_RDATA               (*(REG32_PTR_T)(LCD_IF_BASE_ADDR+0x10))
#define LCD_DBUFF               (*(REG32_PTR_T)(LCD_IF_BASE_ADDR+0x14))
#define LCD_INTCON              (*(REG32_PTR_T)(LCD_IF_BASE_ADDR+0x18))
#define LCD_STATUS              (*(REG32_PTR_T)(LCD_IF_BASE_ADDR+0x1C))
#define LCD_PHTIME              (*(REG32_PTR_T)(LCD_IF_BASE_ADDR+0x20))
#define LCD_RST_TIME            (*(REG32_PTR_T)(LCD_IF_BASE_ADDR+0x24))
#define LCD_DRV_RST             (*(REG32_PTR_T)(LCD_IF_BASE_ADDR+0x28))
#define LCD_WDATA               (*(REG32_PTR_T)(LCD_IF_BASE_ADDR+0x40)) // UNTILL (INCLUDING) 0x5C <--

/* 27. CLCD CONTROLLER */
#define CLCD_CON1               (*(REG32_PTR_T)(LCD_CTRL_BASE_ADDR+0x00))
#define CLCD_CON2               (*(REG32_PTR_T)(LCD_CTRL_BASE_ADDR+0x04))
#define CLCD_TCON1              (*(REG32_PTR_T)(LCD_CTRL_BASE_ADDR+0x08))
#define CLCD_TCON2              (*(REG32_PTR_T)(LCD_CTRL_BASE_ADDR+0x0C))
#define CLCD_TCON3              (*(REG32_PTR_T)(LCD_CTRL_BASE_ADDR+0x10))
#define CLCD_OSD1               (*(REG32_PTR_T)(LCD_CTRL_BASE_ADDR+0x14))
#define CLCD_OSD2               (*(REG32_PTR_T)(LCD_CTRL_BASE_ADDR+0x18))
#define CLCD_OSD3               (*(REG32_PTR_T)(LCD_CTRL_BASE_ADDR+0x1C))
#define CLCD_B1SADDR1           (*(REG32_PTR_T)(LCD_CTRL_BASE_ADDR+0x20))
#define CLCD_B2SADDR1           (*(REG32_PTR_T)(LCD_CTRL_BASE_ADDR+0x24))
#define CLCD_F1SADDR1           (*(REG32_PTR_T)(LCD_CTRL_BASE_ADDR+0x28))
#define CLCD_F2SADDR1           (*(REG32_PTR_T)(LCD_CTRL_BASE_ADDR+0x2C))
#define CLCD_B1SADDR2           (*(REG32_PTR_T)(LCD_CTRL_BASE_ADDR+0x30))
#define CLCD_B2SADDR2           (*(REG32_PTR_T)(LCD_CTRL_BASE_ADDR+0x34))
#define CLCD_F1SADDR2           (*(REG32_PTR_T)(LCD_CTRL_BASE_ADDR+0x38))
#define CLCD_F2SADDR2           (*(REG32_PTR_T)(LCD_CTRL_BASE_ADDR+0x3C))
#define CLCD_B1SADDR3           (*(REG32_PTR_T)(LCD_CTRL_BASE_ADDR+0x40))
#define CLCD_B2SADDR3           (*(REG32_PTR_T)(LCD_CTRL_BASE_ADDR+0x44))
#define CLCD_F1SADDR3           (*(REG32_PTR_T)(LCD_CTRL_BASE_ADDR+0x48))
#define CLCD_F2SADDR3           (*(REG32_PTR_T)(LCD_CTRL_BASE_ADDR+0x4C))
#define CLCD_INTCON             (*(REG32_PTR_T)(LCD_CTRL_BASE_ADDR+0x50))
#define CLCD_KEYCON             (*(REG32_PTR_T)(LCD_CTRL_BASE_ADDR+0x54))
#define CLCD_KEYVAL             (*(REG32_PTR_T)(LCD_CTRL_BASE_ADDR+0x58))
#define CLCD_BGCON              (*(REG32_PTR_T)(LCD_CTRL_BASE_ADDR+0x5C))
#define CLCD_FGCON              (*(REG32_PTR_T)(LCD_CTRL_BASE_ADDR+0x60))
#define CLCD_DITHCON            (*(REG32_PTR_T)(LCD_CTRL_BASE_ADDR+0x64))

/* 28. ATA CONTROLLER */
// TODO

/* 29. CHIP ID */

#define REG_ONE         (*(REG32_PTR_T)(CHIP_ID_BASE_ADDR+0x00))
#define REG_TWO         (*(REG32_PTR_T)(CHIP_ID_BASE_ADDR+0x04))

