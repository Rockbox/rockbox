/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by James Espinoza
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef __IMX31L_H__
#define __IMX31L_H__

/* Most(if not all) of these defines are copied from Nand-Boot v4 provided w/ the Imx31 Linux Bsp*/

#define REG8_PTR_T  volatile unsigned char *
#define REG16_PTR_T volatile unsigned short *
#define REG32_PTR_T volatile unsigned long *

/* Place in the section with the framebuffer */
#define TTB_BASE_ADDR (0x80100000 + 0x00100000 - TTB_SIZE)
#define TTB_SIZE      (0x4000)
#define IRAM_SIZE     (0x4000)
#define TTB_BASE      ((unsigned int *)TTB_BASE_ADDR)
#define FRAME         ((void*)0x80100000)
#define FRAME_SIZE    (240*320*2)

#define DEVBSS_ATTR   __attribute__((section(".devbss"),nocommon))

/*
 * AIPS 1
 */
#define IRAM_BASE_ADDR          0x1fffc000
#define L2CC_BASE_ADDR          0x30000000
#define AIPS1_BASE_ADDR         0x43F00000
#define AIPS1_CTRL_BASE_ADDR    AIPS1_BASE_ADDR
#define MAX_BASE_ADDR           0x43F04000
#define EVTMON_BASE_ADDR        0x43F08000
#define CLKCTL_BASE_ADDR        0x43F0C000
#define ETB_SLOT4_BASE_ADDR     0x43F10000
#define ETB_SLOT5_BASE_ADDR     0x43F14000
#define ECT_CTIO_BASE_ADDR      0x43F18000
#define I2C_BASE_ADDR           0x43F80000
#define I2C3_BASE_ADDR          0x43F84000
#define OTG_BASE_ADDR           0x43F88000
#define ATA_BASE_ADDR           0x43F8C000
#define UART1_BASE_ADDR         0x43F90000
#define UART2_BASE_ADDR         0x43F94000
#define I2C2_BASE_ADDR          0x43F98000
#define OWIRE_BASE_ADDR         0x43F9C000
#define SSI1_BASE_ADDR          0x43FA0000
#define CSPI1_BASE_ADDR         0x43FA4000
#define KPP_BASE_ADDR           0x43FA8000
#define IOMUXC_BASE_ADDR        0x43FAC000
#define UART4_BASE_ADDR         0x43FB0000
#define UART5_BASE_ADDR         0x43FB4000
#define ECT_IP1_BASE_ADDR       0x43FB8000
#define ECT_IP2_BASE_ADDR       0x43FBC000
 
/*
 * SPBA
 */
#define SPBA_BASE_ADDR          0x50000000
#define MMC_SDHC1_BASE_ADDR     0x50004000
#define MMC_SDHC2_BASE_ADDR     0x50008000
#define UART3_BASE_ADDR         0x5000C000
#define CSPI2_BASE_ADDR         0x50010000
#define SSI2_BASE_ADDR          0x50014000
#define SIM_BASE_ADDR           0x50018000
#define IIM_BASE_ADDR           0x5001C000
#define ATA_DMA_BASE_ADDR       0x50020000
#define SPBA_CTRL_BASE_ADDR     0x5003C000

/*
 * AIPS 2
 */
#define AIPS2_BASE_ADDR         0x53F00000
#define AIPS2_CTRL_BASE_ADDR    AIPS2_BASE_ADDR
#define CCM_BASE_ADDR           0x53F80000
#define CSPI3_BASE_ADDR         0x53F84000
#define FIRI_BASE_ADDR          0x53F8C000
#define GPT1_BASE_ADDR          0x53F90000
#define EPIT1_BASE_ADDR         0x53F94000
#define EPIT2_BASE_ADDR         0x53F98000
#define GPIO3_BASE_ADDR         0x53FA4000
#define SCC_BASE                0x53FAC000
#define SCM_BASE                0x53FAE000
#define SMN_BASE                0x53FAF000
#define RNGA_BASE_ADDR          0x53FB0000
#define IPU_CTRL_BASE_ADDR      0x53FC0000
#define AUDMUX_BASE             0x53FC4000
#define MPEG4_ENC_BASE          0x53FC8000
#define GPIO1_BASE_ADDR         0x53FCC000
#define GPIO2_BASE_ADDR         0x53FD0000
#define SDMA_BASE_ADDR          0x53FD4000
#define RTC_BASE_ADDR           0x53FD8000
#define WDOG_BASE_ADDR          0x53FDC000
#define PWM_BASE_ADDR           0x53FE0000
#define RTIC_BASE_ADDR          0x53FEC000

#define WDOG1_BASE_ADDR         WDOG_BASE_ADDR
#define CRM_MCU_BASE_ADDR       CCM_BASE_ADDR

/* IOMUXC */
#define IOMUXC_(o)              (*(REG32_PTR_T)(IOMUXC_BASE_ADDR+(o)))

/* GPR */
#define IOMUXC_GPR              IOMUXC_(0x008)

/* SW_MUX_CTL */
#define SW_MUX_CTL_CSPI3_MISO_CSPI3_SCLK_CSPI3_SPI_RDY_TTM_PAD          IOMUXC_(0x00C)
#define SW_MUX_CTL_ATA_RESET_B_CE_CONTROL_CLKSS_CSPI3_MOSI              IOMUXC_(0x010)
#define SW_MUX_CTL_ATA_CS1_ATA_DIOR_ATA_DIOW_ATA_DMACK                  IOMUXC_(0x014)
#define SW_MUX_CTL_SD1_DATA1_SD1_DATA2_SD1_DATA3_ATA_CS0                IOMUXC_(0x018)
#define SW_MUX_CTL_D3_SPL_SD1_CMD_SD1_CLK_SD1_DATA0                     IOMUXC_(0x01C)
#define SW_MUX_CTL_VSYNC3_CONTRAST_D3_REV_D3_CLS                        IOMUXC_(0x020)
#define SW_MUX_CTL_SER_RS_PAR_RS_WRITE_READ                             IOMUXC_(0x024)
#define SW_MUX_CTL_SD_D_IO_SD_D_CLK_LCS0_LCS1                           IOMUXC_(0x028)
#define SW_MUX_CTL_HSYNC_FPSHIFT_DRDY0_SD_D_I                           IOMUXC_(0x02C)
#define SW_MUX_CTL_LD15_LD16_LD17_VSYNC0                                IOMUXC_(0x030)
#define SW_MUX_CTL_LD11_LD12_LD13_LD14                                  IOMUXC_(0x034)
#define SW_MUX_CTL_LD7_LD8_LD9_LD10                                     IOMUXC_(0x038)
#define SW_MUX_CTL_LD3_LD4_LD5_LD6                                      IOMUXC_(0x03C)
#define SW_MUX_CTL_USBH2_DATA1_LD0_LD1_LD2                              IOMUXC_(0x040)
#define SW_MUX_CTL_USBH2_DIR_USBH2_STP_USBH2_NXT_USBH2_DATA0            IOMUXC_(0x044)
#define SW_MUX_CTL_USBOTG_DATA5_USBOTG_DATA6_USBOTG_DATA7_USBH2_CLK     IOMUXC_(0x048)
#define SW_MUX_CTL_USBOTG_DATA1_USBOTG_DATA2_USBOTG_DATA3_USBOTG_DATA4  IOMUXC_(0x04C)
#define SW_MUX_CTL_USBOTG_DIR_USBOTG_STP_USBOTG_NXT_USBOTG_DATA0        IOMUXC_(0x050)
#define SW_MUX_CTL_USB_PWR_USB_OC_USB_BYP_USBOTG_CLK                    IOMUXC_(0x054)
#define SW_MUX_CTL_TDO_TRSTB_DE_B_SJC_MOD                               IOMUXC_(0x058)
#define SW_MUX_CTL_RTCK_TCK_TMS_TDI                                     IOMUXC_(0x05C)
#define SW_MUX_CTL_KEY_COL4_KEY_COL5_KEY_COL6_KEY_COL7                  IOMUXC_(0x060)
#define SW_MUX_CTL_KEY_COL0_KEY_COL1_KEY_COL2_KEY_COL3                  IOMUXC_(0x064)
#define SW_MUX_CTL_KEY_ROW4_KEY_ROW5_KEY_ROW6_KEY_ROW7                  IOMUXC_(0x068)
#define SW_MUX_CTL_KEY_ROW0_KEY_ROW1_KEY_ROW2_KEY_ROW3                  IOMUXC_(0x06C)
#define SW_MUX_CTL_TXD2_RTS2_CTS2_BATT_LINE                             IOMUXC_(0x070)
#define SW_MUX_CTL_RI_DTE1_DCD_DTE1_DTR_DCE2_RXD2                       IOMUXC_(0x074)
#define SW_MUX_CTL_RI_DCE1_DCD_DCE1_DTR_DTE1_DSR_DTE1                   IOMUXC_(0x078)
#define SW_MUX_CTL_RTS1_CTS1_DTR_DCE1_DSR_DCE1                          IOMUXC_(0x07C)
#define SW_MUX_CTL_CSPI2_SCLK_CSPI2_SPI_RDY_RXD1_TXD1                   IOMUXC_(0x080)
#define SW_MUX_CTL_CSPI2_MISO_CSPI2_SS0_CSPI2_SS1_CSPI2_SS2             IOMUXC_(0x084)
#define SW_MUX_CTL_CSPI1_SS2_CSPI1_SCLK_CSPI1_SPI_RDY_CSPI2_MOSI        IOMUXC_(0x088)
#define SW_MUX_CTL_CSPI1_MOSI_CSPI1_MISO_CSPI1_SS0_CSPI1_SS1            IOMUXC_(0x08C)
#define SW_MUX_CTL_STXD6_SRXD6_SCK6_SFS6                                IOMUXC_(0x090)
#define SW_MUX_CTL_STXD5_SRXD5_SCK5_SFS5                                IOMUXC_(0x094)
#define SW_MUX_CTL_STXD4_SRXD4_SCK4_SFS4                                IOMUXC_(0x098)
#define SW_MUX_CTL_STXD3_SRXD3_SCK3_SFS3                                IOMUXC_(0x09C)
#define SW_MUX_CTL_CSI_HSYNC_CSI_PIXCLK_I2C_CLK_I2C_DAT                 IOMUXC_(0x0A0)
#define SW_MUX_CTL_CSI_D14_CSI_D15_CSI_MCLK_CSI_VSYNC                   IOMUXC_(0x0A4)
#define SW_MUX_CTL_CSI_D10_CSI_D11_CSI_D12_CSI_D13                      IOMUXC_(0x0A8)
#define SW_MUX_CTL_CSI_D6_CSI_D7_CSI_D8_CSI_D9                          IOMUXC_(0x0AC)
#define SW_MUX_CTL_M_REQUEST_M_GRANT_CSI_D4_CSI_D5                      IOMUXC_(0x0B0)
#define SW_MUX_CTL_PC_RST_IOIS16_PC_RW_B_PC_POE                         IOMUXC_(0x0B4)
#define SW_MUX_CTL_PC_VS1_PC_VS2_PC_BVD1_PC_BVD2                        IOMUXC_(0x0B8)
#define SW_MUX_CTL_PC_CD2_B_PC_WAIT_B_PC_READY_PC_PWRON                 IOMUXC_(0x0BC)
#define SW_MUX_CTL_D2_D1_D0_PC_CD1_B                                    IOMUXC_(0x0C0)
#define SW_MUX_CTL_D6_D5_D4_D3                                          IOMUXC_(0x0C4)
#define SW_MUX_CTL_D10_D9_D8_D7                                         IOMUXC_(0x0C8)
#define SW_MUX_CTL_D14_D13_D12_D11                                      IOMUXC_(0x0CC)
#define SW_MUX_CTL_NFWP_B_NFCE_B_NFRB_D15                               IOMUXC_(0x0D0)
#define SW_MUX_CTL_NFWE_B_NFRE_B_NFALE_NFCLE                            IOMUXC_(0x0D4)
#define SW_MUX_CTL_SDQS0_SDQS1_SDQS2_SDQS3                              IOMUXC_(0x0D8)
#define SW_MUX_CTL_SDCKE0_SDCKE1_SDCLK_SDCLK_B                          IOMUXC_(0x0DC)
#define SW_MUX_CTL_RW_RAS_CAS_SDWE                                      IOMUXC_(0x0E0)
#define SW_MUX_CTL_CS5_ECB_LBA_BCLK                                     IOMUXC_(0x0E4)
#define SW_MUX_CTL_CS1_CS2_CS3_CS4                                      IOMUXC_(0x0E8)
#define SW_MUX_CTL_EB0_EB1_OE_CS0                                       IOMUXC_(0x0EC)
#define SW_MUX_CTL_DQM0_DQM1_DQM2_DQM3                                  IOMUXC_(0x0F0)
#define SW_MUX_CTL_SD28_SD29_SD30_SD31                                  IOMUXC_(0x0F4)
#define SW_MUX_CTL_SD24_SD25_SD26_SD27                                  IOMUXC_(0x0F8)
#define SW_MUX_CTL_SD20_SD21_SD22_SD23                                  IOMUXC_(0x0FC)
#define SW_MUX_CTL_SD16_SD17_SD18_SD19                                  IOMUXC_(0x100)
#define SW_MUX_CTL_SD12_SD13_SD14_SD15                                  IOMUXC_(0x104)
#define SW_MUX_CTL_SD8_SD9_SD10_SD11                                    IOMUXC_(0x108)
#define SW_MUX_CTL_SD4_SD5_SD6_SD7                                      IOMUXC_(0x10C)
#define SW_MUX_CTL_SD0_SD1_SD2_SD3                                      IOMUXC_(0x110)
#define SW_MUX_CTL_A24_A25_SDBA1_SDBA0                                  IOMUXC_(0x114)
#define SW_MUX_CTL_A20_A21_A22_A23                                      IOMUXC_(0x118)
#define SW_MUX_CTL_A16_A17_A18_A19                                      IOMUXC_(0x11C)
#define SW_MUX_CTL_A12_A13_A14_A15                                      IOMUXC_(0x120)
#define SW_MUX_CTL_A9_A10_MA10_A11                                      IOMUXC_(0x124)
#define SW_MUX_CTL_A5_A6_A7_A8                                          IOMUXC_(0x128)
#define SW_MUX_CTL_A1_A2_A3_A4                                          IOMUXC_(0x12C)
#define SW_MUX_CTL_DVFS1_VPG0_VPG1_A0                                   IOMUXC_(0x130)
#define SW_MUX_CTL_CKIL_POWER_FAIL_VSTBY_DVFS0                          IOMUXC_(0x134)
#define SW_MUX_CTL_BOOT_MODE1_BOOT_MODE2_BOOT_MODE3_BOOT_MODE4          IOMUXC_(0x138)
#define SW_MUX_CTL_RESET_IN_B_POR_B_CLKO_BOOT_MODE0                     IOMUXC_(0x13C)
#define SW_MUX_CTL_STX0_SRX0_SIMPD0_CKIH                                IOMUXC_(0x140)
#define SW_MUX_CTL_GPIO3_1_SCLK0_SRST0_SVEN0                            IOMUXC_(0x144)
#define SW_MUX_CTL_GPIO1_4_GPIO1_5_GPIO1_6_GPIO3_0                      IOMUXC_(0x148)
#define SW_MUX_CTL_GPIO1_0_GPIO1_1_GPIO1_2_GPIO1_3                      IOMUXC_(0x14C)
#define SW_MUX_CTL_CAPTURE_COMPARE_WATCHDOG_RST_PWMO                    IOMUXC_(0x150)

