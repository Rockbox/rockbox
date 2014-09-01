/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Rockbox port of InfoNES
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

#include "plugin.h"
/*PLUGIN_HEADER*/

#include "InfoNES.h"
#include "InfoNES_System.h"
#include "InfoNES_pAPU.h"
#include "keymaps.h"

int start_application(char *filename);
void poll_event(void);
int LoadSRAM(void);
int SaveSRAM(void);
void get_more(unsigned char **start, size_t *size);
void save(void);
void load(void);
void menu(void);
void sound_onoff(void);
void video_menu(void); 
void set_display_mode(void);
void set_frameskip(void);

char szRomName[ 256 ];
char szSaveName[ 256 ];
int nSRAM_SaveFlag;

extern struct ApuEvent_t *ApuEventQueue;
bool quit = false;
bool sound_on;
BYTE final_wave[2048 * 2];
int waveptr;
int wavflag;
int wavdone;

WORD *wfx;

DWORD dwPad1;
DWORD dwPad2;
DWORD dwSystem;

WORD NesPalette[ 64 ] =
{
LCD_RGBPACK(117, 117, 117) , LCD_RGBPACK(39, 27, 143) , LCD_RGBPACK(0, 0, 171) , LCD_RGBPACK(71, 0, 159) , LCD_RGBPACK(143, 0, 119) , LCD_RGBPACK(171, 0, 19) , LCD_RGBPACK(167, 0, 0) , LCD_RGBPACK(127, 11, 0) , LCD_RGBPACK(67, 47, 0) , LCD_RGBPACK(0, 71, 0) , LCD_RGBPACK(0, 81, 0) , LCD_RGBPACK(0, 63, 23) , LCD_RGBPACK(27, 63, 95) , LCD_RGBPACK(0, 0, 0) , LCD_RGBPACK(0, 0, 0) , LCD_RGBPACK(0, 0, 0) , LCD_RGBPACK(188, 188, 188) , LCD_RGBPACK(0, 115, 239) , LCD_RGBPACK(35, 59, 239) , LCD_RGBPACK(131, 0, 243) , LCD_RGBPACK(191, 0, 191) , LCD_RGBPACK(231, 0, 91) , LCD_RGBPACK(219, 43, 0) , LCD_RGBPACK(203, 79, 15) , LCD_RGBPACK(139, 115, 0) , LCD_RGBPACK(0, 151, 0) , LCD_RGBPACK(0, 171, 0) , LCD_RGBPACK(0, 147, 59) , LCD_RGBPACK(0, 131, 139) , LCD_RGBPACK(0, 0, 0) , LCD_RGBPACK(0, 0, 0) , LCD_RGBPACK(0, 0, 0) , LCD_RGBPACK(255, 255, 255) , LCD_RGBPACK(63, 191, 255) , LCD_RGBPACK(95, 151, 255) , LCD_RGBPACK(167, 139, 253) , LCD_RGBPACK(247, 123, 255) , LCD_RGBPACK(255, 119, 183) , LCD_RGBPACK(255, 119, 99) , LCD_RGBPACK(255, 155, 59) , LCD_RGBPACK(243, 191, 63) , LCD_RGBPACK(131, 211, 19) , LCD_RGBPACK(79, 223, 75) , LCD_RGBPACK(88, 248, 152) , LCD_RGBPACK(0, 235, 219) , LCD_RGBPACK(0, 0, 0) , LCD_RGBPACK(0, 0, 0) , LCD_RGBPACK(0, 0, 0) , LCD_RGBPACK(255, 255, 255) , LCD_RGBPACK(171, 231, 255) , LCD_RGBPACK(199, 215, 255) , LCD_RGBPACK(215, 203, 255) , LCD_RGBPACK(255, 199, 255) , LCD_RGBPACK(255, 199, 219) , LCD_RGBPACK(255, 191, 179) , LCD_RGBPACK(255, 219, 171) , LCD_RGBPACK(255, 231, 163) , LCD_RGBPACK(227, 255, 163) , LCD_RGBPACK(171, 243, 191) , LCD_RGBPACK(179, 255, 207) , LCD_RGBPACK(159, 255, 243) , LCD_RGBPACK(0, 0, 0) , LCD_RGBPACK(0, 0, 0) , LCD_RGBPACK(0, 0, 0)
};

