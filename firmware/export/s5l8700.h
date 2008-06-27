/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: S5L8700X.h 2008-03-27 Marcoen Hirschberg, Bart van Adrichem $
 *
 * Copyright (C) 2008 by Marcoen Hirschberg, Bart van Adrichem
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#define REG8_PTR_T  volatile uint8_t *
#define REG16_PTR_T volatile uint16_t *
#define REG32_PTR_T volatile uint32_t *

/* 04. CALMADM2E */

/* Following registers are mapped on IO Area in data memory area of Calm. */
#define CONFIG0                 (*(REG16_PTR_T)(0x3F000000))     /* configuration/control register 0 */
#define CONFIG1                 (*(REG16_PTR_T)(0x3F000002))     /* configuration/control register 1*/
#define COMMUN                  (*(REG16_PTR_T)(0x3F000004))     /* Communication Control Register */
#define DDATA0                  (*(REG16_PTR_T)(0x3F000006))     /* Communication data from host to ADM */
#define DDATA1                  (*(REG16_PTR_T)(0x3F000008))     /* Communication data from host to ADM */
#define DDATA2                  (*(REG16_PTR_T)(0x3F00000A))     /* Communication data from host to ADM */
#define DDATA3                  (*(REG16_PTR_T)(0x3F00000C))     /* Communication data from host to ADM */
#define DDATA4                  (*(REG16_PTR_T)(0x3F00000E))     /* Communication data from host to ADM */
#define DDATA5                  (*(REG16_PTR_T)(0x3F000010))     /* Communication data from host to ADM */
#define DDATA6                  (*(REG16_PTR_T)(0x3F000012))     /* Communication data from host to ADM */
#define DDATA7                  (*(REG16_PTR_T)(0x3F000014))     /* Communication data from host to ADM */
#define UDATA0                  (*(REG16_PTR_T)(0x3F000016))     /* Communication data from ADM to host */
#define UDATA1                  (*(REG16_PTR_T)(0x3F000018))     /* Communication data from ADM to host */
#define UDATA2                  (*(REG16_PTR_T)(0x3F00001A))     /* Communication data from ADM to host */
#define UDATA3                  (*(REG16_PTR_T)(0x3F00001C))     /* Communication data from ADM to host */
#define UDATA4                  (*(REG16_PTR_T)(0x3F00001E))     /* Communication data from ADM to host */
#define UDATA5                  (*(REG16_PTR_T)(0x3F000020))     /* Communication data from ADM to host */
#define UDATA6                  (*(REG16_PTR_T)(0x3F000022))     /* Communication data from ADM to host */
#define UDATA7                  (*(REG16_PTR_T)(0x3F000024))     /* Communication data from ADM to host */
#define IBASE_H                 (*(REG16_PTR_T)(0x3F000026))     /* Higher half of start address for ADM instruction area */
#define IBASE_L                 (*(REG16_PTR_T)(0x3F000028))     /* Lower half of start address for ADM instruction area */
#define DBASE_H                 (*(REG16_PTR_T)(0x3F00002A))     /* Higher half of start address for CalmRISC data area */
#define DBASE_L                 (*(REG16_PTR_T)(0x3F00002C))     /* Lower half of start address for CalmRISC data area */
#define XBASE_H                 (*(REG16_PTR_T)(0x3F00002E))     /* Higher half of start address for Mac X area */
#define XBASE_L                 (*(REG16_PTR_T)(0x3F000030))     /* Lower half of start address for Mac X area */
#define YBASE_H                 (*(REG16_PTR_T)(0x3F000032))     /* Higher half of start address for Mac Y area */
#define YBASE_L                 (*(REG16_PTR_T)(0x3F000034))     /* Lower half of start address for Mac Y area */
#define S0BASE_H                (*(REG16_PTR_T)(0x3F000036))     /* Higher half of start address for sequential buffer 0 area */
#define S0BASE_L                (*(REG16_PTR_T)(0x3F000038))     /* Lower half of start address for sequential buffer 0 area */
#define S1BASE_H                (*(REG16_PTR_T)(0x3F00003A))     /* Higher half of start address for sequential buffer 1 area */
#define S1BASE_L                (*(REG16_PTR_T)(0x3F00003C))     /* Lower half of start address for sequential buffer 1 area */
#define CACHECON                (*(REG16_PTR_T)(0x3F00003E))     /* Cache Control Register */
#define CACHESTAT               (*(REG16_PTR_T)(0x3F000040))     /* Cache status register */
#define SBFCON                  (*(REG16_PTR_T)(0x3F000042))     /* Sequential Buffer Control Register */
#define SBFSTAT                 (*(REG16_PTR_T)(0x3F000044))     /* Sequential Buffer Status Register */
#define SBL0OFF_H               (*(REG16_PTR_T)(0x3F000046))     /* Higher bits of Offset register of sequential block 0 area */
#define SBL0OFF_L               (*(REG16_PTR_T)(0x3F000048))     /* Lower bits of Offset register of sequential block 0 area */
#define SBL1OFF_H               (*(REG16_PTR_T)(0x3F00004A))     /* Higher bits of Offset register of sequential block 1 area */
#define SBL1OFF_L               (*(REG16_PTR_T)(0x3F00004C))     /* Lower bits of Offset register of sequential block 1 area */
#define SBL0BEGIN_H             (*(REG16_PTR_T)(0x3F00004E))     /* Higher bits of Begin Offset of sequential block 0 area in ring mode */
#define SBL0BEGIN_L             (*(REG16_PTR_T)(0x3F000050))     /* Lower bits of Begin Offset of sequential block 0 area in ring mode */
#define SBL1BEGIN_H             (*(REG16_PTR_T)(0x3F000052))     /* Higher bits of Begin Offset of sequential block 1 area in ring mode */
#define SBL1BEGIN_L             (*(REG16_PTR_T)(0x3F000054))     /* Lower bits of Begin Offset of sequential block 1 area in ring mode */
#define SBL0END_H               (*(REG16_PTR_T)(0x3F000056))     /* Lower bits of End Offset of sequential block 0 area in ring mode */
#define SBL0END_L               (*(REG16_PTR_T)(0x3F000058))     /* Higher bits of End Offset of sequential block 0 area in ring mode */
#define SBL1END_H               (*(REG16_PTR_T)(0x3F00005A))     /* Lower bits of End Offset of sequential block 1 area in ring mode */
#define SBL1END_L               (*(REG16_PTR_T)(0x3F00005C))     /* Higher bits of End Offset of sequential block 1 area in ring mode */

