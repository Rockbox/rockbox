/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Plugin for reprogramming the whole Flash ROM chip with a new content.
* !!! DON'T MESS WITH THIS CODE UNLESS YOU'RE ABSOLUTELY SURE WHAT YOU DO !!!
*
* Copyright (C) 2003 Jörg Hohensohn [IDC]Dragon
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/
#include "plugin.h"

#ifndef SIMULATOR /* only for target */

/* define DUMMY if you only want to "play" with the UI, does no harm */
/* #define DUMMY */

#ifndef UINT8
#define UINT8 unsigned char
#endif

#ifndef UINT16
#define UINT16 unsigned short
#endif

#ifndef UINT32
#define UINT32 unsigned long
#endif

/* platform IDs as I have used them in my firmware templates */
#define ID_RECORDER 0
#define ID_FM       1
#define ID_PLAYER   2
#define ID_REC_V2   3
#define ID_ONDIO_FM 4
#define ID_ONDIO_SP 5

/* Here I have to check for ARCHOS_* defines in source code, which is 
   generally strongly discouraged. But here I'm not checking for a certain 
   feature, I'm checking for the model itself. */
#if defined(ARCHOS_PLAYER)
#define FILE_TYPE "player"
#define KEEP VERSION_ADR /* keep the firmware version */
#define PLATFORM_ID ID_PLAYER
#elif defined(ARCHOS_RECORDER)
#define FILE_TYPE "rec"
#define KEEP MASK_ADR /* keep the mask value */
#define PLATFORM_ID ID_RECORDER
#elif defined(ARCHOS_RECORDERV2)
#define FILE_TYPE "v2"
#define KEEP MASK_ADR /* keep the mask value */
#define PLATFORM_ID ID_REC_V2
#elif defined(ARCHOS_FMRECORDER)
#define FILE_TYPE "fm"
#define KEEP MASK_ADR /* keep the mask value */
#define PLATFORM_ID ID_FM
#elif defined(ARCHOS_ONDIOFM)
#define FILE_TYPE "ondiofm"
#define KEEP MASK_ADR /* keep the mask value */
#define PLATFORM_ID ID_ONDIO_FM
#elif defined(ARCHOS_ONDIOSP)
#define FILE_TYPE "ondiosp"
#define KEEP MASK_ADR /* keep the mask value */
#define PLATFORM_ID ID_ONDIO_SP
#else
#undef PLATFORM_ID /* this platform is not (yet) flashable */
#endif

#ifdef PLATFORM_ID

#if CONFIG_KEYPAD == ONDIO_PAD /* limited keypad */
#define KEY1 BUTTON_LEFT
#define KEY2 BUTTON_UP
#define KEY3 BUTTON_RIGHT
#define KEYNAME1 "[Left]"
#define KEYNAME2 "[Up]"
#define KEYNAME3 "[Right]"
#else /* recorder keypad */
#define KEY1 BUTTON_F1
#define KEY2 BUTTON_F2
#define KEY3 BUTTON_F3
#define KEYNAME1 "[F1]"
#define KEYNAME2 "[F2]"
#define KEYNAME3 "[F3]"
#endif

/* result of the CheckFirmwareFile() function */
typedef enum
{
    eOK = 0,
    eFileNotFound, /* errors from here on */
    eTooBig,
    eTooSmall,
    eReadErr,
    eBadContent,
    eCrcErr,
    eBadPlatform,
} tCheckResult;

/* result of the CheckBootROM() function */
typedef enum
{
    eBootROM, /* the supported boot ROM */
    eUnknown, /* unknown boot ROM */
    eROMless, /* flash mapped to zero */
} tCheckROM;

typedef struct 
{
    UINT8 manufacturer;
    UINT8 id;
    int size;
    char name[32];
} tFlashInfo;

static struct plugin_api* rb; /* here is a global api struct pointer */

#define MASK_ADR     0xFC /* position of hardware mask value in Flash */
#define VERSION_ADR  0xFE /* position of firmware version value in Flash */
#define PLATFORM_ADR 0xFB /* position of my platform ID value in Flash */
#define SEC_SIZE 4096 /* size of one flash sector */
static UINT8* sector; /* better not place this on the stack... */
static volatile UINT8* FB = (UINT8*)0x02000000; /* Flash base address */