#define SW_MUX_OUT_EN_GPIO_DR   0x0
#define SW_MUX_OUT_FUNCTIONAL   0x1
#define SW_MUX_OUT_ALTERNATE_1  0x2
#define SW_MUX_OUT_ALTERNATE_2  0x3
#define SW_MUX_OUT_ALTERNATE_3  0x4
#define SW_MUX_OUT_ALTERNATE_4  0x5
#define SW_MUX_OUT_ALTERNATE_5  0x6
#define SW_MUX_OUT_ALTERNATE_6  0x7

#define SW_MUX_IN_NO_INPUTS     0x0
#define SW_MUX_IN_GPIO_PSR_ISR  0x1
#define SW_MUX_IN_FUNCTIONAL    0x2
#define SW_MUX_IN_ALTERNATE_1   0x3
#define SW_MUX_IN_ALTERNATE_2   0x4

/* Shift above flags into one of the four fields in each register */
#define SW_MUX_CTL_FLD_0(x)     ((x) << 0)
#define SW_MUX_CTL_FLD_1(x)     ((x) << 8)
#define SW_MUX_CTL_FLD_2(x)     ((x) << 16)
#define SW_MUX_CTL_FLD_3(x)     ((x) << 24)

/* SW_PAD_CTL */
#define SW_PAD_CTL_TTM_PAD__X__X                            IOMUXC_(0x154)
#define SW_PAD_CTL_CSPI3_MISO_CSPI3_SCLK_CSPI3_SPI_RDY      IOMUXC_(0x158)
#define SW_PAD_CTL_CE_CONTROL_CLKSS_CSPI3_MOSI              IOMUXC_(0x15C)
#define SW_PAD_CTL_ATA_DIOW_ATA_DMACK_ATA_RESET_B           IOMUXC_(0x160)
#define SW_PAD_CTL_ATA_CS0_ATA_CS1_ATA_DIOR                 IOMUXC_(0x164)
#define SW_PAD_CTL_SD1_DATA1_SD1_DATA2_SD1_DATA3            IOMUXC_(0x168)
#define SW_PAD_CTL_SD1_CMD_SD1_CLK_SD1_DATA0                IOMUXC_(0x16C)
#define SW_PAD_CTL_D3_REV_D3_CLS_D3_SPL                     IOMUXC_(0x170)
#define SW_PAD_CTL_READ_VSYNC3_CONTRAST                     IOMUXC_(0x174)
#define SW_PAD_CTL_SER_RS_PAR_RS_WRITE                      IOMUXC_(0x178)
#define SW_PAD_CTL_SD_D_CLK_LCS0_LCS1                       IOMUXC_(0x17C)
#define SW_PAD_CTL_DRDY0_SD_D_I_SD_D_IO                     IOMUXC_(0x180)
#define SW_PAD_CTL_VSYNC0_HSYNC_FPSHIFT                     IOMUXC_(0x184)
#define SW_PAD_CTL_LD15_LD16_LD17                           IOMUXC_(0x188)
#define SW_PAD_CTL_LD12_LD13_LD14                           IOMUXC_(0x18C)
#define SW_PAD_CTL_LD9_LD10_LD11                            IOMUXC_(0x190)
#define SW_PAD_CTL_LD6_LD7_LD8                              IOMUXC_(0x194)
#define SW_PAD_CTL_LD3_LD4_LD5                              IOMUXC_(0x198)
#define SW_PAD_CTL_LD0_LD1_LD2                              IOMUXC_(0x19C)
#define SW_PAD_CTL_USBH2_NXT_USBH2_DATA0_USBH2_DATA1        IOMUXC_(0x1A0)
#define SW_PAD_CTL_USBH2_CLK_USBH2_DIR_USBH2_STP            IOMUXC_(0x1A4)
#define SW_PAD_CTL_USBOTG_DATA5_USBOTG_DATA6_USBOTG_DATA7   IOMUXC_(0x1A8)
#define SW_PAD_CTL_USBOTG_DATA2_USBOTG_DATA3_USBOTG_DATA4   IOMUXC_(0x1AC)
#define SW_PAD_CTL_USBOTG_NXT_USBOTG_DATA0_USBOTG_DATA1     IOMUXC_(0x1B0)
#define SW_PAD_CTL_USBOTG_CLK_USBOTG_DIR_USBOTG_STP         IOMUXC_(0x1B4)
#define SW_PAD_CTL_USB_PWR_USB_OC_USB_BYP                   IOMUXC_(0x1B8)
#define SW_PAD_CTL_TRSTB_DE_B_SJC_MOD                       IOMUXC_(0x1BC)
#define SW_PAD_CTL_TMS_TDI_TDO                              IOMUXC_(0x1C0)
#define SW_PAD_CTL_KEY_COL7_RTCK_TCK                        IOMUXC_(0x1C4)
#define SW_PAD_CTL_KEY_COL4_KEY_COL5_KEY_COL6               IOMUXC_(0x1C8)
#define SW_PAD_CTL_KEY_COL1_KEY_COL2_KEY_COL3               IOMUXC_(0x1CC)
#define SW_PAD_CTL_KEY_ROW6_KEY_ROW7_KEY_COL0               IOMUXC_(0x1D0)
#define SW_PAD_CTL_KEY_ROW3_KEY_ROW4_KEY_ROW5               IOMUXC_(0x1D4)
#define SW_PAD_CTL_KEY_ROW0_KEY_ROW1_KEY_ROW2               IOMUXC_(0x1D8)
#define SW_PAD_CTL_RTS2_CTS2_BATT_LINE                      IOMUXC_(0x1DC)
#define SW_PAD_CTL_DTR_DCE2_RXD2_TXD2                       IOMUXC_(0x1E0)
#define SW_PAD_CTL_DSR_DTE1_RI_DTE1_DCD_DTE1                IOMUXC_(0x1E4)
#define SW_PAD_CTL_RI_DCE1_DCD_DCE1_DTR_DTE1                IOMUXC_(0x1E8)
#define SW_PAD_CTL_CTS1_DTR_DCE1_DSR_DCE1                   IOMUXC_(0x1EC)
#define SW_PAD_CTL_RXD1_TXD1_RTS1                           IOMUXC_(0x1F0)
#define SW_PAD_CTL_CSPI2_SS2_CSPI2_SCLK_CSPI2_SPI_RDY       IOMUXC_(0x1F4)
#define SW_PAD_CTL_CSPI2_MISO_CSPI2_SS0_CSPI2_SS1           IOMUXC_(0x1F8)
#define SW_PAD_CTL_CSPI1_SCLK_CSPI1_SPI_RDY_CSPI2_MOSI      IOMUXC_(0x1FC)
#define SW_PAD_CTL_CSPI1_SS0_CSPI1_SS1_CSPI1_SS             IOMUXC_(0x200)
#define SW_PAD_CTL_SFS6_CSPI1_MOSI_CSPI1_MISO               IOMUXC_(0x204)
#define SW_PAD_CTL_STXD6_SRXD6_SCK6                         IOMUXC_(0x208)
#define SW_PAD_CTL_SRXD5_SCK5_SFS5                          IOMUXC_(0x20C)
#define SW_PAD_CTL_SCK4_SFS4_STXD5                          IOMUXC_(0x210)
#define SW_PAD_CTL_SFS3_STXD4_SRXD4                         IOMUXC_(0x214)
#define SW_PAD_CTL_STXD3_SRXD3_SCK3                         IOMUXC_(0x218)
#define SW_PAD_CTL_CSI_PIXCLK_I2C_CLK_I2C_DAT               IOMUXC_(0x21C)
#define SW_PAD_CTL_CSI_MCLK_CSI_VSYNC_CSI_HSYNC             IOMUXC_(0x220)
#define SW_PAD_CTL_CSI_D13_CSI_D14_CSI_D15                  IOMUXC_(0x224)
#define SW_PAD_CTL_CSI_D10_CSI_D11_CSI_D12                  IOMUXC_(0x228)
#define SW_PAD_CTL_CSI_D7_CSI_D8_CSI_D9                     IOMUXC_(0x22C)
#define SW_PAD_CTL_CSI_D4_CSI_D5_CSI_D6                     IOMUXC_(0x230)
#define SW_PAD_CTL_PC_POE_M_REQUEST_M_GRANT                 IOMUXC_(0x234)
#define SW_PAD_CTL_PC_RST_IOIS16_PC_RW_B                    IOMUXC_(0x238)
#define SW_PAD_CTL_PC_VS2_PC_BVD1_PC_BVD2                   IOMUXC_(0x23C)
#define SW_PAD_CTL_PC_READY_PC_PWRON_PC_VS1                 IOMUXC_(0x240)
#define SW_PAD_CTL_PC_CD1_B_PC_CD2_B_PC_WAIT_B              IOMUXC_(0x244)
#define SW_PAD_CTL_D2_D1_D0                                 IOMUXC_(0x248)
#define SW_PAD_CTL_D5_D4_D3                                 IOMUXC_(0x24C)
#define SW_PAD_CTL_D8_D7_D6                                 IOMUXC_(0x250)
#define SW_PAD_CTL_D11_D10_D9                               IOMUXC_(0x254)
#define SW_PAD_CTL_D14_D13_D12                              IOMUXC_(0x258)
#define SW_PAD_CTL_NFCE_B_NFRB_D15                          IOMUXC_(0x25C)
#define SW_PAD_CTL_NFALE_NFCLE_NFWP_B                       IOMUXC_(0x260)
#define SW_PAD_CTL_SDQS3_NFWE_B_NFRE_B                      IOMUXC_(0x264)
#define SW_PAD_CTL_SDQS0_SDQS1_SDQS2                        IOMUXC_(0x268)
#define SW_PAD_CTL_SDCKE1_SDCLK_SDCLK_B                     IOMUXC_(0x26C)
#define SW_PAD_CTL_CAS_SDWE_SDCKE0                          IOMUXC_(0x270)
#define SW_PAD_CTL_BCLK_RW_RAS                              IOMUXC_(0x274)
#define SW_PAD_CTL_CS5_ECB_LBA                              IOMUXC_(0x278)
#define SW_PAD_CTL_CS2_CS3_CS4                              IOMUXC_(0x27C)
#define SW_PAD_CTL_OE_CS0_CS1                               IOMUXC_(0x280)
#define SW_PAD_CTL_DQM3_EB0_EB1                             IOMUXC_(0x284)
#define SW_PAD_CTL_DQM0_DQM1_DQM2                           IOMUXC_(0x288)
#define SW_PAD_CTL_SD29_SD30_SD31                           IOMUXC_(0x28C)
#define SW_PAD_CTL_SD26_SD27_SD28                           IOMUXC_(0x290)
#define SW_PAD_CTL_SD23_SD24_SD25                           IOMUXC_(0x294)
#define SW_PAD_CTL_SD20_SD21_SD22                           IOMUXC_(0x298)
#define SW_PAD_CTL_SD17_SD18_SD19                           IOMUXC_(0x29C)
#define SW_PAD_CTL_SD14_SD15_SD16                           IOMUXC_(0x2A0)
#define SW_PAD_CTL_SD11_SD12_SD13                           IOMUXC_(0x2A4)
#define SW_PAD_CTL_SD8_SD9_SD10                             IOMUXC_(0x2A8)
#define SW_PAD_CTL_SD5_SD6_SD7                              IOMUXC_(0x2AC)
#define SW_PAD_CTL_SD2_SD3_SD4                              IOMUXC_(0x2B0)
#define SW_PAD_CTL_SDBA0_SD0_SD1                            IOMUXC_(0x2B4)
#define SW_PAD_CTL_A24_A25_SDBA1                            IOMUXC_(0x2B8)
#define SW_PAD_CTL_A21_A22_A23                              IOMUXC_(0x2BC)
#define SW_PAD_CTL_A18_A19_A20                              IOMUXC_(0x2C0)
#define SW_PAD_CTL_A15_A16_A17                              IOMUXC_(0x2C4)
#define SW_PAD_CTL_A12_A13_A14                              IOMUXC_(0x2C8)
#define SW_PAD_CTL_A10_MA10_A11                             IOMUXC_(0x2CC)
#define SW_PAD_CTL_A7_A8_A9                                 IOMUXC_(0x2D0)
#define SW_PAD_CTL_A4_A5_A6                                 IOMUXC_(0x2D4)
#define SW_PAD_CTL_A1_A2_A3                                 IOMUXC_(0x2D8)
#define SW_PAD_CTL_VPG0_VPG1_A0                             IOMUXC_(0x2DC)
#define SW_PAD_CTL_VSTBY_DVFS0_DVFS1                        IOMUXC_(0x2E0)
#define SW_PAD_CTL_BOOT_MODE4_CKIL_POWER_FAIL               IOMUXC_(0x2E4)
#define SW_PAD_CTL_BOOT_MODE1_BOOT_MODE2_BOOT_MODE3         IOMUXC_(0x2E8)
#define SW_PAD_CTL_POR_B_CLKO_BOOT_MODE0                    IOMUXC_(0x2EC)
#define SW_PAD_CTL_SIMPD0_CKIH_RESET_IN_B                   IOMUXC_(0x2F0)
#define SW_PAD_CTL_SVEN0_STX0_SRX0                          IOMUXC_(0x2F4)
#define SW_PAD_CTL_GPIO3_1_SCLK0_SRST0                      IOMUXC_(0x2F8)
#define SW_PAD_CTL_GPIO1_5_GPIO1_6_GPIO3_0                  IOMUXC_(0x2FC)
#define SW_PAD_CTL_GPIO1_2_GPIO1_3_GPIO1_4                  IOMUXC_(0x300)
#define SW_PAD_CTL_PWMO_GPIO1_0_GPIO1_1                     IOMUXC_(0x304)
#define SW_PAD_CTL_CAPTURE_COMPARE_WATCHDOG_RST             IOMUXC_(0x308)