/* Following registers are components of SFRS of the target system */
#define ADM_CONFIG              (*(REG32_PTR_T)(0x39000000))     /* Configuration/Control Register */
#define ADM_COMMUN              (*(REG32_PTR_T)(0x39000004))     /* Communication Control Register */
#define ADM_DDATA0              (*(REG32_PTR_T)(0x39000010))     /* Communication data from host to ADM */
#define ADM_DDATA1              (*(REG32_PTR_T)(0x39000014))     /* Communication data from host to ADM */
#define ADM_DDATA2              (*(REG32_PTR_T)(0x39000018))     /* Communication data from host to ADM */
#define ADM_DDATA3              (*(REG32_PTR_T)(0x3900001C))     /* Communication data from host to ADM */
#define ADM_DDATA4              (*(REG32_PTR_T)(0x39000020))     /* Communication data from host to ADM */
#define ADM_DDATA5              (*(REG32_PTR_T)(0x39000024))     /* Communication data from host to ADM */
#define ADM_DDATA6              (*(REG32_PTR_T)(0x39000028))     /* Communication data from host to ADM */
#define ADM_DDATA7              (*(REG32_PTR_T)(0x3900002C))     /* Communication data from host to ADM */
#define ADM_UDATA0              (*(REG32_PTR_T)(0x39000030))     /* Communication data from ADM to host */
#define ADM_UDATA1              (*(REG32_PTR_T)(0x39000034))     /* Communication data from ADM to host */
#define ADM_UDATA2              (*(REG32_PTR_T)(0x39000038))     /* Communication data from ADM to host */
#define ADM_UDATA3              (*(REG32_PTR_T)(0x3900003C))     /* Communication data from ADM to host */
#define ADM_UDATA4              (*(REG32_PTR_T)(0x39000040))     /* Communication data from ADM to host */
#define ADM_UDATA5              (*(REG32_PTR_T)(0x39000044))     /* Communication data from ADM to host */
#define ADM_UDATA6              (*(REG32_PTR_T)(0x39000048))     /* Communication data from ADM to host */
#define ADM_UDATA7              (*(REG32_PTR_T)(0x3900004C))     /* Communication data from ADM to host */
#define ADM_IBASE               (*(REG32_PTR_T)(0x39000050))     /* Start Address for ADM Instruction Area */
#define ADM_DBASE               (*(REG32_PTR_T)(0x39000054))     /* Start Address for CalmRISC Data Area */
#define ADM_XBASE               (*(REG32_PTR_T)(0x39000058))     /* Start Address for Mac X Area */
#define ADM_YBASE               (*(REG32_PTR_T)(0x3900005C))     /* Start Address for Mac Y Area */
#define ADM_S0BASE              (*(REG32_PTR_T)(0x39000060))     /* Start Address for Sequential Block 0 Area */
#define ADM_S1BASE              (*(REG32_PTR_T)(0x39000064))     /* Start Address for Sequential Block 1 Area */

/* 05. CLOCK & POWER MANAGEMENT */
#define CLKCON                  (*(REG32_PTR_T)(0x3C500000))     /* Clock control Register */
#define PLL0PMS                 (*(REG32_PTR_T)(0x3C500004))     /* PLL PMS value Register */
#define PLL1PMS                 (*(REG32_PTR_T)(0x3C500008))     /* PLL PMS value Register */
#define PLL0LCNT                (*(REG32_PTR_T)(0x3C500014))     /* PLL0 lock count register */
#define PLL1LCNT                (*(REG32_PTR_T)(0x3C500018))     /* PLL1 lock count register */
#define PLLLOCK                 (*(REG32_PTR_T)(0x3C500020))     /* PLL lock status register */
#define PLLCON                  (*(REG32_PTR_T)(0x3C500024))     /* PLL control register */
#define PWRCON                  (*(REG32_PTR_T)(0x3C500028))     /* Clock power control register */
#define PWRMODE                 (*(REG32_PTR_T)(0x3C50002C))     /* Power mode control register */
#define SWRCON                  (*(REG32_PTR_T)(0x3C500030))     /* Software reset control register */
#define RSTSR                   (*(REG32_PTR_T)(0x3C500034))     /* Reset status register */
#define DSPCLKMD                (*(REG32_PTR_T)(0x3C500038))     /* DSP clock mode register */
#define CLKCON2                 (*(REG32_PTR_T)(0x3C50003C))     /* clock control register 2 */

/* 06. INTERRUPT CONTROLLER UNIT */
#define SRCPND                  (*(REG32_PTR_T)(0x39C00000))     /* Indicates the interrupt request status. */
#define INTMOD                  (*(REG32_PTR_T)(0x39C00004))     /* Interrupt mode register. */
#define INTMSK                  (*(REG32_PTR_T)(0x39C00008))     /* Determines which interrupt source is masked. The */
#define PRIORITY                (*(REG32_PTR_T)(0x39C0000C))     /* IRQ priority control register */
#define INTPND                  (*(REG32_PTR_T)(0x39C00010))     /* Indicates the interrupt request status. */
#define INTOFFSET               (*(REG32_PTR_T)(0x39C00014))     /* Indicates the IRQ interrupt request source */
#define EINTPOL                 (*(REG32_PTR_T)(0x39C00018))     /* Indicates external interrupt polarity */
#define EINTPEND                (*(REG32_PTR_T)(0x39C0001C))     /* Indicates whether external interrupts are pending. */
#define EINTMSK                 (*(REG32_PTR_T)(0x39C00020))     /* Indicates whether external interrupts are masked */

/* 07. MEMORY INTERFACE UNIT (MIU) */

/* SDRAM */
#define MIUCON                  (*(REG32_PTR_T)(0x38200000))     /* External Memory configuration register */
#define MIUCOM                  (*(REG32_PTR_T)(0x38200004))     /* Command and status register */
#define MIUAREF                 (*(REG32_PTR_T)(0x38200008))     /* Auto-refresh control register */
#define MIUMRS                  (*(REG32_PTR_T)(0x3820000C))     /* SDRAM Mode Register Set Value Register */
#define MIUSDPARA               (*(REG32_PTR_T)(0x38200010))     /* SDRAM parameter register */

/* DDR */
#define MEMCONF                 (*(REG32_PTR_T)(0x38200020))     /* External Memory configuration register */
#define USRCMD                  (*(REG32_PTR_T)(0x38200024))     /* Command and Status register */
#define AREF                    (*(REG32_PTR_T)(0x38200028))     /* Auto-refresh control register */
#define MRS                     (*(REG32_PTR_T)(0x3820002C))     /* DRAM mode register set value register */
#define DPARAM                  (*(REG32_PTR_T)(0x38200030))     /* DRAM parameter register (Unit of ‘tXXX’ : tCK */
#define SMEMCONF                (*(REG32_PTR_T)(0x38200034))     /* Static memory mode register set value register */
#define MIUS01PARA              (*(REG32_PTR_T)(0x38200038))     /* SRAM0, SRAM1 static memory parameter register (In S5L8700, SRAM0 is Nor Flash) */
#define MIUS23PARA              (*(REG32_PTR_T)(0x3820003C))     /* SRAM2 and SRAM3 static memory parameter register */

