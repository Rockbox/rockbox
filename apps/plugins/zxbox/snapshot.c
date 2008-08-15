/*
 * Copyright (C) 1996-1998 Szeredi Miklos
 * Email: mszeredi@inf.bme.hu
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. See the file COPYING. 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "zxmisc.h"
#include "helpers.h"
#include "spperif.h"
#include "z80.h"

#include "snapshot.h"
#include "compr.h"
#include "interf.h"

#include "spconf.h"

#include "interf.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#define COMPRESS_SAVE 1

static char quick_snap_file[]= ROCKBOX_DIR "/zxboxq.z80";

typedef struct {
  int isfile;
  int fd;

  unsigned len;
  byte *at;
} SNFILE;


#define sngetc(snfp) ((snfp)->isfile ? getc((snfp)->fd) : snmgetc(snfp))

static int snmgetc(SNFILE *snfp)
{
  if(!snfp->len) return EOF;
  snfp->len--;
  return *snfp->at++;
}

static int snread(void *ptr, int size, SNFILE *snfp)
{
  int i;
  byte *dest;

  if(snfp->isfile)
        return (int) rb->read( snfp->fd,ptr, (size_t) size);

  dest = (byte *) ptr;
  for(i = 0; snfp->len && size; i++, snfp->len--, size--) 
    *dest++ = *snfp->at++;

  return i;
}



/* These structures are taken from 'spconv' by Henk de Groot */

struct sna_s {
  byte i;
  byte lbk;
  byte hbk;
  byte ebk;
  byte dbk;
  byte cbk;
  byte bbk;
  byte fbk;
  byte abk;
  byte l;
  byte h;
  byte e;
  byte d;
  byte c;
  byte b;
  byte iyl;
  byte iyh;
  byte ixl;
  byte ixh;
  byte iff2;
  byte r;
  byte f;
  byte a;
  byte spl;
  byte sph;
  byte im;
  byte border;
};

#define sna_size 27        /* sizeof(struct sna_s)=27 */


struct z80_1_s {
  byte a;              /*00*/
  byte f;           /*01*/
  byte c;           /*02*/
  byte b;           /*03*/
  byte l;           /*04*/
  byte h;           /*05*/
  byte pcl;           /*06*/
  byte pch;           /*07*/
  byte spl;           /*08*/
  byte sph;           /*09*/
  byte i;           /*0A*/
  byte r;           /*0B*/
  byte data;           /*0C*/
  byte e;           /*0D*/
  byte d;           /*0E*/
  byte cbk;           /*0F*/
  byte bbk;           /*10*/
  byte ebk;           /*11*/
  byte dbk;           /*12*/
  byte lbk;           /*13*/
  byte hbk;           /*14*/
  byte abk;           /*15*/
  byte fbk;           /*16*/
  byte iyl;           /*17*/
  byte iyh;           /*18*/
  byte ixl;           /*19*/
  byte ixh;           /*1A*/
  byte iff1;           /*1B*/
  byte iff2;           /*1C*/
  byte im;           /*1D*/
};

#define z80_145_size 0x1e    /* length of z80 V1.45 header */


struct z80_2_s {
/* Extended 2.01 and 3.0 header, flagged with PC=0 */
  byte h2_len_l;       /*1E*/
  byte h2_len_h;       /*1F*/
  byte n_pcl;           /*20*/
  byte n_pch;           /*21*/
  byte hardware;       /*22*/
  byte samram;           /*23*/
  byte if1_paged;      /*24*/
  byte r_ldir_emu;     /*25*/
  byte last_out;       /*26*/
  byte sound_reg[16];  /*27*/

/* Continues with extended 3.0 header, but this part is not used anyway */
};

#define z80_201_ext_size 23    /* length of extended z80 V2.01 header */
#define z80_300_ext_size 54    /* length of extended z80 V3.0  header */


struct z80_page_s {
  byte blklen_l;       /*00*/
  byte blklen_h;       /*01*/
  byte page_num;       /*02*/
};

