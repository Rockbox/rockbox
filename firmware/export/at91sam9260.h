/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *
 * Copyright (C) 2009 by Jorge Pinto
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

/* MATRIX_MRCR : (MATRIX Offset: 0x100) MRCR Register */
#define AT91C_MATRIX_RCA926I  (0x1 <<  0) /* (MATRIX) Remap Command for
ARM926EJ-S Instruction Master */
#define AT91C_MATRIX_RCA926D  (0x1 <<  1) /* (MATRIX) Remap Command for
ARM926EJ-S Data Master */

/* Register definition for MATRIX peripheral */
#define AT91C_MATRIX_MCFG0 (*(volatile unsigned long*)   0xFFFFEE00) /*(MATRIX)
Master Configuration Register 0 (ram96k) */
#define AT91C_MATRIX_MCFG7 (*(volatile unsigned long*)   0xFFFFEE1C) /*(MATRIX)
Master Configuration Register 7 (teak_prog) */
#define AT91C_MATRIX_SCFG1 (*(volatile unsigned long*)   0xFFFFEE44) /*(MATRIX)
Slave Configuration Register 1 (rom) */
#define AT91C_MATRIX_MCFG4 (*(volatile unsigned long*)   0xFFFFEE10) /*(MATRIX)
Master Configuration Register 4 (bridge) */
#define AT91C_MATRIX_VERSION (*(volatile unsigned long*) 0xFFFFEFFC) /*(MATRIX)
Version Register */
#define AT91C_MATRIX_MCFG2 (*(volatile unsigned long*)   0xFFFFEE08) /*(MATRIX)
Master Configuration Register 2 (hperiphs) */
#define AT91C_MATRIX_PRBS0 (*(volatile unsigned long*)   0xFFFFEE84) /*(MATRIX)
PRBS0 (ram0) */
#define AT91C_MATRIX_SCFG3 (*(volatile unsigned long*)   0xFFFFEE4C) /*(MATRIX)
Slave Configuration Register 3 (ebi) */
#define AT91C_MATRIX_MCFG6 (*(volatile unsigned long*)   0xFFFFEE18) /*(MATRIX)
Master Configuration Register 6 (ram16k) */
#define AT91C_MATRIX_EBI (*(volatile unsigned long*)     0xFFFFEF1C) /*(MATRIX)
Slave 3 (ebi) Special Function Register */
#define AT91C_MATRIX_SCFG0 (*(volatile unsigned long*)   0xFFFFEE40) /*(MATRIX)
Slave Configuration Register 0 (ram96k) */
#define AT91C_MATRIX_PRAS0 (*(volatile unsigned long*)   0xFFFFEE80) /*(MATRIX)
PRAS0 (ram0) */
#define AT91C_MATRIX_MCFG3 (*(volatile unsigned long*)   0xFFFFEE0C) /*(MATRIX)
Master Configuration Register 3 (ebi) */
#define AT91C_MATRIX_PRAS1 (*(volatile unsigned long*)   0xFFFFEE88) /*(MATRIX)
PRAS1 (ram1) */
#define AT91C_MATRIX_PRAS2 (*(volatile unsigned long*)   0xFFFFEE90) /*(MATRIX)
PRAS2 (ram2) */
#define AT91C_MATRIX_SCFG2 (*(volatile unsigned long*)   0xFFFFEE48) /*(MATRIX)
Slave Configuration Register 2 (hperiphs) */
#define AT91C_MATRIX_MCFG5 (*(volatile unsigned long*)   0xFFFFEE14) /*(MATRIX)
Master Configuration Register 5 (mailbox) */
#define AT91C_MATRIX_MCFG1 (*(volatile unsigned long*)   0xFFFFEE04) /*(MATRIX)
Master Configuration Register 1 (rom) */
#define AT91C_MATRIX_MRCR (*(volatile unsigned long*)    0xFFFFEF00) /*(MATRIX)
Master Remp Control Register */
#define AT91C_MATRIX_PRBS2 (*(volatile unsigned long*)   0xFFFFEE94) /*(MATRIX)
PRBS2 (ram2) */
#define AT91C_MATRIX_SCFG4 (*(volatile unsigned long*)   0xFFFFEE50) /*(MATRIX)
Slave Configuration Register 4 (bridge) */
#define AT91C_MATRIX_TEAKCFG (*(volatile unsigned long*) 0xFFFFEF2C) /*(MATRIX)
Slave 7 (teak_prog) Special Function Register */
#define AT91C_MATRIX_PRBS1 (*(volatile unsigned long*)   0xFFFFEE8C) /*(MATRIX)
PRBS1 (ram1) */

/* SOFTWARE API DEFINITION  FOR Watchdog Timer Controller Interface */
/* - WDTC_WDCR : (WDTC Offset: 0x0) Periodic Interval Image Register ---- */
#define AT91C_WDTC_WDRSTT     (0x1 <<  0) /* (WDTC) Watchdog Restart */
#define AT91C_WDTC_KEY        (0xFF << 24) /* (WDTC) Watchdog KEY Password */
/* -------- WDTC_WDMR : (WDTC Offset: 0x4) Watchdog Mode Register -------- */
#define AT91C_WDTC_WDV     (0xFFF <<  0) /* (WDTC) Watchdog Timer Restart */
#define AT91C_WDTC_WDFIEN     (0x1 << 12) /* (WDTC) Watchdog Fault Interrupt
Enable */
#define AT91C_WDTC_WDRSTEN    (0x1 << 13) /* (WDTC) Watchdog Reset Enable */
#define AT91C_WDTC_WDRPROC    (0x1 << 14) /* (WDTC) Watchdog Timer Restart */
#define AT91C_WDTC_WDDIS      (0x1 << 15) /* (WDTC) Watchdog Disable */
#define AT91C_WDTC_WDD        (0xFFF << 16) /* (WDTC) Watchdog Delta Value */
#define AT91C_WDTC_WDDBGHLT   (0x1 << 28) /* (WDTC) Watchdog Debug Halt */
#define AT91C_WDTC_WDIDLEHLT  (0x1 << 29) /* (WDTC) Watchdog Idle Halt */
/* -------- WDTC_WDSR : (WDTC Offset: 0x8) Watchdog Status Register ----- */
#define AT91C_WDTC_WDUNF      (0x1 <<  0) /* (WDTC) Watchdog Underflow */
#define AT91C_WDTC_WDERR      (0x1 <<  1) /* (WDTC) Watchdog Error */