#define MIUORG                  (*(REG32_PTR_T)(0x38200040))     /* SDR/DDR selection */
#define MIUDLYDQS               (*(REG32_PTR_T)(0x38200044))     /* DQS/DQS-rst delay parameter */
#define MIUDLYCLK               (*(REG32_PTR_T)(0x38200048))     /* SDR/DDR Clock delay parameter */
#define MIU_DSS_SEL_B           (*(REG32_PTR_T)(0x3820004C))     /* SSTL2 Drive Strength parameter for Bi-direction signal */
#define MIU_DSS_SEL_O           (*(REG32_PTR_T)(0x38200050))     /* SSTL2 Drive Strength parameter for Output signal */
#define MIU_DSS_SEL_C           (*(REG32_PTR_T)(0x38200054))     /* SSTL2 Drive Strength parameter for Clock signal */
#define PAD_DSS_SEL_NOR         (*(REG32_PTR_T)(0x38200058))     /* Wide range I/O Drive Strength parameter for NOR interface */
#define PAD_DSS_SEL_ATA         (*(REG32_PTR_T)(0x3820005C))     /* Wide range I/O Drive Strength parameter for ATA interface */
#define SSTL2_PAD_ON            (*(REG32_PTR_T)(0x38200060))     /* SSTL2 pad ON/OFF select */

/* 08. IODMA CONTROLLER */
#define DMABASE0                (*(REG32_PTR_T)(0x38400000))     /* Base address register for channel 0 */
#define DMACON0                 (*(REG32_PTR_T)(0x38400004))     /* Configuration register for channel 0 */
#define DMATCNT0                (*(REG32_PTR_T)(0x38400008))     /* Transfer count register for channel 0 */
#define DMACADDR0               (*(REG32_PTR_T)(0x3840000C))     /* Current memory address register for channel 0 */
#define DMACTCNT0               (*(REG32_PTR_T)(0x38400010))     /* Current transfer count register for channel 0 */
#define DMACOM0                 (*(REG32_PTR_T)(0x38400014))     /* Channel 0 command register */
#define DMANOFF0                (*(REG32_PTR_T)(0x38400018))     /* Channel 0 offset2 register */
#define DMABASE1                (*(REG32_PTR_T)(0x38400020))     /* Base address register for channel 1 */
#define DMACON1                 (*(REG32_PTR_T)(0x38400024))     /* Configuration register for channel 1 */
#define DMATCNT1                (*(REG32_PTR_T)(0x38400028))     /* Transfer count register for channel 1 */
#define DMACADDR1               (*(REG32_PTR_T)(0x3840002C))     /* Current memory address register for channel 1 */
#define DMACTCNT1               (*(REG32_PTR_T)(0x38400030))     /* Current transfer count register for channel 1 */
#define DMACOM1                 (*(REG32_PTR_T)(0x38400034))     /* Channel 1 command register */
#define DMABASE2                (*(REG32_PTR_T)(0x38400040))     /* Base address register for channel 2 */
#define DMACON2                 (*(REG32_PTR_T)(0x38400044))     /* Configuration register for channel 2 */
#define DMATCNT2                (*(REG32_PTR_T)(0x38400048))     /* Transfer count register for channel 2 */
#define DMACADDR2               (*(REG32_PTR_T)(0x3840004C))     /* Current memory address register for channel 2 */
#define DMACTCNT2               (*(REG32_PTR_T)(0x38400050))     /* Current transfer count register for channel 2 */
#define DMACOM2                 (*(REG32_PTR_T)(0x38400054))     /* Channel 2 command register */
#define DMABASE3                (*(REG32_PTR_T)(0x38400060))     /* Base address register for channel 3 */
#define DMACON3                 (*(REG32_PTR_T)(0x38400064))     /* Configuration register for channel 3 */
#define DMATCNT3                (*(REG32_PTR_T)(0x38400068))     /* Transfer count register for channel 3 */
#define DMACADDR3               (*(REG32_PTR_T)(0x3840006C))     /* Current memory address register for channel 3 */
#define DMACTCNT3               (*(REG32_PTR_T)(0x38400070))     /* Current transfer count register for channel 3 */
#define DMACOM3                 (*(REG32_PTR_T)(0x38400074))     /* Channel 3 command register */
#define DMAALLST                (*(REG32_PTR_T)(0x38400100))     /* All channel status register */

/* 10. REAL TIMER CLOCK (RTC) */
#define RTCCON                  (*(REG32_PTR_T)(0x3D200000))     /* RTC Control Register */
#define RTCRST                  (*(REG32_PTR_T)(0x3D200004))     /* RTC Round Reset Register */
#define RTCALM                  (*(REG32_PTR_T)(0x3D200008))     /* RTC Alarm Control Register */
#define ALMSEC                  (*(REG32_PTR_T)(0x3D20000C))     /* Alarm Second Data Register */
#define ALMMIN                  (*(REG32_PTR_T)(0x3D200010))     /* Alarm Minute Data Register */
#define ALMHOUR                 (*(REG32_PTR_T)(0x3D200014))     /* Alarm Hour Data Register */
#define ALMDATE                 (*(REG32_PTR_T)(0x3D200018))     /* Alarm Date Data Register */
#define ALMDAY                  (*(REG32_PTR_T)(0x3D20001C))     /* Alarm Day of Week Data Register */
#define ALMMON                  (*(REG32_PTR_T)(0x3D200020))     /* Alarm Month Data Register */
#define ALMYEAR                 (*(REG32_PTR_T)(0x3D200024))     /* Alarm Year Data Register */
#define BCDSEC                  (*(REG32_PTR_T)(0x3D200028))     /* BCD Second Register */
#define BCDMIN                  (*(REG32_PTR_T)(0x3D20002C))     /* BCD Minute Register */
#define BCDHOUR                 (*(REG32_PTR_T)(0x3D200030))     /* BCD Hour Register */
#define BCDDATE                 (*(REG32_PTR_T)(0x3D200034))     /* BCD Date Register */
#define BCDDAY                  (*(REG32_PTR_T)(0x3D200038))     /* BCD Day of Week Register */
#define BCDMON                  (*(REG32_PTR_T)(0x3D20003C))     /* BCD Month Register */
#define BCDYEAR                 (*(REG32_PTR_T)(0x3D200040))     /* BCD Year Register */
#define RTCIM                   (*(REG32_PTR_T)(0x3D200044))     /* RTC Interrupt Mode Register */
#define RTCPEND                 (*(REG32_PTR_T)(0x3D200048))     /* RTC Interrupt Pending Register */

/* 09. WATCHDOG TIMER*/
#define WDTCON                  (*(REG32_PTR_T)(0x3C800000))     /* Control Register */
#define WDTCNT                  (*(REG32_PTR_T)(0x3C800004))     /* 11-bits internal counter */

