/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2003 by Jörg Hohensohn
 *
 * Second-level bootloader, with dual-boot feature by holding F1/Menu
 * This is the image being descrambled and executed by the boot ROM.
 * It's task is to copy Rockbox from Flash to DRAM.
 * The image(s) in flash may optionally be compressed with UCL 2e
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

#include "sh7034.h"
#include "bootloader.h"

// prototypes
static void PlatformInit(void);
static void DecompressStart(tImage* pImage);
#ifdef USE_ADC
static int ReadADC(int channel);
#endif
static int ButtonPressed(void);
static tImage* GetStartImage(int nPreferred);
// test functions
static void SetLed(BOOL bOn);
static void UartInit(void);
static UINT8 UartRead(void);
static void UartWrite(UINT8 byte);
static void MiniMon(void);    


#ifdef NO_ROM
/* start with the vector table */
UINT32 vectors[] __attribute__ ((section (".vectors"))) =
{
    (UINT32)_main,           /* entry point, the copy routine */
    (UINT32)(end_stack - 1), /* initial stack pointer */
    FLASH_BASE + 0x200,      /* source of image in flash */
    (UINT32)total_size,      /* size of image */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0x03020080 /* mask and version (just as a suggestion) */
};
#else
/* our binary has to start with a vector to the entry point */
tpMain start_vector[] __attribute__ ((section (".startvector"))) = {main};
#endif

#ifdef NO_ROM /* some code which is only needed for the romless variant */
void _main(void)
{
    UINT32* pSrc;
    UINT32* pDest;
    UINT32* pEnd;
/*
    asm volatile ("ldc %0,sr" : : "r"(0xF0)); // disable interrupts
    asm volatile ("mov.l @%0,r15" : : "r"(4)); // load stack
    asm volatile ("ldc %0,vbr" : : "r"(0)); // load vector base
*/
    /* copy everything to IRAM and continue there */
    pSrc = begin_iramcopy;
    pDest = begin_text;
    pEnd = pDest + (begin_stack - begin_text);

    do
    {
        *pDest++ = *pSrc++;
    }
    while (pDest < pEnd);

    main(); /* jump to the real main() */
}


static void BootInit(void)
{
    /* inits from the boot ROM, whether they make sense or not */
    PBDR  &= 0xFFBF; /* LED off (0x131E) */
    PBCR2  = 0;      /* all GPIO */
    PBIOR |= 0x0040; /* LED output */
    PBIOR &= 0xFFF1; /* LCD lines input */

    /* init DRAM like the boot ROM does */
    PACR2 &= 0xFFFB;
    PACR2 |= 0x0008;
    CASCR  = 0xAF;
    BCR   |= 0x8000;
    WCR1  &= 0xFDFD;
    DCR    = 0x0E00;
    RCR    = 0x5AB0;
    RTCOR  = 0x9605;
    RTCSR  = 0xA518;
}
#endif /* #ifdef NO_ROM */


int main(void)
{
    int nButton;

    PlatformInit(); /* model-specific inits */

    nButton = ButtonPressed();

    if (nButton == 3)
    {   /* F3 means start monitor */
        MiniMon();
    }
    else
    {
        tImage* pImage;
        pImage = GetStartImage(nButton); /* which image */
        DecompressStart(pImage); /* move into place and start it */
    }

    return 0; /* I guess we won't return  ;-) */
}


/* init code that is specific to certain platform */
static void PlatformInit(void)
{
#ifdef NO_ROM
    BootInit(); /* if not started by boot ROM, we need to init what it did */
#endif

#if defined PLATFORM_PLAYER
    BRR1    = 0x19; /* 14400 Baud for monitor */
    PBDRL  |= 0x10; /* set PB4 to 1 to power the hd early (and prepare for
                     * probing in case the charger is connected) */
    PBIORL |= 0x10; /* make PB4 an output */
    PACR2  &= 0xFFFC; /* GPIO for PA0 (charger detection, input by default) */
    if (!(PADRL & 0x01)) /* charger plugged? */
    {   /* we need to probe whether the box is able to control hd power */
        int i;

        PBIORL &= ~0x10; /* set PB4 to input */
        /* wait whether it goes low, max. ~1 ms */
        for (i = 0; (PBDRL & 0x10) && i < 1000; i++);

        if (~(PBDRL & 0x10)) /* pulled low -> power controllable */
            PBDRL &= 0x10;   /* set PB4 low */
        else                 /* still floating high -> not controllable */
            PBDRL |= 0x10;   /* set PB4 high */
        PBIORL |= 0x10;      /* ..and output again */
    }
#elif defined PLATFORM_RECORDER
    BRR1 = 0x02;     /* 115200 Baud for monitor */
    if (ReadADC(7) > 0x100) /* charger plugged? */
    {   /* switch off the HD, else a flat battery may not start */
        PACR2 &= 0xFBFF; /* GPIO for PA5 */
        PAIOR |= 0x0020; /* make PA5 an output (low by default) */
    }
#elif defined PLATFORM_FM
    BRR1   = 0x02;   /* 115200 Baud for monitor */
    PBDR  |= 0x0020; /* set PB5 to keep power (fixes the ON-holding problem) */
    PBIOR |= 0x0020; /* make PB5 an output */
    if (ReadADC(0) < 0x1FF) /* charger plugged? */
    {   /* switch off the HD, else a flat battery may not start */
        PACR2 &= 0xFBFF; /* GPIO for PA5 */
        PAIOR |= 0x0020; /* make PA5 an output (low by default) */
    }
#elif defined PLATFORM_ONDIO
    BRR1   = 0x19;   /* 14400 Baud for monitor */
    PBDR  |= 0x0020; /* set PB5 to keep power (fixes the ON-holding problem) */
    PBIOR |= 0x0020; /* make PB5 an output */
#endif

    /* platform-independent inits */
    DCR |= 0x1000; /* enable burst mode on DRAM */
    BCR |= 0x2000; /* activate Warp mode (simultaneous internal and external
                    * mem access) */
}