/* SOFTWARE API DEFINITION  FOR Watchdog Timer Controller Interface */
/* - WDTC_WDCR : (WDTC Offset: 0x0) Periodic Interval Image Register ---- */
#define AT91C_WDTC_WDRSTT     (0x1 <<  0) /* (WDTC) Watchdog Restart */
#define AT91C_WDTC_KEY        (0xFF << 24) /* (WDTC) Watchdog KEY Password */
/* -------- WDTC_WDMR : (WDTC Offset: 0x4) Watchdog Mode Register -------- */
#define AT91C_WDTC_WDV     (0xFFF <<  0) /* (WDTC) Watchdog Timer Restart */
#define AT91C_WDTC_WDFIEN     (0x1 << 12) /* (WDTC) Watchdog Fault Interrupt
Enable */
#define AT91C_WDTC_WDRSTEN    (0x1 << 13) /* (WDTC) Watchdog Reset Enable */
#define AT91C_WDTC_WDRPROC    (0x1 << 14) /* (WDTC) Watchdog Timer Restart */
#define AT91C_WDTC_WDDIS      (0x1 << 15) /* (WDTC) Watchdog Disable */
#define AT91C_WDTC_WDD        (0xFFF << 16) /* (WDTC) Watchdog Delta Value */
#define AT91C_WDTC_WDDBGHLT   (0x1 << 28) /* (WDTC) Watchdog Debug Halt */
#define AT91C_WDTC_WDIDLEHLT  (0x1 << 29) /* (WDTC) Watchdog Idle Halt */
/* -------- WDTC_WDSR : (WDTC Offset: 0x8) Watchdog Status Register ----- */
#define AT91C_WDTC_WDUNF      (0x1 <<  0) /* (WDTC) Watchdog Underflow */
#define AT91C_WDTC_WDERR      (0x1 <<  1) /* (WDTC) Watchdog Error */

/* Register definition for WDTC peripheral */
#define AT91C_WDTC_WDCR (*(volatile unsigned long*)  0xFFFFFD40) /* (WDTC)
Watchdog Control Register */
#define AT91C_WDTC_WDSR (*(volatile unsigned long*)  0xFFFFFD48) /* (WDTC)
Watchdog Status Register */
#define AT91C_WDTC_WDMR (*(volatile unsigned long*)  0xFFFFFD44) /* (WDTC)
Watchdog Mode Register */

/* CKGR_MOR : (CKGR Offset: 0x0) Main Oscillator Register */
#define AT91C_CKGR_MOSCEN     (0x1 <<  0) /* (CKGR) Main Oscillator Enable */
#define AT91C_CKGR_OSCBYPASS  (0x1 <<  1) /* (CKGR) Main Oscillator Bypass */
#define AT91C_CKGR_OSCOUNT    (0xFF <<  8) /* (CKGR) Main Oscillator Start
-up Time */
/* CKGR_MCFR : (CKGR Offset: 0x4) Main Clock Frequency Register */
#define AT91C_CKGR_MAINF      (0xFFFF <<  0) /* (CKGR) Main Clock Frequency */
#define AT91C_CKGR_MAINRDY    (0x1 << 16) /* (CKGR) Main Clock Ready */
/* CKGR_PLLAR : (CKGR Offset: 0x8) PLL A Register */
#define AT91C_CKGR_DIVA       (0xFF <<  0) /* (CKGR) Divider A Selected */
#define     AT91C_CKGR_DIVA_0        (0x0) /* (CKGR) Divider A output is 0 */
#define     AT91C_CKGR_DIVA_BYPASS    (0x1) /* (CKGR) Divider A is bypassed */
#define AT91C_CKGR_PLLACOUNT  (0x3F <<  8) /* (CKGR) PLL A Counter */
#define AT91C_CKGR_OUTA  (0x3 << 14) /* (CKGR) PLL A Output Frequency Range */
#define     AT91C_CKGR_OUTA_0                    (0x0 << 14) /* (CKGR) Please
refer to the PLLA datasheet */
#define     AT91C_CKGR_OUTA_1                    (0x1 << 14) /* (CKGR) Please
refer to the PLLA datasheet */
#define     AT91C_CKGR_OUTA_2                    (0x2 << 14) /* (CKGR) Please
refer to the PLLA datasheet */
#define     AT91C_CKGR_OUTA_3                    (0x3 << 14) /* (CKGR) Please
refer to the PLLA datasheet */
#define AT91C_CKGR_MULA       (0x7FF << 16) /* (CKGR) PLL A Multiplier */
#define AT91C_CKGR_SRCA       (0x1 << 29) /* (CKGR) */
/* CKGR_PLLBR : (CKGR Offset: 0xc) PLL B Register */
#define AT91C_CKGR_DIVB       (0xFF <<  0) /* (CKGR) Divider B Selected */
#define     AT91C_CKGR_DIVB_0        (0x0) /* (CKGR) Divider B output is 0 */
#define     AT91C_CKGR_DIVB_BYPASS   (0x1) /* (CKGR) Divider B is bypassed */
#define AT91C_CKGR_PLLBCOUNT  (0x3F <<  8) /* (CKGR) PLL B Counter */
#define AT91C_CKGR_OUTB  (0x3 << 14) /* (CKGR) PLL B Output Frequency Range */
#define     AT91C_CKGR_OUTB_0                    (0x0 << 14) /* (CKGR) Please
refer to the PLLB datasheet */
#define     AT91C_CKGR_OUTB_1                    (0x1 << 14) /* (CKGR) Please
refer to the PLLB datasheet */
#define     AT91C_CKGR_OUTB_2                    (0x2 << 14) /* (CKGR) Please
refer to the PLLB datasheet */
#define     AT91C_CKGR_OUTB_3                    (0x3 << 14) /* (CKGR) Please
refer to the PLLB datasheet */
#define AT91C_CKGR_MULB       (0x7FF << 16) /* (CKGR) PLL B Multiplier */
#define AT91C_CKGR_USBDIV     (0x3 << 28) /* (CKGR) Divider for USB Clocks */
#define     AT91C_CKGR_USBDIV_0                    (0x0 << 28) /* (CKGR)
Divider output is PLL clock output */
#define     AT91C_CKGR_USBDIV_1                    (0x1 << 28) /* (CKGR)
Divider output is PLL clock output divided by 2 */
#define     AT91C_CKGR_USBDIV_2                    (0x2 << 28) /* (CKGR)
Divider output is PLL clock output divided by 4 */

