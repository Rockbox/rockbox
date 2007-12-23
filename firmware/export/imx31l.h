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

/* Most(if not all) of these defines are copied from Nand-Boot v4 provided w/ the Imx31 Linux Bsp*/

#define L2CC_BASE_ADDR          0x30000000
 
/*Frame Buffer and TTB defines from gigabeat f/x build*/
#define LCDSADDR1 (*(volatile int *)0x80100000) /* STN/TFT: frame buffer start address 1 */
#define FRAME1 ((short *)0x80100000) //Foreground FB 
#define FRAME2 ((short *)0x84100000) //Background FB - Set to Graphic Window, hence the reason why text is only visible
									 //when background memory is written.
#define LCD_BUFFER_SIZE ((320*240*2))
#define TTB_SIZE (0x4000)
#define TTB_BASE ((unsigned int *)(0x88000000 + (64*1024*1024)-TTB_SIZE))
/*
 * AIPS 1
 */
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

#define WDOG1_BASE_ADDR       WDOG_BASE_ADDR
#define CRM_MCU_BASE_ADDR     CCM_BASE_ADDR

/* ATA */
#define TIME_OFF                (*(volatile unsigned char*)0x43F8C000)
#define TIME_ON                 (*(volatile unsigned char*)0x43F8C001)
#define TIME_1                  (*(volatile unsigned char*)0x43F8C002)
#define TIME_2W                 (*(volatile unsigned char*)0x43F8C003)
#define TIME_2R                 (*(volatile unsigned char*)0x43F8C004)
#define TIME_AX                 (*(volatile unsigned char*)0x43F8C005)
#define TIME_PIO_RDX            (*(volatile unsigned char*)0x43F8C00F)
#define TIME_4                  (*(volatile unsigned char*)0x43F8C007)
#define TIME_9                  (*(volatile unsigned char*)0x43F8C008)

/* Timers */
#define EPITCR1                 (*(volatile long*)EPIT1_BASE_ADDR)
#define EPITSR1                 (*(volatile long*)(EPIT1_BASE_ADDR+0x04))
#define EPITLR1                 (*(volatile long*)(EPIT1_BASE_ADDR+0x08))
#define EPITCMPR1               (*(volatile long*)(EPIT1_BASE_ADDR+0x0C))
#define EPITCNT1                (*(volatile long*)(EPIT1_BASE_ADDR+0x10))
#define EPITCR2                 (*(volatile long*)EPIT2_BASE_ADDR)
#define EPITSR2                 (*(volatile long*)(EPIT2_BASE_ADDR+0x04))
#define EPITLR2                 (*(volatile long*)(EPIT2_BASE_ADDR+0x08))
#define EPITCMPR2               (*(volatile long*)(EPIT2_BASE_ADDR+0x0C))
#define EPITCNT2                (*(volatile long*)(EPIT2_BASE_ADDR+0x10))

/* GPIO */
#define GPIO1_DR                (*(volatile long*)GPIO1_BASE_ADDR)
#define GPIO1_GDIR              (*(volatile long*)(GPIO1_BASE_ADDR+0x04))
#define GPIO1_PSR               (*(volatile long*)(GPIO1_BASE_ADDR+0x08))
#define GPIO1_ICR1              (*(volatile long*)(GPIO1_BASE_ADDR+0x0C))
#define GPIO1_ICR2              (*(volatile long*)(GPIO1_BASE_ADDR+0x10))
#define GPIO1_IMR               (*(volatile long*)(GPIO1_BASE_ADDR+0x14))
#define GPIO1_ISR               (*(volatile long*)(GPIO1_BASE_ADDR+0x18))

#define GPIO2_DR                (*(volatile long*)GPIO2_BASE_ADDR)
#define GPIO2_GDIR              (*(volatile long*)(GPIO2_BASE_ADDR+0x04))
#define GPIO2_PSR               (*(volatile long*)(GPIO2_BASE_ADDR+0x08))
#define GPIO2_ICR1              (*(volatile long*)(GPIO2_BASE_ADDR+0x0C))
#define GPIO2_ICR2              (*(volatile long*)(GPIO2_BASE_ADDR+0x10))
#define GPIO2_IMR               (*(volatile long*)(GPIO2_BASE_ADDR+0x14))
#define GPIO2_ISR               (*(volatile long*)(GPIO2_BASE_ADDR+0x18))

