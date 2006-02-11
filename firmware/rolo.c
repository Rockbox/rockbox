/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Randy D. Wood
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include "lcd.h"
#include "kernel.h"
#include "sprintf.h"
#include "button.h"
#include "file.h"
#include "audio.h"
#include "system.h"
#include "i2c.h"
#include "string.h"
#include "buffer.h"

#if (CONFIG_CPU != TCC730) && !defined(IRIVER_IFP7XX_SERIES) && \
    !defined(IPOD_ARCH)
/* FIX: this doesn't work on Gmini, iFP and iPods yet */

#define IRQ0_EDGE_TRIGGER 0x80

static void rolo_error(const char *text)
{
    lcd_clear_display();
    lcd_puts(0, 0, "ROLO error:");
    lcd_puts_scroll(0, 1, text);
    lcd_update();
    button_get(true);
    button_get(true);
    button_get(true);
    lcd_stop_scroll();
}

#if CONFIG_CPU == SH7034
/* these are in assembler file "descramble.S" */
extern unsigned short descramble(const unsigned char* source,
                                 unsigned char* dest, int length);
extern void rolo_restart(const unsigned char* source, unsigned char* dest,
                         int length);
#else
void rolo_restart(const unsigned char* source, unsigned char* dest,
                  long length)  __attribute__ ((section (".icode")));
void rolo_restart(const unsigned char* source, unsigned char* dest,
                         long length)
{
    long i;
    unsigned char* localdest = dest;

    for(i = 0;i < length;i++)
        *localdest++ = *source++;

#if CONFIG_CPU == MCF5249
    asm (
        "movec.l %0,%%vbr    \n"
        "move.l  (%0)+,%%sp  \n"
        "move.l  (%0),%0     \n"
        "jmp     (%0)        \n"
        : : "a"(dest)
    );
#endif
#if (CONFIG_CPU == PP5002) || (CONFIG_CPU==PP5020)
    /* TODO: Implement for iPod */
#endif
}
#endif

/* This is assigned in the linker control file */
extern unsigned long loadaddress;

/***************************************************************************
 *
 * Name: rolo_load_app(char *filename,int scrambled)
 * Filename must be a fully defined filename including the path and extension
 *
 ***************************************************************************/
int rolo_load(const char* filename)
{
    int fd;
    long length;
#if (CONFIG_CPU == MCF5249) || (CONFIG_CPU == PP5002) || (CONFIG_CPU == PP5020)
    int i;
    unsigned long checksum,file_checksum;
#else
    long file_length;
    unsigned short checksum,file_checksum;
#endif
    unsigned char* ramstart = (void*)&loadaddress;

    lcd_clear_display();
    lcd_puts(0, 0, "ROLO...");
    lcd_puts(0, 1, "Loading");
    lcd_update();

    audio_stop();

    fd = open(filename, O_RDONLY);
    if(-1 == fd) {
        rolo_error("File not found");
        return -1;
    }

    length = filesize(fd) - FIRMWARE_OFFSET_FILE_DATA;

#if (CONFIG_CPU == MCF5249) || (CONFIG_CPU == PP5002) || (CONFIG_CPU == PP5020)
    /* Read and save checksum */
    lseek(fd, FIRMWARE_OFFSET_FILE_CRC, SEEK_SET);
    if (read(fd, &file_checksum, 4) != 4) {
        rolo_error("Error Reading checksum");
        return -1;
    }
    lseek(fd, FIRMWARE_OFFSET_FILE_DATA, SEEK_SET);

    if (read(fd, audiobuf, length) != length) {
        rolo_error("Error Reading File");
        return -1;
    }

    checksum = MODEL_NUMBER;
    
    for(i = 0;i < length;i++) {
        checksum += audiobuf[i];
    }

    /* Verify checksum against file header */
    if (checksum != file_checksum) {
        rolo_error("Checksum Error");
        return -1;
    }

    lcd_puts(0, 1, "Executing     ");
    lcd_update();

    set_irq_level(HIGHEST_IRQ_LEVEL);
#elif CONFIG_CPU == SH7034
    /* Read file length from header and compare to real file length */
    lseek(fd, FIRMWARE_OFFSET_FILE_LENGTH, SEEK_SET);
    if(read(fd, &file_length, 4) != 4) {
        rolo_error("Error Reading File Length");
        return -1;
    }
    if (length != file_length) {
        rolo_error("File length mismatch");
        return -1;
    }
    
    /* Read and save checksum */
    lseek(fd, FIRMWARE_OFFSET_FILE_CRC, SEEK_SET);
    if (read(fd, &file_checksum, 2) != 2) {
        rolo_error("Error Reading checksum");
        return -1;
    }
    lseek(fd, FIRMWARE_OFFSET_FILE_DATA, SEEK_SET);

    /* verify that file can be read and descrambled */
    if ((audiobuf + (2*length)+4) >= audiobufend) {
        rolo_error("Not enough room to load file");
        return -1;
    }

    if (read(fd, &audiobuf[length], length) != (int)length) {
        rolo_error("Error Reading File");
        return -1;
    }

    lcd_puts(0, 1, "Descramble");
    lcd_update();

    checksum = descramble(audiobuf + length, audiobuf, length);
    
    /* Verify checksum against file header */
    if (checksum != file_checksum) {
        rolo_error("Checksum Error");
        return -1;
    }

    lcd_puts(0, 1, "Executing     ");
    lcd_update();

    set_irq_level(HIGHEST_IRQ_LEVEL);
    
    /* Calling these 2 initialization routines was necessary to get the
       the origional Archos version of the firmware to load and execute. */
    system_init();           /* Initialize system for restart */
    i2c_init();              /* Init i2c bus - it seems like a good idea */
    ICR = IRQ0_EDGE_TRIGGER; /* Make IRQ0 edge triggered */
    TSTR = 0xE0;             /* disable all timers */
    /* model-specific de-init, needed when flashed */
    /* Especially the Archos software is picky about this */
#if defined(ARCHOS_RECORDER) || defined(ARCHOS_RECORDERV2) || \
    defined(ARCHOS_FMRECORDER)
    PAIOR = 0x0FA0;
#endif
#endif
    rolo_restart(audiobuf, ramstart, length);

    return 0; /* this is never reached */
}
#else  /* (CONFIG_CPU != TCC730) && !defined(IRIVER_IFP7XX_SERIES) */
int rolo_load(const char* filename)
{
    /* dummy */
    (void)filename;
    return 0;
}

#endif /* (CONFIG_CPU != TCC730) && !defined(IRIVER_IFP7XX_SERIES) */