/* SOFTWARE API DEFINITION  FOR Power Management Controler */
/* -------- PMC_SCER : (PMC Offset: 0x0) System Clock Enable Register ----- */
#define AT91C_PMC_PCK         (0x1 <<  0) /* (PMC) Processor Clock */
#define AT91C_PMC_UHP         (0x1 <<  6) /* (PMC) USB Host Port Clock */
#define AT91C_PMC_UDP         (0x1 <<  7) /* (PMC) USB Device Port Clock */
#define AT91C_PMC_PCK0        (0x1 <<  8) /* (PMC) Programmable Clock Output */
#define AT91C_PMC_PCK1        (0x1 <<  9) /* (PMC) Programmable Clock Output */
#define AT91C_PMC_HCK0        (0x1 << 16) /* (PMC) AHB UHP Clock Output */
#define AT91C_PMC_HCK1        (0x1 << 17) /* (PMC) AHB LCDC Clock Output */
/* -------- PMC_SCDR : (PMC Offset: 0x4) System Clock Disable Register ----- */
 /* -------- PMC_SCSR : (PMC Offset: 0x8) System Clock Status Register ----- */
 /* -------- CKGR_MOR : (PMC Offset: 0x20) Main Oscillator Register -------- */
 /* -------- CKGR_MCFR : (PMC Offset: 0x24) Main Clock Frequency Register--- */
 /* -------- CKGR_PLLAR : (PMC Offset: 0x28) PLL A Register -------- */
 /* -------- CKGR_PLLBR : (PMC Offset: 0x2c) PLL B Register -------- */
 /* -------- PMC_MCKR : (PMC Offset: 0x30) Master Clock Register -------- */
 #define AT91C_PMC_CSS    (0x3 <<  0) /* (PMC) Programmable Clock Selection */
 #define AT91C_PMC_CSS_SLOW_CLK      (0x0) /* (PMC) Slow Clock is selected */
 #define  AT91C_PMC_CSS_MAIN_CLK      (0x1) /* (PMC) Main Clock is selected */
 #define AT91C_PMC_CSS_PLLA_CLK (0x2) /* (PMC) Clock from PLL A is selected */
 #define AT91C_PMC_CSS_PLLB_CLK (0x3) /* (PMC) Clock from PLL B is selected */
 #define AT91C_PMC_PRES   (0x7 <<  2) /* (PMC) Programmable Clock Prescaler */
 #define AT91C_PMC_PRES_CLK     (0x0 <<  2) /* (PMC) Selected clock */
 #define AT91C_PMC_PRES_CLK_2   (0x1 <<  2) /* (PMC) Selected clock
                                                            divided by 2 */
 #define AT91C_PMC_PRES_CLK_4   (0x2 <<  2) /* (PMC) Selected clock
                                                            divided by 4 */
 #define AT91C_PMC_PRES_CLK_8   (0x3 <<  2) /* (PMC) Selected clock
                                                            divided by 8 */
 #define AT91C_PMC_PRES_CLK_16  (0x4 <<  2) /* (PMC) Selected clock
                                                            divided by 16 */
 #define AT91C_PMC_PRES_CLK_32  (0x5 <<  2) /* (PMC) Selected clock
                                                            divided by 32 */
 #define AT91C_PMC_PRES_CLK_64  (0x6 <<  2) /* (PMC) Selected clock
                                                            divided by 64 */
 #define AT91C_PMC_MDIV      (0x3 <<  8) /* (PMC) Master Clock Divisionv */
 #define AT91C_PMC_MDIV_1  (0x0 <<  8) /* (PMC) The master clock and the
                                              processor clock are the same */
 #define AT91C_PMC_MDIV_2  (0x1 <<  8) /* (PMC) The processor clock is twice
                                                as fast as the master clock */
 #define AT91C_PMC_MDIV_3  (0x2 <<  8) /* (PMC) The processor clock is four
                                         times faster than the master clock */
 /* -------- PMC_PCKR : (PMC Offset: 0x40) Programmable Clock Register ----- */
 /* -------- PMC_IER : (PMC Offset: 0x60) PMC Interrupt Enable Register ---- */
 #define AT91C_PMC_MOSCS (0x1 <<  0) /* (PMC) MOSC Status/Enable/Disable/Mask*/
 #define AT91C_PMC_LOCKA (0x1 <<  1) /* (PMC) PLL A Status/Enable/Disable/
                                                                      Mask */
 #define AT91C_PMC_LOCKB (0x1 <<  2) /* (PMC) PLL B Status/Enable/Disable/
                                                                      Mask */
 #define AT91C_PMC_MCKRDY (0x1 <<  3) /* (PMC) Master Clock Status/Enable/
 Disable/Mask */
 #define AT91C_PMC_PCK0RDY (0x1 <<  8) /* (PMC) PCK0_RDY Status/Enable/
 Disable/Mask */
 #define AT91C_PMC_PCK1RDY (0x1 <<  9) /* (PMC) PCK1_RDY Status/Enable/
 Disable/Mask */
 /* ---PMC_IDR : (PMC Offset: 0x64) PMC Interrupt Disable Register -------- */
 /* ------ PMC_SR : (PMC Offset: 0x68) PMC Status Register -------- */
 #define AT91C_PMC_OSCSEL      (0x1 <<  7) /* (PMC) 32kHz Oscillator
 selection status */
 /* -------- PMC_IMR : (PMC Offset: 0x6c) PMC Interrupt Mask Register ---- */

/* SOFTWARE API DEFINITION  FOR Advanced Interrupt Controller */
/* -------- AIC_SMR : (AIC Offset: 0x0) Control Register -------- */
#define AT91C_AIC_PRIOR       (0x7 <<  0) /* (AIC) Priority Level */
#define     AT91C_AIC_PRIOR_LOWEST               (0x0) /* (AIC) Lowest priority
                                                            level */
#define     AT91C_AIC_PRIOR_HIGHEST              (0x7) /* (AIC) Highest
                                                             priority level */
#define AT91C_AIC_SRCTYPE     (0x3 <<  5) /* (AIC) Interrupt Source Type */
#define     AT91C_AIC_SRCTYPE_INT_LEVEL_SENSITIVE  (0x0 <<  5) /* (AIC)
                                Internal Sources Code Label Level Sensitive */
#define     AT91C_AIC_SRCTYPE_INT_EDGE_TRIGGERED   (0x1 <<  5) /* (AIC)
                                 Internal Sources Code Label Edge triggered */
#define     AT91C_AIC_SRCTYPE_EXT_HIGH_LEVEL       (0x2 <<  5) /* (AIC)
                            External Sources Code Label High-level Sensitive */
#define     AT91C_AIC_SRCTYPE_EXT_POSITIVE_EDGE    (0x3 <<  5) /* (AIC)
                         External Sources Code Label Positive Edge triggered */