/* SW_PAD_CTL flags */
#define SW_PAD_CTL_LOOPBACK                         (1 << 9)
#define SW_PAD_CTL_DISABLE_PULL_UP_DOWN_AND_KEEPER  (0 << 7)
#if 0 /* Same as 0 */
#define SW_PAD_CTL_DISABLE_PULL_UP_DOWN_AND_KEEPER  (1 << 7)
#endif
#define SW_PAD_CTL_ENABLE_KEEPER                    (2 << 7)
#define SW_PAD_CTL_ENABLE_PULL_UP_OR_PULL_DOWN      (3 << 7)
#define SW_PAD_CTL_100K_PULL_DOWN                   (0 << 5)
#define SW_PAD_CTL_100K_PULL_UP                     (1 << 5)
#if 0 /* Completeness */
#define SW_PAD_CTL_47K_PULL_UP                      (2 << 5) /* Not in IMX31/L */
#define SW_PAD_CTL_22K_PULL_UP                      (3 << 5) /* Not in IMX31/L */
#endif
#define SW_PAD_CTL_IPP_HYS_STD                      (0 << 4)
#define SW_PAD_CTL_IPP_HYS_SCHIMDT                  (1 << 4)
#define SW_PAD_CTL_IPP_ODE_CMOS                     (0 << 3)
#define SW_PAD_CTL_IPP_ODE_OPEN                     (1 << 3)
#define SW_PAD_CTL_IPP_DSE_STD                      (0 << 1)
#define SW_PAD_CTL_IPP_DSE_HIGH                     (1 << 1)
#define SW_PAD_CTL_IPP_DSE_MAX                      (2 << 1)
#if 0 /* Same as 2 */
#define SW_PAD_CTL_IPP_DSE_MAX                      (3 << 1)
#endif
#define SW_PAD_CTL_IPP_SRE_SLOW                     (0 << 0)
#define SW_PAD_CTL_IPP_SRE_FAST                     (1 << 0)

