/*
 * This config file is for the Apple iPod Mini 2nd Gen
 */
#define IPOD_ARCH 1

/* For Rolo and boot loader */
#define MODEL_NUMBER 11

/* define this if you have recording possibility */
/*#define HAVE_RECORDING 1*/

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP 1

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* LCD dimensions */
#define LCD_WIDTH  138
#define LCD_HEIGHT 110
#define LCD_DEPTH  2   /* 4 colours - 2bpp */

#define LCD_PIXELFORMAT HORIZONTAL_PACKING

/* define this if you can flip your LCD */
#define HAVE_LCD_FLIP

#define CONFIG_KEYPAD IPOD_4G_PAD

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* define this if you have a real-time clock */
#ifndef BOOTLOADER
#define CONFIG_RTC RTC_PCF50605
#endif

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x80000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

/* Define this if you have the WM8721 audio codec */
#define HAVE_WM8721 /* actually WM8731 but no recording */

#define AB_REPEAT_ENABLE 1

/* Define this for LCD backlight available */
#define CONFIG_BACKLIGHT BL_IPODMINI /* port controlled */

/* We can fade the backlight by using PWM */
#define HAVE_BACKLIGHT_PWM_FADING

#ifndef SIMULATOR

/* Define this if you have a PortalPlayer PP5020 */
#define CONFIG_CPU PP5020

/* Define this if you want to use the PP5020 i2c interface */
#define CONFIG_I2C I2C_PP5020

/* Type of mobile power */
#define CONFIG_BATTERY BATT_LIPOL1300

#define BATTERY_SCALE_FACTOR 5865

/* Define this if the platform can charge batteries */
//#define HAVE_CHARGING 1

/* define this if the hardware can be powered off while charging */
//#define HAVE_POWEROFF_WHILE_CHARGING

/* The start address index for ROM builds */
#define ROM_START 0x00000000

/* Define this to the CPU frequency */
#define CPU_FREQ      11289600

#define CONFIG_LCD LCD_IPOD2BPP

/* Offset ( in the firmware file's header ) to the file length */
#define FIRMWARE_OFFSET_FILE_LENGTH 0

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

#define USB_IPODSTYLE

/* define this if the unit can be powered or charged via USB */
#define HAVE_USB_POWER

/* Virtual LED (icon) */
#define CONFIG_LED LED_VIRTUAL

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

#define BOOTFILE_EXT "ipod"
#define BOOTFILE "rockbox." BOOTFILE_EXT

#endif
