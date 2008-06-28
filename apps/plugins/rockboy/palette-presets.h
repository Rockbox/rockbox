/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#ifndef __PALETTE_PRESETS_H__
#define __PALETTE_PRESETS_H__

/* The following is an array of palettes for use in Rockboy. Some were 
 * originally found in GoomBA for Game Boy Advance by FluBBa.
 *
 * The first line contains the colors for the background layer.
 * The second contains the colors for the window layer.
 * The third contains the colors for object 0 layer.
 * The fourth contains the colors for object 1 layer.
 *
 * gnuboy seems to like the colors to be 0xBBGGRR */

int palettes [17][4][4] = { 
                            {
  /* Brown */                 { 0x98d0e0, 0x68a0b0, 0x60707C, 0x2C3C3C },
                              { 0x98d0e0, 0x68a0b0, 0x60707C, 0x2C3C3C },
                              { 0x98d0e0, 0x68a0b0, 0x60707C, 0x2C3C3C },
                              { 0x98d0e0, 0x68a0b0, 0x60707C, 0x2C3C3C }
                            },
                                                    
                            {
  /* Gray */                  { 0xffffff, 0xaaaaaa, 0x555555, 0x000000 },
                              { 0xffffff, 0xaaaaaa, 0x555555, 0x000000 },
                              { 0xffffff, 0xaaaaaa, 0x555555, 0x000000 },
                              { 0xffffff, 0xaaaaaa, 0x555555, 0x000000 }
                            },
                            
                            {
  /* Light Gray */            { 0xffffff, 0xc0c0c0, 0x808080, 0x404040 },
                              { 0xffffff, 0xc0c0c0, 0x808080, 0x404040 },
                              { 0xffffff, 0xc0c0c0, 0x808080, 0x404040 },
                              { 0xffffff, 0xc0c0c0, 0x808080, 0x404040 }
                            },
                            
                            {
  /* Multi-Color 1 */         { 0xffffff, 0xaaaaaa, 0x555555, 0x000000 },
                              { 0xffffff, 0xaaaaaa, 0x555555, 0x000000 },
                              { 0xffefef, 0xe78c5a, 0x9c4a18, 0x000000 },
                              { 0xefefff, 0x635aef, 0x1810ad, 0x000000 }
                            },

                            {
  /* Multi-Color 2 */         { 0xffffff, 0xaaaaaa, 0x555555, 0x000000 },
                              { 0xe7efd6, 0xc6de8c, 0x6b8429, 0x000000 },
                              { 0xffffff, 0x635aef, 0x1810ad, 0x000000 },
                              { 0xffffff, 0xe78c5a, 0x9c4a18, 0x000000 }
                            },
                            
                            {
  /* Adventure Island */      { 0xffffff, 0xffb59c, 0x009431, 0x000000 },
                              { 0xffffff, 0x73cef7, 0x084a8c, 0x9c2121 },
                              { 0xdeffff, 0x73c6ef, 0x5263ff, 0x290000 },
                              { 0xffffff, 0xa5a5e7, 0x29297b, 0x000042 }
                            },
                            
                            {
  /* Adventure Island 2 */    { 0xffffff, 0x75eff7, 0xbd6b29, 0x000000 },
                              { 0xffffff, 0x73cef7, 0x084a8c, 0x9c2121 },
                              { 0xdeffff, 0x73c6ef, 0x5263ff, 0x290000 },
                              { 0xffffff, 0xa5a5e7, 0x29297b, 0x000042 }
                            },
                            
                            {
  /* Balloon Kid */           { 0xa5d6ff, 0xe7efff, 0xde8c10, 0x5a1000 },
                              { 0xffffff, 0x73cef7, 0x084a8c, 0x9c2121 },
                              { 0xc6c6ff, 0x6b6bff, 0x0000ff, 0x000063 },
                              { 0xffffff, 0xef42ef, 0x29297b, 0x000042 }
                            },
                            
                            {
  /* Batman */                { 0xeff7ff, 0xc88089, 0x445084, 0x001042 },
                              { 0xffffff, 0xffa5a5, 0xbd5252, 0xa50000 },
                              { 0xffffff, 0xc6a5a5, 0x8c5252, 0x5a0000 },
                              { 0xffffff, 0xbdb5ad, 0x7b6b5a, 0x422108 }
                            },
                            
                            {
  /* Batman */                { 0xffffff, 0xbdada5, 0x7b5a52, 0x391000 },
  /* Return of Joker */       { 0xffffff, 0xbdada5, 0x7b5a52, 0x391000 },
                              { 0xffffff, 0xbdada5, 0x7b5a52, 0x391000 },
                              { 0xffffff, 0xbdada5, 0x7b5a52, 0x422108 }
                            },
                            
                            {
  /* Bionic Commando */       { 0xfff7ef, 0xadb5ce, 0x2921c6, 0x000039 },
                              { 0xffffff, 0xf7ce94, 0xff3910, 0x4a0000 },
                              { 0xffffff, 0x84adff, 0x00395a, 0x000000 },
                              { 0xefefef, 0x9ca5ad, 0x5a5a6b, 0x081042 }
                            },
                            
                            {
  /* Castlevania Adventure */ { 0xe7d6d6, 0xb5a58c, 0x6b5242, 0x181000 },
                              { 0xffffff, 0xdba5a5, 0xad5252, 0x840000 },
                              { 0xffffff, 0x84e7ff, 0x4252ff, 0x00005a },
                              { 0xffffff, 0xceefff, 0x9cdef7, 0x6bb5f7 }
                            },
                            
                            {
  /* Donkey Kong Land */      { 0xd1ffe2, 0x89ffa9, 0x48a251, 0x032504 },
                              { 0xb4b4ff, 0x4747ff, 0x000080, 0x000000 },
                              { 0xb0f2ff, 0x4dc3d6, 0x115ba3, 0x00006a },
                              { 0x63fff7, 0x42cec6, 0x217b73, 0x000000 }
                            },
                            
                            {
  /* Dr. Mario */             { 0xffffff, 0x66ffff, 0xff4221, 0x521000 },
                              { 0xffffff, 0xd6aaaa, 0xad5555, 0x840000 },
                              { 0xffffff, 0x84e7ff, 0x4252ff, 0x00008c },
                              { 0xffffff, 0x8cceff, 0x5a9cf7, 0x005283 }
                            },
                            
                            {
  /* Kirby */                 { 0x83ffff, 0x3ea5ff, 0x004273, 0x000933 },
                              { 0xffcccc, 0xff7877, 0xc13523, 0x5a0a05 },
                              { 0xc4bdff, 0x604df1, 0x29129f, 0x000000 },
                              { 0xffffff, 0xaaaaaa, 0x555555, 0x000020 }
                            },
                            
                            {
  /* Metroid II */            { 0xdeffef, 0xb2ada2, 0x014284, 0x000000 },
                              { 0xffcef7, 0xff6bce, 0x9c007b, 0x000000 },
                              { 0x55efff, 0x1352ff, 0x001485, 0x000000 },
                              { 0xffefef, 0xe78c5a, 0x9c4a18, 0x210000 }
                            },

                            {
  /* Zelda */                 { 0xa0ffff, 0x67d767, 0x20558c, 0x071546 },
                              { 0xadffff, 0xb78080, 0x592925, 0x000000 },
                              { 0x7fb0ff, 0xe78c5a, 0x9c4a18, 0x000000 },
                              { 0xefefff, 0x635aef, 0x1810ad, 0x000000 }
                            }
                         };

#endif
