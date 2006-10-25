/*
 * This config file is for toshiba Gigabeat F
 */
#define TARGET_TREE /* this target is using the target tree system */

#define TOSHIBA_GIGABEAT_F 1

/* For Rolo and boot loader */
#define MODEL_NUMBER 1

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP 1

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR 1

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* LCD dimensions */
#define LCD_WIDTH  240
#define LCD_HEIGHT 320
#define LCD_DEPTH  16   /* 65k colours */
#define LCD_PIXELFORMAT RGB565 /* rgb565 */


#define CONFIG_KEYPAD GIGABEAT_PAD

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* define this if you have a real-time clock */

#if 0 /* TODO */
#define CONFIG_RTC RTC_S3C2440
#endif

/* Define this for LCD backlight available */
#define CONFIG_BACKLIGHT BL_GIGABEAT /* port controlled PWM */

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x80000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

/* Define this if you have the WM8975 audio codec */
#define HAVE_WM8751

#define BATTERY_CAPACITY_DEFAULT 830 /* default battery capacity */

#ifndef SIMULATOR

/* Define this if you have a Motorola SCF5249 */
#define CONFIG_CPU S3C2440

/* Define this if you want to use coldfire's i2c interface */
#define CONFIG_I2C I2C_S3C2440

/* Type of mobile power */
#define CONFIG_BATTERY BATT_LIPOL1300
#define BATTERY_CAPACITY_MIN 1300 /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 3200 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 50   /* capacity increment */
#define BATTERY_TYPES_COUNT  1    /* only one type */

#define BATTERY_SCALE_FACTOR 6852 /* FIX: this value is picked at random */

/* Hardware controlled charging? FIXME */
#define CONFIG_CHARGING CHARGING_SIMPLE

/* define this if the hardware can be powered off while charging */
#define HAVE_POWEROFF_WHILE_CHARGING

/* The size of the flash ROM */
#define FLASH_SIZE 0x400000

/* Define this to the CPU frequency */
#define CPU_FREQ 16934400

/* Define this if you have ATA power-off control */
#if 0 /* TODO */
#define HAVE_ATA_POWER_OFF
#endif

/* Virtual LED (icon) */
#define CONFIG_LED LED_VIRTUAL

#define CONFIG_LCD LCD_GIGABEAT

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

#define USB_GIGABEAT_STYLE

/* Define this if you have adjustable CPU frequency */
#if 0 /* TODO */
#define HAVE_ADJUSTABLE_CPU_FREQ
#endif

#define BOOTFILE_EXT "gigabeat"
#define BOOTFILE "rockbox." BOOTFILE_EXT

#if 0 /* TODO */
#define HAVE_BACKLIGHT_BRIGHTNESS
#endif

#endif