/* Shift above flags into one of the three fields in each register */
#define SW_PAD_CTL_FLD_0(x)                         ((x) << 0)
#define SW_PAD_CTL_FLD_1(x)                         ((x) << 10)
#define SW_PAD_CTL_FLD_2(x)                         ((x) << 20)

/* IPU */
#define IPU_CONF                (*(REG32_PTR_T)(IPU_CTRL_BASE_ADDR+0x00))
#define IPU_CHA_BUF0_RDY        (*(REG32_PTR_T)(IPU_CTRL_BASE_ADDR+0x04))
#define IPU_CHA_BUF1_RDY        (*(REG32_PTR_T)(IPU_CTRL_BASE_ADDR+0x08))
#define IPU_CHA_DB_MODE_SEL     (*(REG32_PTR_T)(IPU_CTRL_BASE_ADDR+0x0C))
#define IPU_CHA_CUR_BUF         (*(REG32_PTR_T)(IPU_CTRL_BASE_ADDR+0x10))
#define IPU_FS_PROC_FLOW        (*(REG32_PTR_T)(IPU_CTRL_BASE_ADDR+0x14))
#define IPU_FS_DISP_FLOW        (*(REG32_PTR_T)(IPU_CTRL_BASE_ADDR+0x18))
#define IPU_TASKS_STAT          (*(REG32_PTR_T)(IPU_CTRL_BASE_ADDR+0x1C))
#define IPU_IMA_ADDR            (*(REG32_PTR_T)(IPU_CTRL_BASE_ADDR+0x20))
#define IPU_IMA_DATA            (*(REG32_PTR_T)(IPU_CTRL_BASE_ADDR+0x24))
#define IPU_INT_CTRL_1          (*(REG32_PTR_T)(IPU_CTRL_BASE_ADDR+0x28))
#define IPU_INT_CTRL_2          (*(REG32_PTR_T)(IPU_CTRL_BASE_ADDR+0x2C))
#define IPU_INT_CTRL_3          (*(REG32_PTR_T)(IPU_CTRL_BASE_ADDR+0x30))
#define IPU_INT_CTRL_4          (*(REG32_PTR_T)(IPU_CTRL_BASE_ADDR+0x34))
#define IPU_INT_CTRL_5          (*(REG32_PTR_T)(IPU_CTRL_BASE_ADDR+0x38))
#define IPU_INT_STAT_1          (*(REG32_PTR_T)(IPU_CTRL_BASE_ADDR+0x3C))
#define IPU_INT_STAT_2          (*(REG32_PTR_T)(IPU_CTRL_BASE_ADDR+0x40))
#define IPU_INT_STAT_3          (*(REG32_PTR_T)(IPU_CTRL_BASE_ADDR+0x44))
#define IPU_INT_STAT_4          (*(REG32_PTR_T)(IPU_CTRL_BASE_ADDR+0x48))
#define IPU_INT_STAT_5          (*(REG32_PTR_T)(IPU_CTRL_BASE_ADDR+0x4C))
#define IPU_BRK_CTRL_1          (*(REG32_PTR_T)(IPU_CTRL_BASE_ADDR+0x50))
#define IPU_BRK_CTRL_2          (*(REG32_PTR_T)(IPU_CTRL_BASE_ADDR+0x54))
#define IPU_BRK_STAT            (*(REG32_PTR_T)(IPU_CTRL_BASE_ADDR+0x58))
#define IPU_DIAGB_CTRL          (*(REG32_PTR_T)(IPU_CTRL_BASE_ADDR+0x60))


/* ATA */
#define ATA_TIME_OFF            (*(REG8_PTR_T)(ATA_BASE_ADDR+0x00))
#define ATA_TIME_ON             (*(REG8_PTR_T)(ATA_BASE_ADDR+0x01))
#define ATA_TIME_1              (*(REG8_PTR_T)(ATA_BASE_ADDR+0x02))
#define ATA_TIME_2W             (*(REG8_PTR_T)(ATA_BASE_ADDR+0x03))
    /* PIO */
#define ATA_TIME_2R             (*(REG8_PTR_T)(ATA_BASE_ADDR+0x04))
#define ATA_TIME_AX             (*(REG8_PTR_T)(ATA_BASE_ADDR+0x05))
#define ATA_TIME_4              (*(REG8_PTR_T)(ATA_BASE_ADDR+0x07))
#define ATA_TIME_9              (*(REG8_PTR_T)(ATA_BASE_ADDR+0x08))
    /* MDMA */
#define ATA_TIME_M              (*(REG8_PTR_T)(ATA_BASE_ADDR+0x09))
#define ATA_TIME_JN             (*(REG8_PTR_T)(ATA_BASE_ADDR+0x0A))
#define ATA_TIME_D              (*(REG8_PTR_T)(ATA_BASE_ADDR+0x0B))
#define ATA_TIME_K              (*(REG8_PTR_T)(ATA_BASE_ADDR+0x0C))
    /* UDMA */
#define ATA_TIME_ACK            (*(REG8_PTR_T)(ATA_BASE_ADDR+0x0D))
#define ATA_TIME_ENV            (*(REG8_PTR_T)(ATA_BASE_ADDR+0x0E))
#define ATA_TIME_PIO_RDX        (*(REG8_PTR_T)(ATA_BASE_ADDR+0x0F))
#define ATA_TIME_ZAH            (*(REG8_PTR_T)(ATA_BASE_ADDR+0x10))
#define ATA_TIME_MLIX           (*(REG8_PTR_T)(ATA_BASE_ADDR+0x11))
#define ATA_TIME_DVH            (*(REG8_PTR_T)(ATA_BASE_ADDR+0x12))
#define ATA_TIME_DZFS           (*(REG8_PTR_T)(ATA_BASE_ADDR+0x13))
#define ATA_TIME_DVS            (*(REG8_PTR_T)(ATA_BASE_ADDR+0x14))
#define ATA_TIME_CVS            (*(REG8_PTR_T)(ATA_BASE_ADDR+0x15))
#define ATA_TIME_SS             (*(REG8_PTR_T)(ATA_BASE_ADDR+0x16))
#define ATA_TIME_CYC            (*(REG8_PTR_T)(ATA_BASE_ADDR+0x17))
    /* */
#define ATA_FIFO_DATA_32        (*(REG32_PTR_T)(ATA_BASE_ADDR+0x18))
#define ATA_FIFO_DATA_16        (*(REG16_PTR_T)(ATA_BASE_ADDR+0x1c))
#define ATA_FIFO_FILL           (*(REG8_PTR_T)(ATA_BASE_ADDR+0x20))
    /* Actually ATA_CONTROL but conflicts arise */
#define ATA_INTF_CONTROL        (*(REG8_PTR_T)(ATA_BASE_ADDR+0x24))
#define ATA_INTERRUPT_PENDING   (*(REG8_PTR_T)(ATA_BASE_ADDR+0x28))
#define ATA_INTERRUPT_ENABLE    (*(REG8_PTR_T)(ATA_BASE_ADDR+0x2c))
#define ATA_INTERRUPT_CLEAR     (*(REG8_PTR_T)(ATA_BASE_ADDR+0x30))
#define ATA_FIFO_ALARM          (*(REG8_PTR_T)(ATA_BASE_ADDR+0x34))
#define ATA_DRIVE_DATA          (*(REG16_PTR_T)(ATA_BASE_ADDR+0xA0))
#define ATA_DRIVE_FEATURES      (*(REG8_PTR_T)(ATA_BASE_ADDR+0xA4))
#define ATA_DRIVE_SECTOR_COUNT  (*(REG8_PTR_T)(ATA_BASE_ADDR+0xA8))
#define ATA_DRIVE_SECTOR_NUM    (*(REG8_PTR_T)(ATA_BASE_ADDR+0xAC))
#define ATA_DRIVE_CYL_LOW       (*(REG8_PTR_T)(ATA_BASE_ADDR+0xB0))
#define ATA_DRIVE_CYL_HIGH      (*(REG8_PTR_T)(ATA_BASE_ADDR+0xB4))
#define ATA_DRIVE_CYL_HEAD      (*(REG8_PTR_T)(ATA_BASE_ADDR+0xB8))
#define ATA_DRIVE_STATUS        (*(REG8_PTR_T)(ATA_BASE_ADDR+0xBC)) /* rd */
#define ATA_DRIVE_COMMAND       (*(REG8_PTR_T)(ATA_BASE_ADDR+0xBC)) /* wr */
#define ATA_ALT_DRIVE_STATUS    (*(REG8_PTR_T)(ATA_BASE_ADDR+0xD8)) /* rd */
#define ATA_DRIVE_CONTROL       (*(REG8_PTR_T)(ATA_BASE_ADDR+0xD8)) /* wr */

/* ATA_INTF_CONTROL flags */
#define ATA_FIFO_RST            (1 << 7)
#define ATA_ATA_RST             (1 << 6)
#define ATA_FIFO_TX_EN          (1 << 5)
#define ATA_FIFO_RCV_EN         (1 << 4)
#define ATA_DMA_PENDING         (1 << 3)
#define ATA_DMA_ULTRA_SELECTED  (1 << 2)
#define ATA_DMA_WRITE           (1 << 1)
#define ATA_IORDY_EN            (1 << 0)

/* ATA_INTERRUPT_PENDING, ATA_INTERRUPT_ENABLE, ATA_INTERRUPT_CLEAR flags */
#define ATA_INTRQ1              (1 << 7)
#define ATA_FIFO_UNDERFLOW      (1 << 6)
#define ATA_FIFO_OVERFLOW       (1 << 5)
#define ATA_CONTROLLER_IDLE     (1 << 4)
#define ATA_INTRQ2              (1 << 3)

/* EPIT */
#define EPITCR1                 (*(REG32_PTR_T)(EPIT1_BASE_ADDR+0x00))
#define EPITSR1                 (*(REG32_PTR_T)(EPIT1_BASE_ADDR+0x04))
#define EPITLR1                 (*(REG32_PTR_T)(EPIT1_BASE_ADDR+0x08))
#define EPITCMPR1               (*(REG32_PTR_T)(EPIT1_BASE_ADDR+0x0C))
#define EPITCNT1                (*(REG32_PTR_T)(EPIT1_BASE_ADDR+0x10))