#define GPIO3_DR                (*(volatile long*)GPIO3_BASE_ADDR)
#define GPIO3_GDIR              (*(volatile long*)(GPIO3_BASE_ADDR+0x04))
#define GPIO3_PSR               (*(volatile long*)(GPIO3_BASE_ADDR+0x08))
#define GPIO3_ICR1              (*(volatile long*)(GPIO3_BASE_ADDR+0x0C))
#define GPIO3_ICR2              (*(volatile long*)(GPIO3_BASE_ADDR+0x10))
#define GPIO3_IMR               (*(volatile long*)(GPIO3_BASE_ADDR+0x14))
#define GPIO3_ISR               (*(volatile long*)(GPIO3_BASE_ADDR+0x18))

/* SPI */
#define CSPI_RXDATA1            (*(volatile long*)CSPI1_BASE_ADDR)
#define CSPI_TXDATA1            (*(volatile long*)(CSPI1_BASE_ADDR+0x04))
#define CSPI_CONREG1            (*(volatile long*)(CSPI1_BASE_ADDR+0x08))
#define CSPI_INTREG1            (*(volatile long*)(CSPI1_BASE_ADDR+0x0C))
#define CSPI_DMAREG1            (*(volatile long*)(CSPI1_BASE_ADDR+0x10))
#define CSPI_STATREG1           (*(volatile long*)(CSPI1_BASE_ADDR+0x14))
#define CSPI_PERIODREG1         (*(volatile long*)(CSPI1_BASE_ADDR+0x18))
#define CSPI_TESTREG1           (*(volatile long*)(CSPI1_BASE_ADDR+0x1C0))

#define CSPI_RXDATA2            (*(volatile long*)CSPI2_BASE_ADDR)
#define CSPI_TXDATA2            (*(volatile long*)(CSPI2_BASE_ADDR+0x04))
#define CSPI_CONREG2            (*(volatile long*)(CSPI2_BASE_ADDR+0x08))
#define CSPI_INTREG2            (*(volatile long*)(CSPI2_BASE_ADDR+0x0C))
#define CSPI_DMAREG2            (*(volatile long*)(CSPI2_BASE_ADDR+0x10))
#define CSPI_STATREG2           (*(volatile long*)(CSPI2_BASE_ADDR+0x14))
#define CSPI_PERIODREG2         (*(volatile long*)(CSPI2_BASE_ADDR+0x18))
#define CSPI_TESTREG2           (*(volatile long*)(CSPI2_BASE_ADDR+0x1C0))

/* RTC */
#define RTC_HOURMIN             (*(volatile long*)RTC_BASE_ADDR)
#define RTC_SECONDS             (*(volatile long*)(RTC_BASE_ADDR+0x04))
#define RTC_ALRM_HM             (*(volatile long*)(RTC_BASE_ADDR+0x08))
#define RTC_ALRM_SEC            (*(volatile long*)(RTC_BASE_ADDR+0x0C))
#define RTC_CTL                 (*(volatile long*)(RTC_BASE_ADDR+0x10))
#define RTC_ISR                 (*(volatile long*)(RTC_BASE_ADDR+0x14))
#define RTC_IENR                (*(volatile long*)(RTC_BASE_ADDR+0x18))
#define RTC_STPWCH              (*(volatile long*)(RTC_BASE_ADDR+0x1C))
#define RTC_DAYR                (*(volatile long*)(RTC_BASE_ADDR+0x20))
#define RTC_DAYALARM            (*(volatile long*)(RTC_BASE_ADDR+0x24))

/* Keypad */
#define KPP_KPCR                (*(volatile short*)KPP_BASE_ADDR)
#define KPP_KPSR                (*(volatile short*)(KPP_BASE_ADDR+0x2))
#define KPP_KDDR                (*(volatile short*)(KPP_BASE_ADDR+0x4))
#define KPP_KPDR                (*(volatile short*)(KPP_BASE_ADDR+0x6))

