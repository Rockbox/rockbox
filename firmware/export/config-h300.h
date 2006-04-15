/*
 * This config file is for iriver H320, H340
 */
#define IRIVER_H300_SERIES 1

/* For Rolo and boot loader */
#define MODEL_NUMBER 2

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP 1

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR 1

/* define this if you can flip your LCD */
#define HAVE_LCD_FLIP

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* LCD dimensions */
#define LCD_WIDTH  220
#define LCD_HEIGHT 176
#define LCD_DEPTH  16   /* 65k colours */
#define LCD_PIXELFORMAT RGB565 /* rgb565 */

/* remote LCD */
#define LCD_REMOTE_WIDTH  128
#define LCD_REMOTE_HEIGHT 64
#define LCD_REMOTE_DEPTH  1

#define CONFIG_KEYPAD IRIVER_H300_PAD

#define CONFIG_REMOTE_KEYPAD H300_REMOTE

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* define this if you have a real-time clock */
#define CONFIG_RTC RTC_PCF50606

/* Define this if you have an remote lcd */
#define HAVE_REMOTE_LCD

/* Define this for LCD backlight available */
#define CONFIG_BACKLIGHT BL_IRIVER_H300 /* port controlled PWM */

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x80000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

#define AB_REPEAT_ENABLE 1

#define CONFIG_TUNER TEA5767
#define CONFIG_TUNER_XTAL 32768

#define HAVE_UDA1380

/* define this if you have recording possibility */
#define HAVE_RECORDING 1

#define BATTERY_CAPACITY_DEFAULT 1300 /* default battery capacity */

#ifndef SIMULATOR

/* Define this if you have a Motorola SCF5249 */
#define CONFIG_CPU MCF5249

/* Define this if you want to use coldfire's i2c interface */
#define CONFIG_I2C I2C_COLDFIRE

/* Type of mobile power */
#define CONFIG_BATTERY BATT_LIPOL1300
#define BATTERY_CAPACITY_MIN 1300 /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 3200 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 50   /* capacity increment */
#define BATTERY_TYPES_COUNT  1    /* only one type */
#define BATTERY_SCALE_FACTOR 23437 /* FIX: this value is picked at random */

/* Define if we have a hardware defect that causes ticking on the audio line */
#define HAVE_REMOTE_LCD_TICKING

/* Define this if the platform can charge batteries */
#define HAVE_CHARGING 1

/* For units with a hardware charger that reports charge state */
#define HAVE_CHARGE_STATE 1

/* define this if the hardware can be powered off while charging */
#define HAVE_POWEROFF_WHILE_CHARGING

/* The size of the flash ROM */
#define FLASH_SIZE 0x400000

/* Define this to the CPU frequency */
#define CPU_FREQ      11289600

/* Define this if you have ATA power-off control */
#define HAVE_ATA_POWER_OFF

/* Virtual LED (icon) */
#define CONFIG_LED LED_VIRTUAL

#define CONFIG_LCD LCD_H300

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

#define USB_IRIVERSTYLE

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

#define BOOTFILE_EXT "iriver"
#define BOOTFILE "rockbox." BOOTFILE_EXT

#define HAVE_BACKLIGHT_BRIGHTNESS

/* define this if the unit can be powered or charged via USB */
#define HAVE_USB_POWER

#endif