/* - AIC_CISR : (AIC Offset: 0x114) AIC Core Interrupt Status Register ----- */
#define AT91C_AIC_NFIQ        (0x1 <<  0) /* (AIC) NFIQ Status */
#define AT91C_AIC_NIRQ        (0x1 <<  1) /* (AIC) NIRQ Status */
/* - AIC_DCR : (AIC Offset: 0x138) AIC Debug Control Register (Protect) ---- */
#define AT91C_AIC_DCR_PROT    (0x1 <<  0) /* (AIC) Protection Mode */
#define AT91C_AIC_DCR_GMSK    (0x1 <<  1) /* (AIC) General Mask */

/* Register definition for AIC peripheral */
#define AT91C_AIC_IVR   (*(volatile unsigned long*)  0xFFFFF100) /* (AIC) IRQ
Vector Register */
#define AIC_IVR 0x00000100
/* (AIC) Source Mode Register */
#define AT91C_AIC_SMR(a) (*(volatile unsigned long*)  (0xFFFFF000 + 4*(a)))
#define AT91C_AIC_FVR   (*(volatile unsigned long*)  0xFFFFF104) /* (AIC) FIQ
Vector Register */
#define AT91C_AIC_DCR   (*(volatile unsigned long*)  0xFFFFF138) /* (AIC) Debug
Control Register (Protect) */
#define AT91C_AIC_EOICR (*(volatile unsigned long*)  0xFFFFF130) /* (AIC) End
of Interrupt Command Register */
#define AIC_EOICR 0x00000130
/* (AIC) Source Vector Register */
#define AT91C_AIC_SVR(a) (*(volatile unsigned long*)  (0xFFFFF080 + 4*(a)))
#define AT91C_AIC_FFSR  (*(volatile unsigned long*)  0xFFFFF148) /* (AIC) Fast
Forcing Status Register */
#define AT91C_AIC_ICCR  (*(volatile unsigned long*)  0xFFFFF128) /* (AIC)
Interrupt Clear Command Register */
#define AT91C_AIC_ISR   (*(volatile unsigned long*)  0xFFFFF108) /* (AIC)
Interrupt Status Register */
#define AT91C_AIC_IMR   (*(volatile unsigned long*)  0xFFFFF110) /* (AIC)
Interrupt Mask Register */
#define AT91C_AIC_IPR   (*(volatile unsigned long*)  0xFFFFF10C) /* (AIC)
Interrupt Pending Register */
#define AT91C_AIC_FFER  (*(volatile unsigned long*)  0xFFFFF140) /* (AIC)
Fast Forcing Enable Register */
#define AT91C_AIC_IECR  (*(volatile unsigned long*)  0xFFFFF120) /* (AIC)
Interrupt Enable Command Register */
#define AT91C_AIC_ISCR  (*(volatile unsigned long*)  0xFFFFF12C) /* (AIC)
Interrupt Set Command Register */
#define AT91C_AIC_FFDR  (*(volatile unsigned long*)  0xFFFFF144) /* (AIC)
Fast Forcing Disable Register */
#define AT91C_AIC_CISR  (*(volatile unsigned long*)  0xFFFFF114) /* (AIC)
Core Interrupt Status Register */
#define AT91C_AIC_IDCR  (*(volatile unsigned long*)  0xFFFFF124) /* (AIC)
Interrupt Disable Command Register */
#define AT91C_AIC_SPU   (*(volatile unsigned long*)  0xFFFFF134) /* (AIC)
Spurious Vector Register */

/* SOFTWARE API DEFINITION  FOR Periodic Interval Timer Controller Interface */
/* PITC_PIMR : (PITC Offset: 0x0) Periodic Interval Mode Register -------- */
#define AT91C_PITC_PIV        (0xFFFFF <<  0) /* (PITC) Periodic Interval
Value */
#define AT91C_PITC_PITEN      (0x1 << 24) /* (PITC) Periodic Interval Timer
Enabled */
#define AT91C_PITC_PITIEN     (0x1 << 25) /* (PITC) Periodic Interval Timer
Interrupt Enable */
/* --- PITC_PISR : (PITC Offset: 0x4) Periodic Interval Status Register - */
#define AT91C_PITC_PITS       (0x1 <<  0) /* (PITC) Periodic Interval Timer
Status */
/* - PITC_PIVR : (PITC Offset: 0x8) Periodic Interval Value Register ---- */
#define AT91C_PITC_CPIV       (0xFFFFF <<  0) /* (PITC) Current Periodic
Interval Value */
#define AT91C_PITC_PICNT (0xFFF << 20) /* (PITC) Periodic Interval Counter */
/* PITC_PIIR : (PITC Offset: 0xc) Periodic Interval Image Register ---- */

/* Register definition for AIC peripheral */
#define AT91C_AIC_IVR   (*(volatile unsigned long*)  0xFFFFF100) /* (AIC) IRQ
Vector Register */
#define AIC_IVR 0x00000100
#define AT91C_AIC_EOICR (*(volatile unsigned long*)  0xFFFFF130) /* (AIC) End
of Interrupt Command Register */
#define AIC_EOICR 0x00000130