#define EPITCR2                 (*(REG32_PTR_T)(EPIT2_BASE_ADDR+0x00))
#define EPITSR2                 (*(REG32_PTR_T)(EPIT2_BASE_ADDR+0x04))
#define EPITLR2                 (*(REG32_PTR_T)(EPIT2_BASE_ADDR+0x08))
#define EPITCMPR2               (*(REG32_PTR_T)(EPIT2_BASE_ADDR+0x0C))
#define EPITCNT2                (*(REG32_PTR_T)(EPIT2_BASE_ADDR+0x10))

#define EPITCR_CLKSRC_OFF               (0 << 24)
#define EPITCR_CLKSRC_IPG_CLK           (1 << 24)
#define EPITCR_CLKSRC_IPG_CLK_HIGHFREQ  (2 << 24)
#define EPITCR_CLKSRC_IPG_CLK_32K       (3 << 24)
#define EPITCR_OM_DISCONNECTED          (0 << 22)
#define EPITCR_OM_TOGGLE                (1 << 22)
#define EPITCR_OM_CLEAR                 (2 << 22)
#define EPITCR_OM_SET                   (3 << 22)
#define EPITCR_STOPEN                   (1 << 21)
#define EPITCR_DOZEN                    (1 << 20)
#define EPITCR_WAITEN                   (1 << 19)
#define EPITCR_DBGEN                    (1 << 18)
#define EPITCR_IOVW                     (1 << 17)
#define EPITCR_SWR                      (1 << 16)
#define EPITCR_PRESCALER(n)             ((n) << 4) /* Divide by n+1 */
#define EPITCR_RLD                      (1 << 3)
#define EPITCR_OCIEN                    (1 << 2)
#define EPITCR_ENMOD                    (1 << 1)
#define EPITCR_EN                       (1 << 0)

#define EPITSR_OCIF                     (1 << 0)

/* GPIO */
#define GPIO_DR_I               0x00 /* Offset - 0x00 */
#define GPIO_GDIR_I             0x01 /* Offset - 0x04 */
#define GPIO_PSR_I              0x02 /* Offset - 0x08 */
#define GPIO_ICR1_I             0x03 /* Offset - 0x0C */
#define GPIO_ICR2_I             0x04 /* Offset - 0x10 */
#define GPIO_IMR_I              0x05 /* Offset - 0x14 */
#define GPIO_ISR_I              0x06 /* Offset - 0x18 */

#define GPIO1_DR                (((REG32_PTR_T)GPIO1_BASE_ADDR)[GPIO_DR_I])
#define GPIO1_GDIR              (((REG32_PTR_T)GPIO1_BASE_ADDR)[GPIO_GDIR_I])
#define GPIO1_PSR               (((REG32_PTR_T)GPIO1_BASE_ADDR)[GPIO_PSR_I])
#define GPIO1_ICR1              (((REG32_PTR_T)GPIO1_BASE_ADDR)[GPIO_ICR1_I])
#define GPIO1_ICR2              (((REG32_PTR_T)GPIO1_BASE_ADDR)[GPIO_ICR2_I])
#define GPIO1_IMR               (((REG32_PTR_T)GPIO1_BASE_ADDR)[GPIO_IMR_I])
#define GPIO1_ISR               (((REG32_PTR_T)GPIO1_BASE_ADDR)[GPIO_ISR_I])

#define GPIO2_DR                (((REG32_PTR_T)GPIO2_BASE_ADDR)[GPIO_DR_I])
#define GPIO2_GDIR              (((REG32_PTR_T)GPIO2_BASE_ADDR)[GPIO_GDIR_I])
#define GPIO2_PSR               (((REG32_PTR_T)GPIO2_BASE_ADDR)[GPIO_PSR_I])
#define GPIO2_ICR1              (((REG32_PTR_T)GPIO2_BASE_ADDR)[GPIO_ICR1_I])
#define GPIO2_ICR2              (((REG32_PTR_T)GPIO2_BASE_ADDR)[GPIO_ICR2_I])
#define GPIO2_IMR               (((REG32_PTR_T)GPIO2_BASE_ADDR)[GPIO_IMR_I])
#define GPIO2_ISR               (((REG32_PTR_T)GPIO2_BASE_ADDR)[GPIO_ISR_I])

#define GPIO3_DR                (((REG32_PTR_T)GPIO3_BASE_ADDR)[GPIO_DR_I])
#define GPIO3_GDIR              (((REG32_PTR_T)GPIO3_BASE_ADDR)[GPIO_GDIR_I])
#define GPIO3_PSR               (((REG32_PTR_T)GPIO3_BASE_ADDR)[GPIO_PSR_I])
#define GPIO3_ICR1              (((REG32_PTR_T)GPIO3_BASE_ADDR)[GPIO_ICR1_I])
#define GPIO3_ICR2              (((REG32_PTR_T)GPIO3_BASE_ADDR)[GPIO_ICR2_I])
#define GPIO3_IMR               (((REG32_PTR_T)GPIO3_BASE_ADDR)[GPIO_IMR_I])
#define GPIO3_ISR               (((REG32_PTR_T)GPIO3_BASE_ADDR)[GPIO_ISR_I])

/* CSPI */
#define CSPI_RXDATA_I           0x00 /* Offset - 0x00  */
#define CSPI_TXDATA_I           0x01 /* Offset - 0x04  */
#define CSPI_CONREG_I           0x02 /* Offset - 0x08  */
#define CSPI_INTREG_I           0x03 /* Offset - 0x0C  */
#define CSPI_DMAREG_I           0x04 /* Offset - 0x10  */
#define CSPI_STATREG_I          0x05 /* Offset - 0x14  */
#define CSPI_PERIODREG_I        0x06 /* Offset - 0x18  */
#define CSPI_TESTREG_I          0x70 /* Offset - 0x1C0 */

#define CSPI_RXDATA1            (((REG32_PTR_T)CSPI1_BASE_ADDR)[CSPI_RXDATA_I])
#define CSPI_TXDATA1            (((REG32_PTR_T)CSPI1_BASE_ADDR)[CSPI_TXDATA_I])
#define CSPI_CONREG1            (((REG32_PTR_T)CSPI1_BASE_ADDR)[CSPI_CONREG_I])
#define CSPI_INTREG1            (((REG32_PTR_T)CSPI1_BASE_ADDR)[CSPI_INTREG_I])
#define CSPI_DMAREG1            (((REG32_PTR_T)CSPI1_BASE_ADDR)[CSPI_DMAREG_I])
#define CSPI_STATREG1           (((REG32_PTR_T)CSPI1_BASE_ADDR)[CSPI_STATREG_I])
#define CSPI_PERIODREG1         (((REG32_PTR_T)CSPI1_BASE_ADDR)[CSPI_PERIODREG_I])
#define CSPI_TESTREG1           (((REG32_PTR_T)CSPI1_BASE_ADDR)[CSPI_TESTREG_I])

#define CSPI_RXDATA2            (((REG32_PTR_T)CSPI2_BASE_ADDR)[CSPI_RXDATA_I])
#define CSPI_TXDATA2            (((REG32_PTR_T)CSPI2_BASE_ADDR)[CSPI_TXDATA_I])
#define CSPI_CONREG2            (((REG32_PTR_T)CSPI2_BASE_ADDR)[CSPI_CONREG_I])
#define CSPI_INTREG2            (((REG32_PTR_T)CSPI2_BASE_ADDR)[CSPI_INTREG_I])
#define CSPI_DMAREG2            (((REG32_PTR_T)CSPI2_BASE_ADDR)[CSPI_DMAREG_I])
#define CSPI_STATREG2           (((REG32_PTR_T)CSPI2_BASE_ADDR)[CSPI_STATREG_I])
#define CSPI_PERIODREG2         (((REG32_PTR_T)CSPI2_BASE_ADDR)[CSPI_PERIODREG_I])
#define CSPI_TESTREG2           (((REG32_PTR_T)CSPI2_BASE_ADDR)[CSPI_TESTREG_I])

#define CSPI_RXDATA3            (((REG32_PTR_T)CSPI3_BASE_ADDR)[CSPI_RXDATA_I])
#define CSPI_TXDATA3            (((REG32_PTR_T)CSPI3_BASE_ADDR)[CSPI_TXDATA_I])
#define CSPI_CONREG3            (((REG32_PTR_T)CSPI3_BASE_ADDR)[CSPI_CONREG_I])
#define CSPI_INTREG3            (((REG32_PTR_T)CSPI3_BASE_ADDR)[CSPI_INTREG_I])
#define CSPI_DMAREG3            (((REG32_PTR_T)CSPI3_BASE_ADDR)[CSPI_DMAREG_I])
#define CSPI_STATREG3           (((REG32_PTR_T)CSPI3_BASE_ADDR)[CSPI_STATREG_I])
#define CSPI_PERIODREG3         (((REG32_PTR_T)CSPI3_BASE_ADDR)[CSPI_PERIODREG_I])
#define CSPI_TESTREG3           (((REG32_PTR_T)CSPI3_BASE_ADDR)[CSPI_TESTREG_I])

/* CSPI CONREG flags/fields */
#define CSPI_CONREG_CHIP_SELECT_SS0     (0 << 24)
#define CSPI_CONREG_CHIP_SELECT_SS1     (1 << 24)
#define CSPI_CONREG_CHIP_SELECT_SS2     (2 << 24)
#define CSPI_CONREG_CHIP_SELECT_SS3     (3 << 24)
#define CSPI_CONREG_CHIP_SELECT_MASK    (3 << 24)
#define CSPI_CONREG_DRCTL_DONT_CARE     (0 << 20)
#define CSPI_CONREG_DRCTL_TRIG_FALLING  (1 << 20)
#define CSPI_CONREG_DRCTL_TRIG_LOW      (2 << 20)
#define CSPI_CONREG_DRCTL_TRIG_RSV      (3 << 20)
#define CSPI_CONREG_DRCTL_MASK          (3 << 20)
#define CSPI_CONREG_DATA_RATE_DIV_4     (0 << 16)
#define CSPI_CONREG_DATA_RATE_DIV_8     (1 << 16)
#define CSPI_CONREG_DATA_RATE_DIV_16    (2 << 16)
#define CSPI_CONREG_DATA_RATE_DIV_32    (3 << 16)
#define CSPI_CONREG_DATA_RATE_DIV_64    (4 << 16)
#define CSPI_CONREG_DATA_RATE_DIV_128   (5 << 16)
#define CSPI_CONREG_DATA_RATE_DIV_256   (6 << 16)
#define CSPI_CONREG_DATA_RATE_DIV_512   (7 << 16)
#define CSPI_CONREG_DATA_RATE_DIV_MASK  (7 << 16)
#define CSPI_BITCOUNT(n)                ((n) << 8)
#define CSPI_CONREG_SSPOL               (1 << 7)
#define CSPI_CONREG_SSCTL               (1 << 6)
#define CSPI_CONREG_PHA                 (1 << 6)
#define CSPI_CONREG_POL                 (1 << 4)
#define CSPI_CONREG_SMC                 (1 << 3)
#define CSPI_CONREG_XCH                 (1 << 2)
#define CSPI_CONREG_MODE                (1 << 1)
#define CSPI_CONREG_EN                  (1 << 0)

