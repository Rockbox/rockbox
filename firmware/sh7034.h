/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __SH7034_H__
#define __SH7034_H__

#define GBR           0x00000000

#define SCISMR0       0x05FFFEC0
#define SCIBRR0       0x05FFFEC1
#define SCISCR0       0x05FFFEC2
#define SCITDR0       0x05FFFEC3
#define SCISSR0       0x05FFFEC4
#define SCIRDR0       0x05FFFEC5
#define SCISMR1       0x05FFFEC8
#define SCIBRR1       0x05FFFEC9
#define SCISCR1       0x05FFFECA
#define SCITDR1       0x05FFFECB
#define SCISSR1       0x05FFFECC
#define SCIRDR1       0x05FFFECD

#define ADDRA         0x05FFFEE0
#define ADDRAH        0x05FFFEE0
#define ADDRAL        0x05FFFEE1
#define ADDRB         0x05FFFEE2
#define ADDRBH        0x05FFFEE2
#define ADDRBL        0x05FFFEE3
#define ADDRC         0x05FFFEE4
#define ADDRCH        0x05FFFEE4
#define ADDRCL        0x05FFFEE5
#define ADDRD         0x05FFFEE6
#define ADDRDH        0x05FFFEE6
#define ADDRDL        0x05FFFEE6
#define ADCSR         0x05FFFEE8
#define ADCR          0x05FFFEE9

#define ITUTSTR       0x05FFFF00
#define ITUTSNC       0x05FFFF01
#define ITUTMDR       0x05FFFF02
#define ITUTFCR       0x05FFFF03
#define ITUTCR0       0x05FFFF04
#define ITUTIOR0      0x05FFFF05
#define ITUTIER0      0x05FFFF06
#define ITUTSR0       0x05FFFF07
#define ITUTCNT0      0x05FFFF08
#define ITUGRA0       0x05FFFF0A
#define ITUGRB0       0x05FFFF0C
#define ITUTCR1       0x05FFFF0E
#define ITUTIOR1      0x05FFFF0F
#define ITUTIER1      0x05FFFF10
#define ITUTSR1       0x05FFFF11
#define ITUTCNT1      0x05FFFF12
#define ITUGRA1       0x05FFFF14
#define ITUGRB1       0x05FFFF16
#define ITUTCR2       0x05FFFF18
#define ITUTIOR2      0x05FFFF19
#define ITUTIER2      0x05FFFF1A
#define ITUTSR2       0x05FFFF1B
#define ITUTCNT2      0x05FFFF1C
#define ITUGRA2       0x05FFFF1E
#define ITUGRB2       0x05FFFF20
#define ITUTCR3       0x05FFFF22
#define ITUTIOR3      0x05FFFF23
#define ITUTIER3      0x05FFFF24
#define ITUTSR3       0x05FFFF25
#define ITUTCNT3      0x05FFFF26
#define ITUGRA3       0x05FFFF28
#define ITUGRB3       0x05FFFF2A
#define ITUBRA3       0x05FFFF2C
#define ITUBRB3       0x05FFFF2E
#define ITUTOCR       0x05FFFF31
#define ITUTCR4       0x05FFFF32
#define ITUTIOR4      0x05FFFF33
#define ITUTIER4      0x05FFFF34
#define ITUTSR4       0x05FFFF35
#define ITUTCNT4      0x05FFFF36
#define ITUGRA4       0x05FFFF38
#define ITUGRB4       0x05FFFF3A
#define ITUBRA4       0x05FFFF3C
#define ITUBRB4       0x05FFFF3E

#define DMACSAR0      0x05FFFF40
#define DMACDAR0      0x05FFFF44
#define DMACOR        0x05FFFF48
#define DMACTCR0      0x05FFFF4A
#define DMACCHCR0     0x05FFFF4E
#define DMACSAR1      0x05FFFF50
#define DMACDAR1      0x05FFFF54
#define DMACTCR1      0x05FFFF5A
#define DMACCHCR1     0x05FFFF5E
#define DMACSAR2      0x05FFFF60
#define DMACDAR2      0x05FFFF64
#define DMACTCR2      0x05FFFF6A
#define DMACCHCR2     0x05FFFF6E
#define DMACSAR3      0x05FFFF70
#define DMACDAR3      0x05FFFF74
#define DMACTCR3      0x05FFFF7A
#define DMACCHCR3     0x05FFFF7E

#define INTCIPRAB     0x05FFFF84
#define INTCIPRA      0x05FFFF84
#define INTCIPRB      0x05FFFF86
#define INTCIPRCD     0x05FFFF88
#define INTCIPRC      0x05FFFF88
#define INTCIPRD      0x05FFFF8A
#define INTCIPRE      0x05FFFF8C
#define INTCICR       0x05FFFF8E

#define UBCBAR        0x05FFFF90
#define UBCBARH       0x05FFFF90
#define UBCBARL       0x05FFFF92
#define UBCBAMR       0x05FFFF94
#define UBCBAMRH      0x05FFFF94
#define UBCBAMRL      0x05FFFF96
#define UBCBBR        0x05FFFF98

#define BSCBCR        0x05FFFFA0
#define BSCWCR1       0x05FFFFA2
#define BSCWCR2       0x05FFFFA4
#define BSCWCR3       0x05FFFFA6
#define BSCDCR        0x05FFFFA8
#define BSCPCR        0x05FFFFAA
#define BSCRCR        0x05FFFFAC
#define BSCRTCSR      0x05FFFFAE
#define BSCRTCNT      0x05FFFFB0
#define BSCRTCOR      0x05FFFFB2

#define WDTTCSR       0x05FFFFB8
#define WDTTCNT       0x05FFFFB9
#define WDTRSTCSR     0x05FFFFBB

#define SBYCR         0x05FFFFBC

#define PABDR         0x05FFFFC0
#define PADR          0x05FFFFC0
#define PBDR          0x05FFFFC2
#define PABIOR        0x05FFFFC4
#define PAIOR         (*((volatile unsigned short *)0x05FFFFC4))
#define PBIOR         (*((volatile unsigned short *)0x05FFFFC6))
#define PACR          0x05FFFFC8
#define PACR1         0x05FFFFC8
#define PACR2         0x05FFFFCA
#define PBCR          0x05FFFFCC
#define PBCR1         0x05FFFFCC
#define PBCR2         0x05FFFFCE
#define PCDR          0x05FFFFD1

#define CASCR         0x05FFFFEE


#endif
