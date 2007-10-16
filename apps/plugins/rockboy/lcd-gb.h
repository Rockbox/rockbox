

#ifndef __LCD_GB_H__
#define __LCD_GB_H__

#include "defs.h"

struct vissprite
{
    byte *buf;
    int x;
    byte pal, pri;
};

struct scan
{
    int bg[64];
    int wnd[64];
#if LCD_DEPTH == 1
    byte buf[8][256];
#elif LCD_DEPTH == 2
    byte buf[4][256];
#elif LCD_DEPTH > 4
    byte buf[256];
#endif
    un16 pal[64];
    byte pri[256];
    struct vissprite vs[16];
    int ns, l, x, y, s, t, u, v, wx, wy, wt, wv;
};

struct obj
{
    byte y;
    byte x;
    byte pat;
    byte flags;
};

struct lcd
{
    byte vbank[2][8192];
    union
    {
        byte mem[256];
        struct obj obj[40];
    } oam;
    byte pal[128];
};

extern struct lcd lcd;
extern struct scan scan;

void lcd_begin(void) ICODE_ATTR;
void lcd_refreshline(void) ICODE_ATTR;
void pal_write(int i, byte b) ICODE_ATTR;
void pal_write_dmg(int i, int mapnum, byte d) ICODE_ATTR;
void vram_write(addr a, byte b) ICODE_ATTR;
void vram_dirty(void) ICODE_ATTR;
void pal_dirty(void) ICODE_ATTR;
void lcd_reset(void);

#endif