/***************** Flash Functions *****************/


/* read the manufacturer and device ID */
bool ReadID(volatile UINT8* pBase, UINT8* pManufacturerID, UINT8* pDeviceID)
{
    UINT8 not_manu, not_id; /* read values before switching to ID mode */
    UINT8 manu, id; /* read values when in ID mode */
    
    pBase = (UINT8*)((UINT32)pBase & 0xFFF80000); /* down to 512k align */
    
    /* read the normal content */
    not_manu = pBase[0]; /* should be 'A' (0x41) and 'R' (0x52) */
    not_id   = pBase[1]; /*  from the "ARCH" marker */
    
    pBase[0x5555] = 0xAA; /* enter command mode */
    pBase[0x2AAA] = 0x55;
    pBase[0x5555] = 0x90; /* ID command */
    rb->sleep(HZ/50); /* Atmel wants 20ms pause here */
    
    manu = pBase[0];
    id   = pBase[1];
    
    pBase[0] = 0xF0; /* reset flash (back to normal read mode) */
    rb->sleep(HZ/50); /* Atmel wants 20ms pause here */
    
    /* I assume success if the obtained values are different from
    the normal flash content. This is not perfectly bulletproof, they 
    could theoretically be the same by chance, causing us to fail. */
    if (not_manu != manu || not_id != id) /* a value has changed */
    {
        *pManufacturerID = manu; /* return the results */
        *pDeviceID = id;
        return true; /* success */
    }
    return false; /* fail */
}


/* erase the sector which contains the given address */
bool EraseSector(volatile UINT8* pAddr)
{
#ifdef DUMMY
    (void)pAddr; /* prevents warning */
    return true;
#else
    volatile UINT8* pBase = (UINT8*)((UINT32)pAddr & 0xFFF80000); /* round down to 512k align */
    unsigned timeout = 43000; /* the timeout loop should be no less than 25ms */
    
    pBase[0x5555] = 0xAA; /* enter command mode */
    pBase[0x2AAA] = 0x55;
    pBase[0x5555] = 0x80; /* erase command */
    pBase[0x5555] = 0xAA; /* enter command mode */
    pBase[0x2AAA] = 0x55;
    *pAddr = 0x30; /* erase the sector */

    /* I counted 7 instructions for this loop -> min. 0.58 us per round */
    /* Plus memory waitstates it will be much more, gives margin */
    while (*pAddr != 0xFF && --timeout); /* poll for erased */

    return (timeout != 0);
#endif
}


/* address must be in an erased location */
inline bool ProgramByte(volatile UINT8* pAddr, UINT8 data)
{
#ifdef DUMMY
    (void)pAddr; /* prevents warnings */
    (void)data;
    return true;
#else
    unsigned timeout = 35; /* the timeout loop should be no less than 20us */
    
    if (~*pAddr & data) /* just a safety feature, not really necessary */
        return false; /* can't set any bit from 0 to 1 */
    
    FB[0x5555] = 0xAA; /* enter command mode */
    FB[0x2AAA] = 0x55;
    FB[0x5555] = 0xA0; /* byte program command */
    
    *pAddr = data;
    
    /* I counted 7 instructions for this loop -> min. 0.58 us per round */
    /* Plus memory waitstates it will be much more, gives margin */
    while (*pAddr != data && --timeout); /* poll for programmed */
    
    return (timeout != 0);
#endif
}


/* this returns true if supported and fills the info struct */
bool GetFlashInfo(tFlashInfo* pInfo)
{
    rb->memset(pInfo, 0, sizeof(tFlashInfo));
    
    if (!ReadID(FB, &pInfo->manufacturer, &pInfo->id))
        return false;
    
    if (pInfo->manufacturer == 0xBF) /* SST */
    {
        if (pInfo->id == 0xD6)
        {
            pInfo->size = 256* 1024; /* 256k */
            rb->strcpy(pInfo->name, "SST39VF020");
            return true;
        }
        else if (pInfo->id == 0xD7)
        {
            pInfo->size = 512* 1024; /* 512k */
            rb->strcpy(pInfo->name, "SST39VF040");
            return true;
        }
        else
            return false;
    }
    return false;
}