#define z80_pg_size 3        /* sizeof(struct z80_page_s)=3 */


static int savfd;
static int memptr;

int compr_read_byte(void)
{
  if(memptr < 0x10000) return z80_proc.mem[memptr++];
  else return -1;
}

void compr_put_byte(int i)
{
  putc(i, savfd);
}


#define STORE_NORMAL_REGS(head) \
  head.f = RF;     /* F reg */  \
  head.a = RA;     /* A reg */  \
  head.b = RB;     /* B reg */  \
  head.c = RC;     /* C reg */  \
  head.d = RD;     /* D reg */  \
  head.e = RE;     /* E reg */  \
  head.h = RH;     /* H reg */  \
  head.l = RL;     /* L reg */  \
  head.fbk = FBK;  /* F' reg */ \
  head.abk = ABK;  /* A' reg */ \
  head.bbk = BBK;  /* B' reg */ \
  head.cbk = CBK;  /* C' reg */ \
  head.dbk = DBK;  /* D' reg */ \
  head.ebk = EBK;  /* E' reg */ \
  head.hbk = HBK;  /* H' reg */ \
  head.lbk = LBK;  /* L' reg */ \
  head.iyh = YH;   /* IY reg */ \
  head.iyl = YL;                \
  head.ixh = XH;   /* IX reg */ \
  head.ixl = XL


#define LOAD_NORMAL_REGS(head) \
  RF = head.f;     /* F reg */  \
  RA = head.a;     /* A reg */  \
  RB = head.b;     /* B reg */  \
  RC = head.c;     /* C reg */  \
  RD = head.d;     /* D reg */  \
  RE = head.e;     /* E reg */  \
  RH = head.h;     /* H reg */  \
  RL = head.l;     /* L reg */  \
  FBK = head.fbk;  /* F' reg */ \
  ABK = head.abk;  /* A' reg */ \
  BBK = head.bbk;  /* B' reg */ \
  CBK = head.cbk;  /* C' reg */ \
  DBK = head.dbk;  /* D' reg */ \
  EBK = head.ebk;  /* E' reg */ \
  HBK = head.hbk;  /* H' reg */ \
  LBK = head.lbk;  /* L' reg */ \
  YH = head.iyh;   /* IY reg */ \
  YL = head.iyl;                \
  XH = head.ixh;   /* IX reg */ \
  XL = head.ixl


static void snsh_z80_save(int fd)
{
  struct z80_1_s z80;

  int to_comp = COMPRESS_SAVE;

  STORE_NORMAL_REGS(z80);

  z80.i = RI;     /* I reg */
  z80.r = RR;     /* R reg */

  z80.sph = SPH;  /* SP reg */
  z80.spl = SPL;
  z80.pch = PCH;  /* PC reg */
  z80.pcl = PCL;

  z80.iff1 = z80_proc.iff1;         /* iff1 */
  z80.iff2 = z80_proc.iff2;         /* iff2 */

  z80.im = (z80_proc.it_mode & 0x03) | 0x60;
/*
                        Bit 0-1: Interrupt mode (0, 1 or 2)
                        Bit 2  : 1=Issue 2 emulation
                        Bit 3  : 1=Double interrupt frequency
                        Bit 4-5: 1=High video synchronisation
                                 3=Low video synchronisation
                                 0,2=Normal
                        Bit 6-7: 0=Cursor/Protek/AGF joystick
                                 1=Kempston joystick
                                 2=Sinclair 1 joystick
                                 3=Sinclair 2 joystick
*/

  z80.data = ((RR >> 7) & 0x01) |
           ((z80_proc.ula_outport & 0x07) << 1) |
       (to_comp ? 0x20 : 0);
/*
                        Bit 0  : Bit 7 of the R-register
                        Bit 1-3: Border colour
                        Bit 4  : 1=Basic SamRom switched in
                        Bit 5  : 1=Block of data is compressed
                        Bit 6-7: No meaning
*/
  rb->write(fd,&z80,z80_145_size);

  if(!to_comp)
      rb->write(fd,z80_proc.mem + 0x4000,0xC000);    
  else {
    memptr = 0x4000;
    savfd = fd;
    compr();
  }
}



