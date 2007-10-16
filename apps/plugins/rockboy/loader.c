#include "rockmacros.h"
#include "defs.h"
#include "regs.h"
#include "mem.h"
#include "hw.h"
#include "lcd-gb.h"
#include "rtc-gb.h"
#include "save.h"
#include "sound.h"

/* From http://www.semis.demon.co.uk/Gameboy/Gbspec.txt (4/17/2007)
 * Cartridge type:
 *          0 - ROM ONLY                12 - ROM+MBC3+RAM
 *          1 - ROM+MBC1                13 - ROM+MBC3+RAM+BATT
 *          2 - ROM+MBC1+RAM            19 - ROM+MBC5
 *          3 - ROM+MBC1+RAM+BATT       1A - ROM+MBC5+RAM
 *          5 - ROM+MBC2                1B - ROM+MBC5+RAM+BATT
 *          6 - ROM+MBC2+BATTERY        1C - ROM+MBC5+RUMBLE
 *          8 - ROM+RAM                 1D - ROM+MBC5+RUMBLE+SRAM
 *          9 - ROM+RAM+BATTERY         1E - ROM+MBC5+RUMBLE+SRAM+BATT
 *          B - ROM+MMM01               1F - Pocket Camera
 *          C - ROM+MMM01+SRAM          FD - Bandai TAMA5
 *          D - ROM+MMM01+SRAM+BATT     FE - Hudson HuC-3
 *          F - ROM+MBC3+TIMER+BATT     FF - Hudson HuC-1
 *         10 - ROM+MBC3+TIMER+RAM+BATT
 *         11 - ROM+MBC3
 */

static const int mbc_table[256] =
{
    MBC_NONE,
    MBC_MBC1,
    MBC_MBC1,
    MBC_MBC1 | MBC_BAT,
    0,
    MBC_MBC2,
    MBC_MBC2 | MBC_BAT,
    0,
    0,
    MBC_BAT,
    0,
    0,
    0,
    MBC_BAT, 
    0, 
    MBC_MBC3 | MBC_BAT | MBC_RTC,
    MBC_MBC3 | MBC_BAT | MBC_RTC,
    MBC_MBC3, 
    MBC_MBC3, 
    MBC_MBC3 | MBC_BAT,
    0,
    0, 
    0, 
    0, 
    0,
    MBC_MBC5, 
    MBC_MBC5,
    MBC_MBC5 | MBC_BAT, 
    MBC_RUMBLE, 
    MBC_RUMBLE, 
    MBC_RUMBLE | MBC_BAT,
    0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    MBC_HUC3, 
    MBC_HUC1
};

static const unsigned short romsize_table[56] =
{
    2,   4,   8,  16,  32,  64, 128, 256,
    512, 0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0, 128, 128, 128,   0
    /* 0, 0, 0, 0, 72, 80, 96  -- actual values but bad to use these! */
};

/* Ram size should be no larger then 16 banks 1Mbit */
static const unsigned char ramsize_table[5] =
{
    0, 1, 1, 4, 16
};

static char *romfile;
static char sramfile[500];
static char rtcfile[500];
static char saveprefix[500];

static int forcebatt, nobatt;
static int forcedmg;

static int memfill = -1, memrand = -1;

static void initmem(void *mem, int size)
{
    char *p = mem;
    if (memrand >= 0)
    {
        srand(memrand ? memrand : -6 ); /* time(0)); */
        while(size--) *(p++) = rand();
    }
    else if (memfill >= 0)
        memset(p, memfill, size);
}

static byte *loadfile(int fd, int *len)
{
    int c;
    byte *d;

    *len=lseek(fd,0,SEEK_END);
    d=malloc((*len)*sizeof(char)+64);
    if(d==0)
    {
        die("Not enough memory");
        return 0;
    }
    lseek(fd,0,SEEK_SET);
    
    c = read(fd, d, *len);

    return d;
}