/*********** Utility Functions ************/


/* Tool function to calculate a CRC32 across some buffer */
/* third argument is either 0xFFFFFFFF to start or value from last piece */
unsigned crc_32(unsigned char* buf, unsigned len, unsigned crc32)
{
    /* CCITT standard polynomial 0x04C11DB7 */
    static const unsigned crc32_lookup[16] = 
    {   /* lookup table for 4 bits at a time is affordable */
        0x00000000, 0x04C11DB7, 0x09823B6E, 0x0D4326D9, 
        0x130476DC, 0x17C56B6B, 0x1A864DB2, 0x1E475005, 
        0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61, 
        0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD
    };
    
    unsigned char byte;
    unsigned t;

    while (len--)
    {   
        byte = *buf++; /* get one byte of data */

        /* upper nibble of our data */
        t = crc32 >> 28; /* extract the 4 most significant bits */
        t ^= byte >> 4; /* XOR in 4 bits of data into the extracted bits */
        crc32 <<= 4; /* shift the CRC register left 4 bits */     
        crc32 ^= crc32_lookup[t]; /* do the table lookup and XOR the result */

        /* lower nibble of our data */
        t = crc32 >> 28; /* extract the 4 most significant bits */
        t ^= byte & 0x0F; /* XOR in 4 bits of data into the extracted bits */
        crc32 <<= 4; /* shift the CRC register left 4 bits */     
        crc32 ^= crc32_lookup[t]; /* do the table lookup and XOR the result */
    }
    
    return crc32;
}


/*********** Firmware File Functions + helpers ************/

/* test if the version number is consistent with the platform */
bool CheckPlatform(int platform_id, UINT16 version)
{
    if (version == 200)
    {   /* for my very first firmwares, I foolishly changed it to 200 */
        return (platform_id == ID_RECORDER || platform_id == ID_FM);
    }
    else if (version == 123)
    {   /* it can be a FM or V2 recorder */
        return (platform_id == ID_FM || platform_id == ID_REC_V2);
    }
    else if (version == 132)
    {   /* newer Ondio, and seen on a V2 recorder */
        return (platform_id == ID_ONDIO_SP || platform_id == ID_ONDIO_FM
            || platform_id == ID_REC_V2);
    }
    else if (version == 104)
    {   /* classic Ondio128 */
        return (platform_id == ID_ONDIO_FM);
    }
    else if (version >= 115 && version <= 129)
    {   /* the range of Recorders seen so far */
        return (platform_id == ID_RECORDER);
    }
    else if (version == 0 || (version >= 300 && version <= 508))
    {   /* for very old players, I've seen zero */
        return (platform_id == ID_PLAYER);
    }

    return false; /* unknown */
}