/* ROMPATCH and AVIC */
#define ROMPATCH_BASE_ADDR      0x60000000
#define AVIC_BASE_ADDR          0x68000000

#define INTCNTL                 (*(volatile long*)AVIC_BASE_ADDR)
#define NIMASK                  (*(volatile long*)(AVIC_BASE_ADDR+0x004))
#define INTENNUM                (*(volatile long*)(AVIC_BASE_ADDR+0x008))
#define INTDISNUM               (*(volatile long*)(AVIC_BASE_ADDR+0x00C))
#define INTENABLEH              (*(volatile long*)(AVIC_BASE_ADDR+0x010))
#define INTENABLEL              (*(volatile long*)(AVIC_BASE_ADDR+0x014))
#define INTTYPEH                (*(volatile long*)(AVIC_BASE_ADDR+0x018))
#define INTTYPEL                (*(volatile long*)(AVIC_BASE_ADDR+0x01C))
#define NIPRIORITY7             (*(volatile long*)(AVIC_BASE_ADDR+0x020))
#define NIPRIORITY6             (*(volatile long*)(AVIC_BASE_ADDR+0x024))
#define NIPRIORITY5             (*(volatile long*)(AVIC_BASE_ADDR+0x028))
#define NIPRIORITY4             (*(volatile long*)(AVIC_BASE_ADDR+0x02C))
#define NIPRIORITY3             (*(volatile long*)(AVIC_BASE_ADDR+0x030))
#define NIPRIORITY2             (*(volatile long*)(AVIC_BASE_ADDR+0x034))
#define NIPRIORITY1             (*(volatile long*)(AVIC_BASE_ADDR+0x038))
#define NIPRIORITY0             (*(volatile long*)(AVIC_BASE_ADDR+0x03C))
#define NIVECSR                 (*(volatile long*)(AVIC_BASE_ADDR+0x040))
#define FIVECSR                 (*(volatile long*)(AVIC_BASE_ADDR+0x044))
#define INTSRCH                 (*(volatile long*)(AVIC_BASE_ADDR+0x048))
#define INTSRCL                 (*(volatile long*)(AVIC_BASE_ADDR+0x04C))
#define INTFRCH                 (*(volatile long*)(AVIC_BASE_ADDR+0x050))
#define INTFRCL                 (*(volatile long*)(AVIC_BASE_ADDR+0x054))
#define NIPNDH                  (*(volatile long*)(AVIC_BASE_ADDR+0x058))
#define NIPNDL                  (*(volatile long*)(AVIC_BASE_ADDR+0x05C))
#define FIPNDH                  (*(volatile long*)(AVIC_BASE_ADDR+0x060))
#define FIPNDL                  (*(volatile long*)(AVIC_BASE_ADDR+0x064))

/* The vectors go all the way up to 63. 4 bytes for each */
#define VECTOR_BASE_ADDR 	AVIC_BASE_ADDR+0x100
#define VECTOR0                 (*(volatile long*)VECTOR_BASE_ADDR)

#define NIDIS                   (1 << 22)
#define FIDIS                   (1 << 21)
 
/* Since AVIC vector registers are NOT used, we reserve some for various
 * purposes. Copied from Linux source code. */
#define AVIC_VEC_0              0x100   /* For WFI workaround used by Linux kernel */
#define AVIC_VEC_1              0x104   /* For system revision used by Linux kernel */
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
#define WEIM_CTRL_CS0           WEIM_BASE_ADDR
#define WEIM_CTRL_CS1           (WEIM_BASE_ADDR + 0x10)
#define WEIM_CTRL_CS2           (WEIM_BASE_ADDR + 0x20)
#define WEIM_CTRL_CS3           (WEIM_BASE_ADDR + 0x30)
#define WEIM_CTRL_CS4           (WEIM_BASE_ADDR + 0x40)
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

// SDRAM
#define RAM_BANK0_BASE          SDRAM_BASE_ADDR

/*
 * IRQ Controller Register Definitions.
 */
#define AVIC_NIMASK                     REG32_PTR(AVIC_BASE_ADDR + (0x04))
#define AVIC_INTTYPEH                   REG32_PTR(AVIC_BASE_ADDR + (0x18))
#define AVIC_INTTYPEL                   REG32_PTR(AVIC_BASE_ADDR + (0x1C))

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