BYTE NesPaletteRGB[64][3] = {
 { 112, 112, 112, },{ 32,  24, 136, },{  0,   0, 168, },{ 64,   0, 152,},
 { 136,   0, 112, },{168,   0,  16, },{160,   0,   0, },{120,   8,   0,},
 {  64,  40,   0, },{  0,  64,   0, },{  0,  80,   0, },{  0,  56,  16,},
 {  24,  56,  88, },{  0,   0,   0, },{  0,   0,   0, },{  0,   0,   0,},
 { 184, 184, 184, },{  0, 112, 232, },{ 32,  56, 232, },{128,   0, 240,},
 { 184,   0, 184, },{224,   0,  88, },{216,  40,   0, },{200,  72,   8,},
 { 136, 112,   0, },{  0, 144,   0, },{  0, 168,   0, },{  0, 144,  56,},
 {   0, 128, 136, },{  0,   0,   0, },{  0,   0,   0, },{  0,   0,   0,},
 { 248, 248, 248, },{ 56, 184, 248, },{ 88, 144, 248, },{ 64, 136, 248,},
 { 240, 120, 248, },{248, 112, 176, },{248, 112,  96, },{248, 152,  56,},
 { 240, 184,  56, },{128, 208,  16, },{ 72, 216,  72, },{ 88, 248, 152,},
 {   0, 232, 216, },{  0,   0,   0, },{  0,   0,   0, },{  0,   0,   0,},
 { 248, 248, 248, },{168, 224, 248, },{192, 208, 248, },{208, 200, 248,},
 { 248, 192, 248, },{248, 192, 216, },{248, 184, 176, },{248, 216, 168,},
 { 248, 224, 160, },{224, 248, 160, },{168, 240, 184, },{176, 248, 200,},
 { 152, 248, 240, },{  0,   0,   0, },{  0,   0,   0, },{  0,   0,   0 }
};

/* main plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    size_t size = 0xffff;
    sound_on = false;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif
    rb->backlight_on();
    rb->lcd_clear_display();

    VROM = (BYTE*)rb->plugin_get_audio_buffer(&size);
    ROM = (BYTE*) VROM + 0xffff * 8;
    wfx = (WORD*)(ROM + 0xffff * 16);
    ApuEventQueue = (struct ApuEvent_t*)(wfx + LCD_HEIGHT * NES_DISP_WIDTH);

  /* Start ROM file if specified */
  if (parameter)
  {
    if (start_application((char*) parameter))
    {
      /* MainLoop */
      InfoNES_Main();

      /* End */
      SaveSRAM();
    }
    else
    {
      rb->splash(HZ, "Not a valid NES ROM!!");
      return PLUGIN_OK;
    }
  }
  else
  {
     rb->splash(HZ*3/2, "Play NES ROM file!");
     return PLUGIN_OK;
  }
return PLUGIN_OK;
}

/* Start application */
int start_application(char *filename)
{
  rb->strcpy( szRomName, filename );

  if(InfoNES_Load(szRomName)==0) {
    LoadSRAM();
    return 1;
  }
  return 0;
}

/* Load SRAM */
int LoadSRAM(void)
{
  int fp;
  unsigned char pSrcBuf[ SRAM_SIZE ];
  unsigned char chData;
  unsigned char chTag;
  int nRunLen;
  int nDecoded;
  int nDecLen;
  int nIdx;

  nSRAM_SaveFlag = 0;

  if ( !ROM_SRAM )
    return 0;

  nSRAM_SaveFlag = 1;

  rb->strcpy( szSaveName, szRomName );
  rb->strcpy( rb->strrchr( szSaveName, '.' ) + 1, "srm" );

  fp = rb->open( szSaveName, O_RDWR | O_CREAT); 
  if ( !fp )
    return -1;

  rb->read(fp, pSrcBuf, SRAM_SIZE);
  rb->close( fp );

  nDecoded = 0;
  nDecLen = 0;

  chTag = pSrcBuf[ nDecoded++ ];

  while ( nDecLen < 8192 )
  {
    chData = pSrcBuf[ nDecoded++ ];

    if ( chData == chTag )
    {
      chData = pSrcBuf[ nDecoded++ ];
      nRunLen = pSrcBuf[ nDecoded++ ];
      for ( nIdx = 0; nIdx < nRunLen + 1; ++nIdx )
      {
        SRAM[ nDecLen++ ] = chData;
      }
    }
    else
    {
      SRAM[ nDecLen++ ] = chData;
    }
  }
  return 0;
}