/* Register definition for PIOA peripheral */
#define AT91C_PIOA_ODR  (*(volatile unsigned long*)  0xFFFFF414) /* (PIOA)
Output Disable Registerr */
#define AT91C_PIOA_SODR (*(volatile unsigned long*)  0xFFFFF430) /* (PIOA)
Set Output Data Register */
#define AT91C_PIOA_ISR  (*(volatile unsigned long*)  0xFFFFF44C) /* (PIOA)
Interrupt Status Register */
#define AT91C_PIOA_ABSR (*(volatile unsigned long*)  0xFFFFF478) /* (PIOA)
AB Select Status Register */
#define AT91C_PIOA_IER  (*(volatile unsigned long*)  0xFFFFF440) /* (PIOA)
Interrupt Enable Register */
#define AT91C_PIOA_PPUDR (*(volatile unsigned long*)     0xFFFFF460) /* (PIOA)
Pull-up Disable Register */
#define AT91C_PIOA_IMR  (*(volatile unsigned long*)  0xFFFFF448) /* (PIOA)
Interrupt Mask Register */
#define AT91C_PIOA_PER  (*(volatile unsigned long*)  0xFFFFF400) /* (PIOA)
PIO Enable Register */
#define AT91C_PIOA_IFDR (*(volatile unsigned long*)  0xFFFFF424) /* (PIOA)
Input Filter Disable Register */
#define AT91C_PIOA_OWDR (*(volatile unsigned long*)  0xFFFFF4A4) /* (PIOA)
Output Write Disable Register */
#define AT91C_PIOA_MDSR (*(volatile unsigned long*)  0xFFFFF458) /* (PIOA)
Multi-driver Status Register */
#define AT91C_PIOA_IDR  (*(volatile unsigned long*)  0xFFFFF444) /* (PIOA)
Interrupt Disable Register */
#define AT91C_PIOA_ODSR (*(volatile unsigned long*)  0xFFFFF438) /* (PIOA)
Output Data Status Register */
#define AT91C_PIOA_PPUSR (*(volatile unsigned long*)  0xFFFFF468) /* (PIOA)
Pull-up Status Register */
#define AT91C_PIOA_OWSR (*(volatile unsigned long*)  0xFFFFF4A8) /* (PIOA)
Output Write Status Register */
#define AT91C_PIOA_BSR  (*(volatile unsigned long*)  0xFFFFF474) /* (PIOA)
Select B Register */
#define AT91C_PIOA_OWER (*(volatile unsigned long*)  0xFFFFF4A0) /* (PIOA)
Output Write Enable Register */
#define AT91C_PIOA_IFER (*(volatile unsigned long*)  0xFFFFF420) /* (PIOA)
Input Filter Enable Register */
#define AT91C_PIOA_PDSR (*(volatile unsigned long*)  0xFFFFF43C) /* (PIOA)
Pin Data Status Register */
#define AT91C_PIOA_PPUER (*(volatile unsigned long*)  0xFFFFF464) /* (PIOA)
Pull-up Enable Register */
#define AT91C_PIOA_OSR  (*(volatile unsigned long*)  0xFFFFF418) /* (PIOA)
Output Status Register */
#define AT91C_PIOA_ASR  (*(volatile unsigned long*)  0xFFFFF470) /* (PIOA)
Select A Register */
#define AT91C_PIOA_MDDR (*(volatile unsigned long*)  0xFFFFF454) /* (PIOA)
Multi-driver Disable Register */
#define AT91C_PIOA_CODR (*(volatile unsigned long*)  0xFFFFF434) /* (PIOA)
Clear Output Data Register */
#define AT91C_PIOA_MDER (*(volatile unsigned long*)  0xFFFFF450) /* (PIOA)
Multi-driver Enable Register */
#define AT91C_PIOA_PDR  (*(volatile unsigned long*)  0xFFFFF404) /* (PIOA)
PIO Disable Register */
#define AT91C_PIOA_IFSR (*(volatile unsigned long*)  0xFFFFF428) /* (PIOA)
Input Filter Status Register */
#define AT91C_PIOA_OER  (*(volatile unsigned long*)  0xFFFFF410) /* (PIOA)
Output Enable Register */
#define AT91C_PIOA_PSR  (*(volatile unsigned long*)  0xFFFFF408) /* (PIOA)
PIO Status Register */

/* Register definition for PIOB peripheral */
#define AT91C_PIOB_OWDR (*(volatile unsigned long*)  0xFFFFF6A4) /* (PIOB)
Output Write Disable Register */
#define AT91C_PIOB_MDER (*(volatile unsigned long*)  0xFFFFF650) /* (PIOB)
Multi-driver Enable Register */
#define AT91C_PIOB_PPUSR (*(volatile unsigned long*)     0xFFFFF668) /* (PIOB)
Pull-up Status Register */
#define AT91C_PIOB_IMR  (*(volatile unsigned long*)  0xFFFFF648) /* (PIOB)
Interrupt Mask Register */
#define AT91C_PIOB_ASR  (*(volatile unsigned long*)  0xFFFFF670) /* (PIOB)
Select A Register */
#define AT91C_PIOB_PPUDR (*(volatile unsigned long*)     0xFFFFF660) /* (PIOB)
Pull-up Disable Register */
#define AT91C_PIOB_PSR  (*(volatile unsigned long*)  0xFFFFF608) /* (PIOB)
PIO Status Register */
#define AT91C_PIOB_IER  (*(volatile unsigned long*)  0xFFFFF640) /* (PIOB)
Interrupt Enable Register */
#define AT91C_PIOB_CODR (*(volatile unsigned long*)  0xFFFFF634) /* (PIOB)
Clear Output Data Register */
#define AT91C_PIOB_OWER (*(volatile unsigned long*)  0xFFFFF6A0) /* (PIOB)
Output Write Enable Register */
#define AT91C_PIOB_ABSR (*(volatile unsigned long*)  0xFFFFF678) /* (PIOB)
AB Select Status Register */
#define AT91C_PIOB_IFDR (*(volatile unsigned long*)  0xFFFFF624) /* (PIOB)
Input Filter Disable Register */
#define AT91C_PIOB_PDSR (*(volatile unsigned long*)  0xFFFFF63C) /* (PIOB)
Pin Data Status Register */
#define AT91C_PIOB_IDR  (*(volatile unsigned long*)  0xFFFFF644) /* (PIOB)
Interrupt Disable Register */
#define AT91C_PIOB_OWSR (*(volatile unsigned long*)  0xFFFFF6A8) /* (PIOB)
Output Write Status Register */
#define AT91C_PIOB_PDR  (*(volatile unsigned long*)  0xFFFFF604) /* (PIOB)
PIO Disable Register */
#define AT91C_PIOB_ODR  (*(volatile unsigned long*)  0xFFFFF614) /* (PIOB)
Output Disable Registerr */
#define AT91C_PIOB_IFSR (*(volatile unsigned long*)  0xFFFFF628) /* (PIOB)
Input Filter Status Register */
#define AT91C_PIOB_PPUER (*(volatile unsigned long*)     0xFFFFF664) /* (PIOB)
Pull-up Enable Register */
#define AT91C_PIOB_SODR (*(volatile unsigned long*)  0xFFFFF630) /* (PIOB)
Set Output Data Register */
#define AT91C_PIOB_ISR  (*(volatile unsigned long*)  0xFFFFF64C) /* (PIOB)
Interrupt Status Register */
#define AT91C_PIOB_ODSR (*(volatile unsigned long*)  0xFFFFF638) /* (PIOB)
Output Data Status Register */
#define AT91C_PIOB_OSR  (*(volatile unsigned long*)  0xFFFFF618) /* (PIOB)
Output Status Register */
#define AT91C_PIOB_MDSR (*(volatile unsigned long*)  0xFFFFF658) /* (PIOB)
Multi-driver Status Register */
#define AT91C_PIOB_IFER (*(volatile unsigned long*)  0xFFFFF620) /* (PIOB)
Input Filter Enable Register */
#define AT91C_PIOB_BSR  (*(volatile unsigned long*)  0xFFFFF674) /* (PIOB)
Select B Register */
#define AT91C_PIOB_MDDR (*(volatile unsigned long*)  0xFFFFF654) /* (PIOB)
Multi-driver Disable Register */
#define AT91C_PIOB_OER  (*(volatile unsigned long*)  0xFFFFF610) /* (PIOB)
Output Enable Register */
#define AT91C_PIOB_PER  (*(volatile unsigned long*)  0xFFFFF600) /* (PIOB)
PIO Enable Register */