tCheckResult CheckFirmwareFile(char* filename, int chipsize, bool is_romless)
{
    int i;
    int fd;
    int fileleft; /* size info, how many left for reading */
    int fileread = 0; /* total size as read from the file */
    int read_now; /* how many to read for this sector */
    int got_now; /* how many gotten for this sector */
    unsigned crc32 = 0xFFFFFFFF; /* CCITT init value */
    unsigned file_crc; /* CRC value read from file */
    bool has_crc;
    
    fd = rb->open(filename, O_RDONLY);
    if (fd < 0)
        return eFileNotFound;
    
    fileleft = rb->filesize(fd);
    if (fileleft > chipsize)
    {
        rb->close(fd);
        return eTooBig;
    }
    else if (fileleft < 50000) /* give it some reasonable lower limit */
    {
        rb->close(fd);
        return eTooSmall;
    }
    
    if (fileleft == 256*1024)
    {   // original dumped firmware file has no CRC nor platform ID
        has_crc = false;
    }
    else
    {
        has_crc = true;
        fileleft -= sizeof(unsigned); // exclude the last 4 bytes
    }

    /* do some sanity checks */
    
    got_now = rb->read(fd, sector, SEC_SIZE); /* read first sector */
    fileread += got_now;
    fileleft -= got_now;
    if (got_now != SEC_SIZE)
    {
        rb->close(fd);
        return eReadErr;
    }

    /* version number in file plausible with this hardware? */
    if (!CheckPlatform(PLATFORM_ID, *(UINT16*)(sector + VERSION_ADR)))
    {
        rb->close(fd);
        return eBadPlatform;
    }
    
    if (has_crc)
    {
        crc32 = crc_32(sector, SEC_SIZE, crc32); /* checksum */
        
        /* in addition to the CRC, my files also have a platform ID */
        if (sector[PLATFORM_ADR] != PLATFORM_ID) /* for our hardware? */
        {
            rb->close(fd);
            return eBadPlatform;
        }
    }

    if (is_romless)
    {   /* in this case, there is not much we can check */
        if (*(UINT32*)sector != 0x00000200) /* reset vector */
        {
            rb->close(fd);
            return eBadContent;
        }
    }
    else
    {
        /* compare some bytes which have to be identical */
        if (*(UINT32*)sector != 0x41524348) /* "ARCH" */
        {
            rb->close(fd);
            return eBadContent;
        }
    
        for (i = 0x30; i<MASK_ADR-2; i++) /* leave two bytes for me */
        {
            if (sector[i] != FB[i])
            {
                rb->close(fd);
                return eBadContent;
            }
        }
    }
    
    /* check if we can read the whole file, and do checksum */
    do
    {
        read_now = MIN(SEC_SIZE, fileleft);
        got_now = rb->read(fd, sector, read_now);
        fileread += got_now;
        fileleft -= got_now;

        if (read_now != got_now)
        {
            rb->close(fd);
            return eReadErr;
        }

        if (has_crc)
        {
            crc32 = crc_32(sector, got_now, crc32); /* checksum */
        }
    } while (fileleft);

    if (has_crc)
    {
        got_now = rb->read(fd, &file_crc, sizeof(file_crc));
        if (got_now != sizeof(file_crc))
        {
            rb->close(fd);
            return eReadErr;
        }
    }
    
    /*  must be EOF now */
    got_now = rb->read(fd, sector, SEC_SIZE);
    rb->close(fd);
    if (got_now != 0)
        return eReadErr;

    if (has_crc && file_crc != crc32)
        return eCrcErr;
    
    return eOK;
}


/* returns the # of failures, 0 on success */
unsigned ProgramFirmwareFile(char* filename, int chipsize)
{
    int i, j;
    int fd;
    int read = SEC_SIZE; /* how many for this sector */
    UINT16 keep = *(UINT16*)(FB + KEEP); /* we must keep this! */
    unsigned failures = 0;
    
    fd = rb->open(filename, O_RDONLY);
    if (fd < 0)
        return false;
    
    for (i=0; i<chipsize; i+=SEC_SIZE)
    {
        if (!EraseSector(FB + i))
        {
            /* nothing we can do, let the programming count the errors */
        }
        
        if (read == SEC_SIZE) /* not EOF yet */
        {
            read = rb->read(fd, sector, SEC_SIZE);
            if (i==0)
            {   /* put original value back in */
                *(UINT16*)(sector + KEEP) = keep;
            }
            
            for (j=0; j<read; j++)
            {
                if (!ProgramByte(FB + i + j, sector[j]))
                {
                    failures++;
                }
            }
        }
    }
    
    rb->close(fd);
    
    return failures;
}


/* returns the # of failures, 0 on success */
unsigned VerifyFirmwareFile(char* filename)
{
    int i=0, j;
    int fd;
    int read = SEC_SIZE; /* how many for this sector */
    unsigned failures = 0;
    
    fd = rb->open(filename, O_RDONLY);
    if (fd < 0)
        return false;
    
    do
    {
        read = rb->read(fd, sector, SEC_SIZE);
        
        for (j=0; j<read; j++)
        {
             /* position of keep value is no error */
            if (FB[i] != sector[j] && i != KEEP && i != (KEEP+1))
            {
                failures++;
            }
            i++;
        }
    }
    while (read == SEC_SIZE);
    
    rb->close(fd);
    
    return failures;
}