int SaveSRAM(void)
{
  int fp;
  int nUsedTable[ 256 ];
  unsigned char chData;
  unsigned char chPrevData;
  unsigned char chTag;
  int nIdx;
  int nEncoded;
  int nEncLen;
  int nRunLen;
  unsigned char pDstBuf[ SRAM_SIZE ];

  if ( !nSRAM_SaveFlag )
    return 0;

  rb->memset( nUsedTable, 0, sizeof nUsedTable );

  for ( nIdx = 0; nIdx < SRAM_SIZE; ++nIdx )
  {
    ++nUsedTable[ SRAM[ nIdx++ ] ];
  }
  for ( nIdx = 1, chTag = 0; nIdx < 256; ++nIdx )
  {
    if ( nUsedTable[ nIdx ] < nUsedTable[ chTag ] )
      chTag = nIdx;
  }

  nEncoded = 0;
  nEncLen = 0;
  nRunLen = 1;

  pDstBuf[ nEncLen++ ] = chTag;

  chPrevData = SRAM[ nEncoded++ ];

  while ( nEncoded < SRAM_SIZE && nEncLen < SRAM_SIZE - 133 )
  {
    chData = SRAM[ nEncoded++ ];

    if ( chPrevData == chData && nRunLen < 256 )
      ++nRunLen;
    else
    {
      if ( nRunLen >= 4 || chPrevData == chTag )
      {
        pDstBuf[ nEncLen++ ] = chTag;
        pDstBuf[ nEncLen++ ] = chPrevData;
        pDstBuf[ nEncLen++ ] = nRunLen - 1;
      }
      else
      {
        for ( nIdx = 0; nIdx < nRunLen; ++nIdx )
          pDstBuf[ nEncLen++ ] = chPrevData;
      }

      chPrevData = chData;
      nRunLen = 1;
    }

  }
  if ( nRunLen >= 4 || chPrevData == chTag )
  {
    pDstBuf[ nEncLen++ ] = chTag;
    pDstBuf[ nEncLen++ ] = chPrevData;
    pDstBuf[ nEncLen++ ] = nRunLen - 1;
  }
  else
  {
    for ( nIdx = 0; nIdx < nRunLen; ++nIdx )
      pDstBuf[ nEncLen++ ] = chPrevData;
  }

  fp = rb->open( szSaveName, O_CREAT | O_TRUNC | O_WRONLY );
  if ( !fp )
  return -1;

  rb->write(fp, pDstBuf, nEncLen);
  rb->close( fp );
  return 0;
}

/* Menu Screen */
int InfoNES_Menu(void){
  if(quit) return -1;
  return 0;
}

/* Read ROM image file */
int InfoNES_ReadRom( const char *pszFileName ){
  int fp;

  fp=rb->open(pszFileName, O_RDONLY);
  if(!fp) return -1;

  rb->read( fp, &NesHeader, sizeof NesHeader);
  if( rb->memcmp( NesHeader.byID, "NES\x1a", 4 )!=0){
    rb->close( fp );
    return -1;
  }

  rb->memset( SRAM, 0, SRAM_SIZE );

  if(NesHeader.byInfo1 & 4){
    rb->read(fp, &SRAM[ 0x1000 ], 512);
  }

  rb->read(fp, ROM, 0x4000 * NesHeader.byRomSize);

  if(NesHeader.byVRomSize>0){
    rb->read(fp, VROM, 0x2000 * NesHeader.byVRomSize);
  }

  rb->close( fp );
  return 0;

}

/* Release memory for ROM */
void InfoNES_ReleaseRom(void){
  return;
}

/* Screen scale, shrink, and resolution change defines */
int DisplaySize = 0;
#define scale   0
#define center  1
#define half    2

