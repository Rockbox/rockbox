/* define this if you have recording possibility */
#define HAVE_RECORDING 1

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP 1

/* LCD dimensions */
#define LCD_WIDTH  160
#define LCD_HEIGHT 128

/* define this if you have the Recorder's 10-key keyboard */
#define HAVE_RECORDER_KEYPAD 1

#ifndef SIMULATOR

/* Define this if the platform has batteries */
#define HAVE_BATTERIES 1

/* The start address index for ROM builds */
#define ROM_START 0x11010

/* Define this for programmable LED available */
#define HAVE_LED

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT

#endif