/***************** Support Functions *****************/

/* check if we have "normal" boot ROM or flash mirrored to zero */
tCheckROM CheckBootROM(void)
{
    unsigned boot_crc;
    unsigned* pFlash = (unsigned*)FB;
    unsigned* pRom = (unsigned*)0x0;
    unsigned i;

    boot_crc = crc_32((unsigned char*)0x0, 64*1024, 0xFFFFFFFF);
    if (boot_crc == 0x56DBA4EE) /* the known boot ROM */
        return eBootROM;

    /* check if ROM is a flash mirror */
    for (i=0; i<256*1024/sizeof(unsigned); i++)
    {
        if (*pRom++ != *pFlash++)
        {   /* difference means no mirror */
            return eUnknown;
        }
    }

    return eROMless;
}


/***************** User Interface Functions *****************/

int WaitForButton(void)
{
    int button;
    
    do
    {
        button = rb->button_get(true);
    } while (button & BUTTON_REL);
    
    return button;
}

#ifdef HAVE_LCD_BITMAP
/* Recorder implementation */

/* helper for DoUserDialog() */
void ShowFlashInfo(tFlashInfo* pInfo)
{
    char buf[32];
    
    if (!pInfo->manufacturer)
    {
        rb->lcd_puts(0, 0, "Flash: M=?? D=??");
        rb->lcd_puts(0, 1, "Impossible to program");
    }
    else
    {
        rb->snprintf(buf, sizeof(buf), "Flash: M=%02x D=%02x",
            pInfo->manufacturer, pInfo->id);
        rb->lcd_puts(0, 0, buf);
        
        
        if (pInfo->size)
        {
            rb->lcd_puts(0, 1, pInfo->name);
            rb->snprintf(buf, sizeof(buf), "Size: %d KB", pInfo->size / 1024);
            rb->lcd_puts(0, 2, buf);
        }
        else
        {
            rb->lcd_puts(0, 1, "Unsupported chip");
        }
        
    }
    
    rb->lcd_update();
}