/* Transfer work frame contents to screen */
void InfoNES_LoadFrame(void){
int x, y;

switch ( DisplaySize )
{
    case scale: /* Scaled to fit display */
        for(y = 0; y < LCD_HEIGHT; y++) {
        for(x = 0; x < LCD_WIDTH; x++) {
            rb->lcd_framebuffer[y * LCD_WIDTH + x] = WorkFrame[x * NES_DISP_WIDTH / LCD_WIDTH + y * NES_DISP_HEIGHT / LCD_HEIGHT * NES_DISP_WIDTH];
	}}
    break;

    case center: /* Actual NES screen resolution */
        for(y = 0; y < NES_DISP_HEIGHT; y++) {
        for(x = 0; x < NES_DISP_WIDTH; x++) {
            rb->lcd_set_foreground(WorkFrame[x + y * NES_DISP_WIDTH]);
            rb->lcd_drawpixel(x, y);
        }}
    break;

    case half: /* Half NES screen resolution */
        for(y = 0; y < NES_DISP_HEIGHT; y += 2) {
        for(x = 0; x < NES_DISP_WIDTH; x += 2) {
            rb->lcd_set_foreground(WorkFrame[x + y * NES_DISP_WIDTH]);
            rb->lcd_drawpixel(x/2, y/2);
        }}
    break;
}

rb->lcd_update();
}

#ifndef CPUFREQ_MAX
#define CPUFREQ_MAX 80000000l
#endif

/* Get a joypad state */
void InfoNES_PadState( DWORD *pdwPad1, DWORD *pdwPad2, DWORD *pdwSystem )
{
poll_event();
*pdwPad1 = dwPad1;
*pdwPad2 = dwPad2;
*pdwSystem = dwSystem;
}

static const int joy_commit_range = 3276;

#ifdef INFONES_SCROLLWHEEL
/* Scrollwheel events are posted directly and not polled by the button
   driver - synthesize polling */
static inline unsigned int read_scroll_wheel(void)
{
    unsigned int buttons = BUTTON_NONE;
    unsigned int btn;

    /* Empty out the button queue and see if any scrollwheel events were
       posted */
    do
    {
        btn = rb->button_get_w_tmo(0);
        buttons |= btn;
    }
    while (btn != BUTTON_NONE);

    return buttons & (SCROLL_CC | SCROLL_CW);
}
#endif

void poll_event(void)
{
    int bs;

    bs = rb->button_status();

#ifdef INFONES_SCROLLWHEEL
    bs |= read_scroll_wheel();
#endif

    if(bs & NES_BUTTON_UP)
        dwPad1 |= 1 << 4;
    else
        dwPad1 &= ~(1 << 4);
    if(bs & NES_BUTTON_DOWN)
        dwPad1 |= 1 << 5;
    else
        dwPad1 &= ~(1 << 5);
    if(bs & NES_BUTTON_LEFT)
        dwPad1 |= 1 << 6;
    else
        dwPad1 &= ~(1 << 6);
    if(bs & NES_BUTTON_RIGHT)
        dwPad1 |= 1 << 7;
    else
        dwPad1 &= ~(1 << 7);
    if(bs & NES_BUTTON_A)
        dwPad1 |= 1 << 0;
    else
        dwPad1 &= ~(1 << 0);
    if(bs & NES_BUTTON_B)
        dwPad1 |= 1 << 1;
    else
        dwPad1 &= ~(1 << 1);
    if(bs & NES_BUTTON_SELECT)
        dwPad1 |= 1 << 2;
    else
        dwPad1 &= ~(1 << 2);
    if(bs & NES_BUTTON_START)
        dwPad1 |= 1 << 3;
    else
        dwPad1 &= ~(1 << 3);

    rb->yield();

    if(bs & NES_BUTTON_MENU)
        menu();

    return;
}

/* memcpy */
void *InfoNES_MemoryCopy( void *dest, const void *src, int count ){
  rb->memcpy( dest, src, count );
  return dest;
}

/* memset */
void *InfoNES_MemorySet( void *dest, int c, int count )
{
  rb->memset( dest, c, count);
  return dest;
}

/* Print debug message */
void InfoNES_DebugPrint( char *pszMsg )
{
  rb->splashf(HZ*3/2, "%s\n", pszMsg);
}

/* Wait */
void InfoNES_Wait(void)
{
}

/* Sound Initialize */
void InfoNES_SoundInit( void )
{
}

void waveout(void *udat,BYTE *stream,int len)
{
    (void)stream;
    (void)udat;

    if(!wavdone)
    {
	rb->pcm_play_data(NULL, NULL, &final_wave[(wavflag -1) << 10], len);
	wavflag = 0; wavdone= 1;
    }
}

void get_more(unsigned char **start, size_t *size)
{
  static unsigned char buf[4048];
  int i;
  if ( wavflag )
  {
	for(i = 0; i < 4048; i++)
	buf[i] = (&final_wave[(wavflag - 1) << 10])[i/4];
	*start = buf;
	*size = 4048;
	wavdone = 1;
  }

	return;
}

