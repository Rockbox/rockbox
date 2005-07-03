
#include <stdio.h>
#include <string.h>


#include "rockmacros.h"
#include "defs.h"
#include "regs.h"
#include "mem.h"
#include "hw.h"
#include "lcd-gb.h"
#include "rtc-gb.h"
#include "rc.h"
#include "save.h"
#include "sound.h"


static int mbc_table[256] =
{
	0, 1, 1, 1, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 3,
	3, 3, 3, 3, 0, 0, 0, 0, 0, 5, 5, 5, MBC_RUMBLE, MBC_RUMBLE, MBC_RUMBLE, 0,
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
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, MBC_HUC3, MBC_HUC1
};

static int rtc_table[256] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0
};

static int batt_table[256] =
{
	0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0,
	1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0,
	0
};

static int romsize_table[256] =
{
	2, 4, 8, 16, 32, 64, 128, 256, 512,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 128, 128, 128
	/* 0, 0, 72, 80, 96  -- actual values but bad to use these! */
};

static int ramsize_table[256] =
{
	1, 1, 1, 4, 16,
	4 /* FIXME - what value should this be?! */
};


static char *romfile;
static char sramfile[500];
static char rtcfile[500];
static char saveprefix[500];

static char *savename;
static char *savedir = "/.rockbox/rockboy";

static int saveslot;

static int forcebatt, nobatt;
static int forcedmg;

static int memfill = -1, memrand = -1;

//static byte romMemory[4*1025*1024];
int mp3_buffer_size;
void *bufferpos;

static void initmem(void *mem, int size)
{
	char *p = mem;
	if (memrand >= 0)
	{
		srand(memrand ? memrand : -6 ); //time(0));
		while(size--) *(p++) = rand();
	}
	else if (memfill >= 0)
		memset(p, memfill, size);
}

static byte *loadfile(int fd, int *len)
{
	int c, l = 0, p = 0;
	
	byte *d, buf[512];
	d=malloc(32768);
	for(;;)
	{
		c = read(fd, buf, sizeof buf);
		if (c <= 0) break;
		l += c;
		memcpy(d+p, buf, c);
		p += c;
	}
	setmallocpos(d+p+64);
	*len = l;
	return d;
}

//static byte sram[65536];

int rom_load(void)
{
	int fd;
	byte c, *data, *header;
	int len = 0, rlen;

	fd = open(romfile, O_RDONLY);
	
	if (fd<0) {
	  die("cannot open rom file");
	  die(romfile);
	  return 1;
	}

	data = loadfile(fd, &len);
	header = data; // no zip. = decompress(data, &len);
	
	memcpy(rom.name, header+0x0134, 16);
	if (rom.name[14] & 0x80) rom.name[14] = 0;
	if (rom.name[15] & 0x80) rom.name[15] = 0;
	rom.name[16] = 0;

	c = header[0x0147];
	mbc.type = mbc_table[c];
	mbc.batt = (batt_table[c] && !nobatt) || forcebatt;
//	mbc.batt = 1; // always store savegame mem.
	rtc.batt = rtc_table[c];
	mbc.romsize = romsize_table[header[0x0148]];
	mbc.ramsize = ramsize_table[header[0x0149]];

	if (!mbc.romsize)  { 
		die("unknown ROM size %02X\n", header[0x0148]);
		return 1;
	}
	if (!mbc.ramsize) { 
		die("unknown SRAM size %02X\n", header[0x0149]);
		return 1;
	}

	rlen = 16384 * mbc.romsize;
	rom.bank = (void *) data; //realloc(data, rlen);
	if (rlen > len) memset(rom.bank[0]+len, 0xff, rlen - len);

	ram.sbank = malloc(8192 * mbc.ramsize);
	//ram.ibank = malloc(4096*8);

	initmem(ram.sbank, 8192 * mbc.ramsize);
	initmem(ram.ibank, 4096 * 8);

	mbc.rombank = 1;
	mbc.rambank = 0;

	c = header[0x0143];
        hw.cgb = ((c == 0x80) || (c == 0xc0)) && !forcedmg;
	hw.gba = 0; //(hw.cgb && gbamode);
		
	close(fd);

	return 0;
}