static void snsh_sna_save(int fd)
{
  struct sna_s sna;
  byte saves1, saves2;

  STORE_NORMAL_REGS(sna);

  sna.i = RI;     /* I reg */
  sna.r = RR;     /* R reg */

  sna.border = z80_proc.ula_outport & 0x07;

  SP -= 2;

  sna.sph = SPH;  /* SP reg */
  sna.spl = SPL;

  saves1 = z80_proc.mem[SP];
  saves2 = z80_proc.mem[(dbyte)(SP+1)]; 
  if(SP >= 0x4000) {
    z80_proc.mem[SP] = PCL;
    if(SP < 0xFFFF) z80_proc.mem[SP+1] = PCH;
  }

  sna.iff2 = z80_proc.iff2 ? 0xff : 0x00;         /* iff2 */

  sna.im = z80_proc.it_mode & 0x03;

  rb->write(fd,&sna, sna_size);
  rb->write(fd,z80_proc.mem + 0x4000, 0xC000);

  if(SP > 0x4000) {
    z80_proc.mem[SP] = saves1;
    if(SP < 0xFFFF) z80_proc.mem[SP+1] = saves2;
  }
  
  SP += 2;
}

#define GET_DATA(c) { \
  if(!datalen) break;        \
  c = sngetc(fp);             \
  if(c == EOF) break;       \
  if(datalen > 0) datalen--; \
} 


static void read_compressed_data(SNFILE *fp, byte *start, unsigned size, 
                 long datalen)
{
  int j;
  int times, last_ed, ch;
  byte *p, *end;
  
  p = start;
  end = start+size;
  last_ed = 0;
  while(p < end) {
    GET_DATA(ch);
    if(ch != 0xED) {
      last_ed = 0;
      *p++ = ch;
    }
    else {
      if(last_ed) {
    last_ed = 0;
    p--;
    GET_DATA(times);
    if(times == 0) break;

    GET_DATA(ch);
    if(p + times > end) {
      put_msg("Warning: Repeat parameter too large in snapshot");
      times = (int) ((long) end - (long) p);
    }
    for(j = 0; j < times; j++) *p++ = ch;
      }
      else {
    last_ed = 1;
    *p++ = 0xED;
      }
    }
  }

  if(datalen < 0) {
    if(sngetc(fp) != 0 || sngetc(fp) != 0xED || 
       sngetc(fp) != 0xED || sngetc(fp) != 0)
      put_msg("Warning: Illegal ending of snapshot");
  }

  if(datalen > 0) {
    while(datalen) {
      if(sngetc(fp) == EOF) break;
      datalen--;
    }
    put_msg("Warning: Page too long in snapshot");
  }

  if(p < end) put_msg("Warning: Page too short in snapshot");
}

static int read_header(void *p, int size, SNFILE *fp)
{
  int res;

  res = snread(p, size, fp);
  if(res != size) {
    put_msg("Error, End Of File in snapshot header");
    return 0;
  }
  return 1;
}

static int read_z80_page(SNFILE *fp)
{
  struct z80_page_s page;
  unsigned datalen;
  unsigned pos = 0;
  int validpage;

  int res;

  res = snread(&page, z80_pg_size, fp);
  if(res == 0) return 0;
  if(res != z80_pg_size) {
    put_msg("Error, End Of File in page header");
    return 0;
  }
  
  datalen = (page.blklen_h << 8) | page.blklen_l;
  
  validpage = 1;
  switch(page.page_num) {
  case 4:
    pos = 0x8000;
    break;

  case 5:
    pos = 0xC000;
    break;

  case 8:
    pos = 0x4000;
    break;
    
  default:
    validpage = 0;
    while(datalen) {
      if(sngetc(fp) == EOF) {
    put_msg("Warning: Page too short in snapshot");
    break;
      }
      datalen--;
    }
  }
  
  if(validpage) read_compressed_data(fp, z80_proc.mem+pos, 0x4000, 
                     (long) datalen);
  return 1;
}