/* Sound Open */
int InfoNES_SoundOpen( int samples_per_sync, int sample_rate )
{
    (void)samples_per_sync;
    rb->pcm_set_frequency(11025);
    sample_rate=11025;

    return 1;
}

/* Sound Close */
void InfoNES_SoundClose( void )
{
}

void InfoNES_SoundOutput( int samples, BYTE *wave1, BYTE *wave2, BYTE *wave3, BYTE *wave4, BYTE *wave5 )
{
  int i;

  {
    for (i = 0; i < samples; i++)
    {
#if 1
      final_wave[ waveptr ] =
	( wave1[i] + wave2[i] + wave3[i] + wave4[i] + wave5[i] ) / 5;
#else
      final_wave[ waveptr ] = wave4[i];
#endif

     waveptr++;
      if ( waveptr == 2048 )
      {
	waveptr = 0;
	wavflag = 2;
      }
      else if ( waveptr == 1024 )
      {
	wavflag = 1;
      }
    }

    if ( wavflag )
    {
      if(sound_on)
      {
          rb->pcm_play_data(&get_more, NULL, NULL, 0);
          rb->yield();
      }
     wavflag = 0;
    }
  }
}


/* Print system message */
void InfoNES_MessageBox(char *pszMsg, ...)
{
    (void)pszMsg;
}

void menu(void)
{
    MENUITEM_STRINGLIST(menu, "Menu", NULL, "Load", "Save", "Video", "Sound", "Quit");

    switch (rb->do_menu(&menu, NULL, NULL, false))
    {
        case 0:
            load();
            break;
        case 1:
            save();
            break;
        case 2:
            video_menu();
            break;
        case 3:
            sound_onoff();
            break;
        case 4:
            dwSystem|=PAD_SYS_QUIT;
            quit=1;
            break;
        default:
            break;	
    }
    return;
}

void sound_onoff(void)
{
    MENUITEM_STRINGLIST(smenu, "Sound", NULL, "On", "Off");

    switch(rb->do_menu(&smenu, NULL, NULL, false))
    {
        case 0:
	    sound_on = true;
            return;
        case 1:
            sound_on = false;
            return;
        default:
            return;
    }
}

void video_menu(void)
{
    MENUITEM_STRINGLIST(smenu, "Video", NULL, "Display Size", "Frameskip");

    switch(rb->do_menu(&smenu, NULL, NULL, false))
    {
        case 0:
            set_display_mode();
            break;
        case 1:
            set_frameskip();
            break;
        default:
            break;
    }
}

void set_display_mode(void)
{
    MENUITEM_STRINGLIST(smenu, "Display Size", NULL, "Stretch", "Centered", "Half Size");

    switch(rb->do_menu(&smenu, NULL, NULL, false))
    {
        case 0:
            DisplaySize = scale;
            return;
        case 1:
            DisplaySize = center;
            return;
        case 2:
            DisplaySize = half;
            return;
        default:
            return;
    }
}

void set_frameskip(void)
{
    MENUITEM_STRINGLIST(smenu, "Frameskip", NULL, "0", "1", "2", "3", "4", "5");

    switch(rb->do_menu(&smenu, NULL, NULL, false))
    {
        case 0:
            FrameSkip = 0;
            return;
        case 1:
            FrameSkip = 1;
            return;
        case 2:
            FrameSkip = 2;
            return;
        case 3:
            FrameSkip = 3;
            return;
        case 4:
            FrameSkip = 4;
            return;
        case 5:
            FrameSkip = 5;
            return;
        default:
            return;
    }
}


#include "InfoNES.h"
#include "InfoNES_Mapper.h"
extern WORD PC; extern BYTE SP; extern BYTE F; extern BYTE A; extern BYTE X; extern BYTE Y;
extern BYTE RAM[ RAM_SIZE ]; extern BYTE SRAM[ SRAM_SIZE ]; extern BYTE PPURAM[ PPURAM_SIZE ]; extern BYTE SPRRAM[ SPRRAM_SIZE ];
extern BYTE ChrBuf[ 256 * 2 * 8 * 8 ];
extern BYTE *ROMBANK0; extern BYTE *ROMBANK1; extern BYTE *ROMBANK2; extern BYTE *ROMBANK3;
extern BYTE IRQ_State;