int sram_load(void)
{
	int fd;
	char meow[500];

	if (!mbc.batt || !sramfile || !*sramfile) return -1;

	/* Consider sram loaded at this point, even if file doesn't exist */
	ram.loaded = 1;

	fd = open(sramfile, O_RDONLY);
        snprintf(meow,499,"Opening %s %d",sramfile,fd);
	rb->splash(HZ*2, true, meow);		
	if (fd<0) return -1;
        snprintf(meow,499,"Loading savedata from %s",sramfile);
        rb->splash(HZ*2, true, meow);	
	read(fd,ram.sbank, 8192*mbc.ramsize);
	close(fd);
	
	return 0;
}


int sram_save(void)
{
	int fd;
	char meow[500];

	/* If we crash before we ever loaded sram, DO NOT SAVE! */
	if (!mbc.batt || !sramfile || !ram.loaded || !mbc.ramsize)
		return -1;
	fd = open(sramfile, O_WRONLY|O_CREAT|O_TRUNC);
//	snprintf(meow,499,"Opening %s %d",sramfile,fd);
//	rb->splash(HZ*2, true, meow);
	if (fd<0) return -1;
	snprintf(meow,499,"Saving savedata to %s",sramfile);
	rb->splash(HZ*2, true, meow);
	write(fd,ram.sbank, 8192*mbc.ramsize);
	close(fd);
	
	return 0;
}


void state_save(int n)
{
	int fd;
	char name[500];

	if (n < 0) n = saveslot;
	if (n < 0) n = 0;
	snprintf(name, 499,"%s.%03d", saveprefix, n);

	if ((fd = open(name, O_WRONLY|O_CREAT|O_TRUNC)>=0))
	{
		savestate(fd);
		close(fd);
	}
}


void state_load(int n)
{
	int fd;
	char name[500];

	if (n < 0) n = saveslot;
	if (n < 0) n = 0;
	snprintf(name, 499, "%s.%03d", saveprefix, n);

	if ((fd = open(name, O_RDONLY)>=0))
	{
		loadstate(fd);
		close(fd);
		vram_dirty();
		pal_dirty();
		sound_dirty();
		mem_updatemap();
	}
}

void rtc_save(void)
{
	int fd;
	if (!rtc.batt) return;
	if ((fd = open(rtcfile, O_WRONLY|O_CREAT|O_TRUNC))<0) return;
	rtc_save_internal(fd);
	close(fd);
}

void rtc_load(void)
{
	int fd;
	if (!rtc.batt) return;
	if ((fd = open(rtcfile, O_RDONLY))<0) return;
	rtc_load_internal(fd);
	close(fd);
}


void loader_unload(void)
{
	sram_save();
//	if (romfile) free(romfile);
//	if (sramfile) free(sramfile);
//	if (saveprefix) free(saveprefix);
//	if (rom.bank) free(rom.bank);
//	if (ram.sbank) free(ram.sbank);
	romfile =   0;
	rom.bank = 0;
	ram.sbank = 0;
	mbc.type = mbc.romsize = mbc.ramsize = mbc.batt = 0;
}

/*
static char *base(char *s)
{
	char *p;
	p = strrchr(s, '/');
	if (p) return p+1;
	return s;
}


static char *ldup(char *s)
{
	int i;
	char *n, *p;
	p = n = malloc(strlen(s));
	for (i = 0; s[i]; i++) if (isalnum(s[i])) *(p++) = tolower(s[i]);
	*p = 0;
	return n;
}*/

void cleanup(void)
{
	sram_save();
	rtc_save();
	// IDEA - if error, write emergency savestate..? 
}

void loader_init(char *s)
{
	char *name;
	DIR* dir;

//	sys_checkdir(savedir, 1); /* needs to be writable */
	dir=opendir(savedir);
	if(!dir)
	  mkdir(savedir,0);
	else
	  closedir(dir);

	romfile = s;
	if(rom_load())
		return;
	vid_settitle(rom.name);
	name = rom.name;
	
	snprintf(saveprefix, 499, "%s/%s", savedir, name);

	strcpy(sramfile, saveprefix);
	strcat(sramfile, ".sav");

	strcpy(rtcfile, saveprefix);
	strcat(rtcfile, ".rtc");
	
	sram_load();
	rtc_load();

	//atexit(cleanup);
}

rcvar_t loader_exports[] =
{
	RCV_STRING("savedir", &savedir),
	RCV_STRING("savename", &savename),
	RCV_INT("saveslot", &saveslot),
	RCV_BOOL("forcebatt", &forcebatt),
	RCV_BOOL("nobatt", &nobatt),
	RCV_BOOL("forcedmg", &forcedmg),
	RCV_INT("memfill", &memfill),
	RCV_INT("memrand", &memrand),
	RCV_END
};









