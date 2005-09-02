/* For Rolo and boot loader */
#define MODEL_NUMBER 2

/* define this if you have recording possibility */
/*#define HAVE_RECORDING 1*/

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP 1

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR 1

/* LCD dimensions */
#define LCD_WIDTH  220
#define LCD_HEIGHT 176
#define LCD_DEPTH  18   /* 262.144 colours */

/* remote LCD */
#define LCD_REMOTE_WIDTH  128
#define LCD_REMOTE_HEIGHT 64
#define LCD_REMOTE_DEPTH  1

#define CONFIG_KEYPAD IRIVER_H300_PAD

#define CONFIG_REMOTE_KEYPAD H300_REMOTE

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* Define this if you have an remote lcd */
#define HAVE_REMOTE_LCD

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x40000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0xC0000

#ifndef SIMULATOR

/* Define this if you have a Motorola SCF5249 */
#define CONFIG_CPU MCF5249

/* Define this if you want to use coldfire's i2c interface */
#define CONFIG_I2C I2C_COLDFIRE

#define HAVE_UDA1380

/* Type of mobile power */
#define CONFIG_BATTERY BATT_LIPOL1300

#define BATTERY_SCALE_FACTOR 16665 /* FIX: this value is picked at random */

/* Define this if the platform can charge batteries */
#define HAVE_CHARGING 1

/* define this if the hardware can be powered off while charging */
#define HAVE_POWEROFF_WHILE_CHARGING

/* The start address index for ROM builds */
#define ROM_START 0x11010

/* Define this for LCD backlight available */
#define CONFIG_BACKLIGHT BL_IRIVER /* port controlled */

/* Define this to the CPU frequency */
#define CPU_FREQ      11289600

/* Define this if you have ATA power-off control */
#define HAVE_ATA_POWER_OFF

#define CONFIG_LCD LCD_S1D15E06

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

#define USB_IRIVERSTYLE

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

#define BOOTFILE_EXT "iriver"
#define BOOTFILE "rockbox." BOOTFILE_EXT

/* Define this if you can control the S/PDIF power */
#define HAVE_SPDIF_POWER

#endif
