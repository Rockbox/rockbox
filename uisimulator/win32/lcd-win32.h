#ifndef __LCDWIN32_H__
#define __LCDWIN32_H__

#include "uisw32.h"
#include "lcd.h"

// BITMAPINFO2
typedef struct
{
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD bmiColors[2];
} BITMAPINFO2;

#ifdef HAVE_LCD_BITMAP

extern unsigned char        display[DISP_X][DISP_Y/8]; // the display
#else
#define DISP_X              112
#define DISP_Y              64
#endif


extern char                 bitmap[DISP_Y][DISP_X]; // the ui display
extern BITMAPINFO2          bmi; // bitmap information


#endif // #ifndef __LCDWIN32_H__