/* Register definition for PIOC peripheral */
#define AT91C_PIOC_OWDR (*(volatile unsigned long*)  0xFFFFF8A4) /* (PIOC)
Output Write Disable Register */
#define AT91C_PIOC_SODR (*(volatile unsigned long*)  0xFFFFF830) /* (PIOC)
Set Output Data Register */
#define AT91C_PIOC_PPUER (*(volatile unsigned long*)     0xFFFFF864) /* (PIOC)
Pull-up Enable Register */
#define AT91C_PIOC_CODR (*(volatile unsigned long*)  0xFFFFF834) /* (PIOC)
Clear Output Data Register */
#define AT91C_PIOC_PSR  (*(volatile unsigned long*)  0xFFFFF808) /* (PIOC)
PIO Status Register */
#define AT91C_PIOC_PDR  (*(volatile unsigned long*)  0xFFFFF804) /* (PIOC)
PIO Disable Register */
#define AT91C_PIOC_ODR  (*(volatile unsigned long*)  0xFFFFF814) /* (PIOC)
Output Disable Register */
#define AT91C_PIOC_PPUSR (*(volatile unsigned long*)     0xFFFFF868) /* (PIOC)
Pull-up Status Register */
#define AT91C_PIOC_ABSR (*(volatile unsigned long*)  0xFFFFF878) /* (PIOC)
AB Select Status Register */
#define AT91C_PIOC_IFSR (*(volatile unsigned long*)  0xFFFFF828) /* (PIOC)
Input Filter Status Register */
#define AT91C_PIOC_OER  (*(volatile unsigned long*)  0xFFFFF810) /* (PIOC)
Output Enable Register */
#define AT91C_PIOC_IMR  (*(volatile unsigned long*)  0xFFFFF848) /* (PIOC)
Interrupt Mask Register */
#define AT91C_PIOC_ASR  (*(volatile unsigned long*)  0xFFFFF870) /* (PIOC)
Select A Register */
#define AT91C_PIOC_MDDR (*(volatile unsigned long*)  0xFFFFF854) /* (PIOC)
Multi-driver Disable Register */
#define AT91C_PIOC_OWSR (*(volatile unsigned long*)  0xFFFFF8A8) /* (PIOC)
Output Write Status Register */
#define AT91C_PIOC_PER  (*(volatile unsigned long*)  0xFFFFF800) /* (PIOC)
PIO Enable Register */
#define AT91C_PIOC_IDR  (*(volatile unsigned long*)  0xFFFFF844) /* (PIOC)
Interrupt Disable Register */
#define AT91C_PIOC_MDER (*(volatile unsigned long*)  0xFFFFF850) /* (PIOC)
Multi-driver Enable Register */
#define AT91C_PIOC_PDSR (*(volatile unsigned long*)  0xFFFFF83C) /* (PIOC)
Pin Data Status Register */
#define AT91C_PIOC_MDSR (*(volatile unsigned long*)  0xFFFFF858) /* (PIOC)
Multi-driver Status Register */
#define AT91C_PIOC_OWER (*(volatile unsigned long*)  0xFFFFF8A0) /* (PIOC)
Output Write Enable Register */
#define AT91C_PIOC_BSR  (*(volatile unsigned long*)  0xFFFFF874) /* (PIOC)
Select B Register */
#define AT91C_PIOC_PPUDR (*(volatile unsigned long*)     0xFFFFF860) /* (PIOC)
Pull-up Disable Register */
#define AT91C_PIOC_IFDR (*(volatile unsigned long*)  0xFFFFF824) /* (PIOC)
Input Filter Disable Register */
#define AT91C_PIOC_IER  (*(volatile unsigned long*)  0xFFFFF840) /* (PIOC)
Interrupt Enable Register */
#define AT91C_PIOC_OSR  (*(volatile unsigned long*)  0xFFFFF818) /* (PIOC)
Output Status Register */
#define AT91C_PIOC_ODSR (*(volatile unsigned long*)  0xFFFFF838) /* (PIOC)
Output Data Status Register */
#define AT91C_PIOC_ISR  (*(volatile unsigned long*)  0xFFFFF84C) /* (PIOC)
Interrupt Status Register */
#define AT91C_PIOC_IFER (*(volatile unsigned long*)  0xFFFFF820) /* (PIOC)
Input Filter Enable Register */

/* Register definition for PMC peripheral */
#define AT91C_PMC_PCER  (*(volatile unsigned long*)  0xFFFFFC10) /* (PMC)
Peripheral Clock Enable Register */
#define AT91C_PMC_PCKR  (*(volatile unsigned long*)  0xFFFFFC40) /* (PMC)
Programmable Clock Register */
#define AT91C_PMC_MCKR  (*(volatile unsigned long*)  0xFFFFFC30) /* (PMC)
Master Clock Register */
#define AT91C_PMC_PLLAR (*(volatile unsigned long*)  0xFFFFFC28) /* (PMC)
PLL A Register */
#define AT91C_PMC_PCDR  (*(volatile unsigned long*)  0xFFFFFC14) /* (PMC)
Peripheral Clock Disable Register */
#define AT91C_PMC_SCSR  (*(volatile unsigned long*)  0xFFFFFC08) /* (PMC)
System Clock Status Register */
#define AT91C_PMC_MCFR  (*(volatile unsigned long*)  0xFFFFFC24) /* (PMC)
Main Clock  Frequency Register */
#define AT91C_PMC_IMR   (*(volatile unsigned long*)  0xFFFFFC6C) /* (PMC)
Interrupt Mask Register */
#define AT91C_PMC_IER   (*(volatile unsigned long*)  0xFFFFFC60) /* (PMC)
Interrupt Enable Register */
#define AT91C_PMC_MOR   (*(volatile unsigned long *) 0xFFFFFC20) /* (PMC)
Main Oscillator Register */
#define AT91C_PMC_IDR   (*(volatile unsigned long *)  0xFFFFFC64) /* (PMC)
Interrupt Disable Register */
#define AT91C_PMC_PLLBR (*(volatile unsigned long*)  0xFFFFFC2C) /* (PMC)
PLL B Register */
#define AT91C_PMC_SCDR  (*(volatile unsigned long*)  0xFFFFFC04) /* (PMC)
System Clock Disable Register */
#define AT91C_PMC_PCSR  (*(volatile unsigned long*)  0xFFFFFC18) /* (PMC)
Peripheral Clock Status Register */
#define AT91C_PMC_SCER  (*(volatile unsigned long*)  0xFFFFFC00) /* (PMC)
System Clock Enable Register */
#define AT91C_PMC_SR    (*(volatile unsigned long*)  0xFFFFFC68) /* (PMC)
Status Register */

