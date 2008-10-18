/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Dave Chapman
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

#ifndef _WM8758_H
#define _WM8758_H

/* volume/balance/treble/bass interdependency */
#define VOLUME_MIN -570
#define VOLUME_MAX  60

#define AUDIOHW_CAPS (BASS_CAP | TREBLE_CAP | BASS_CUTOFF_CAP | TREBLE_CUTOFF_CAP)

extern int tenthdb2master(int db);
extern int tenthdb2mixer(int db);

extern void audiohw_set_master_vol(int vol_l, int vol_r);
extern void audiohw_set_lineout_vol(int vol_l, int vol_r);
extern void audiohw_set_mixer_vol(int channel1, int channel2);
extern void audiohw_set_sample_rate(int sampling_control);

#define RESET                   0x00
#define RESET_RESET             0x0

#define PWRMGMT1                0x01
#define PWRMGMT1_VMIDSEL_OFF    (0 << 0)
#define PWRMGMT1_VMIDSEL_75K    (1 << 0)
#define PWRMGMT1_VMIDSEL_300K   (2 << 0)
#define PWRMGMT1_VMIDSEL_5K     (3 << 0)
#define PWRMGMT1_BUFIOEN        (1 << 2)
#define PWRMGMT1_BIASEN         (1 << 3)
#define PWRMGMT1_MICBEN         (1 << 4)
#define PWRMGMT1_PLLEN          (1 << 5)
#define PWRMGMT1_OUT3MIXEN      (1 << 6)
#define PWRMGMT1_OUT4MIXEN      (1 << 7)
#define PWRMGMT1_BUFDCOPEN      (1 << 8)

#define PWRMGMT2                0x02
#define PWRMGMT2_ADCENL         (1 << 0)
#define PWRMGMT2_ADCENR         (1 << 1)
#define PWRMGMT2_INPGAENL       (1 << 2)
#define PWRMGMT2_INPGAENR       (1 << 3)
#define PWRMGMT2_BOOSTENL       (1 << 4)
#define PWRMGMT2_BOOSTENR       (1 << 5)
#define PWRMGMT2_SLEEP          (1 << 6)
#define PWRMGMT2_LOUT1EN        (1 << 7)
#define PWRMGMT2_ROUT1EN        (1 << 8)

#define PWRMGMT3                0x03
#define PWRMGMT3_DACENL         (1 << 0)
#define PWRMGMT3_DACENR         (1 << 1)
#define PWRMGMT3_LMIXEN         (1 << 2)
#define PWRMGMT3_RMIXEN         (1 << 3)
#define PWRMGMT3_ROUT2EN        (1 << 5)
#define PWRMGMT3_LOUT2EN        (1 << 6)
#define PWRMGMT3_OUT3EN         (1 << 7)
#define PWRMGMT3_OUT4EN         (1 << 8)

#define AINTFCE                 0x04
#define AINTFCE_MONO            (1 << 0)
#define AINTFCE_ALRSWAP         (1 << 1)
#define AINTFCE_DLRSWAP         (1 << 2)
#define AINTFCE_FORMAT_MSB_RJUST (0 << 3)
#define AINTFCE_FORMAT_MSB_LJUST (1 << 3)
#define AINTFCE_FORMAT_I2S      (2 << 3)
#define AINTFCE_FORMAT_DSP      (3 << 3)
#define AINTFCE_FORMAT_MASK     (3 << 3)
#define AINTFCE_IWL_16BIT       (0 << 5)
#define AINTFCE_IWL_20BIT       (1 << 5)
#define AINTFCE_IWL_24BIT       (2 << 5)
#define AINTFCE_IWL_32BIT       (3 << 5)
#define AINTFCE_IWL_MASK        (3 << 5)
#define AINTFCE_LRP             (1 << 7)
#define AINTFCE_BCP             (1 << 8)

#define COMPCTRL                0x05 /* unused */

#define CLKCTRL                 0x06
#define CLKCTRL_MS              (1 << 0)
#define CLKCTRL_BCLKDIV_1       (0 << 2)
#define CLKCTRL_BCLKDIV_2       (1 << 2)
#define CLKCTRL_BCLKDIV_4       (2 << 2)
#define CLKCTRL_BCLKDIV_8       (3 << 2)
#define CLKCTRL_BCLKDIV_16      (4 << 2)
#define CLKCTRL_BCLKDIV_32      (5 << 2)
#define CLKCTRL_MCLKDIV_1       (0 << 5)
#define CLKCTRL_MCLKDIV_1_5     (1 << 5)
#define CLKCTRL_MCLKDIV_2       (2 << 5)
#define CLKCTRL_MCLKDIV_3       (3 << 5)
#define CLKCTRL_MCLKDIV_4       (4 << 5)
#define CLKCTRL_MCLKDIV_6       (5 << 5)
#define CLKCTRL_MCLKDIV_8       (6 << 5)
#define CLKCTRL_MCLKDIV_12      (7 << 5)
#define CLKCTRL_CLKSEL          (1 << 8)