static void snsh_z80_load(SNFILE *fp)
{
  struct z80_1_s z80;

  if(!read_header(&z80, z80_145_size, fp)) return;
  if(z80.pch == 0 && z80.pcl == 0) {
    struct z80_2_s z80_2;
    int ext_size, rem;
    if(!read_header(&z80_2, 2, fp)) return;
    ext_size = z80_2.h2_len_l | (z80_2.h2_len_h << 8);
    if(ext_size < z80_201_ext_size) {
      put_msg("Error in Z80 header");
      return;
    }
    if(!read_header(&z80_2.n_pcl, z80_201_ext_size, fp)) return;
    rem = ext_size - z80_201_ext_size;
    for(; rem; rem--) sngetc(fp);

    if(z80_2.hardware >= 3 && (ext_size == z80_201_ext_size || 
                z80_2.hardware >= 4)) {
      put_msg("Can't load non 48k snapshot");
      return;
    }
    if(z80_2.if1_paged) {
      put_msg("Can't load snapshot: IF1 roma paged in");
      return;
    }

    PCH = z80_2.n_pch;
    PCL = z80_2.n_pcl;

    while(read_z80_page(fp));
  }
  else {
    if(z80.data == 0xFF) z80.data = 1;
    if(z80.data & 0x20) 
      read_compressed_data(fp, z80_proc.mem + 0x4000, 0xC000, -1);
    else {
      if(snread(z80_proc.mem + 0x4000, 0xC000, fp) != 0xC000) 
    put_msg("Warning: Snapshot file too short (z80)");
      else if(sngetc(fp) != EOF) 
    put_msg("Warning: Snapshot file too long");
    }

    PCH = z80.pch;
    PCL = z80.pcl;
  }


  LOAD_NORMAL_REGS(z80);

  RI = z80.i;     /* I reg */
  RR = (z80.r & 0x7F) | ((z80.data & 0x01) << 7);     /* R reg */

  SPH = z80.sph;  /* SP reg */
  SPL = z80.spl;

  z80_proc.ula_outport = (z80_proc.ula_outport & ~(0x07)) |
    ((z80.data >> 1) & 0x07);

/*
                        Bit 0  : Bit 7 of the R-register
                        Bit 1-3: Border colour
                        Bit 4  : 1=Basic SamRom switched in
                        Bit 5  : 1=Block of data is compressed
                        Bit 6-7: No meaning
*/

  z80_proc.iff1 = z80.iff1 ? 1 : 0;
  z80_proc.iff2 = z80.iff2 ? 1 : 0;

  z80_proc.it_mode = z80.im & 0x03;

/*
                        Bit 0-1: Interrupt mode (0, 1 or 2)
                        Bit 2  : 1=Issue 2 emulation
                        Bit 3  : 1=Double interrupt frequency
                        Bit 4-5: 1=High video synchronisation
                                 3=Low video synchronisation
                                 0,2=Normal
                        Bit 6-7: 0=Cursor/Protek/AGF joystick
                                 1=Kempston joystick
                                 2=Sinclair 1 joystick
                                 3=Sinclair 2 joystick
*/

  z80_proc.haltstate = 0;

  sp_init_screen_mark();
}