/* 11. 16 BIT TIMER */
#define TACON                   (*(REG32_PTR_T)(0x3C700000))     /* Control Register for timer A */
#define TACMD                   (*(REG32_PTR_T)(0x3C700004))     /* Command Register for timer A */
#define TADATA0                 (*(REG32_PTR_T)(0x3C700008))     /* Data0 Register */
#define TADATA1                 (*(REG32_PTR_T)(0x3C70000C))     /* Data1 Register */
#define TAPRE                   (*(REG32_PTR_T)(0x3C700010))     /* Pre-scale register */
#define TACNT                   (*(REG32_PTR_T)(0x3C700014))     /* Counter register */
#define TBCON                   (*(REG32_PTR_T)(0x3C700020))     /* Control Register for timer B */
#define TBCMD                   (*(REG32_PTR_T)(0x3C700024))     /* Command Register for timer B */
#define TBDATA0                 (*(REG32_PTR_T)(0x3C700028))     /* Data0 Register */
#define TBDATA1                 (*(REG32_PTR_T)(0x3C70002C))     /* Data1 Register */
#define TBPRE                   (*(REG32_PTR_T)(0x3C700030))     /* Pre-scale register */
#define TBCNT                   (*(REG32_PTR_T)(0x3C700034))     /* Counter register */
#define TCCON                   (*(REG32_PTR_T)(0x3C700040))     /* Control Register for timer C */
#define TCCMD                   (*(REG32_PTR_T)(0x3C700044))     /* Command Register for timer C */
#define TCDATA0                 (*(REG32_PTR_T)(0x3C700048))     /* Data0 Register */
#define TCDATA1                 (*(REG32_PTR_T)(0x3C70004C))     /* Data1 Register */
#define TCPRE                   (*(REG32_PTR_T)(0x3C700050))     /* Pre-scale register */
#define TCCNT                   (*(REG32_PTR_T)(0x3C700054))     /* Counter register */
#define TDCON                   (*(REG32_PTR_T)(0x3C700060))     /* Control Register for timer D */
#define TDCMD                   (*(REG32_PTR_T)(0x3C700064))     /* Command Register for timer D */
#define TDDATA0                 (*(REG32_PTR_T)(0x3C700068))     /* Data0 Register */
#define TDDATA1                 (*(REG32_PTR_T)(0x3C70006C))     /* Data1 Register */
#define TDPRE                   (*(REG32_PTR_T)(0x3C700070))     /* Pre-scale register */
#define TDCNT                   (*(REG32_PTR_T)(0x3C700074))     /* Counter register */

/* 12. NAND FLASH CONTROLER */
#define FMCTRL0                 (*(REG32_PTR_T)(0x3C200000))     /* Control Register0 */
#define FMCTRL1                 (*(REG32_PTR_T)(0x3C200004))     /* Control Register1 */
#define FMCMD                   (*(REG32_PTR_T)(0x3C200008))     /* Command Register */
#define FMADDR0                 (*(REG32_PTR_T)(0x3C20000C))     /* Address Register0 */
#define FMADDR1                 (*(REG32_PTR_T)(0x3C200010))     /* Address Register1 */
#define FMADDR2                 (*(REG32_PTR_T)(0x3C200014))     /* Address Register2 */
#define FMADDR3                 (*(REG32_PTR_T)(0x3C200018))     /* Address Register3 */
#define FMADDR4                 (*(REG32_PTR_T)(0x3C20001C))     /* Address Register4 */
#define FMADDR5                 (*(REG32_PTR_T)(0x3C200020))     /* Address Register5 */
#define FMADDR6                 (*(REG32_PTR_T)(0x3C200024))     /* Address Register6 */
#define FMADDR7                 (*(REG32_PTR_T)(0x3C200028))     /* Address Register7 */
#define FMANUM                  (*(REG32_PTR_T)(0x3C20002C))     /* Address Counter Register */
#define FMDNUM                  (*(REG32_PTR_T)(0x3C200030))     /* Data Counter Register */
#define FMDATAW0                (*(REG32_PTR_T)(0x3C200034))     /* Write Data Register0 */
#define FMDATAW1                (*(REG32_PTR_T)(0x3C200038))     /* Write Data Register1 */
#define FMDATAW2                (*(REG32_PTR_T)(0x3C20003C))     /* Write Data Register2 */
#define FMDATAW3                (*(REG32_PTR_T)(0x3C200040))     /* Write Data Register3 */
#define FMCSTAT                 (*(REG32_PTR_T)(0x3C200048))     /* Status Register */
#define FMSYND0                 (*(REG32_PTR_T)(0x3C20004C))     /* Hamming Syndrome0 */
#define FMSYND1                 (*(REG32_PTR_T)(0x3C200050))     /* Hamming Syndrome1 */
#define FMSYND2                 (*(REG32_PTR_T)(0x3C200054))     /* Hamming Syndrome2 */
#define FMSYND3                 (*(REG32_PTR_T)(0x3C200058))     /* Hamming Syndrome3 */
#define FMSYND4                 (*(REG32_PTR_T)(0x3C20005C))     /* Hamming Syndrome4 */
#define FMSYND5                 (*(REG32_PTR_T)(0x3C200060))     /* Hamming Syndrome5 */
#define FMSYND6                 (*(REG32_PTR_T)(0x3C200064))     /* Hamming Syndrome6 */
#define FMSYND7                 (*(REG32_PTR_T)(0x3C200068))     /* Hamming Syndrome7 */
#define FMFIFO                  (*(REG32_PTR_T)(0x3C200080))     /* WRITE/READ FIFO FIXME */
#define RSCRTL                  (*(REG32_PTR_T)(0x3C200100))     /* Reed-Solomon Control Register */
#define RSPaity0_0              (*(REG32_PTR_T)(0x3C200110))     /* On-the-fly Parity Register0[31:0] */
#define RSPaity0_1              (*(REG32_PTR_T)(0x3C200114))     /* On-the-fly Parity Register0[63:32] */
#define RSPaity0_2              (*(REG32_PTR_T)(0x3C200118))     /* On-the-fly Parity Register0[71:64] */
#define RSPaity1_0              (*(REG32_PTR_T)(0x3C200120))     /* On-the-fly Parity Register1[31:0] */
#define RSPaity1_1              (*(REG32_PTR_T)(0x3C200124))     /* On-the-fly Parity Register1[63:32] */
#define RSPaity1_2              (*(REG32_PTR_T)(0x3C200128))     /* On-the-fly Parity Register1[71:64] */
#define RSPaity2_0              (*(REG32_PTR_T)(0x3C200130))     /* On-the-fly Parity Register2[31:0] */
#define RSPaity2_1              (*(REG32_PTR_T)(0x3C200134))     /* On-the-fly Parity Register2[63:32] */
#define RSPaity2_2              (*(REG32_PTR_T)(0x3C200138))     /* On-the-fly Parity Register2[71:64] */
#define RSPaity3_0              (*(REG32_PTR_T)(0x3C200140))     /* On-the-fly Parity Register3[31:0] */
#define RSPaity3_1              (*(REG32_PTR_T)(0x3C200144))     /* On-the-fly Parity Register3[63:32] */
#define RSPaity3_2              (*(REG32_PTR_T)(0x3C200148))     /* On-the-fly Parity Register3[71:64] */
#define RSSynd0_0               (*(REG32_PTR_T)(0x3C200150))     /* On-the-fly Synd Register0[31:0] */
#define RSSynd0_1               (*(REG32_PTR_T)(0x3C200154))     /* On-the-fly Synd Register0[63:32] */
#define RSSynd0_2               (*(REG32_PTR_T)(0x3C200158))     /* On-the-fly Synd Register0[71:64] */
#define RSSynd1_0               (*(REG32_PTR_T)(0x3C200160))     /* On-the-fly Synd Register1[31:0] */
#define RSSynd1_1               (*(REG32_PTR_T)(0x3C200164))     /* On-the-fly Synd Register1[63:32] */
#define RSSynd1_2               (*(REG32_PTR_T)(0x3C200168))     /* On-the-fly Synd Register1[71:64] */
#define RSSynd2_0               (*(REG32_PTR_T)(0x3C200170))     /* On-the-fly Synd Register2[31:0] */
#define RSSynd2_1               (*(REG32_PTR_T)(0x3C200174))     /* On-the-fly Synd Register2[63:32] */
#define RSSynd2_2               (*(REG32_PTR_T)(0x3C200178))     /* On-the-fly Synd Register2[71:64] */
#define RSSynd3_0               (*(REG32_PTR_T)(0x3C200180))     /* On-the-fly Synd Register3[31:0] */
#define RSSynd3_1               (*(REG32_PTR_T)(0x3C200184))     /* On-the-fly Synd Register3[63:32] */
#define RSSynd3_2               (*(REG32_PTR_T)(0x3C200188))     /* On-the-fly Synd Register3[71:64] */
#define FlagSynd                (*(REG32_PTR_T)(0x3C200190))     /* On-the-fly ECC Result Flag */