#define ADDCTRL                 0x07
#define ADDCTRL_SLOWCLKEN       (1 << 0)
#define ADDCTRL_SR_48kHz        (0 << 1)
#define ADDCTRL_SR_32kHz        (1 << 1)
#define ADDCTRL_SR_24kHz        (2 << 1)
#define ADDCTRL_SR_16kHz        (3 << 1)
#define ADDCTRL_SR_12kHz        (4 << 1)
#define ADDCTRL_SR_8kHz         (5 << 1)
#define ADDCTRL_SR_MASK         (7 << 1)

/* unused */
#define GPIOCTRL                0x08
#define JACKDETECTCTRL1         0x09

#define DACCTRL                 0x0a
#define DACCTRL_DACLPOL         (1 << 0)
#define DACCTRL_DACRPOL         (1 << 1)
#define DACCTRL_AMUTE           (1 << 2)
#define DACCTRL_DACOSR128       (1 << 3)
#define DACCTRL_SOFTMUTE        (1 << 6)

#define LDACVOL                 0x0b
#define LDACVOL_MASK            0xff
#define LDACVOL_DACVU           (1 << 8)

#define RDACVOL                 0x0c
#define RDACVOL_MASK            0xff
#define RDACVOL_DACVU           (1 << 8)

#define JACKDETECTCTRL2         0x0d /* unused */

#define ADCCTRL                 0x0e
#define ADCCTRL_ADCLPOL         (1 << 0)
#define ADCCTRL_ADCRPOL         (1 << 1)
#define ADCCTRL_ADCOSR128       (1 << 3)
#define ADCCTRL_HPFCUT_MASK     (7 << 4)
#define ADCCTRL_HPFAPP          (1 << 7)
#define ADCCTRL_HPFEN           (1 << 8)

#define LADCVOL                 0x0f
#define LADCVOL_MASK            0xff
#define LADCVOL_ADCVU           (1 << 8)

#define RADCVOL                 0x10
#define RADCVOL_MASK            0xff
#define RADCVOL_ADCVU           (1 << 8)

#define EQ1                     0x12
#define EQ5                     0x16
/* note: the WM8983 used for reference has a true 5 band EQ, but the WM8758
 * does only have low shelf & high shelf (tested). Not sure about 3D mode. */
#define EQ1_EQ3DMODE            (1 << 8)
#define EQ_GAIN_MASK            0x1f
#define EQ_CUTOFF_MASK          (3 << 5)
#define EQ_GAIN_VALUE(x)        (((-x) + 12) & 0x1f)
#define EQ_CUTOFF_VALUE(x)      ((((x) - 1) & 0x03) << 5)

/* unused */
#define DACLIMITER1             0x18
#define DACLIMITER2             0x19
#define NOTCHFILTER1            0x1b
#define NOTCHFILTER2            0x1c
#define NOTCHFILTER3            0x1d
#define NOTCHFILTER4            0x1e
#define ALCCONTROL1             0x20
#define ALCCONTROL2             0x21
#define ALCCONTROL3             0x22
#define NOISEGATE               0x23

#define PLLN                    0x24
#define PLLN_PLLN_MASK          0x0f
#define PLLN_PLLPRESCALE        (1 << 4)

#define PLLK1                   0x25
#define PLLK1_MASK              0x3f

#define PLLK2                   0x26
#define PLLK3                   0x27

#define THREEDCTRL              0x29
#define THREEDCTRL_DEPTH3D_MASK 0x0f

#define OUT4TOADC               0x2a
#define OUT4TOADC_OUT1DEL       (1 << 0)
#define OUT4TOADC_DELEN         (1 << 1)
#define OUT4TOADC_POBCTRL       (1 << 2)
#define OUT4TOADC_OUT4_2LNR     (1 << 5)
#define OUT4TOADC_OUT4_ADCVOL_MASK (7 << 6)

#define BEEPCTRL                0x2b
#define BEEPCTRL_BEEPEN         (1 << 0)
#define BEEPCTRL_BEEPVOL_MASK   (7 << 1)
#define BEEPCTRL_INVROUT2       (1 << 4)
#define BEEPCTRL_MUTERPGA2INV   (1 << 5)
#define BEEPCTRL_BYPR2LMIX      (1 << 7)
#define BEEPCTRL_BYPL2RMIX      (1 << 8)

