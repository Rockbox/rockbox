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

#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include "sh7034.h"
#include "config.h"

#define FREQ CPU_FREQ
#define BAUDRATE 9600

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifndef MIN
#define MIN(a, b) (((a)<(b))?(a):(b))
#endif

#ifndef MAX
#define MAX(a, b) (((a)>(b))?(a):(b))
#endif

#ifdef LITTLE_ENDIAN
#define SWAB16(x) (x)
#define SWAB32(x) (x)
#endif

#define nop \
  asm volatile ("nop")

#define or_b(mask, address) \
  asm                                     \
    ("or.b\t%0,@(r0,gbr)"                 \
     :                                    \
     : /* %0 */ "I"((char)(mask)),        \
       /* %1 */ "z"(address-GBR))

#define and_b(mask, address) \
  asm                                       \
    ("and.b\t%0,@(r0,gbr)"                  \
     :                                      \
     : /* %0 */ "I"((char)(mask)),         \
       /* %1 */ "z"(address-GBR))

#define xor_b(mask, address) \
  asm                                        \
    ("xor.b\t%0,@(r0,gbr)"                   \
     :                                       \
     : /* %0 */ "I"((char)(mask)),           \
       /* %1 */ "z"(address-GBR))

#ifndef SIMULATOR

static inline short SWAB16(short value)
  /*
    result[15..8] = value[ 7..0];
    result[ 7..0] = value[15..8];
  */    
{
    short result;
    asm volatile ("swap.b\t%1,%0" : "=r"(result) : "r"(value));
    return result;
}

static inline long SWAW32(long value)
  /*
    result[31..16] = value[15.. 0];
    result[15.. 0] = value[31..16];
  */    
{
    long result;
    asm volatile ("swap.w\t%1,%0" : "=r"(result) : "r"(value));
    return result;
}

static inline long SWAB32(long value)
  /*
    result[31..24] = value[ 7.. 0];
    result[23..16] = value[15.. 8];
    result[15.. 8] = value[23..16];
    result[ 7.. 0] = value[31..24];
  */    
{
    asm volatile ("swap.b\t%0,%0\n"
                  "swap.w\t%0,%0\n"
                  "swap.b\t%0,%0\n" : "+r"(value));
    return value;
}

/* Test And Set - UNTESTED */
static inline int tas (volatile int *pointer)
  {
    int result;
    asm volatile ("tas.b\t@%1;movt\t%0" : "=t"(result) : "r"((char *)pointer) : "memory");
    return result;
  }

static inline void sti (void)
  {
    asm volatile ("ldc\t%0,sr" : : "r"(0<<4));
  }

static inline void cli (void)
  {
    asm volatile ("ldc\t%0,sr" : : "r"(15<<4));
  }

/* Compare And Swap */
static inline int cas (volatile int *pointer,int requested_value,int new_value)
  {
    cli();
    if (*pointer == requested_value)
      {
        *pointer = new_value;
        sti ();
        return 1;
      }
    sti ();
    return 0;
  }

static inline int cas2 (volatile int *pointer1,volatile int *pointer2,int requested_value1,int requested_value2,int new_value1,int new_value2)
  {
    cli();
    if (*pointer1 == requested_value1 && *pointer2 == requested_value2)
      {
        *pointer1 = new_value1;
        *pointer2 = new_value2;
        sti ();
        return 1;
      }
    sti ();
    return 0;
  }

#endif

extern void system_reboot (void);
extern void system_init(void);

#endif