/* 13. SECURE DIGITAL CARD INTERFACE (SDCI) */
#define SDCI_CTRL               (*(REG32_PTR_T)(0x3C300000))     /* Control Register */
#define SDCI_DCTRL              (*(REG32_PTR_T)(0x3C300004))     /* Data Control Register */
#define SDCI_CMD                (*(REG32_PTR_T)(0x3C300008))     /* Command Register */
#define SDCI_ARGU               (*(REG32_PTR_T)(0x3C30000C))     /* Argument Register */
#define SDCI_STATE              (*(REG32_PTR_T)(0x3C300010))     /* State Register */
#define SDCI_STAC               (*(REG32_PTR_T)(0x3C300014))     /* Status Clear Register */
#define SDCI_DSTA               (*(REG32_PTR_T)(0x3C300018))     /* Data Status Register */
#define SDCI_FSTA               (*(REG32_PTR_T)(0x3C30001C))     /* FIFO Status Register */
#define SDCI_RESP0              (*(REG32_PTR_T)(0x3C300020))     /* Response0 Register */
#define SDCI_RESP1              (*(REG32_PTR_T)(0x3C300024))     /* Response1 Register */
#define SDCI_RESP2              (*(REG32_PTR_T)(0x3C300028))     /* Response2 Register */
#define SDCI_RESP3              (*(REG32_PTR_T)(0x3C30002C))     /* Response3 Register */
#define SDCI_CLKDIV             (*(REG32_PTR_T)(0x3C300030))     /* Clock Divider Register */
#define SDIO_CSR                (*(REG32_PTR_T)(0x3C300034))     /* SDIO Control & Status Register */
#define SDIO_IRQ                (*(REG32_PTR_T)(0x3C300038))     /* Interrupt Source Register */

/* 14. MEMORY STICK HOST CONTROLLER */
#define MSPRE                   (*(REG32_PTR_T)(0x3C600000))     /* Prescaler Register */
#define MSINTEN                 (*(REG32_PTR_T)(0x3C600004))     /* Interrupt Enable Register */
#define MSCMD                   (*(REG32_PTR_T)(0x3C601000))     /* Command Register */
#define MSFIFO                  (*(REG32_PTR_T)(0x3C601008))     /* Receive/Transmit Register */
#define MSPP                    (*(REG32_PTR_T)(0x3C601010))     /* Parallel Port Control/Data Register */
#define MSCTRL2                 (*(REG32_PTR_T)(0x3C601014))     /* Control Register 2 */
#define MSACD                   (*(REG32_PTR_T)(0x3C601018))     /* ACD Command Register */

/* 15. SPDIF TRANSMITTER (SPDIFOUT) */
#define SPDCLKCON               (*(REG32_PTR_T)(0x3CB00000))     /* Clock Control Register */
#define SPDCON                  (*(REG32_PTR_T)(0x3CB00004))     /* Control Register 0020 */
#define SPDBSTAS                (*(REG32_PTR_T)(0x3CB00008))     /* Burst Status Register */
#define SPDCSTAS                (*(REG32_PTR_T)(0x3CB0000C))     /* Channel Status Register 0x2000 8000 */
#define SPDDAT                  (*(REG32_PTR_T)(0x3CB00010))     /* SPDIFOUT Data Buffer */
#define SPDCNT                  (*(REG32_PTR_T)(0x3CB00014))     /* Repetition Count Register */

/* 16. REED-SOLOMON ECC CODEC */
#define DATA_PTR                (*(REG32_PTR_T)(0x39E00004))     /* Data Area Start Pointer */
#define SPARE_PTR               (*(REG32_PTR_T)(0x39E00008))     /* Spare Area Start Pointer */
#define ECC_CTRL                (*(REG32_PTR_T)(0x39E0000C))     /* ECC Control Register */
#define ECC_RESULT              (*(REG32_PTR_T)(0x39E00010))     /* ECC Result */
#define ECC_EVAL0               (*(REG32_PTR_T)(0x39E00020))     /* Error Eval0 Poly */
#define ECC_EVAL1               (*(REG32_PTR_T)(0x39E00024))     /* Error Eval1 Poly */
#define ECC_LOC0                (*(REG32_PTR_T)(0x39E00028))     /* Error Loc0 Poly */
#define ECC_LOC1                (*(REG32_PTR_T)(0x39E0002C))     /* Error Loc1 Poly */
#define PARITY0                 (*(REG32_PTR_T)(0x39E00030))     /* Encode Parity0 Poly */
#define PARITY1                 (*(REG32_PTR_T)(0x39E00034))     /* Encode Pariyt1 Poly */
#define PARITY2                 (*(REG32_PTR_T)(0x39E00038))     /* Encode Parity2 Poly */
#define INT_CLR                 (*(REG32_PTR_T)(0x39E00040))     /* Interrupt Clear Register */
#define SYND0                   (*(REG32_PTR_T)(0x39E00044))     /* Syndrom0 Poly */
#define SYND1                   (*(REG32_PTR_T)(0x39E00048))     /* Syndrom1 Poly */
#define SYND2                   (*(REG32_PTR_T)(0x39E0004C))     /* Syndrom2 Poly */

/* 17. IIS Tx/Rx INTERFACE */
#define I2SCLKCON               (*(REG32_PTR_T)(0x3CA00000))     /* Clock Control Register */
#define I2STXCON                (*(REG32_PTR_T)(0x3CA00004))     /* Tx configuration Register */
#define I2STXCOM                (*(REG32_PTR_T)(0x3CA00008))     /* Tx command Register */
#define I2STXDB0                (*(REG32_PTR_T)(0x3CA00010))     /* Tx data buffer */
#define I2SRXCON                (*(REG32_PTR_T)(0x3CA00030))     /* Rx configuration Register */
#define I2SRXCOM                (*(REG32_PTR_T)(0x3CA00034))     /* Rx command Register */
#define I2SRXDB                 (*(REG32_PTR_T)(0x3CA00038))     /* Rx data buffer */
#define I2SSTATUS               (*(REG32_PTR_T)(0x3CA0003C))     /* status register */

/* 18. IIC BUS INTERFACE */
#define IICCON                  (*(REG32_PTR_T)(0x3C900000))     /* Control Register */
#define IICSTAT                 (*(REG32_PTR_T)(0x3C900004))     /* Control/Status Register */
#define IICADD                  (*(REG32_PTR_T)(0x3C900008))     /* Bus Address Register */
#define IICDS                   (*(REG32_PTR_T)(0x3C90000C))