/* CSPI INTREG flags */
#define CSPI_INTREG_TCEN                (1 << 8)
#define CSPI_INTREG_BOEN                (1 << 7)
#define CSPI_INTREG_ROEN                (1 << 6)
#define CSPI_INTREG_RFEN                (1 << 5)
#define CSPI_INTREG_RHEN                (1 << 4)
#define CSPI_INTREG_RREN                (1 << 3)
#define CSPI_INTREG_TFEN                (1 << 2)
#define CSPI_INTREG_THEN                (1 << 1)
#define CSPI_INTREG_TEEN                (1 << 0)

/* CSPI DMAREG flags */
#define CSPI_DMAREG_RFDEN               (1 << 5)
#define CSPI_DMAREG_RHDEN               (1 << 4)
#define CSPI_DMAREG_THDEN               (1 << 1)
#define CSPI_DMAREG_TEDEN               (1 << 0)

/* CSPI STATREG flags */
#define CSPI_STATREG_TC                 (1 << 8) /* w1c */
#define CSPI_STATREG_BO                 (1 << 7) /* w1c */
#define CSPI_STATREG_RO                 (1 << 6)
#define CSPI_STATREG_RF                 (1 << 5)
#define CSPI_STATREG_RH                 (1 << 4)
#define CSPI_STATREG_RR                 (1 << 3)
#define CSPI_STATREG_TF                 (1 << 2)
#define CSPI_STATREG_TH                 (1 << 1)
#define CSPI_STATREG_TE                 (1 << 0)

/* CSPI PERIODREG flags */
#define CSPI_PERIODREG_CSRC             (1 << 15)

/* CSPI TESTREG flags */
#define CSPI_TESTREG_SWAP               (1 << 15)
#define CSPI_TESTREG_LBC                (1 << 14)

/* RTC */
#define RTC_HOURMIN             (*(REG32_PTR_T)(RTC_BASE_ADDR+0x00))
#define RTC_SECONDS             (*(REG32_PTR_T)(RTC_BASE_ADDR+0x04))
#define RTC_ALRM_HM             (*(REG32_PTR_T)(RTC_BASE_ADDR+0x08))
#define RTC_ALRM_SEC            (*(REG32_PTR_T)(RTC_BASE_ADDR+0x0C))
#define RTC_CTL                 (*(REG32_PTR_T)(RTC_BASE_ADDR+0x10))
#define RTC_ISR                 (*(REG32_PTR_T)(RTC_BASE_ADDR+0x14))
#define RTC_IENR                (*(REG32_PTR_T)(RTC_BASE_ADDR+0x18))
#define RTC_STPWCH              (*(REG32_PTR_T)(RTC_BASE_ADDR+0x1C))
#define RTC_DAYR                (*(REG32_PTR_T)(RTC_BASE_ADDR+0x20))
#define RTC_DAYALARM            (*(REG32_PTR_T)(RTC_BASE_ADDR+0x24))

/* Keypad */
#define KPP_KPCR                (*(REG16_PTR_T)(KPP_BASE_ADDR+0x0))
#define KPP_KPSR                (*(REG16_PTR_T)(KPP_BASE_ADDR+0x2))
#define KPP_KDDR                (*(REG16_PTR_T)(KPP_BASE_ADDR+0x4))
#define KPP_KPDR                (*(REG16_PTR_T)(KPP_BASE_ADDR+0x6))

/* KPP_KPSR bits */
#define KPP_KPSR_KRIE           (1 << 9)
#define KPP_KPSR_KDIE           (1 << 8)
#define KPP_KPSR_KRSS           (1 << 3)
#define KPP_KPSR_KDSC           (1 << 2)
#define KPP_KPSR_KPKR           (1 << 1)
#define KPP_KPSR_KPKD           (1 << 0)

/* ROMPATCH and AVIC */
#define ROMPATCH_BASE_ADDR      0x60000000

/* Since AVIC vector registers are NOT used, we reserve some for various
 * purposes. Copied from Linux source code. */
#define CHIP_REV_1_0            0x10
#define CHIP_REV_2_0            0x20
#define SYSTEM_REV_ID_REG       (AVIC_BASE_ADDR + AVIC_VEC_1)
#define SYSTEM_REV_ID_MAG       0xF00C

/*
 * NAND, SDRAM, WEIM, M3IF, EMI controllers
 */
#define EXT_MEM_CTRL_BASE       0xB8000000
#define NFC_BASE                EXT_MEM_CTRL_BASE
#define ESDCTL_BASE             0xB8001000
#define WEIM_BASE_ADDR          0xB8002000
#define WEIM_CTRL_CS0           (WEIM_BASE_ADDR+0x00)
#define WEIM_CTRL_CS1           (WEIM_BASE_ADDR+0x10)
#define WEIM_CTRL_CS2           (WEIM_BASE_ADDR+0x20)
#define WEIM_CTRL_CS3           (WEIM_BASE_ADDR+0x30)
#define WEIM_CTRL_CS4           (WEIM_BASE_ADDR+0x40)
#define M3IF_BASE               0xB8003000
#define PCMCIA_CTL_BASE         0xB8004000

/*
 * Memory regions and CS
 */
#define IPU_MEM_BASE_ADDR       0x70000000
#define CSD0_BASE_ADDR          0x80000000
#define CSD1_BASE_ADDR          0x90000000
#define CS0_BASE_ADDR           0xA0000000
#define CS1_BASE_ADDR           0xA8000000
#define CS2_BASE_ADDR           0xB0000000
#define CS3_BASE_ADDR           0xB2000000
#define CS4_BASE_ADDR           0xB4000000
#define CS4_BASE_PSRAM          0xB5000000
#define CS5_BASE_ADDR           0xB6000000
#define PCMCIA_MEM_BASE_ADDR    0xC0000000

#define INTERNAL_ROM_VA         0xF0000000

/*
 * SDRAM
 */
#define RAM_BANK0_BASE          SDRAM_BASE_ADDR

/*
 * IRQ Controller Register Definitions.
 */
#define AVIC_BASE_ADDR         0x68000000
#define INTCNTL                (*(REG32_PTR_T)(AVIC_BASE_ADDR+0x00))
#define NIMASK                 (*(REG32_PTR_T)(AVIC_BASE_ADDR+0x04))
#define INTENNUM               (*(REG32_PTR_T)(AVIC_BASE_ADDR+0x08))
#define INTDISNUM              (*(REG32_PTR_T)(AVIC_BASE_ADDR+0x0C))
#define INTENABLEH             (*(REG32_PTR_T)(AVIC_BASE_ADDR+0x10))
#define INTENABLEL             (*(REG32_PTR_T)(AVIC_BASE_ADDR+0x14))
#define INTTYPEH               (*(REG32_PTR_T)(AVIC_BASE_ADDR+0x18))
#define INTTYPEL               (*(REG32_PTR_T)(AVIC_BASE_ADDR+0x1C))
#define NIPRIORITY(n)          (((REG32_PTR_T)(AVIC_BASE_ADDR+0x20))[n])
#define NIPRIORITY7            (*(REG32_PTR_T)(AVIC_BASE_ADDR+0x20))
#define NIPRIORITY6            (*(REG32_PTR_T)(AVIC_BASE_ADDR+0x24))
#define NIPRIORITY5            (*(REG32_PTR_T)(AVIC_BASE_ADDR+0x28))
#define NIPRIORITY4            (*(REG32_PTR_T)(AVIC_BASE_ADDR+0x2C))
#define NIPRIORITY3            (*(REG32_PTR_T)(AVIC_BASE_ADDR+0x30))
#define NIPRIORITY2            (*(REG32_PTR_T)(AVIC_BASE_ADDR+0x34))
#define NIPRIORITY1            (*(REG32_PTR_T)(AVIC_BASE_ADDR+0x38))
#define NIPRIORITY0            (*(REG32_PTR_T)(AVIC_BASE_ADDR+0x3C))
#define NIVECSR                (*(REG32_PTR_T)(AVIC_BASE_ADDR+0x40))
#define FIVECSR                (*(REG32_PTR_T)(AVIC_BASE_ADDR+0x44))
#define INTSRCH                (*(REG32_PTR_T)(AVIC_BASE_ADDR+0x48))
#define INTSRCL                (*(REG32_PTR_T)(AVIC_BASE_ADDR+0x4C))
#define INTFRCH                (*(REG32_PTR_T)(AVIC_BASE_ADDR+0x50))
#define INTFRCL                (*(REG32_PTR_T)(AVIC_BASE_ADDR+0x54))
#define NIPNDH                 (*(REG32_PTR_T)(AVIC_BASE_ADDR+0x58))
#define NIPNDL                 (*(REG32_PTR_T)(AVIC_BASE_ADDR+0x5C))
#define FIPNDH                 (*(REG32_PTR_T)(AVIC_BASE_ADDR+0x60))
#define FIPNDL                 (*(REG32_PTR_T)(AVIC_BASE_ADDR+0x64))
#define VECTOR_BASE_ADDR       (AVIC_BASE_ADDR+0x100)
#define VECTOR(n)              (((REG32_PTR_T)VECTOR_BASE_ADDR)[n])

/* The vectors go all the way up to 63. 4 bytes for each */
#define INTCNTL_ABFLAG         (1 << 25)
#define INTCNTL_ABFEN          (1 << 24)
#define INTCNTL_NIDIS          (1 << 22)
#define INTCNTL_FIDIS          (1 << 21)
#define INTCNTL_NIAD           (1 << 20)
#define INTCNTL_FIAD           (1 << 19)
#define INTCNTL_NM             (1 << 18)

/* L210 */
#define L2CC_BASE_ADDR                  0x30000000
#define L2_CACHE_LINE_SIZE              32
#define L2_CACHE_CTL_REG                0x100
#define L2_CACHE_AUX_CTL_REG            0x104
#define L2_CACHE_SYNC_REG               0x730
#define L2_CACHE_INV_LINE_REG           0x770
#define L2_CACHE_INV_WAY_REG            0x77C
#define L2_CACHE_CLEAN_LINE_REG         0x7B0
#define L2_CACHE_CLEAN_INV_LINE_REG     0x7F0