extern BYTE IRQ_Wiring;
extern BYTE NMI_State;
extern BYTE NMI_Wiring;
extern WORD g_wPassedClocks;
extern BYTE g_byTestTable[ 256 ];

extern int SpriteJustHit;

extern struct value_table_tag g_ASLTable[ 256 ];
extern struct value_table_tag g_LSRTable[ 256 ];
extern struct value_table_tag g_ROLTable[ 2 ][ 256 ];
extern struct value_table_tag g_RORTable[ 2 ][ 256 ];

extern DWORD Map1_bank1;
extern DWORD Map1_bank2;
extern DWORD Map1_bank3;
extern DWORD Map1_bank4;
extern BYTE  Map1_Regs[ 4 ];
extern DWORD Map1_Cnt;
extern BYTE  Map1_Latch;
extern WORD  Map1_Last_Write_Addr;
extern DWORD Map1_HI1;
extern DWORD Map1_HI2;

extern enum Map1_Size_t Map1_Size;

extern DWORD Map1_256K_base;
extern DWORD Map1_swap;
extern BYTE *SRAMBANK;

extern BYTE PPU_R0;
extern BYTE PPU_R1;
extern BYTE PPU_R2;
extern BYTE PPU_R3;
extern BYTE PPU_R7;

extern BYTE  Map4_Regs[ 8 ];
extern DWORD Map4_Rom_Bank;
extern DWORD Map4_Prg0, Map4_Prg1;
extern DWORD Map4_Chr01, Map4_Chr23;
extern DWORD Map4_Chr4, Map4_Chr5, Map4_Chr6, Map4_Chr7;

extern BYTE Map4_IRQ_Enable;
extern BYTE Map4_IRQ_Cnt;
extern BYTE Map4_IRQ_Latch;
extern BYTE Map4_IRQ_Request;
extern BYTE Map4_IRQ_Present;
extern BYTE Map4_IRQ_Present_Vbl;

