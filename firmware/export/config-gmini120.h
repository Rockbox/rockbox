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
#define CONFIG_KEYPAD GMINI100_PAD

#ifndef SIMULATOR

/* Define this if you have a TCC730 (CalmRISC16) */
#define CONFIG_CPU TCC730

/* Define this if you have a gmini 100 style LCD */
#define CONFIG_LCD LCD_GMINI100

#define CONFIG_I2C I2C_GMINI

/* Define this if you do software codec */
#define CONFIG_HWCODEC MASNONE

/* Type of mobile power, FIXME: probably different, make new type */
#define CONFIG_BATTERY BATT_LIION2200
#define BATTERY_SCALE_FACTOR 6465
/* chosen values at random -- jyp */

/* Define this if the platform can charge batteries */
#define HAVE_CHARGING 1

#define CPU_FREQ 30000000
/* approximate value (and false in general since freq is variable) */

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* Always enable debug till we stabilize */
#define EMULATOR

#define USB_GMINISTYLE

#define CONFIG_I2C I2C_GMINI

#define CONFIG_BACKLIGHT BL_GMINI

#define GMINI_ARCH

#endif