/* Register definition for PITC peripheral */
#define AT91C_PITC_PIVR (*(volatile unsigned long*)  0xFFFFFD38) /* (PITC)
Period Interval Value Register */
#define AT91C_PITC_PISR (*(volatile unsigned long*)  0xFFFFFD34) /* (PITC)
Period Interval Status Register */
#define AT91C_PITC_PIIR (*(volatile unsigned long*)  0xFFFFFD3C) /* (PITC)
Period Interval Image Register */
#define AT91C_PITC_PIMR (*(volatile unsigned long*)  0xFFFFFD30) /* (PITC)
Period Interval Mode Register */

/* PIO DEFINITIONS FOR AT91SAM9260 */
#define AT91C_PIO_PA0        (1 <<  0) /* Pin Controlled by PA0 */
#define AT91C_PIO_PA1        (1 <<  1) /* Pin Controlled by PA1 */
#define AT91C_PIO_PA10       (1 << 10) /* Pin Controlled by PA10 */
#define AT91C_PIO_PA11       (1 << 11) /* Pin Controlled by PA11 */
#define AT91C_PIO_PA12       (1 << 12) /* Pin Controlled by PA12 */
#define AT91C_PIO_PA13       (1 << 13) /* Pin Controlled by PA13 */
#define AT91C_PIO_PA14       (1 << 14) /* Pin Controlled by PA14 */
#define AT91C_PIO_PA15       (1 << 15) /* Pin Controlled by PA15 */
#define AT91C_PIO_PA16       (1 << 16) /* Pin Controlled by PA16 */
#define AT91C_PIO_PA17       (1 << 17) /* Pin Controlled by PA17 */
#define AT91C_PIO_PA18       (1 << 18) /* Pin Controlled by PA18 */
#define AT91C_PIO_PA19       (1 << 19) /* Pin Controlled by PA19 */
#define AT91C_PIO_PA2        (1 <<  2) /* Pin Controlled by PA2 */
#define AT91C_PIO_PA20       (1 << 20) /* Pin Controlled by PA20 */
#define AT91C_PIO_PA21       (1 << 21) /* Pin Controlled by PA21 */
#define AT91C_PIO_PA22       (1 << 22) /* Pin Controlled by PA22 */
#define AT91C_PIO_PA23       (1 << 23) /* Pin Controlled by PA23 */
#define AT91C_PIO_PA24       (1 << 24) /* Pin Controlled by PA24 */
#define AT91C_PIO_PA25       (1 << 25) /* Pin Controlled by PA25 */
#define AT91C_PIO_PA26       (1 << 26) /* Pin Controlled by PA26 */
#define AT91C_PIO_PA27       (1 << 27) /* Pin Controlled by PA27 */
#define AT91C_PIO_PA28       (1 << 28) /* Pin Controlled by PA28 */
#define AT91C_PIO_PA29       (1 << 29) /* Pin Controlled by PA29 */
#define AT91C_PIO_PA3        (1 <<  3) /* Pin Controlled by PA3 */
#define AT91C_PIO_PA30       (1 << 30) /* Pin Controlled by PA30 */
#define AT91C_PIO_PA31       (1 << 31) /* Pin Controlled by PA31 */
#define AT91C_PIO_PA4        (1 <<  4) /* Pin Controlled by PA4 */
#define AT91C_PIO_PA5        (1 <<  5) /* Pin Controlled by PA5 */
#define AT91C_PIO_PA6        (1 <<  6) /* Pin Controlled by PA6 */
#define AT91C_PIO_PA7        (1 <<  7) /* Pin Controlled by PA7 */
#define AT91C_PIO_PA8        (1 <<  8) /* Pin Controlled by PA8 */
#define AT91C_PIO_PA9        (1 <<  9) /* Pin Controlled by PA9 */
#define AT91C_PIO_PB0        (1 <<  0) /* Pin Controlled by PB0 */
#define AT91C_PIO_PB1        (1 <<  1) /* Pin Controlled by PB1 */
#define AT91C_PIO_PB10       (1 << 10) /* Pin Controlled by PB10 */
#define AT91C_PIO_PB11       (1 << 11) /* Pin Controlled by PB11 */
#define AT91C_PIO_PB12       (1 << 12) /* Pin Controlled by PB12 */
#define AT91C_PIO_PB13       (1 << 13) /* Pin Controlled by PB13 */
#define AT91C_PIO_PB14       (1 << 14) /* Pin Controlled by PB14 */
#define AT91C_PIO_PB15       (1 << 15) /* Pin Controlled by PB15 */
#define AT91C_PIO_PB16       (1 << 16) /* Pin Controlled by PB16 */
#define AT91C_PIO_PB17       (1 << 17) /* Pin Controlled by PB17 */
#define AT91C_PIO_PB18       (1 << 18) /* Pin Controlled by PB18 */
#define AT91C_PIO_PB19       (1 << 19) /* Pin Controlled by PB19 */
#define AT91C_PIO_PB2        (1 <<  2) /* Pin Controlled by PB2 */
#define AT91C_PIO_PB20       (1 << 20) /* Pin Controlled by PB20 */
#define AT91C_PIO_PB21       (1 << 21) /* Pin Controlled by PB21 */
#define AT91C_PIO_PB22       (1 << 22) /* Pin Controlled by PB22 */
#define AT91C_PIO_PB23       (1 << 23) /* Pin Controlled by PB23 */
#define AT91C_PIO_PB24       (1 << 24) /* Pin Controlled by PB24 */
#define AT91C_PIO_PB25       (1 << 25) /* Pin Controlled by PB25 */
#define AT91C_PIO_PB26       (1 << 26) /* Pin Controlled by PB26 */
#define AT91C_PIO_PB27       (1 << 27) /* Pin Controlled by PB27 */
#define AT91C_PIO_PB28       (1 << 28) /* Pin Controlled by PB28 */
#define AT91C_PIO_PB29       (1 << 29) /* Pin Controlled by PB29 */
#define AT91C_PIO_PB3        (1 <<  3) /* Pin Controlled by PB3 */
#define AT91C_PIO_PB30       (1 << 30) /* Pin Controlled by PB30 */
#define AT91C_PIO_PB31       (1 << 31) /* Pin Controlled by PB31 */
#define AT91C_PIO_PB4        (1 <<  4) /* Pin Controlled by PB4 */
#define AT91C_PIO_PB5        (1 <<  5) /* Pin Controlled by PB5 */
#define AT91C_PIO_PB6        (1 <<  6) /* Pin Controlled by PB6 */
#define AT91C_PIO_PB7        (1 <<  7) /* Pin Controlled by PB7 */
#define AT91C_PIO_PB8        (1 <<  8) /* Pin Controlled by PB8 */
#define AT91C_PIO_PB9        (1 <<  9) /* Pin Controlled by PB9 */
#define AT91C_PIO_PC0        (1 <<  0) /* Pin Controlled by PC0 */
#define AT91C_PIO_PC1        (1 <<  1) /* Pin Controlled by PC1 */
#define AT91C_PIO_PC10       (1 << 10) /* Pin Controlled by PC10 */
#define AT91C_PIO_PC11       (1 << 11) /* Pin Controlled by PC11 */
#define AT91C_PIO_PC12       (1 << 12) /* Pin Controlled by PC12 */
#define AT91C_PIO_PC13       (1 << 13) /* Pin Controlled by PC13 */
#define AT91C_PIO_PC14       (1 << 14) /* Pin Controlled by PC14 */
#define AT91C_PIO_PC15       (1 << 15) /* Pin Controlled by PC15 */
#define AT91C_PIO_PC16       (1 << 16) /* Pin Controlled by PC16 */
#define AT91C_PIO_PC17       (1 << 17) /* Pin Controlled by PC17 */
#define AT91C_PIO_PC18       (1 << 18) /* Pin Controlled by PC18 */
#define AT91C_PIO_PC19       (1 << 19) /* Pin Controlled by PC19 */
#define AT91C_PIO_PC2        (1 <<  2) /* Pin Controlled by PC2 */
#define AT91C_PIO_PC20       (1 << 20) /* Pin Controlled by PC20 */
#define AT91C_PIO_PC21       (1 << 21) /* Pin Controlled by PC21 */
#define AT91C_PIO_PC22       (1 << 22) /* Pin Controlled by PC22 */
#define AT91C_PIO_PC23       (1 << 23) /* Pin Controlled by PC23 */
#define AT91C_PIO_PC24       (1 << 24) /* Pin Controlled by PC24 */
#define AT91C_PIO_PC25       (1 << 25) /* Pin Controlled by PC25 */
#define AT91C_PIO_PC26       (1 << 26) /* Pin Controlled by PC26 */
#define AT91C_PIO_PC27       (1 << 27) /* Pin Controlled by PC27 */
#define AT91C_PIO_PC28       (1 << 28) /* Pin Controlled by PC28 */
#define AT91C_PIO_PC29       (1 << 29) /* Pin Controlled by PC29 */
#define AT91C_PIO_PC3        (1 <<  3) /* Pin Controlled by PC3 */
#define AT91C_PIO_PC30       (1 << 30) /* Pin Controlled by PC30 */
#define AT91C_PIO_PC31       (1 << 31) /* Pin Controlled by PC31 */
#define AT91C_PIO_PC4        (1 <<  4) /* Pin Controlled by PC4 */
#define AT91C_PIO_PC5        (1 <<  5) /* Pin Controlled by PC5 */
#define AT91C_PIO_PC6        (1 <<  6) /* Pin Controlled by PC6 */
#define AT91C_PIO_PC7        (1 <<  7) /* Pin Controlled by PC7 */
#define AT91C_PIO_PC8        (1 <<  8) /* Pin Controlled by PC8 */
#define AT91C_PIO_PC9        (1 <<  9) /* Pin Controlled by PC9 */