/* CCM */
#define CLKCTL_CCMR                     (*(volatile long*)(CCM_BASE_ADDR+0x00))
#define CLKCTL_PDR0                     (*(volatile long*)(CCM_BASE_ADDR+0x04))
#define CLKCTL_PDR1                     (*(volatile long*)(CCM_BASE_ADDR+0x08))
#define CLKCTL_PDR2                     (*(volatile long*)(CCM_BASE_ADDR+0x64))
#define CLKCTL_RCSR                     (*(volatile long*)(CCM_BASE_ADDR+0x0C))
#define CLKCTL_MPCTL                    (*(volatile long*)(CCM_BASE_ADDR+0x10))
#define CLKCTL_UPCTL                    (*(volatile long*)(CCM_BASE_ADDR+0x14))
#define CLKCTL_SPCTL                    (*(volatile long*)(CCM_BASE_ADDR+0x18))
#define CLKCTL_COSR                     (*(volatile long*)(CCM_BASE_ADDR+0x1C))
#define CLKCTL_PMCR0                    (*(volatile long*)(CCM_BASE_ADDR+0x5C))
#define PLL_REF_CLK                     26000000

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
#define URXD1 (*(volatile int*)UART1_BASE_ADDR)
#define UTXD1 (*(volatile int*)(UART1_BASE_ADDR+0x40))
#define UCR1_1 (*(volatile int *)(UART1_BASE_ADDR+0x80))
#define UCR2_1 (*(volatile int* )(UART1_BASE_ADDR+0x84))
#define UCR3_1 (*(volatile int* )(UART1_BASE_ADDR+0x88))
#define UCR4_1 (*(volatile int* )(UART1_BASE_ADDR+0x8C))
#define UFCR1 (*(volatile int *)(UART1_BASE_ADDR+ 0x90))
#define USR1_1 (*(volatile int *)(UART1_BASE_ADDR+0x94))
#define USR2_1 (*(volatile int *)(UART1_BASE_ADDR+0x98))
#define UTS1    (*(volatile int *)(UART1_BASE_ADDR+0xB4))

/*
 * UART Control Register 0 Bit Fields.
 */