/* move the image into place and start it */
static void DecompressStart(tImage* pImage)
{
    UINT32* pSrc;
    UINT32* pDest;

    pSrc = pImage->image;
    pDest = pImage->pDestination;

    if (pSrc != pDest) /* if not linked to that flash address */
    {
        if (pImage->flags & IF_UCL_2E)
        {   /* UCL compressed, algorithm 2e */
            UINT32 dst_len; /* dummy */
            ucl_nrv2e_decompress_8((UINT8*)pSrc, (UINT8*)pDest, &dst_len);
        }
        else
        {   /* uncompressed, copy it */
            UINT32 size = pImage->size;
            UINT32* pEnd;
            size = (size + 3) / 4; /* round up to 32bit-words */
            pEnd = pDest + size;

            do
            {
                *pDest++ = *pSrc++;
            }
            while (pDest < pEnd);
        }
    }

    pImage->pExecute();
}

#ifdef USE_ADC
static int ReadADC(int channel)
{
    /* after channel 3, the ports wrap and get re-used */
    volatile UINT16* pResult = (UINT16*)(ADDRAH_ADDR + 2 * (channel & 0x03));
    int timeout = 266; /* conversion takes 266 clock cycles */

    ADCSR = 0x20 | channel; /* start single conversion */
    while (((ADCSR & 0x80) == 0) && (--timeout)); /* 6 instructions per round*/

    return (timeout == 0) ? -1 : *pResult>>6;
}
#endif


/* This function is platform-dependent,
 * until I figure out how to distinguish at runtime. */
static int ButtonPressed(void) /* return 1,2,3 for F1,F2,F3, 0 if none pressed */
{
#ifdef USE_ADC
    int value = ReadADC(CHANNEL);

    if (value >= F1_LOWER && value <= F1_UPPER) /* in range */
        return 1;
    else if (value >= F2_LOWER && value <= F2_UPPER) /* in range */
        return 2;
    else if (value >= F3_LOWER && value <= F3_UPPER) /* in range */
        return 3;
#else
    int value = PCDR;

    if (!(value & F1_MASK))
        return 1;
    else if (!(value & F2_MASK))
        return 2;
    else if (!(value & F3_MASK))
        return 3;
#endif

    return 0;
}


/* Determine the image to be started */
static tImage* GetStartImage(int nPreferred)
{
    tImage* pImage1;
    tImage* pImage2 = NULL; /* default to not present */
    UINT32 pos;
    UINT32* pFlash = (UINT32*)FLASH_BASE;

    /* determine the first image position */
    pos = pFlash[2] + pFlash[3]; /* position + size of the bootloader
                                  * = after it */
    pos = (pos + 3) & ~3; /* be sure it's 32 bit aligned */

    pImage1 = (tImage*)pos;

    if (pImage1->size != 0)
    {   /* check for second image */
        pos = (UINT32)(&pImage1->image) + pImage1->size;
        pImage2 = (tImage*)pos;

        /* does it make sense? (not in FF or 00 erazed space) */
        if (pImage2->pDestination == (void*)0xFFFFFFFF
         || pImage2->size == 0xFFFFFFFF
         || pImage2->pExecute == (void*)0xFFFFFFFF
         || pImage2->flags == 0xFFFFFFFF
         || pImage2->pDestination == NULL)
         /* size, execute and flags can legally be 0 */
        {
            pImage2 = NULL; /* invalidate */
        }
    }

    if (pImage2 == NULL || nPreferred == 1)
    {   /* no second image or overridden: return the first */
        return pImage1;
    }

    return pImage2; /* return second image */
}

