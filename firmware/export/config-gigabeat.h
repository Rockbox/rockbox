/*
 * This config file is for toshiba Gigabeat F
 */
#define TARGET_TREE /* this target is using the target tree system */

#define TOSHIBA_GIGABEAT_F 1

/* For Rolo and boot loader */
#define MODEL_NUMBER 18

/* define this if you use an ATA controller */
#define HAVE_ATA

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR

/* define this if you want album art for this target */
#define HAVE_ALBUMART

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* define this if you have access to the pitchscreen */
#define HAVE_PITCHSCREEN

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* define this if the target has volume keys which can be used in the lists */
#define HAVE_VOLUME_IN_LIST

/* LCD dimensions */
#define LCD_WIDTH  240
#define LCD_HEIGHT 320
#define LCD_DEPTH  16   /* 65k colours */
#define LCD_PIXELFORMAT RGB565 /* rgb565 */

/* Define this if your LCD can be enabled/disabled */
#define HAVE_LCD_ENABLE

/* Define this if your LCD can be put to sleep. HAVE_LCD_ENABLE
   should be defined as well. */
#define HAVE_LCD_SLEEP
/* We don't use a setting but a fixed delay after the backlight has
 * turned off */
#define LCD_SLEEP_TIMEOUT (5*HZ)

#define CONFIG_KEYPAD GIGABEAT_PAD

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* define this if you have a real-time clock */
#define CONFIG_RTC RTC_S3C2440

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT

#define HAVE_BUTTON_LIGHT

#define HAVE_BACKLIGHT_BRIGHTNESS

#define HAVE_BUTTONLIGHT_BRIGHTNESS

/* Main LCD backlight brightness range and defaults */
#define MIN_BRIGHTNESS_SETTING          1   /* 0.5 mA */
#define MAX_BRIGHTNESS_SETTING          12  /* 32 mA */
#define DEFAULT_BRIGHTNESS_SETTING      10  /* 16 mA */

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x80000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

#define AB_REPEAT_ENABLE 1

/* Define this if you have the WM8975 audio codec */
#define HAVE_WM8751

/* Define this if you want to use the adaptive bass capibility of the 8751 */
/* #define USE_ADAPTIVE_BASS */

#define HW_SAMPR_CAPS (SAMPR_CAP_88 | SAMPR_CAP_44 | SAMPR_CAP_22 | \
                       SAMPR_CAP_11)

#define HAVE_HEADPHONE_DETECTION

#define BATTERY_CAPACITY_DEFAULT 2000 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 1500        /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 2500        /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 25         /* capacity increment */
#define BATTERY_TYPES_COUNT  1          /* only one type */

/* Hardware controlled charging with monitoring */
#define CONFIG_CHARGING CHARGING_MONITOR

/* define this if the unit can be powered or charged via USB */
#define HAVE_USB_POWER

#ifndef SIMULATOR

/* The LCD on a Gigabeat is 240x320 - it is portrait */
#define HAVE_PORTRAIT_LCD

/* Define this if you have a Motorola SCF5249 */
#define CONFIG_CPU S3C2440

/* Define this if you want to use coldfire's i2c interface */
#define CONFIG_I2C I2C_S3C2440

/* define this if the hardware can be powered off while charging */
#define HAVE_POWEROFF_WHILE_CHARGING

/* The size of the flash ROM */
#define FLASH_SIZE 0x400000

/* Define this to the CPU frequency */
#define CPU_FREQ 16934400

/* Define this if you have ATA power-off control */
#define HAVE_ATA_POWER_OFF

/* Virtual LED (icon) */
#define CONFIG_LED LED_VIRTUAL

#define CONFIG_LCD LCD_GIGABEAT

/* define this if the backlight can be set to a brightness */
#define HAVE_BACKLIGHT_SET_FADING
#define __BACKLIGHT_INIT

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

/* Define this if you have adjustable CPU frequency */
/* #define HAVE_ADJUSTABLE_CPU_FREQ */

#define BOOTFILE_EXT "gigabeat"
#define BOOTFILE "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"

#endif
