/* Note: this is just a basic early version that needs attention and
   corrections! */

/* define this if you have recording possibility */
#define HAVE_RECORDING 1

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP 1

/* LCD dimensions */
#define LCD_WIDTH  128
#define LCD_HEIGHT 64

/* When button.h is updated with gmini details, then fix this: */
#define CONFIG_KEYPAD /*GMINI_PAD */ IRIVER_H100_PAD

#ifndef SIMULATOR

/* Define this if you have a CalmRISC16 */
#define CONFIG_CPU CR16

/* Type of mobile power, FIXME: probably different, make new type */
#define CONFIG_BATTERY

/* Define this if the platform can charge batteries */
#define HAVE_CHARGING 1

#endif