/* diagnostic functions */

static void SetLed(BOOL bOn)
{
    if (bOn)
        PBDR |= 0x0040;
    else
        PBDR &= ~0x0040;
}


static void UartInit(void)
{
    PBIOR &= 0xFBFF; /* input: RXD1 remote pin */
    PBCR1 |= 0x00A0; /* set PB11+PB10 to UART */
    PBCR1 &= 0xFFAF; /* clear bits 6, 4 -> UART */
    SMR1   = 0x00;   /* async format 8N1, baud generator input is CPU clock */
    SCR1   = 0x30;   /* transmit+receive enable */
    PBCR1 &= 0x00FF; /* set bit 12...15 as GPIO */
    SSR1  &= 0xBF;   /* clear bit 6 (RDRF, receive data register full) */
}


static UINT8 UartRead(void)
{
    UINT8 byte;
    while (!(SSR1 & SCI_RDRF)); /* wait for char to be available */
    byte = RDR1;
    SSR1 &= ~SCI_RDRF;
    return byte;
}


static void UartWrite(UINT8 byte)
{
    while (!(SSR1 & SCI_TDRE)); /* wait for transmit buffer empty */
    TDR1 = byte;
    SSR1 &= ~SCI_TDRE;
}


/* include the mini monitor as a rescue feature, started with F3 */
static void MiniMon(void)
{
    UINT8 cmd;
    UINT32 addr;
    UINT32 size;
    UINT32 content;
    volatile UINT8* paddr = NULL;
    volatile UINT8* pflash = NULL; /* flash base address */

    UartInit();

    while (1)
    {
        cmd = UartRead();
        switch (cmd)
        {
        case BAUDRATE:
            content = UartRead();
            UartWrite(cmd); /* acknowledge by returning the command value */
            while (!(SSR1 & SCI_TEND)); /* wait for empty shift register,
                                         * before changing baudrate */
            BRR1 = content;
            break;

        case ADDRESS:
            addr = (UartRead() << 24) | (UartRead() << 16)
                  | (UartRead() << 8) | UartRead();
            paddr = (UINT8*)addr;
            pflash = (UINT8*)(addr & 0xFFF80000); /* round down to 512k align*/
            UartWrite(cmd); /* acknowledge by returning the command value */
            break;

        case BYTE_READ:
            content = *paddr++;
            UartWrite(content); /* the content is the ack */
            break;

        case BYTE_WRITE:
            content = UartRead();
            *paddr++ = content;
            UartWrite(cmd); /* acknowledge by returning the command value */
            break;

        case BYTE_READ16:
            size = 16;
            while (size--)
            {
                content = *paddr++;
                UartWrite(content); /* the content is the ack */
            }
            break;

        case BYTE_WRITE16:
            size = 16;
            while (size--)
            {
                content = UartRead();
                *paddr++ = content;
            }
            UartWrite(cmd); /* acknowledge by returning the command value */
            break;

        case BYTE_FLASH:
            content = UartRead();
            pflash[0x5555] = 0xAA; /* set flash to command mode */
            pflash[0x2AAA] = 0x55;
            pflash[0x5555] = 0xA0; /* byte program command */
            *paddr++ = content;
            UartWrite(cmd); /* acknowledge by returning the command value */
            break;

        case BYTE_FLASH16:
            size = 16;
            while (size--)
            {
                content = UartRead();
                pflash[0x5555] = 0xAA; /* set flash to command mode */
                pflash[0x2AAA] = 0x55;
                pflash[0x5555] = 0xA0; /* byte program command */
                *paddr++ = content;
            }
            UartWrite(cmd); /* acknowledge by returning the command value */
            break;

        case HALFWORD_READ:
            content = *(UINT16*)paddr;
            paddr += 2;
            UartWrite(content >> 8);   /* highbyte */
            UartWrite(content & 0xFF); /* lowbyte */
            break;

        case HALFWORD_WRITE:
            content = UartRead() << 8 | UartRead();
            *(UINT16*)paddr = content;
            paddr += 2;
            UartWrite(cmd); /* acknowledge by returning the command value */
            break;

        case EXECUTE:
            {
                tpFunc pFunc = (tpFunc)paddr;
                pFunc();
                UartWrite(cmd); /* acknowledge by returning the command value*/
            }
            break;

        case VERSION:
            UartWrite(1); /* return our version number */
            break;

        default:
            {
                SetLed(TRUE);
                UartWrite(~cmd); /* error acknowledge */
            }

        } /* case */
    } /* while (1) */
}