static int rom_load(void)
{
    int fd;
    byte c, *data, *header;
    int len = 0, rlen;

    fd = open(romfile, O_RDONLY);
    
    if (fd<0)
    {
      die("cannot open rom file %s", romfile);
      return 1;
    }

    data = loadfile(fd, &len);
    close(fd);
    if(data==0)
    {
        die("Not Enough Memory");
        return 1;
    }
    header = data; /* no zip. = decompress(data, &len); */
    
    memcpy(rom.name, header+0x0134, 16);
    if (rom.name[14] & 0x80) rom.name[14] = 0;
    if (rom.name[15] & 0x80) rom.name[15] = 0;
    rom.name[16] = 0;

    c = header[0x0147];
    mbc.type = mbc_table[c]&(MBC_MBC1|MBC_MBC2|MBC_MBC3|MBC_MBC5|MBC_RUMBLE|MBC_HUC1|MBC_HUC3);
    mbc.batt = ((mbc_table[c]&MBC_BAT) && !nobatt) || forcebatt;
    rtc.batt = mbc_table[c]&MBC_RTC;

    if(header[0x0148]<10 || (header[0x0148]>51 && header[0x0148]<55))
        mbc.romsize = romsize_table[header[0x0148]];
    else
    {
        die("unknown ROM size %02X\n", header[0x0148]);
        return 1;
    }
    
    if(header[0x0149]<5)
        mbc.ramsize = ramsize_table[header[0x0149]];
    else
    {
        die("unknown SRAM size %02X\n", header[0x0149]);
        return 1;
    }

    rlen = 16384 * mbc.romsize;
    rom.bank = (void *) data; /* realloc(data, rlen); */
    if (rlen > len) memset(rom.bank[0]+len, 0xff, rlen - len);

    /* This is the size of the ram on the cartridge
     * See http://www.semis.demon.co.uk/Gameboy/Gbspec.txt
     * for a full description. (8192*number of banks)
     */
    ram.sbank = malloc(8192 * mbc.ramsize);
    if(ram.sbank==0 && mbc.ramsize!=0)
    {
        die("Not enough Memory");
        return 1;
    }

    /* ram.ibank = malloc(4096*8); */

    initmem(ram.sbank, 8192 * mbc.ramsize);
    initmem(ram.ibank, 4096 * 8);

    mbc.rombank = 1;
    mbc.rambank = 0;

    c = header[0x0143];
        hw.cgb = ((c == 0x80) || (c == 0xc0)) && !forcedmg;

    return 0;
}

static int sram_load(void)
{
    int fd;
    char meow[500];

    if (!mbc.batt || !*sramfile) return -1;

    /* Consider sram loaded at this point, even if file doesn't exist */
    ram.loaded = 1;

    fd = open(sramfile, O_RDONLY);
        snprintf(meow,499,"Opening %s %d",sramfile,fd);
    rb->splash(HZ*2, meow);        
    if (fd<0) return -1;
        snprintf(meow,499,"Loading savedata from %s",sramfile);
        rb->splash(HZ*2, meow);    
    read(fd,ram.sbank, 8192*mbc.ramsize);
    close(fd);
    
    return 0;
}


static int sram_save(void)
{
    int fd;
    char meow[500];

    /* If we crash before we ever loaded sram, DO NOT SAVE! */
    if (!mbc.batt || !ram.loaded || !mbc.ramsize)
        return -1;
    fd = open(sramfile, O_WRONLY|O_CREAT|O_TRUNC);
    if (fd<0) return -1;
    snprintf(meow,499,"Saving savedata to %s",sramfile);
    rb->splash(HZ*2, meow);
    write(fd,ram.sbank, 8192*mbc.ramsize);
    close(fd);
    
    return 0;
}

static void rtc_save(void)
{
    int fd;
    if (!rtc.batt) return;
    if ((fd = open(rtcfile, O_WRONLY|O_CREAT|O_TRUNC))<0) return;
    rtc_save_internal(fd);
    close(fd);
}

static void rtc_load(void)
{
    int fd;
    if (!rtc.batt) return;
    if ((fd = open(rtcfile, O_RDONLY))<0) return;
    rtc_load_internal(fd);
    close(fd);
}

void cleanup(void)
{
    sram_save();
    rtc_save();
    /* IDEA - if error, write emergency savestate..? */
}

void loader_init(char *s)
{
    romfile = s;
    if(rom_load())
        return;
    rb->splash(HZ/2, rom.name);
    
    snprintf(saveprefix, 499, "%s/%s", savedir, rom.name);

    strcpy(sramfile, saveprefix);
    strcat(sramfile, ".sav");

    strcpy(rtcfile, saveprefix);
    strcat(rtcfile, ".rtc");
    
    sram_load();
    rtc_load();
}