#define         EUartUCR1_ADEN      (1 << 15)   // Auto detect interrupt
#define         EUartUCR1_ADBR      (1 << 14)   // Auto detect baud rate
#define         EUartUCR1_TRDYEN    (1 << 13)   // Transmitter ready interrupt enable
#define         EUartUCR1_IDEN      (1 << 12)   // Idle condition interrupt
#define         EUartUCR1_RRDYEN    (1 << 9)    // Recv ready interrupt enable
#define         EUartUCR1_RDMAEN    (1 << 8)    // Recv ready DMA enable
#define         EUartUCR1_IREN      (1 << 7)    // Infrared interface enable
#define         EUartUCR1_TXMPTYEN  (1 << 6)    // Transimitter empt  interrupt enable
#define         EUartUCR1_RTSDEN    (1 << 5)    // RTS delta interrupt enable
#define         EUartUCR1_SNDBRK    (1 << 4)    // Send break
#define         EUartUCR1_TDMAEN    (1 << 3)    // Transmitter ready DMA enable
#define         EUartUCR1_DOZE      (1 << 1)    // Doze
#define         EUartUCR1_UARTEN    (1 << 0)    // UART enabled
#define         EUartUCR2_ESCI      (1 << 15)   // Escape seq interrupt enable
#define         EUartUCR2_IRTS      (1 << 14)   // Ignore RTS pin
#define         EUartUCR2_CTSC      (1 << 13)   // CTS pin control
#define         EUartUCR2_CTS       (1 << 12)   // Clear to send
#define         EUartUCR2_ESCEN     (1 << 11)   // Escape enable
#define         EUartUCR2_PREN      (1 << 8)    // Parity enable
#define         EUartUCR2_PROE      (1 << 7)    // Parity odd/even
#define         EUartUCR2_STPB      (1 << 6)    // Stop
#define         EUartUCR2_WS        (1 << 5)    // Word size
#define         EUartUCR2_RTSEN     (1 << 4)    // Request to send interrupt enable
#define         EUartUCR2_ATEN      (1 << 3)    // Aging timer enable
#define         EUartUCR2_TXEN      (1 << 2)    // Transmitter enabled
#define         EUartUCR2_RXEN      (1 << 1)    // Receiver enabled
#define         EUartUCR2_SRST_     (1 << 0)    // SW reset
#define         EUartUCR3_PARERREN  (1 << 12)   // Parity enable
#define         EUartUCR3_FRAERREN  (1 << 11)   // Frame error interrupt enable
#define         EUartUCR3_ADNIMP    (1 << 7)    // Autobaud detection not improved
#define         EUartUCR3_RXDSEN    (1 << 6)    // Receive status interrupt  enable
#define         EUartUCR3_AIRINTEN  (1 << 5)    // Async IR wake interrupt enable
#define         EUartUCR3_AWAKEN    (1 << 4)    // Async wake interrupt enable
#define         EUartUCR3_RXDMUXSEL (1 << 2)    // RXD muxed input selected
#define         EUartUCR3_INVT      (1 << 1)    // Inverted Infrared transmission
#define         EUartUCR3_ACIEN     (1 << 0)    // Autobaud counter interrupt  enable
#define         EUartUCR4_CTSTL_32  (32 << 10)  // CTS trigger level (32 chars)
#define         EUartUCR4_INVR      (1 << 9)    // Inverted infrared reception
#define         EUartUCR4_ENIRI     (1 << 8)    // Serial infrared interrupt enable
#define         EUartUCR4_WKEN      (1 << 7)    // Wake interrupt enable
#define         EUartUCR4_IRSC      (1 << 5)    // IR special case
#define         EUartUCR4_LPBYP     (1 << 4)    // Low power bypass
#define         EUartUCR4_TCEN      (1 << 3)    // Transmit complete interrupt  enable
#define         EUartUCR4_BKEN      (1 << 2)    // Break condition interrupt enable
#define         EUartUCR4_OREN      (1 << 1)    // Receiver overrun interrupt enable
#define         EUartUCR4_DREN      (1 << 0)    // Recv data ready interrupt enable
#define         EUartUFCR_RXTL_SHF  0           // Receiver trigger level shift
#define         EUartUFCR_RFDIV_1   (5 << 7)    // Reference freq divider (div> 1)
#define         EUartUFCR_RFDIV_2   (4 << 7)    // Reference freq divider (div> 2)
#define         EUartUFCR_RFDIV_3   (3 << 7)            // Reference freq  divider (div 3)
#define         EUartUFCR_RFDIV_4   (2 << 7)            // Reference freq divider (div 4)
#define         EUartUFCR_RFDIV_5   (1 << 7)            // Reference freq divider (div 5)
#define         EUartUFCR_RFDIV_6   (0 << 7)            // Reference freq divider (div 6)
#define         EUartUFCR_RFDIV_7   (6 << 7)            // Reference freq divider (div 7)
#define         EUartUFCR_TXTL_SHF  10          // Transmitter trigger level shift
#define         EUartUSR1_PARITYERR (1 << 15)   // Parity error interrupt flag
#define         EUartUSR1_RTSS      (1 << 14)   // RTS pin status
#define         EUartUSR1_TRDY      (1 << 13)   // Transmitter ready interrupt/dma flag
#define         EUartUSR1_RTSD      (1 << 12)   // RTS delta
#define         EUartUSR1_ESCF      (1 << 11)   // Escape seq interrupt flag
#define         EUartUSR1_FRAMERR   (1 << 10)   // Frame error interrupt flag
#define         EUartUSR1_RRDY      (1 << 9)    // Receiver ready  interrupt/dma flag
#define         EUartUSR1_AGTIM     (1 << 8)    // Aging timeout interrupt status
#define         EUartUSR1_RXDS      (1 << 6)    // Receiver idle interrupt flag
#define         EUartUSR1_AIRINT    (1 << 5)    // Async IR wake interrupt flag
#define         EUartUSR1_AWAKE     (1 << 4)    // Aysnc wake interrupt flag
#define         EUartUSR2_ADET      (1 << 15)   // Auto baud rate detect  complete
#define         EUartUSR2_TXFE      (1 << 14)   // Transmit buffer FIFO empty
#define         EUartUSR2_IDLE      (1 << 12)   // Idle condition
#define         EUartUSR2_ACST      (1 << 11)   // Autobaud counter stopped
#define         EUartUSR2_IRINT     (1 << 8)    // Serial infrared interrupt flag
#define         EUartUSR2_WAKE      (1 << 7)    // Wake
#define         EUartUSR2_RTSF      (1 << 4)    // RTS edge interrupt flag
#define         EUartUSR2_TXDC      (1 << 3)    // Transmitter complete
#define         EUartUSR2_BRCD      (1 << 2)    // Break condition
#define         EUartUSR2_ORE       (1 << 1)    // Overrun error
#define         EUartUSR2_RDR       (1 << 0)    // Recv data ready
#define         EUartUTS_FRCPERR    (1 << 13)   // Force parity error
#define         EUartUTS_LOOP       (1 << 12)   // Loop tx and rx
#define         EUartUTS_TXEMPTY    (1 << 6)    // TxFIFO empty
#define         EUartUTS_RXEMPTY    (1 << 5)    // RxFIFO empty
#define         EUartUTS_TXFULL     (1 << 4)    // TxFIFO full
#define         EUartUTS_RXFULL     (1 << 3)    // RxFIFO full
#define         EUartUTS_SOFTRST    (1 << 0)    // Software reset