#define INCTRL                  0x2c
#define INCTRL_LIP2INPGA        (1 << 0)
#define INCTRL_LIN2INPGA        (1 << 1)
#define INCTRL_L2_2INPGA        (1 << 2)
#define INCTRL_RIP2INPGA        (1 << 4)
#define INCTRL_RIN2INPGA        (1 << 5)
#define INCTRL_R2_2INPGA        (1 << 6)
#define INCTRL_MBVSEL           (1 << 8)

#define LINPGAVOL               0x2d
#define LINPGAVOL_INPGAVOL_MASK 0x3f
#define LINPGAVOL_INPGAMUTEL    (1 << 6)
#define LINPGAVOL_INPGAZCL      (1 << 7)
#define LINPGAVOL_INPGAVU       (1 << 8)

#define RINPGAVOL               0x2e
#define RINPGAVOL_INPGAVOL_MASK 0x3f
#define RINPGAVOL_INPGAMUTER    (1 << 6)
#define RINPGAVOL_INPGAZCR      (1 << 7)
#define RINPGAVOL_INPGAVU       (1 << 8)

#define LADCBOOST               0x2f
#define LADCBOOST_AUXL2BOOST_MASK (7 << 0)
#define LADCBOOST_L2_2BOOST_MASK  (7 << 4)
#define LADCBOOST_L2_2BOOST(x)  ((x) << 4)
#define LADCBOOST_PGABOOSTL     (1 << 8)

#define RADCBOOST               0x30
#define RADCBOOST_AUXR2BOOST_MASK (7 << 0)
#define RADCBOOST_R2_2BOOST_MASK  (7 << 4)
#define RADCBOOST_R2_2BOOST(x)  ((x) << 4)
#define RADCBOOST_PGABOOSTR     (1 << 8)

#define OUTCTRL                 0x31
#define OUTCTRL_VROI            (1 << 0)
#define OUTCTRL_TSDEN           (1 << 1)
#define OUTCTRL_SPKBOOST        (1 << 2)
#define OUTCTRL_OUT3BOOST       (1 << 3)
#define OUTCTRL_OUT4BOOST       (1 << 4)
#define OUTCTRL_DACR2LMIX       (1 << 5)
#define OUTCTRL_DACL2RMIX       (1 << 6)

#define LOUTMIX                 0x32
#define LOUTMIX_DACL2LMIX       (1 << 0)
#define LOUTMIX_BYPL2LMIX       (1 << 1)
#define LOUTMIX_BYP2LMIXVOL_MASK (7 << 2)
#define LOUTMIX_BYP2LMIXVOL(x)  ((x) << 2)
#define LOUTMIX_AUXL2LMIX       (1 << 5)
#define LOUTMIX_AUXLMIXVOL_MASK (7 << 6)

#define ROUTMIX                 0x33
#define ROUTMIX_DACR2RMIX       (1 << 0)
#define ROUTMIX_BYPR2RMIX       (1 << 1)
#define ROUTMIX_BYP2RMIXVOL_MASK (7 << 2)
#define ROUTMIX_BYP2RMIXVOL(x)  ((x) << 2)
#define ROUTMIX_AUXR2RMIX       (1 << 5)
#define ROUTMIX_AUXRMIXVOL_MASK (7 << 6)

#define LOUT1VOL                0x34
#define LOUT1VOL_MASK           0x3f
#define LOUT1VOL_LOUT1MUTE      (1 << 6)
#define LOUT1VOL_LOUT1ZC        (1 << 7)
#define LOUT1VOL_OUT1VU         (1 << 8)

#define ROUT1VOL                0x35
#define ROUT1VOL_MASK           0x3f
#define ROUT1VOL_ROUT1MUTE      (1 << 6)
#define ROUT1VOL_ROUT1ZC        (1 << 7)
#define ROUT1VOL_OUT1VU         (1 << 8)

#define LOUT2VOL                0x36
#define LOUT2VOL_MASK           0x3f
#define LOUT2VOL_LOUT2MUTE      (1 << 6)
#define LOUT2VOL_LOUT2ZC        (1 << 7)
#define LOUT2VOL_OUT2VU         (1 << 8)

#define ROUT2VOL                0x37
#define ROUT2VOL_MASK           0x3f
#define ROUT2VOL_ROUT2MUTE      (1 << 6)
#define ROUT2VOL_ROUT2ZC        (1 << 7)
#define ROUT2VOL_OUT2VU         (1 << 8)


/* Dummy definition, to be removed when the audio driver API gets reworked. */
#define WM8758_44100HZ     0

#endif /* _WM8758_H */