/* 19. SPI (SERIAL PERHIPERAL INTERFACE) */
#define SPCLKCON                (*(REG32_PTR_T)(0x3CD00000))     /* Clock Control Register */
#define SPCON                   (*(REG32_PTR_T)(0x3CD00004))     /* Control Register */
#define SPSTA                   (*(REG32_PTR_T)(0x3CD00008))     /* Status Register */
#define SPPIN                   (*(REG32_PTR_T)(0x3CD0000C))     /* Pin Control Register */
#define SPTDAT                  (*(REG32_PTR_T)(0x3CD00010))     /* Tx Data Register */
#define SPRDAT                  (*(REG32_PTR_T)(0x3CD00014))     /* Rx Data Register */
#define SPPRE                   (*(REG32_PTR_T)(0x3CD00018))     /* Baud Rate Prescaler Register */

/* 20. ADC CONTROLLER */
#define ADCCON                  (*(REG32_PTR_T)(0x3CE00000))     /* ADC Control Register */
#define ADCTSC                  (*(REG32_PTR_T)(0x3CE00004))     /* ADC Touch Screen Control Register */
#define ADCDLY                  (*(REG32_PTR_T)(0x3CE00008))     /* ADC Start or Interval Delay Register */
#define ADCDAT0                 (*(REG32_PTR_T)(0x3CE0000C))     /* ADC Conversion Data Register */
#define ADCDAT1                 (*(REG32_PTR_T)(0x3CE00010))     /* ADC Conversion Data Register */
#define ADCUPDN                 (*(REG32_PTR_T)(0x3CE00014))     /* Stylus Up or Down Interrpt Register */

/* 21. USB 2.0 FUNCTION CONTROLER SPECIAL REGISTER */
#define IR                      (*(REG32_PTR_T)(0x38800000))     /* Index Register */
#define EIR                     (*(REG32_PTR_T)(0x38800004))     /* Endpoint Interrupt Register */
#define EIER                    (*(REG32_PTR_T)(0x38800008))     /* Endpoint Interrupt Enable Register */
#define FAR                     (*(REG32_PTR_T)(0x3880000C))     /* Function Address Register */
#define FNR                     (*(REG32_PTR_T)(0x38800010))     /* Frame Number Register */
#define EDR                     (*(REG32_PTR_T)(0x38800014))     /* Endpoint Direction Register */
#define TR                      (*(REG32_PTR_T)(0x38800018))     /* Test Register */
#define SSR                     (*(REG32_PTR_T)(0x3880001C))     /* System Status Register */
#define SCR                     (*(REG32_PTR_T)(0x38800020))     /* System Control Register */
#define EP0SR                   (*(REG32_PTR_T)(0x38800024))     /* EP0 Status Register */
#define EP0CR                   (*(REG32_PTR_T)(0x38800028))     /* EP0 Control Register */
#define ESR                     (*(REG32_PTR_T)(0x3880002C))     /* Endpoints Status Register */
#define ECR                     (*(REG32_PTR_T)(0x38800030))     /* Endpoints Control Register */
#define BRCR                    (*(REG32_PTR_T)(0x38800034))     /* Byte Read Count Register */
#define BWCR                    (*(REG32_PTR_T)(0x38800038))     /* Byte Write Count Register */
#define MPR                     (*(REG32_PTR_T)(0x3880003C))     /* Max Packet Register */
#define MCR                     (*(REG32_PTR_T)(0x38800040))     /* Master Control Register */
#define MTCR                    (*(REG32_PTR_T)(0x38800044))     /* Master Transfer Counter Register */
#define MFCR                    (*(REG32_PTR_T)(0x38800048))     /* Master FIFO Counter Register */
#define MTTCR1                  (*(REG32_PTR_T)(0x3880004C))     /* Master Total Transfer Counter1 Register */
#define MTTCR2                  (*(REG32_PTR_T)(0x38800050))     /* Master Total Transfer Counter2 Register */
#define EP0BR                   (*(REG32_PTR_T)(0x38800060))     /* EP0 Buffer Register */
#define EP1BR                   (*(REG32_PTR_T)(0x38800064))     /* EP1 Buffer Register */
#define EP2BR                   (*(REG32_PTR_T)(0x38800068))     /* EP2 Buffer Register */
#define EP3BR                   (*(REG32_PTR_T)(0x3880006C))     /* EP3 Buffer Register */
#define EP4BR                   (*(REG32_PTR_T)(0x38800070))     /* EP4 Buffer Register */
#define EP5BR                   (*(REG32_PTR_T)(0x38800074))     /* EP5 Buffer Register */
#define EP6BR                   (*(REG32_PTR_T)(0x38800078))     /* EP6 Buffer Register */
#define MICR                    (*(REG32_PTR_T)(0x38800084))     /* Master Interface Counter Register */
#define MBAR1                   (*(REG32_PTR_T)(0x38800088))     /* Memory Base Address Register1 */
#define MBAR2                   (*(REG32_PTR_T)(0x3880008C))     /* Memory Base Address Register2 */
#define MCAR1                   (*(REG32_PTR_T)(0x38800094))     /* Memory Current Address Register1 */
#define MCAR2                   (*(REG32_PTR_T)(0x38800098))     /* Memory Current Address Register2 */

/* 22. USB 1.1 HOST CONTROLER SPECIAL REGISTER */
#define HcRevision              (*(REG32_PTR_T)(0x38600000))
#define HcControl               (*(REG32_PTR_T)(0x38600004))
#define HcCommandStatus         (*(REG32_PTR_T)(0x38600008))
#define HcInterruptStatus       (*(REG32_PTR_T)(0x3860000C))
#define HcInterruptEnable       (*(REG32_PTR_T)(0x38600010))
#define HcInterruptDisable      (*(REG32_PTR_T)(0x38600014))
#define HcHCCA                  (*(REG32_PTR_T)(0x38600018))
#define HcPeriodCurrentED       (*(REG32_PTR_T)(0x3860001C))
#define HcControlHeadED         (*(REG32_PTR_T)(0x38600020))
#define HcControlCurrentED      (*(REG32_PTR_T)(0x38600024))
#define HcBulkHeadED            (*(REG32_PTR_T)(0x38600028))
#define HcBulkCurrentED         (*(REG32_PTR_T)(0x3860002C))
#define HcDoneHead              (*(REG32_PTR_T)(0x38600030))
#define HcFmInterval            (*(REG32_PTR_T)(0x38600034))
#define HcFmRemaining           (*(REG32_PTR_T)(0x38600038))
#define HcFmNumber              (*(REG32_PTR_T)(0x3860003C))
#define HcPeriodicStart         (*(REG32_PTR_T)(0x38600040))
#define HcLSThreshold           (*(REG32_PTR_T)(0x38600044))
#define HcRhDescriptorA         (*(REG32_PTR_T)(0x38600048))
#define HcRhDescriptorB         (*(REG32_PTR_T)(0x3860004C))
#define HcRhStatus              (*(REG32_PTR_T)(0x38600050))
#define HcRhPortStatus          (*(REG32_PTR_T)(0x38600054))