/* Kind of our main function, defines the application flow. */
void DoUserDialog(char* filename)
{
    tFlashInfo FlashInfo;
    char buf[32];
    char default_filename[32];
    int button;
    int rc; /* generic return code */
    int memleft;
    tCheckROM result;
    bool is_romless;

    /* this can only work if Rockbox runs in DRAM, not flash ROM */
    if ((UINT8*)rb >= FB && (UINT8*)rb < FB + 4096*1024) /* 4 MB max */
    {   /* we're running from flash */
        rb->splash(HZ*3, true, "Not from ROM");
        return; /* exit */
    }

    /* test if the user is running the correct plugin for this box */
    if (!CheckPlatform(PLATFORM_ID, *(UINT16*)(FB + VERSION_ADR)))
    {
        rb->splash(HZ*3, true, "Wrong plugin");
        return; /* exit */
    }

    /* refuse to work if the power may fail meanwhile */
    if (!rb->battery_level_safe())
    {
        rb->splash(HZ*3, true, "Battery too low!");
        return; /* exit */
    }
    
    /* check boot ROM */
    result = CheckBootROM();
    if (result == eUnknown)
    {   /* no support for any other yet */
        rb->splash(HZ*3, true, "Wrong boot ROM");
        return; /* exit */
    }
    is_romless = (result == eROMless);

    /* compose filename if none given */
    if (filename == NULL)
    {
        rb->snprintf(
            default_filename, 
            sizeof(default_filename),
            "/firmware_%s%s.bin",
            FILE_TYPE,
            is_romless ? "_norom" : "");
        filename = default_filename;
    }

    /* "allocate" memory */
    sector = rb->plugin_get_buffer(&memleft);
    if (memleft < SEC_SIZE) /* need buffer for a flash sector */
    {
        rb->splash(HZ*3, true, "Out of memory");
        return; /* exit */
    }

    rb->lcd_setfont(FONT_SYSFIXED);

    rc = GetFlashInfo(&FlashInfo);
    ShowFlashInfo(&FlashInfo);
    if (FlashInfo.size == 0) /* no valid chip */
    {
        rb->splash(HZ*3, true, "Sorry!");
        return; /* exit */
    }
    
    rb->lcd_puts(0, 3, "using file:");
    rb->lcd_puts_scroll(0, 4, filename);
    rb->lcd_puts(0, 6, KEYNAME1 " to check file");
    rb->lcd_puts(0, 7, "other key to exit");
    rb->lcd_update();
    
    button = WaitForButton();
    if (button != KEY1)
    {
        return;
    }
    
    rb->lcd_clear_display();
    rb->lcd_puts(0, 0, "checking...");
    rb->lcd_update();
    
    rc = CheckFirmwareFile(filename, FlashInfo.size, is_romless);
    rb->lcd_puts(0, 0, "checked:");
    switch (rc)
    {
    case eOK:
        rb->lcd_puts(0, 1, "File OK.");
        break;
    case eFileNotFound:
        rb->lcd_puts(0, 1, "File not found.");
        rb->lcd_puts(0, 2, "Put this in root:");
        rb->lcd_puts_scroll(0, 4, filename);
        break;
    case eTooBig:
        rb->lcd_puts(0, 1, "File too big,");
        rb->lcd_puts(0, 2, "larger than chip.");
        break;
    case eTooSmall:
        rb->lcd_puts(0, 1, "File too small.");
        rb->lcd_puts(0, 2, "Incomplete?");
        break;
    case eReadErr:
        rb->lcd_puts(0, 1, "Read error.");
        break;
    case eBadContent:
        rb->lcd_puts(0, 1, "File invalid.");
        rb->lcd_puts(0, 2, "Sanity check fail.");
        break;
    case eCrcErr:
        rb->lcd_puts(0, 1, "File invalid.");
        rb->lcd_puts(0, 2, "CRC check failed,");
        rb->lcd_puts(0, 3, "checksum mismatch.");
        break;
    case eBadPlatform:
        rb->lcd_puts(0, 1, "Wrong file for");
        rb->lcd_puts(0, 2, "this hardware.");
        break;
    default:
        rb->lcd_puts(0, 1, "Check failed.");
        break;
    }
    
    if (rc == eOK)
    {
        rb->lcd_puts(0, 6, KEYNAME2 " to program");
        rb->lcd_puts(0, 7, "other key to exit");
    }
    else
    {   /* error occured */
        rb->lcd_puts(0, 6, "Any key to exit");
    }
    
    rb->lcd_update();
    
    button = WaitForButton();
    if (button != KEY2 || rc != eOK)
    {
        return;
    }
    
    rb->lcd_clear_display();
    rb->lcd_puts(0, 0, "Program all Flash?");
    rb->lcd_puts(0, 1, "Are you sure?");
    rb->lcd_puts(0, 2, "If it goes wrong,");
    rb->lcd_puts(0, 3, "it kills your box!");
    rb->lcd_puts(0, 4, "See documentation.");
    
    rb->lcd_puts(0, 6, KEYNAME3 " to proceed");
    rb->lcd_puts(0, 7, "other key to exit");
    rb->lcd_update();
    
    button = WaitForButton();
    if (button != KEY3)
    {
        return;
    }
    
    rb->lcd_clear_display();
    rb->lcd_puts(0, 0, "Programming...");
    rb->lcd_update();
    
    rc = ProgramFirmwareFile(filename, FlashInfo.size);
    if (rc)
    {   /* errors */
        rb->lcd_clear_display();
        rb->lcd_puts(0, 0, "Panic:");
        rb->lcd_puts(0, 1, "Programming fail!");
        rb->snprintf(buf, sizeof(buf), "%d errors", rc);
        rb->lcd_puts(0, 2, buf);
        rb->lcd_update();
        button = WaitForButton();
    }
    
    rb->lcd_clear_display();
    rb->lcd_puts(0, 0, "Verifying...");
    rb->lcd_update();
    
    rc = VerifyFirmwareFile(filename);
    
    rb->lcd_clear_display();
    if (rc == 0)
    {
        rb->lcd_puts(0, 0, "Verify OK.");
    }
    else
    {
        rb->lcd_puts(0, 0, "Panic:");
        rb->lcd_puts(0, 1, "Verify fail!");
        rb->snprintf(buf, sizeof(buf), "%d errors", rc);
        rb->lcd_puts(0, 2, buf);
    }
    rb->lcd_puts(0, 7, "Any key to exit");
    rb->lcd_update();
    
    button = WaitForButton();
}