#define L2CC_CACHE_SYNC                 (*(REG32_PTR_T)(L2CC_BASE_ADDR+L2_CACHE_SYNC_REG))

/* CCM */
#define CLKCTL_CCMR                     (*(REG32_PTR_T)(CCM_BASE_ADDR+0x00))
#define CLKCTL_PDR0                     (*(REG32_PTR_T)(CCM_BASE_ADDR+0x04))
#define CLKCTL_PDR1                     (*(REG32_PTR_T)(CCM_BASE_ADDR+0x08))
#define CLKCTL_RCSR                     (*(REG32_PTR_T)(CCM_BASE_ADDR+0x0C))
#define CLKCTL_MPCTL                    (*(REG32_PTR_T)(CCM_BASE_ADDR+0x10))
#define CLKCTL_UPCTL                    (*(REG32_PTR_T)(CCM_BASE_ADDR+0x14))
#define CLKCTL_SPCTL                    (*(REG32_PTR_T)(CCM_BASE_ADDR+0x18))
#define CLKCTL_COSR                     (*(REG32_PTR_T)(CCM_BASE_ADDR+0x1C))
#define CLKCTL_CGR0                     (*(REG32_PTR_T)(CCM_BASE_ADDR+0x20))
#define CLKCTL_CGR1                     (*(REG32_PTR_T)(CCM_BASE_ADDR+0x24))
#define CLKCTL_CGR2                     (*(REG32_PTR_T)(CCM_BASE_ADDR+0x28))
#define CLKCTL_WIMR0                    (*(REG32_PTR_T)(CCM_BASE_ADDR+0x2C))
#define CLKCTL_LDC                      (*(REG32_PTR_T)(CCM_BASE_ADDR+0x30))
#define CLKCTL_DCVR0                    (*(REG32_PTR_T)(CCM_BASE_ADDR+0x34))
#define CLKCTL_DCVR1                    (*(REG32_PTR_T)(CCM_BASE_ADDR+0x38))
#define CLKCTL_DCVR2                    (*(REG32_PTR_T)(CCM_BASE_ADDR+0x3C))
#define CLKCTL_DCVR3                    (*(REG32_PTR_T)(CCM_BASE_ADDR+0x40))
#define CLKCTL_LTR0                     (*(REG32_PTR_T)(CCM_BASE_ADDR+0x44))
#define CLKCTL_LTR1                     (*(REG32_PTR_T)(CCM_BASE_ADDR+0x48))
#define CLKCTL_LTR2                     (*(REG32_PTR_T)(CCM_BASE_ADDR+0x4C))
#define CLKCTL_LTR3                     (*(REG32_PTR_T)(CCM_BASE_ADDR+0x50))
#define CLKCTL_LTBR0                    (*(REG32_PTR_T)(CCM_BASE_ADDR+0x54))
#define CLKCTL_LTBR1                    (*(REG32_PTR_T)(CCM_BASE_ADDR+0x58))
#define CLKCTL_PMCR0                    (*(REG32_PTR_T)(CCM_BASE_ADDR+0x5C))
#define CLKCTL_PMCR1                    (*(REG32_PTR_T)(CCM_BASE_ADDR+0x60))
#define CLKCTL_PDR2                     (*(REG32_PTR_T)(CCM_BASE_ADDR+0x64))

#define CGR0_SD_MMC1(cg)                ((cg) << 0*2)
#define CGR0_SD_MMC2(cg)                ((cg) << 1*2)
#define CGR0_GPT(cg)                    ((cg) << 2*2)
#define CGR0_EPIT1(cg)                  ((cg) << 3*2)
#define CGR0_EPIT2(cg)                  ((cg) << 4*2)
#define CGR0_IIM(cg)                    ((cg) << 5*2)
#define CGR0_ATA(cg)                    ((cg) << 6*2)
#define CGR0_SDMA(cg)                   ((cg) << 7*2)
#define CGR0_CSPI3(cg)                  ((cg) << 8*2)
#define CGR0_RNG(cg)                    ((cg) << 9*2)
#define CGR0_UART1(cg)                  ((cg) << 10*2)
#define CGR0_UART2(cg)                  ((cg) << 11*2)
#define CGR0_SSI1(cg)                   ((cg) << 12*2)
#define CGR0_I2C1(cg)                   ((cg) << 13*2)
#define CGR0_I2C2(cg)                   ((cg) << 14*2)
#define CGR0_I2C3(cg)                   ((cg) << 15*2)

#define CGR1_HANTRO(cg)                 ((cg) << 0*2)
#define CGR1_MEMSTICK1(cg)              ((cg) << 1*2)
#define CGR1_MEMSTICK2(cg)              ((cg) << 2*2)
#define CGR1_CSI(cg)                    ((cg) << 3*2)
#define CGR1_RTC(cg)                    ((cg) << 4*2)
#define CGR1_WDOG(cg)                   ((cg) << 5*2)
#define CGR1_PWM(cg)                    ((cg) << 6*2)
#define CGR1_SIM(cg)                    ((cg) << 7*2)
#define CGR1_ECT(cg)                    ((cg) << 8*2)
#define CGR1_USBOTG(cg)                 ((cg) << 9*2)
#define CGR1_KPP(cg)                    ((cg) << 10*2)
#define CGR1_IPU(cg)                    ((cg) << 11*2)
#define CGR1_UART3(cg)                  ((cg) << 12*2)
#define CGR1_UART4(cg)                  ((cg) << 13*2)
#define CGR1_UART5(cg)                  ((cg) << 14*2)
#define CGR1_1_WIRE(cg)                 ((cg) << 15*2)

#define CGR2_SSI2(cg)                   ((cg) << 0*2)
#define CGR2_CSPI1(cg)                  ((cg) << 1*2)
#define CGR2_CSPI2(cg)                  ((cg) << 2*2)
#define CGR2_GACC(cg)                   ((cg) << 3*2)
#define CGR2_EMI(cg)                    ((cg) << 4*2)
#define CGR2_RTIC(cg)                   ((cg) << 5*2)
#define CGR2_FIR(cg)                    ((cg) << 6*2)

#define WIM_GPIO3                       (1 << 0)
#define WIM_GPIO2                       (1 << 1)
#define WIM_GPIO1                       (1 << 2)
#define WIM_PCMCIA                      (1 << 3)
#define WIM_WDT                         (1 << 4)
#define WIM_USB_OTG                     (1 << 5)
#define WIM_IPI_INT_UH2                 (1 << 6)
#define WIM_IPI_INT_UH1                 (1 << 7)
#define WIM_IPI_INT_UART5_ANDED         (1 << 8)
#define WIM_IPI_INT_UART4_ANDED         (1 << 9)
#define WIM_IPI_INT_UART3_ANDED         (1 << 10)
#define WIM_IPI_INT_UART2_ANDED         (1 << 11)
#define WIM_IPI_INT_UART1_ANDED         (1 << 12)
#define WIM_IPI_INT_SIM_DATA_IRQ        (1 << 13)
#define WIM_IPI_INT_SDHC2               (1 << 14)
#define WIM_IPI_INT_SDHC1               (1 << 15)
#define WIM_IPI_INT_RTC                 (1 << 16)
#define WIM_IPI_INT_PWM                 (1 << 17)
#define WIM_IPI_INT_KPP                 (1 << 18)
#define WIM_IPI_INT_IIM                 (1 << 19)
#define WIM_IPI_INT_GPT                 (1 << 20)
#define WIM_IPI_INT_FIR                 (1 << 21)
#define WIM_IPI_INT_EPIT2               (1 << 22)
#define WIM_IPI_INT_EPIT1               (1 << 23)
#define WIM_IPI_INT_CSPI2               (1 << 24)
#define WIM_IPI_INT_CSPI1               (1 << 25)
#define WIM_IPI_INT_POWER_FAIL          (1 << 26)
#define WIM_IPI_INT_CSPI3               (1 << 27)
#define WIM_RESERVED28                  (1 << 28)
#define WIM_RESERVED29                  (1 << 29)
#define WIM_RESERVED30                  (1 << 30)
#define WIM_RESERVED31                  (1 << 31)

/* WEIM - CS0 */
#define CSCRU                           0x00
#define CSCRL                           0x04
#define CSCRA                           0x08

/* ESDCTL */
#define ESDCTL_ESDCTL0                  0x00
#define ESDCTL_ESDCFG0                  0x04
#define ESDCTL_ESDCTL1                  0x08
#define ESDCTL_ESDCFG1                  0x0C
#define ESDCTL_ESDMISC                  0x10

/* More UART 1 Register defines */
#define URXD1                           (*(REG32_PTR_T)(UART1_BASE_ADDR+0x00))
#define UTXD1                           (*(REG32_PTR_T)(UART1_BASE_ADDR+0x40))
#define UCR1_1                          (*(REG32_PTR_T)(UART1_BASE_ADDR+0x80))
#define UCR2_1                          (*(REG32_PTR_T)(UART1_BASE_ADDR+0x84))
#define UCR3_1                          (*(REG32_PTR_T)(UART1_BASE_ADDR+0x88))
#define UCR4_1                          (*(REG32_PTR_T)(UART1_BASE_ADDR+0x8C))
#define UFCR1                           (*(REG32_PTR_T)(UART1_BASE_ADDR+0x90))
#define USR1_1                          (*(REG32_PTR_T)(UART1_BASE_ADDR+0x94))
#define USR2_1                          (*(REG32_PTR_T)(UART1_BASE_ADDR+0x98))
#define UTS1                            (*(REG32_PTR_T)(UART1_BASE_ADDR+0xB4))

/*
 * UART Control Register 0 Bit Fields.
 */