/* 23. USB 2.0 PHY CONTROL */
#define PHYCTRL                 (*(REG32_PTR_T)(0x3C400000))     /* USB2.0 PHY Control Register */
#define PHYPWR                  (*(REG32_PTR_T)(0x3C400004))     /* USB2.0 PHY Power Control Register */
#define URSTCON                 (*(REG32_PTR_T)(0x3C400008))     /* USB Reset Control Register */
#define UCLKCON                 (*(REG32_PTR_T)(0x3C400010))     /* USB Clock Control Register */

/* 24. GPIO PORT CONTROLL */
#define PCON0                   (*(REG32_PTR_T)(0x3CF00000))     /* Configures the pins of port 0 */
#define PDAT0                   (*(REG32_PTR_T)(0x3CF00004))     /* The data register for port 0 */
#define PCON1                   (*(REG32_PTR_T)(0x3CF00010))     /* Configures the pins of port 0 */
#define PDAT1                   (*(REG32_PTR_T)(0x3CF00014))     /* The data register for port 0 */
#define PCON2                   (*(REG32_PTR_T)(0x3CF00020))     /* Configures the pins of port 0 */
#define PDAT2                   (*(REG32_PTR_T)(0x3CF00024))     /* The data register for port 0 */
#define PCON3                   (*(REG32_PTR_T)(0x3CF00030))     /* Configures the pins of port 0 */
#define PDAT3                   (*(REG32_PTR_T)(0x3CF00034))     /* The data register for port 0 */
#define PCON4                   (*(REG32_PTR_T)(0x3CF00040))     /* Configures the pins of port 0 */
#define PDAT4                   (*(REG32_PTR_T)(0x3CF00044))     /* The data register for port 0 */
#define PCON5                   (*(REG32_PTR_T)(0x3CF00050))     /* Configures the pins of port 0 */
#define PDAT5                   (*(REG32_PTR_T)(0x3CF00054))     /* The data register for port 0 */
#define PCON6                   (*(REG32_PTR_T)(0x3CF00060))     /* Configures the pins of port 0 */
#define PDAT6                   (*(REG32_PTR_T)(0x3CF00064))     /* The data register for port 0 */
#define PCON7                   (*(REG32_PTR_T)(0x3CF00070))     /* Configures the pins of port 0 */
#define PDAT7                   (*(REG32_PTR_T)(0x3CF00074))     /* The data register for port 0 */
#define PCON10                  (*(REG32_PTR_T)(0x3CF000A0))     /* Configures the pins of port 0 */
#define PDAT10                  (*(REG32_PTR_T)(0x3CF000A4))     /* The data register for port 0 */
#define PCON_ASRAM              (*(REG32_PTR_T)(0x3CF000F0))     /* Configures the pins of port nor flash */
#define PCON_SDRAM              (*(REG32_PTR_T)(0x3CF000F4))     /* Configures the pins of port sdram */
#define PCON11                  (*(REG32_PTR_T)(0x3CF000F8))     /* Configures the pins of port 11 */
#define PDAT11                  (*(REG32_PTR_T)(0x3CF000FC))     /* The data register for port 11 */

/* 25. UART */

/* UART 0 */
#define ULCON0                  (*(REG32_PTR_T)(0x3CC00000))     /* Line Control Register */
#define UCON0                   (*(REG32_PTR_T)(0x3CC00004))     /* Control Register */
#define UFCON0                  (*(REG32_PTR_T)(0x3CC00008))     /* FIFO Control Register */
#define UMCON0                  (*(REG32_PTR_T)(0x3CC0000C))     /* Modem Control Register */
#define UTRSTAT0                (*(REG32_PTR_T)(0x3CC00010))     /* Tx/Rx Status Register */
#define UERSTAT0                (*(REG32_PTR_T)(0x3CC00014))     /* Rx Error Status Register */
#define UFSTAT0                 (*(REG32_PTR_T)(0x3CC00018))     /* FIFO Status Register */
#define UMSTAT0                 (*(REG32_PTR_T)(0x3CC0001C))     /* Modem Status Register */
#define UTXH0                   (*(REG32_PTR_T)(0x3CC00020))     /* Transmit Buffer Register */
#define URXH0                   (*(REG32_PTR_T)(0x3CC00024))     /* Receive Buffer Register */
#define UBRDIV0                 (*(REG32_PTR_T)(0x3CC00028))     /* Baud Rate Divisor Register */

/* UART 1*/
#define ULCON1                  (*(REG32_PTR_T)(0x3CC08000))     /* Line Control Register */
#define UCON1                   (*(REG32_PTR_T)(0x3CC08004))     /* Control Register */
#define UFCON1                  (*(REG32_PTR_T)(0x3CC08008))     /* FIFO Control Register */
#define UMCON1                  (*(REG32_PTR_T)(0x3CC0800C))     /* Modem Control Register */
#define UTRSTAT1                (*(REG32_PTR_T)(0x3CC08010))     /* Tx/Rx Status Register */
#define UERSTAT1                (*(REG32_PTR_T)(0x3CC08014))     /* Rx Error Status Register */
#define UFSTAT1                 (*(REG32_PTR_T)(0x3CC08018))     /* FIFO Status Register */
#define UMSTAT1                 (*(REG32_PTR_T)(0x3CC0801C))     /* Modem Status Register */
#define UTXH1                   (*(REG32_PTR_T)(0x3CC08020))     /* Transmit Buffer Register */
#define URXH1                   (*(REG32_PTR_T)(0x3CC08024))     /* Receive Buffer Register */
#define UBRDIV1                 (*(REG32_PTR_T)(0x3CC08028))     /* Baud Rate Divisor Register */

/* 26. LCD INTERFACE CONTROLLER */
#define LCD_CON                 (*(REG32_PTR_T)(0x3C100000))     /* Control register. */
#define LCD_WCMD                (*(REG32_PTR_T)(0x3C100004))     /* Write command register. */
#define LCD_RCMD                (*(REG32_PTR_T)(0x3C10000C))     /* Read command register. */
#define LCD_RDATA               (*(REG32_PTR_T)(0x3C100010))     /* Read data register. */
#define LCD_DBUFF               (*(REG32_PTR_T)(0x3C100014))     /* Read Data buffer */
#define LCD_INTCON              (*(REG32_PTR_T)(0x3C100018))     /* Interrupt control register */
#define LCD_STATUS              (*(REG32_PTR_T)(0x3C10001C))     /* LCD Interface status 0106 */
#define LCD_PHTIME              (*(REG32_PTR_T)(0x3C100020))     /* Phase time register 0060 */
#define LCD_RST_TIME            (*(REG32_PTR_T)(0x3C100024))     /* Reset active period 07FF */
#define LCD_DRV_RST             (*(REG32_PTR_T)(0x3C100028))     /* Reset drive signal */
#define LCD_WDATA               (*(REG32_PTR_T)(0x3C100040))     /* Write data register FIXME */