#define         DelayTimerPresVal   3

#define         L2CC_ENABLED

/* Assuming 26MHz input clock */
/*                         PD          MFD           MFI          MFN */
#define         MPCTL_PARAM_208  ((1 << 26) + (0   << 16) + (8  << 10) + (0 << 0))
#define         MPCTL_PARAM_399  ((0 << 26) + (51  << 16) + (7  << 10) + (35 << 0))
#define         MPCTL_PARAM_532  ((0 << 26) + (51  << 16) + (10 << 10) + (12 << 0))

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

#define PBC_BSTAT2                   0x2
#define PBC_BCTRL1                   0x4
#define PBC_BCTRL1_CLR               0x6
#define PBC_BCTRL2                   0x8
#define PBC_BCTRL2_CLR               0xA
#define PBC_BCTRL3                   0xC
#define PBC_BCTRL3_CLR               0xE
#define PBC_BCTRL4                   0x10
#define PBC_BCTRL4_CLR               0x12
#define PBC_BSTAT1                   0x14
#define MX31EVB_CS_LAN_BASE        (CS4_BASE_ADDR + 0x00020000 +  0x300)
#define MX31EVB_CS_UART_BASE       (CS4_BASE_ADDR + 0x00010000)

#define         REDBOOT_IMAGE_SIZE              0x40000
 
#define         SDRAM_WORKAROUND_FULL_PAGE
 
#define       ARMHIPG_208_52_52         /* ARM: 208MHz, HCLK=IPG=52MHz*/
#define       ARMHIPG_52_52_52          /* ARM: 52MHz, HCLK=IPG=52MHz*/
#define       ARMHIPG_399_66_66
#define         ARMHIPG_399_133_66
 
/* MX31 EVB SDRAM is from 0x80000000, 64M */
#define         SDRAM_BASE_ADDR                 CSD0_BASE_ADDR
#define         SDRAM_SIZE                      0x04000000

#define         UART_WIDTH_32         /* internal UART is 32bit access only */
#define         EXT_UART_x16

#define         UART_WIDTH_32         /* internal UART is 32bit access only */

#define         FLASH_BURST_MODE_ENABLE 1
#define         SDRAM_COMPARE_CONST1    0x55555555
#define         SDRAM_COMPARE_CONST2    0xAAAAAAAA
#define         UART_FIFO_CTRL          0x881
#define         TIMEOUT                 1000
#define         writel(v,a)         (*(volatile int *) (a) = (v))
#define         readl(a)            (*(volatile int *)(a))
#define         writew(v,a)         (*(volatile short *) (a) = (v))
#define         readw(a)            (*(volatile short *)(a))