#define EUARTUCR1_ADEN      (1 << 15)   // Auto detect interrupt
#define EUARTUCR1_ADBR      (1 << 14)   // Auto detect baud rate
#define EUARTUCR1_TRDYEN    (1 << 13)   // Transmitter ready interrupt enable
#define EUARTUCR1_IDEN      (1 << 12)   // Idle condition interrupt
#define EUARTUCR1_RRDYEN    (1 << 9)    // Recv ready interrupt enable
#define EUARTUCR1_RDMAEN    (1 << 8)    // Recv ready DMA enable
#define EUARTUCR1_IREN      (1 << 7)    // Infrared interface enable
#define EUARTUCR1_TXMPTYEN  (1 << 6)    // Transimitter empt  interrupt enable
#define EUARTUCR1_RTSDEN    (1 << 5)    // RTS delta interrupt enable
#define EUARTUCR1_SNDBRK    (1 << 4)    // Send break
#define EUARTUCR1_TDMAEN    (1 << 3)    // Transmitter ready DMA enable
#define EUARTUCR1_DOZE      (1 << 1)    // Doze
#define EUARTUCR1_UARTEN    (1 << 0)    // UART enabled
#define EUARTUCR2_ESCI      (1 << 15)   // Escape seq interrupt enable
#define EUARTUCR2_IRTS      (1 << 14)   // Ignore RTS pin
#define EUARTUCR2_CTSC      (1 << 13)   // CTS pin control
#define EUARTUCR2_CTS       (1 << 12)   // Clear to send
#define EUARTUCR2_ESCEN     (1 << 11)   // Escape enable
#define EUARTUCR2_PREN      (1 << 8)    // Parity enable
#define EUARTUCR2_PROE      (1 << 7)    // Parity odd/even
#define EUARTUCR2_STPB      (1 << 6)    // Stop
#define EUARTUCR2_WS        (1 << 5)    // Word size
#define EUARTUCR2_RTSEN     (1 << 4)    // Request to send interrupt enable
#define EUARTUCR2_ATEN      (1 << 3)    // Aging timer enable
#define EUARTUCR2_TXEN      (1 << 2)    // Transmitter enabled
#define EUARTUCR2_RXEN      (1 << 1)    // Receiver enabled
#define EUARTUCR2_SRST_     (1 << 0)    // SW reset
#define EUARTUCR3_PARERREN  (1 << 12)   // Parity enable
#define EUARTUCR3_FRAERREN  (1 << 11)   // Frame error interrupt enable
#define EUARTUCR3_ADNIMP    (1 << 7)    // Autobaud detection not improved
#define EUARTUCR3_RXDSEN    (1 << 6)    // Receive status interrupt  enable
#define EUARTUCR3_AIRINTEN  (1 << 5)    // Async IR wake interrupt enable
#define EUARTUCR3_AWAKEN    (1 << 4)    // Async wake interrupt enable
#define EUARTUCR3_RXDMUXSEL (1 << 2)    // RXD muxed input selected
#define EUARTUCR3_INVT      (1 << 1)    // Inverted Infrared transmission
#define EUARTUCR3_ACIEN     (1 << 0)    // Autobaud counter interrupt  enable
#define EUARTUCR4_CTSTL_32  (32 << 10)  // CTS trigger level (32 chars)
#define EUARTUCR4_INVR      (1 << 9)    // Inverted infrared reception
#define EUARTUCR4_ENIRI     (1 << 8)    // Serial infrared interrupt enable
#define EUARTUCR4_WKEN      (1 << 7)    // Wake interrupt enable
#define EUARTUCR4_IRSC      (1 << 5)    // IR special case
#define EUARTUCR4_LPBYP     (1 << 4)    // Low power bypass
#define EUARTUCR4_TCEN      (1 << 3)    // Transmit complete interrupt  enable
#define EUARTUCR4_BKEN      (1 << 2)    // Break condition interrupt enable
#define EUARTUCR4_OREN      (1 << 1)    // Receiver overrun interrupt enable
#define EUARTUCR4_DREN      (1 << 0)    // Recv data ready interrupt enable
#define EUARTUFCR_RXTL_SHF  0           // Receiver trigger level shift
#define EUARTUFCR_RFDIV_1   (5 << 7)    // Reference freq divider (div> 1)
#define EUARTUFCR_RFDIV_2   (4 << 7)    // Reference freq divider (div> 2)
#define EUARTUFCR_RFDIV_3   (3 << 7)    // Reference freq  divider (div 3)
#define EUARTUFCR_RFDIV_4   (2 << 7)    // Reference freq divider (div 4)
#define EUARTUFCR_RFDIV_5   (1 << 7)    // Reference freq divider (div 5)
#define EUARTUFCR_RFDIV_6   (0 << 7)    // Reference freq divider (div 6)
#define EUARTUFCR_RFDIV_7   (6 << 7)    // Reference freq divider (div 7)
#define EUARTUFCR_TXTL_SHF  10          // Transmitter trigger level shift
#define EUARTUSR1_PARITYERR (1 << 15)   // Parity error interrupt flag
#define EUARTUSR1_RTSS      (1 << 14)   // RTS pin status
#define EUARTUSR1_TRDY      (1 << 13)   // Transmitter ready interrupt/dma flag
#define EUARTUSR1_RTSD      (1 << 12)   // RTS delta
#define EUARTUSR1_ESCF      (1 << 11)   // Escape seq interrupt flag
#define EUARTUSR1_FRAMERR   (1 << 10)   // Frame error interrupt flag
#define EUARTUSR1_RRDY      (1 << 9)    // Receiver ready  interrupt/dma flag
#define EUARTUSR1_AGTIM     (1 << 8)    // Aging timeout interrupt status
#define EUARTUSR1_RXDS      (1 << 6)    // Receiver idle interrupt flag
#define EUARTUSR1_AIRINT    (1 << 5)    // Async IR wake interrupt flag
#define EUARTUSR1_AWAKE     (1 << 4)    // Aysnc wake interrupt flag
#define EUARTUSR2_ADET      (1 << 15)   // Auto baud rate detect  complete
#define EUARTUSR2_TXFE      (1 << 14)   // Transmit buffer FIFO empty
#define EUARTUSR2_IDLE      (1 << 12)   // Idle condition
#define EUARTUSR2_ACST      (1 << 11)   // Autobaud counter stopped
#define EUARTUSR2_IRINT     (1 << 8)    // Serial infrared interrupt flag
#define EUARTUSR2_WAKE      (1 << 7)    // Wake
#define EUARTUSR2_RTSF      (1 << 4)    // RTS edge interrupt flag
#define EUARTUSR2_TXDC      (1 << 3)    // Transmitter complete
#define EUARTUSR2_BRCD      (1 << 2)    // Break condition
#define EUARTUSR2_ORE       (1 << 1)    // Overrun error
#define EUARTUSR2_RDR       (1 << 0)    // Recv data ready
#define EUARTUTS_FRCPERR    (1 << 13)   // Force parity error
#define EUARTUTS_LOOP       (1 << 12)   // Loop tx and rx
#define EUARTUTS_TXEMPTY    (1 << 6)    // TxFIFO empty
#define EUARTUTS_RXEMPTY    (1 << 5)    // RxFIFO empty
#define EUARTUTS_TXFULL     (1 << 4)    // TxFIFO full
#define EUARTUTS_RXFULL     (1 << 3)    // RxFIFO full
#define EUARTUTS_SOFTRST    (1 << 0)    // Software reset

#define L2CC_ENABLED

/* Assuming 26MHz input clock */
/*                         PD          MFD           MFI          MFN */
#define MPCTL_PARAM_208  ((1 << 26) + (0   << 16) + (8  << 10) + (0 << 0))
#define MPCTL_PARAM_399  ((0 << 26) + (51  << 16) + (7  << 10) + (35 << 0))
#define MPCTL_PARAM_532  ((0 << 26) + (51  << 16) + (10 << 10) + (12 << 0))

/* UPCTL                   PD             MFD              MFI          MFN */
#define UPCTL_PARAM_288  (((1-1) << 26) + ((13-1) << 16) + (5  << 10) + (7  << 0))
#define UPCTL_PARAM_240  (((2-1) << 26) + ((13-1) << 16) + (9  << 10) + (3  << 0))

/* PDR0 */
#define PDR0_208_104_52     0xFF870D48  /* ARM=208MHz, HCLK=104MHz, IPG=52MHz */
#define PDR0_399_66_66      0xFF872B28  /* ARM=399MHz, HCLK=IPG=66.5MHz */
#define PDR0_399_133_66     0xFF871650  /* ARM=399MHz, HCLK=133MHz, IPG=66.5MHz */
#define PDR0_532_133_66     0xFF871E58  /* ARM=532MHz, HCLK=133MHz, IPG=66MHz */
#define PDR0_665_83_66      0xFF873D78  /* ARM=532MHz, HCLK=133MHz, IPG=66MHz */
#define PDR0_665_133_66     0xFF872660  /* ARM=532MHz, HCLK=133MHz, IPG=66MHz */

#define PBC_BASE            CS4_BASE_ADDR    /* Peripheral Bus Controller */

#define PBC_BSTAT2                  0x2
#define PBC_BCTRL1                  0x4
#define PBC_BCTRL1_CLR              0x6
#define PBC_BCTRL2                  0x8
#define PBC_BCTRL2_CLR              0xA
#define PBC_BCTRL3                  0xC
#define PBC_BCTRL3_CLR              0xE
#define PBC_BCTRL4                  0x10
#define PBC_BCTRL4_CLR              0x12
#define PBC_BSTAT1                  0x14
#define MX31EVB_CS_LAN_BASE         (CS4_BASE_ADDR + 0x00020000 +  0x300)
#define MX31EVB_CS_UART_BASE        (CS4_BASE_ADDR + 0x00010000)

#define REDBOOT_IMAGE_SIZE              0x40000
 
#define SDRAM_WORKAROUND_FULL_PAGE
 
#define ARMHIPG_208_52_52         /* ARM: 208MHz, HCLK=IPG=52MHz*/
#define ARMHIPG_52_52_52          /* ARM: 52MHz, HCLK=IPG=52MHz*/
#define ARMHIPG_399_66_66
#define ARMHIPG_399_133_66
 
/* MX31 EVB SDRAM is from 0x80000000, 64M */
#define SDRAM_BASE_ADDR                 CSD0_BASE_ADDR
#define SDRAM_SIZE                      0x04000000

#define UART_WIDTH_32         /* internal UART is 32bit access only */
#define EXT_UART_x16

#define UART_WIDTH_32         /* internal UART is 32bit access only */

#define FLASH_BURST_MODE_ENABLE 1
#define SDRAM_COMPARE_CONST1    0x55555555
#define SDRAM_COMPARE_CONST2    0xAAAAAAAA
#define UART_FIFO_CTRL          0x881
#define TIMEOUT                 1000
#define writel(v,a)             (*(REG32_PTR_T)(a) = (v))
#define readl(a)                (*(REG32_PTR_T)(a))
#define writew(v,a)             (*(REG16_PTR_T)(a) = (v))
#define readw(a)                (*(REG16_PTR_T)(a))

#define USB_BASE                OTG_BASE_ADDR

#endif /* __IMX31L_H__ */