void save(void)
{
	int fd;
	char fname[256];
	rb->strcpy(fname, szRomName);
	rb->strcpy( rb->strrchr( fname, '.' ) + 1, "sav" );
	
	fd = rb->open(fname, O_WRONLY | O_CREAT | O_TRUNC);

	rb->write(fd, &PC, 2);
	rb->write(fd, &SP, 1);
	rb->write(fd, &F, 1);
	rb->write(fd, &A, 1);
	rb->write(fd, &X, 1);
	rb->write(fd, &Y, 1);
	rb->write(fd, RAM, RAM_SIZE);
	rb->write(fd, SRAM, SRAM_SIZE);
	rb->write(fd, PPURAM, PPURAM_SIZE);
	rb->write(fd, SPRRAM, SPRRAM_SIZE);
	rb->write(fd, *PPUBANK, 16 * sizeof(*PPUBANK));
	rb->write(fd, ChrBuf, 256 * 2 * 8 * 8 );
	rb->write(fd, &ROMBANK0, 4); rb->write(fd, &ROMBANK1, 4); rb->write(fd, &ROMBANK2, 4); rb->write(fd, &ROMBANK3, 4);
	rb->write(fd, &IRQ_Wiring, 1);
	rb->write(fd, &NMI_Wiring, 1);
	rb->write(fd, &g_wPassedClocks, 2);
	rb->write(fd, g_byTestTable, 256);
	rb->write(fd, g_ASLTable, 256 * 2);
	rb->write(fd, g_LSRTable, 256 * 2);
	rb->write(fd, g_ROLTable, 256 * 2 * 2);
	rb->write(fd, g_RORTable, 256 * 2 * 2);

	rb->write(fd, &PPU_R1, 1);
	rb->write(fd, &PPU_R2, 1);
	rb->write(fd, &PPU_R3, 1);
	rb->write(fd, &PPU_R7, 1);
	
	rb->write(fd, &PPU_Addr, 2);
	rb->write(fd, &PPU_Temp, 2);
	rb->write(fd, &PPU_Increment, 2);

	rb->write(fd, PalTable, 32 * 2);

	rb->write(fd, &PPU_Scr_V, 1);
	rb->write(fd, &PPU_Scr_V_Next, 1);
	rb->write(fd, &PPU_Scr_V_Byte, 1);
	rb->write(fd, &PPU_Scr_V_Byte_Next, 1);
	rb->write(fd, &PPU_Scr_V_Bit, 1);
	rb->write(fd, &PPU_Scr_V_Bit_Next, 1);
	rb->write(fd, &PPU_Scr_H, 1);
	rb->write(fd, &PPU_Scr_H_Next, 1);
	rb->write(fd, &PPU_Scr_H_Byte, 1);
	rb->write(fd, &PPU_Scr_H_Byte_Next, 1);
	rb->write(fd, &PPU_Scr_H_Bit, 1);
	rb->write(fd, &PPU_Scr_H_Bit_Next, 1);

	rb->write(fd, PPU_ScanTable, 263);
	rb->write(fd, &PPU_NameTableBank, 1);
	rb->write(fd, &PPU_Latch_Flag, 1);

	rb->write(fd, &IRQ_State, 1);
	rb->write(fd, &IRQ_Wiring, 1);
	rb->write(fd, &NMI_State, 1);
	rb->write(fd, &NMI_Wiring, 1);

	rb->write(fd, APU_Reg, 0x18);
	rb->write(fd, &ChrBufUpdate, 1);

	rb->write(fd, &PPU_UpDown_Clip, 1);
	rb->write(fd, &FrameIRQ_Enable, 1);
	rb->write(fd, &FrameStep, 2);

	rb->write(fd, &byVramWriteEnable, 1);
	rb->write(fd, &SpriteJustHit, 4);

	rb->write(fd, &FrameCnt, 2);

	if(MapperNo == 1)
	{
		rb->write(fd, &Map1_bank1, 4);
		rb->write(fd, &Map1_bank2, 4);
		rb->write(fd, &Map1_bank3, 4);
		rb->write(fd, &Map1_bank4, 4);
		rb->write(fd, Map1_Regs, 4);

		rb->write(fd, &Map1_Cnt, 4);
		rb->write(fd, &Map1_Latch, 1);
		rb->write(fd, &Map1_Last_Write_Addr, 2);
		rb->write(fd, &Map1_HI1, 4);
		rb->write(fd, &Map1_HI2, 4);
		rb->write(fd, &Map1_256K_base, 4);
		rb->write(fd, &Map1_swap, 4);
	}
	if(MapperNo == 4)
	{
		rb->write(fd, Map4_Regs, 8);
		rb->write(fd, &Map4_Rom_Bank, 4);
		rb->write(fd, &Map4_Prg0, 4);
		rb->write(fd, &Map4_Prg1, 4);
		rb->write(fd, &Map4_Chr01, 4);
		rb->write(fd, &Map4_Chr23, 4);
		rb->write(fd, &Map4_Chr4, 4);
		rb->write(fd, &Map4_Chr5, 4);
		rb->write(fd, &Map4_Chr6, 4);
		rb->write(fd, &Map4_Chr7, 4);

		rb->write(fd, &Map4_IRQ_Enable, 1);
		rb->write(fd, &Map4_IRQ_Cnt, 1);
		rb->write(fd, &Map4_IRQ_Latch, 1);
		rb->write(fd, &Map4_IRQ_Request, 1);
		rb->write(fd, &Map4_IRQ_Present, 1);
		rb->write(fd, &Map4_IRQ_Present_Vbl, 1);
	}
	

	rb->close(fd);
	return;
}