#else /* HAVE_LCD_BITMAP */
/* Player implementation */

/* helper for DoUserDialog() */
void ShowFlashInfo(tFlashInfo* pInfo)
{
    char buf[32];
    
    if (!pInfo->manufacturer)
    {
        rb->lcd_puts_scroll(0, 0, "Flash: M=? D=?");
        rb->lcd_puts_scroll(0, 1, "Impossible to program");
        WaitForButton();
    }
    else
    {
        rb->snprintf(buf, sizeof(buf), "Flash: M=%02x D=%02x",
            pInfo->manufacturer, pInfo->id);
        rb->lcd_puts_scroll(0, 0, buf);
        
        if (pInfo->size)
        {
            rb->snprintf(buf, sizeof(buf), "Size: %d KB", pInfo->size / 1024);
            rb->lcd_puts_scroll(0, 1, buf);
        }
        else
        {
            rb->lcd_puts_scroll(0, 1, "Unsupported chip");
            WaitForButton();
        }
    }
}


void DoUserDialog(char* filename)
{
    tFlashInfo FlashInfo;
    char buf[32];
    char default_filename[32];
    int button;
    int rc; /* generic return code */
    int memleft;
    tCheckROM result;
    bool is_romless;

    /* this can only work if Rockbox runs in DRAM, not flash ROM */
    if ((UINT8*)rb >= FB && (UINT8*)rb < FB + 4096*1024) /* 4 MB max */
    {   /* we're running from flash */
        rb->splash(HZ*3, true, "Not from ROM");
        return; /* exit */
    }

    /* test if the user is running the correct plugin for this box */
    if (!CheckPlatform(PLATFORM_ID, *(UINT16*)(FB + VERSION_ADR)))
    {
        rb->splash(HZ*3, true, "Wrong version");
        return; /* exit */
    }

    /* refuse to work if the power may fail meanwhile */
    if (!rb->battery_level_safe())
    {
        rb->splash(HZ*3, true, "Batt. too low!");
        return; /* exit */
    }
    
    /* check boot ROM */
    result = CheckBootROM();
    if (result == eUnknown)
    {   /* no support for any other yet */
        rb->splash(HZ*3, true, "Wrong boot ROM");
        return; /* exit */
    }
    is_romless = (result == eROMless);

    /* compose filename if none given */
    if (filename == NULL)
    {
        rb->snprintf(
            default_filename, 
            sizeof(default_filename),
            "/firmware_%s%s.bin",
            FILE_TYPE,
            is_romless ? "_norom" : "");
        filename = default_filename;
    }

    /* "allocate" memory */
    sector = rb->plugin_get_buffer(&memleft);
    if (memleft < SEC_SIZE) /* need buffer for a flash sector */
    {
        rb->splash(HZ*3, true, "Out of memory");
        return; /* exit */
    }

    rc = GetFlashInfo(&FlashInfo);
    ShowFlashInfo(&FlashInfo);
    
    if (FlashInfo.size == 0) /* no valid chip */
    {
        return; /* exit */
    }
    
    rb->lcd_puts_scroll(0, 0, filename);
    rb->lcd_puts_scroll(0, 1, "[Menu] to check");
    
    button = WaitForButton();
    if (button != BUTTON_MENU)
    {
        return;
    }
    
    rb->lcd_clear_display();
    rb->lcd_puts(0, 0, "Checking...");
    
    rc = CheckFirmwareFile(filename, FlashInfo.size, is_romless);
    rb->lcd_puts(0, 0, "Checked:");
    switch (rc)
    {
    case eOK:
        rb->lcd_puts(0, 1, "File OK.");
        break;
    case eFileNotFound:
        rb->lcd_puts_scroll(0, 0, "File not found:");
        rb->lcd_puts_scroll(0, 1, filename);
        break;
    case eTooBig:
        rb->lcd_puts_scroll(0, 0, "File too big,");
        rb->lcd_puts_scroll(0, 1, "larger than chip.");
        break;
    case eTooSmall:
        rb->lcd_puts_scroll(0, 0, "File too small.");
        rb->lcd_puts_scroll(0, 1, "Incomplete?");
        break;
    case eReadErr:
        rb->lcd_puts_scroll(0, 0, "Read error.");
        break;
    case eBadContent:
        rb->lcd_puts_scroll(0, 0, "File invalid.");
        rb->lcd_puts_scroll(0, 1, "Sanity check failed.");
        break;
    case eCrcErr:
        rb->lcd_puts_scroll(0, 0, "File invalid.");
        rb->lcd_puts_scroll(0, 1, "CRC check failed.");
        break;
    case eBadPlatform:
        rb->lcd_puts_scroll(0, 0, "Wrong file for");
        rb->lcd_puts_scroll(0, 1, "this hardware.");
        break;
    default:
        rb->lcd_puts_scroll(0, 0, "Check failed.");
        break;
    }

    rb->sleep(HZ*3);

    if (rc == eOK)
    {
        rb->lcd_puts_scroll(0, 0, "[On] to program,");
        rb->lcd_puts_scroll(0, 1, "other key to exit.");
    }
    else
    {   /* error occured */
        return;
    }
    
    button = WaitForButton();

    if (button != BUTTON_ON)
    {
        return;
    }
    
    rb->lcd_clear_display();
    rb->lcd_puts_scroll(0, 0, "Are you sure?");
    rb->lcd_puts_scroll(0, 1, "[+] to proceed.");
    
    button = WaitForButton();
    
    if (button != BUTTON_RIGHT)
    {
        return;
    }
    
    rb->lcd_clear_display();
    rb->lcd_puts_scroll(0, 0, "Programming...");
    
    rc = ProgramFirmwareFile(filename, FlashInfo.size);
    
    if (rc)
    {   /* errors */
        rb->lcd_clear_display();
        rb->lcd_puts_scroll(0, 0, "Programming failed!");
        rb->snprintf(buf, sizeof(buf), "%d errors", rc);
        rb->lcd_puts_scroll(0, 1, buf);
        WaitForButton();
    }
    
    rb->lcd_clear_display();
    rb->lcd_puts_scroll(0, 0, "Verifying...");
    
    rc = VerifyFirmwareFile(filename);
    
    rb->lcd_clear_display();
    
    if (rc == 0)
    {
        rb->lcd_puts_scroll(0, 0, "Verify OK.");
    }
    else
    {
        rb->snprintf(buf, sizeof(buf), "Verify failed! %d errors", rc);
        rb->lcd_puts_scroll(0, 0, buf);
    }
    
    rb->lcd_puts_scroll(0, 1, "Press any key to exit.");
    WaitForButton();
}

#endif /* not HAVE_LCD_BITMAP */


/***************** Plugin Entry Point *****************/

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int oldmode;

    /* this macro should be called as the first thing you do in the plugin.
    it test that the api version and model the plugin was compiled for
    matches the machine it is running on */
    TEST_PLUGIN_API(api);
    
    rb = api; /* copy to global api pointer */
    
    /* now go ahead and have fun! */
    oldmode = rb->system_memory_guard(MEMGUARD_NONE); /*disable memory guard */
    DoUserDialog((char*) parameter);
    rb->system_memory_guard(oldmode);              /* re-enable memory guard */

    return PLUGIN_OK;
}

#endif /* ifdef PLATFORM_ID */
#endif /* #ifndef SIMULATOR */