/* 27. CLCD CONTROLLER */
#define LCDCON1                 (*(REG32_PTR_T)(0x39200000))     /* LCD control 1 register */
#define LCDCON2                 (*(REG32_PTR_T)(0x39200004))     /* LCD control 2 register */
#define LCDTCON1                (*(REG32_PTR_T)(0x39200008))     /* LCD time control 1 register */
#define LCDTCON2                (*(REG32_PTR_T)(0x3920000C))     /* LCD time control 2 register */
#define LCDTCON3                (*(REG32_PTR_T)(0x39200010))     /* LCD time control 3 register */
#define LCDOSD1                 (*(REG32_PTR_T)(0x39200014))     /* LCD OSD control 1 register */
#define LCDOSD2                 (*(REG32_PTR_T)(0x39200018))     /* LCD OSD control 2 register */
#define LCDOSD3                 (*(REG32_PTR_T)(0x3920001C))     /* LCD OSD control 3 register */
#define LCDB1SADDR1             (*(REG32_PTR_T)(0x39200020))     /* Frame buffer start address register for Back-Ground buffer 1 */
#define LCDB2SADDR1             (*(REG32_PTR_T)(0x39200024))     /* Frame buffer start address register for Back-Ground buffer 2 */
#define LCDF1SADDR1             (*(REG32_PTR_T)(0x39200028))     /* Frame buffer start address register for Fore-Ground (OSD) buffer 1 */
#define LCDF2SADDR1             (*(REG32_PTR_T)(0x3920002C))     /* Frame buffer start address register for Fore-Ground (OSD) buffer 2 */
#define LCDB1SADDR2             (*(REG32_PTR_T)(0x39200030))     /* Frame buffer end address register for Back-Ground buffer 1 */
#define LCDB2SADDR2             (*(REG32_PTR_T)(0x39200034))     /* Frame buffer end address register for Back-Ground buffer 2 */
#define LCDF1SADDR2             (*(REG32_PTR_T)(0x39200038))     /* Frame buffer end address register for Fore-Ground (OSD) buffer 1 */
#define LCDF2SADDR2             (*(REG32_PTR_T)(0x3920003C))     /* Frame buffer end address register for Fore-Ground (OSD) buffer 2 */
#define LCDB1SADDR3             (*(REG32_PTR_T)(0x39200040))     /* Virtual screen address set for Back-Ground buffer 1 */
#define LCDB2SADDR3             (*(REG32_PTR_T)(0x39200044))     /* Virtual screen address set for Back-Ground buffer 2 */
#define LCDF1SADDR3             (*(REG32_PTR_T)(0x39200048))     /* Virtual screen address set for Fore-Ground(OSD) buffer 1 */
#define LCDF2SADDR3             (*(REG32_PTR_T)(0x3920004C))     /* Virtual screen address set for Fore-Ground(OSD) buffer 2 */
#define LCDINTCON               (*(REG32_PTR_T)(0x39200050))     /* Indicate the LCD interrupt control register */
#define KEYCON                  (*(REG32_PTR_T)(0x39200054))     /* Color key control register */
#define COLVAL                  (*(REG32_PTR_T)(0x39200058))     /* Color key value ( transparent value) register */
#define BGCON                   (*(REG32_PTR_T)(0x3920005C))     /* Back-Ground color control */
#define FGCON                   (*(REG32_PTR_T)(0x39200060))     /* Fore-Ground color control */
#define DITHMODE                (*(REG32_PTR_T)(0x39200064))     /* Dithering mode register. */

/* 28. ATA CONTROLLER */
#define ATA_CONTROL             (*(REG32_PTR_T)(0x38E00000))     /* Enable and clock down status */
#define ATA_STATUS              (*(REG32_PTR_T)(0x38E00004))     /* Status */
#define ATA_COMMAND             (*(REG32_PTR_T)(0x38E00008))     /* Command */
#define ATA_SWRST               (*(REG32_PTR_T)(0x38E0000C))     /* Software reset */
#define ATA_IRQ                 (*(REG32_PTR_T)(0x38E00010))     /* Interrupt sources */
#define ATA_IRQ_MASK            (*(REG32_PTR_T)(0x38E00014))     /* Interrupt mask */
#define ATA_CFG                 (*(REG32_PTR_T)(0x38E00018))     /* Configuration for ATA interface */
#define ATA_PIO_TIME            (*(REG32_PTR_T)(0x38E0002C))     /* PIO timing */
#define ATA_UDMA_TIME           (*(REG32_PTR_T)(0x38E00030))     /* UDMA timing */
#define ATA_XFR_NUM             (*(REG32_PTR_T)(0x38E00034))     /* Transfer number */
#define ATA_XFR_CNT             (*(REG32_PTR_T)(0x38E00038))     /* Current transfer count */
#define ATA_TBUF_START          (*(REG32_PTR_T)(0x38E0003C))     /* Start address of track buffer */
#define ATA_TBUF_SIZE           (*(REG32_PTR_T)(0x38E00040))     /* Size of track buffer */
#define ATA_SBUF_START          (*(REG32_PTR_T)(0x38E00044))     /* Start address of Source buffer1 */
#define ATA_SBUF_SIZE           (*(REG32_PTR_T)(0x38E00048))     /* Size of source buffer1 */
#define ATA_CADR_TBUF           (*(REG32_PTR_T)(0x38E0004C))     /* Current write address of track buffer */
#define ATA_CADR_SBUF           (*(REG32_PTR_T)(0x38E00050))     /* Current read address of source buffer */
#define ATA_PIO_DTR             (*(REG32_PTR_T)(0x38E00054))     /* PIO device data register */
#define ATA_PIO_FED             (*(REG32_PTR_T)(0x38E00058))     /* PIO device Feature/Error register */
#define ATA_PIO_SCR             (*(REG32_PTR_T)(0x38E0005C))     /* PIO sector count register */
#define ATA_PIO_LLR             (*(REG32_PTR_T)(0x38E00060))     /* PIO device LBA low register */
#define ATA_PIO_LMR             (*(REG32_PTR_T)(0x38E00064))     /* PIO device LBA middle register */
#define ATA_PIO_LHR             (*(REG32_PTR_T)(0x38E00068))     /* PIO device LBA high register */
#define ATA_PIO_DVR             (*(REG32_PTR_T)(0x38E0006C))     /* PIO device register */
#define ATA_PIO_CSD             (*(REG32_PTR_T)(0x38E00070))     /* PIO device command/status register */
#define ATA_PIO_DAD             (*(REG32_PTR_T)(0x38E00074))     /* PIO control/alternate status register */
#define ATA_PIO_READY           (*(REG32_PTR_T)(0x38E00078))     /* PIO data read/write ready */
#define ATA_PIO_RDATA           (*(REG32_PTR_T)(0x38E0007C))     /* PIO read data from device register */
#define BUS_FIFO_STATUS         (*(REG32_PTR_T)(0x38E00080))     /* Reserved */
#define ATA_FIFO_STATUS         (*(REG32_PTR_T)(0x38E00084))     /* Reserved */

/* 29. CHIP ID */
#define REG_ONE                 (*(REG32_PTR_T)(0x3D100000))     /* Receive the first 32 bits from a fuse box */
#define REG_TWO                 (*(REG32_PTR_T)(0x3D100004))     /* Receive the other 8 bits from a fuse box */