void load(void)
{
	int fd;
	char fname[256];
	rb->strcpy(fname, szRomName);
	rb->strcpy( rb->strrchr( fname, '.' ) + 1, "sav" );
	
	fd = rb->open(fname, O_RDONLY);
	if(!fd)
		return;

	rb->read(fd, &PC, 2);
	rb->read(fd, &SP, 1);
	rb->read(fd, &F, 1);
	rb->read(fd, &A, 1);
	rb->read(fd, &X, 1);
	rb->read(fd, &Y, 1);
	rb->read(fd, RAM, RAM_SIZE);
	rb->read(fd, SRAM, SRAM_SIZE);
	rb->read(fd, PPURAM, PPURAM_SIZE);
	rb->read(fd, SPRRAM, SPRRAM_SIZE);
	rb->read(fd, *PPUBANK, 16 * sizeof(*PPUBANK));
	rb->read(fd, ChrBuf, 256 * 2 * 8 * 8 );
	rb->read(fd, &ROMBANK0, 4); rb->read(fd, &ROMBANK1, 4); rb->read(fd, &ROMBANK2, 4); rb->read(fd, &ROMBANK3, 4);
	rb->read(fd, &IRQ_Wiring, 1);
	rb->read(fd, &NMI_Wiring, 1);
	rb->read(fd, &g_wPassedClocks, 2);
	rb->read(fd, g_byTestTable, 256);
	rb->read(fd, g_ASLTable, 256 * 2);
	rb->read(fd, g_LSRTable, 256 * 2);
	rb->read(fd, g_ROLTable, 256 * 2 * 2);
	rb->read(fd, g_RORTable, 256 * 2 * 2);

	rb->read(fd, &PPU_R1, 1);
	rb->read(fd, &PPU_R2, 1);
	rb->read(fd, &PPU_R3, 1);
	rb->read(fd, &PPU_R7, 1);

	rb->read(fd, &PPU_Addr, 2);
	rb->read(fd, &PPU_Temp, 2);
	rb->read(fd, &PPU_Increment, 2);

	rb->read(fd, PalTable, 32 * 2);

	rb->read(fd, &PPU_Scr_V, 1);
	rb->read(fd, &PPU_Scr_V_Next, 1);
	rb->read(fd, &PPU_Scr_V_Byte, 1);
	rb->read(fd, &PPU_Scr_V_Byte_Next, 1);
	rb->read(fd, &PPU_Scr_V_Bit, 1);
	rb->read(fd, &PPU_Scr_V_Bit_Next, 1);
	rb->read(fd, &PPU_Scr_H, 1);
	rb->read(fd, &PPU_Scr_H_Next, 1);
	rb->read(fd, &PPU_Scr_H_Byte, 1);
	rb->read(fd, &PPU_Scr_H_Byte_Next, 1);
	rb->read(fd, &PPU_Scr_H_Bit, 1);
	rb->read(fd, &PPU_Scr_H_Bit_Next, 1);

	rb->read(fd, PPU_ScanTable, 263);
	rb->read(fd, &PPU_NameTableBank, 1);
	rb->read(fd, &PPU_Latch_Flag, 1);

	rb->read(fd, &IRQ_State, 1);
	rb->read(fd, &IRQ_Wiring, 1);
	rb->read(fd, &NMI_State, 1);
	rb->read(fd, &NMI_Wiring, 1);

	rb->read(fd, APU_Reg, 0x18);
	rb->read(fd, &ChrBufUpdate, 1);

	rb->read(fd, &PPU_UpDown_Clip, 1);
	rb->read(fd, &FrameIRQ_Enable, 1);
	rb->read(fd, &FrameStep, 2);

	rb->read(fd, &byVramWriteEnable, 1);
	rb->read(fd, &SpriteJustHit, 4);

	rb->read(fd, &FrameCnt, 2);


	if(MapperNo == 1)
	{
		rb->read(fd, &Map1_bank1, 4);
		rb->read(fd, &Map1_bank2, 4);
		rb->read(fd, &Map1_bank3, 4);
		rb->read(fd, &Map1_bank4, 4);
		rb->read(fd, Map1_Regs, 4);

		rb->read(fd, &Map1_Cnt, 4);
		rb->read(fd, &Map1_Latch, 1);
		rb->read(fd, &Map1_Last_Write_Addr, 2);
		rb->read(fd, &Map1_HI1, 4);
		rb->read(fd, &Map1_HI2, 4);
		rb->read(fd, &Map1_256K_base, 4);
		rb->read(fd, &Map1_swap, 4);
	} 

	if(MapperNo == 4)
	{
		rb->read(fd, Map4_Regs, 8);
		rb->read(fd, &Map4_Rom_Bank, 4);
		rb->read(fd, &Map4_Prg0, 4);
		rb->read(fd, &Map4_Prg1, 4);
		rb->read(fd, &Map4_Chr01, 4);
		rb->read(fd, &Map4_Chr23, 4);
		rb->read(fd, &Map4_Chr4, 4);
		rb->read(fd, &Map4_Chr5, 4);
		rb->read(fd, &Map4_Chr6, 4);
		rb->read(fd, &Map4_Chr7, 4);

		rb->read(fd, &Map4_IRQ_Enable, 1);
		rb->read(fd, &Map4_IRQ_Cnt, 1);
		rb->read(fd, &Map4_IRQ_Latch, 1);
		rb->read(fd, &Map4_IRQ_Request, 1);
		rb->read(fd, &Map4_IRQ_Present, 1);
		rb->read(fd, &Map4_IRQ_Present_Vbl, 1);
	}


	rb->close(fd);
	return;
}
