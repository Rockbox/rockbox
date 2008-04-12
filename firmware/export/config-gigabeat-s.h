/*
 * This config file is for toshiba Gigabeat S
 */

#define NO_LOW_BATTERY_SHUTDOWN
#define TARGET_TREE /* this target is using the target tree system */

#define TOSHIBA_GIGABEAT_S 1

/* For Rolo and boot loader */
#define MODEL_NUMBER 21

/* define this if you use an ATA controller */
#define HAVE_ATA

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR

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

#define CONFIG_KEYPAD GIGABEAT_S_PAD

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* define this if you have a real-time clock */
//#define CONFIG_RTC RTC_IMX31L

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT

#define HAVE_LCD_ENABLE

//#define HAVE_BACKLIGHT_BRIGHTNESS

/* Main LCD backlight brightness range and defaults */
#define MIN_BRIGHTNESS_SETTING          0   /* 0.5 mA */
#define MAX_BRIGHTNESS_SETTING          63  /* 32 mA */
#define DEFAULT_BRIGHTNESS_SETTING      39  /* 20 mA */

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x80000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

/* Define this if you have the WM8975 audio codec */
#define HAVE_WM8978


#define HW_SAMPR_CAPS (SAMPR_CAP_88 | SAMPR_CAP_44 | SAMPR_CAP_22 | \
                       SAMPR_CAP_11)

#define HAVE_HEADPHONE_DETECTION

#ifndef SIMULATOR

/* The LCD on a Gigabeat is 240x320 - it is portrait */
#define HAVE_PORTRAIT_LCD

#define CONFIG_CPU IMX31L

/* Define this if you want to use coldfire's i2c interface */
#define CONFIG_I2C I2C_IMX31L

/* Define the bitmask of serial interface modules (CSPI) used */
#define SPI_MODULE_MASK (USE_CSPI2_MODULE)

/* Define this if target has an additional number of threads specific to it */
#define TARGET_EXTRA_THREADS 1

/* Type of mobile power - check this out */
#define BATTERY_CAPACITY_DEFAULT 2000 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 1500        /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 2500        /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 25         /* capacity increment */
#define BATTERY_TYPES_COUNT  1          /* only one type */

/* Hardware controlled charging with monitoring */
#define CONFIG_CHARGING CHARGING_MONITOR

/* define this if the hardware can be powered off while charging */
#define HAVE_POWEROFF_WHILE_CHARGING

/* The size of the flash ROM */
#define FLASH_SIZE 0x400000

/* Define this to the CPU frequency */
/* TODO */
#define CPU_FREQ 16934400

/* define this if the unit can be powered or charged via USB */
#define HAVE_USB_POWER

/* Define this if you have ATA power-off control */
#define HAVE_ATA_POWER_OFF

/* Virtual LED (icon) */
#define CONFIG_LED LED_VIRTUAL

#define CONFIG_LCD LCD_GIGABEAT

/* define this if the backlight can be set to a brightness */
//#define HAVE_BACKLIGHT_SET_FADING
#define __BACKLIGHT_INIT

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

#define HAVE_SERIAL

/*Remove Comments from UART_INT to enable the UART interrupts,*/
/*otherwise iterrupts will be disabled. For now we will test */
/*UART state by polling the registers, and if necessary update this */
/*method by using the interrupts instead*/
//#define UART_INT

/* Define this if you have adjustable CPU frequency */
/* #define HAVE_ADJUSTABLE_CPU_FREQ */

#define BOOTFILE_EXT "gigabeat"
#define BOOTFILE "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"

#endif
