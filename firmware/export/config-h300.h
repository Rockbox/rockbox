/* define this if you have recording possibility */
/*#define HAVE_RECORDING 1*/

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP 1

/* LCD dimensions */
#define LCD_WIDTH  220
#define LCD_HEIGHT 176

/* remote LCD */
#define LCD_REMOTE_WIDTH  128
#define LCD_REMOTE_HEIGHT 64

#define CONFIG_KEYPAD IRIVER_H300_PAD

#define CONFIG_REMOTE_KEYPAD H300_REMOTE

/* Define this if you do software codec */
#define CONFIG_HWCODEC MASNONE

/* Define this if you have an remote lcd */
#define HAVE_REMOTE_LCD

#ifndef SIMULATOR

/* Define this if you have a Motorola SCF5249 */
#define CONFIG_CPU MCF5249

#define CONFIG_I2C I2C_H100

#define HAVE_UDA1380

/* Type of mobile power */
#define CONFIG_BATTERY BATT_LIPOL1300

#define BATTERY_SCALE_FACTOR 16665 /* FIX: this value is picked at random */

/* Define this if the platform can charge batteries */
#define HAVE_CHARGING 1

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

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

#define BOOTFILE_EXT ".iriver"
#define BOOTFILE "rockbox" BOOTFILE_EXT

/* Define this if you can control the S/PDIF power */
#define HAVE_SPDIF_POWER

#endif