/* PERIPHERAL ID DEFINITIONS FOR AT91SAM9260 */
#define AT91C_ID_FIQ    ( 0) /* Advanced Interrupt Controller (FIQ) */
#define AT91C_ID_SYS    ( 1) /* System Controller */
#define AT91C_ID_PIOA   ( 2) /* Parallel IO Controller A */
#define AT91C_ID_PIOB   ( 3) /* Parallel IO Controller B */
#define AT91C_ID_PIOC   ( 4) /* Parallel IO Controller C */
#define AT91C_ID_ADC    ( 5) /* ADC */
#define AT91C_ID_US0    ( 6) /* USART 0 */
#define AT91C_ID_US1    ( 7) /* USART 1 */
#define AT91C_ID_US2    ( 8) /* USART 2 */
#define AT91C_ID_MCI    ( 9) /* Multimedia Card Interface 0 */
#define AT91C_ID_UDP    (10) /* USB Device Port */
#define AT91C_ID_TWI    (11) /* Two-Wire Interface */
#define AT91C_ID_SPI0   (12) /* Serial Peripheral Interface 0 */
#define AT91C_ID_SPI1   (13) /* Serial Peripheral Interface 1 */
#define AT91C_ID_SSC0   (14) /* Serial Synchronous Controller 0 */
#define AT91C_ID_TC0    (17) /* Timer Counter 0 */
#define AT91C_ID_TC1    (18) /* Timer Counter 1 */
#define AT91C_ID_TC2    (19) /* Timer Counter 2 */
#define AT91C_ID_UHP    (20) /* USB Host Port */
#define AT91C_ID_EMAC   (21) /* Ethernet Mac */
#define AT91C_ID_HISI   (22) /* Image Sensor Interface */
#define AT91C_ID_US3    (23) /* USART 3 */
#define AT91C_ID_US4    (24) /* USART 4 */
#define AT91C_ID_US5    (25) /* USART 5 */
#define AT91C_ID_TC3    (26) /* Timer Counter 3 */
#define AT91C_ID_TC4    (27) /* Timer Counter 4 */
#define AT91C_ID_TC5    (28) /* Timer Counter 5 */
#define AT91C_ID_IRQ0   (29) /* Advanced Interrupt Controller (IRQ0) */
#define AT91C_ID_IRQ1   (30) /* Advanced Interrupt Controller (IRQ1) */
#define AT91C_ID_IRQ2   (31) /* Advanced Interrupt Controller (IRQ2) */
#define AT91C_ALL_INT   (0xFFFE7FFF) /* ALL VALID INTERRUPTS */

/* MEMORY MAPPING DEFINITIONS FOR AT91SAM9260 */
#define AT91C_IRAM_1     (0x00200000) /* Maximum IRAM_1 Area : 4Kbyte
base address */
#define AT91C_IRAM_1_SIZE    (0x00001000) /* Maximum IRAM_1 Area : 4Kbyte size
 in byte (4 Kbytes) */
#define AT91C_EBI_SDRAM_32BIT    (0x20000000) /* SDRAM on EBI Chip Select 1
base address */
#define AT91C_BASE_AIC       0xFFFFF000 /* (AIC) Base Address */

/* Timer frequency */
/* timer is based on PCLK and minimum division is 2 */
#define TIMER_FREQ (49156800/2)