static void snsh_sna_load(SNFILE *fp)
{
  struct sna_s sna;

  if(!read_header(&sna, sna_size, fp)) return;

  if(snread(z80_proc.mem+0x4000, 0xC000, fp) != 0xC000) 
    put_msg("Warning: Snapshot file too short (sna)");
  else if(sngetc(fp) != EOF) 
    put_msg("Warning: Snapshot file too long");

  LOAD_NORMAL_REGS(sna);

  RI = sna.i;     /* I reg */
  RR = sna.r;     /* R reg */

  z80_proc.ula_outport = (z80_proc.ula_outport & ~(0x07)) |
    (sna.border & 0x07);

  SPH = sna.sph;  /* SP reg */
  SPL = sna.spl;

  PCL = z80_proc.mem[SP];
  if(SP >= 0x4000) z80_proc.mem[SP] = 0;
  SP++;
  PCH = z80_proc.mem[SP];
  if(SP >= 0x4000) z80_proc.mem[SP] = 0;
  SP++;

  z80_proc.iff1 = z80_proc.iff2 = sna.iff2 ? 1 : 0;
  z80_proc.it_mode = sna.im & 0x03;
  
  z80_proc.haltstate = 0;

  sp_init_screen_mark();
}

static void save_snapshot_file_type(char *name, int type)
{
  int snsh;
  snsh = rb->open(name, O_WRONLY);
  if(snsh < 0) {
      snsh = rb->creat(name);
      if(snsh < 0) {
        put_msg("Could not create snapshot file");
        return;
      }
  }
  
  if(type == SN_SNA) snsh_sna_save(snsh);
  else if(type == SN_Z80) snsh_z80_save(snsh);

  rb->close(snsh);
}

void save_snapshot_file(char *name)
{
  int type;

  rb->strncpy(filenamebuf, name, MAXFILENAME-10);
  filenamebuf[MAXFILENAME-10] = '\0';

  type = SN_Z80;
  if(check_ext(filenamebuf, "z80")) type = SN_Z80;
  else if(check_ext(filenamebuf, "sna")) type = SN_SNA;
  else {
    add_extension(filenamebuf, "z80");
    type = SN_Z80;
  }

  save_snapshot_file_type(filenamebuf, type);
  char msgbuf [MAXFILENAME];
  rb->snprintf(msgbuf,MAXFILENAME, "Saved snapshot to file %s", filenamebuf);
  put_msg(msgbuf);
}

void save_quick_snapshot(void)
{
  save_snapshot_file_type(quick_snap_file, SN_Z80);
}

void save_snapshot(void)
{
  char name[MAXFILENAME];
  name[0]='/';
  name[1]='\0';
  put_msg("Enter name of snapshot file to save:");
  if (!rb->kbd_input((char*)&name, sizeof name))
    save_snapshot_file(&name[0]);
}


void load_snapshot_file_type(char *name, int type)
{
  int filetype = FT_SNAPSHOT;
  int snsh;
  SNFILE snfil;

  rb->strncpy(filenamebuf, name, MAXFILENAME-10);
  filenamebuf[MAXFILENAME-10] = '\0';

  spcf_find_file_type(filenamebuf, &filetype, &type);
  if(type < 0) type = SN_Z80;

  snsh = rb->open(filenamebuf, O_RDONLY);
  if(snsh < 0) {
#ifndef USE_GRAY
  rb->splashf(HZ, "Could not open snapshot file `%s'",filenamebuf);
#endif
    return;
  }
  
  snfil.isfile = 1;
  snfil.fd = snsh;

  if(type == SN_SNA) snsh_sna_load(&snfil);
  else if(type == SN_Z80) snsh_z80_load(&snfil);

  rb->close(snsh);
}

void snsh_z80_load_intern(byte *p, unsigned len)
{
  SNFILE snfil;
  
  snfil.isfile = 0;
  snfil.at = p;
  snfil.len = len;
  
  snsh_z80_load(&snfil);
}

void load_quick_snapshot(void)
{
  int  qsnap;
  qsnap = rb->open(quick_snap_file,O_RDONLY);
  if(qsnap < 0) {
    put_msg("No quick snapshot saved yet");
    return;
  }
  else
	  rb->close ( qsnap );
  load_snapshot_file_type(quick_snap_file, SN_Z80);
}


void load_snapshot(void)
{
  char *name;

  put_msg("Enter name of snapshot file to load:");

  name = spif_get_filename();
  if(name == NULL) return;

  load_snapshot_file_type(name, -1);
}

