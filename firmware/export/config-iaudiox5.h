/*
 * This config file is for iAudio X5
 */
#define TARGET_TREE /* this target is using the target tree system */

/* For Rolo and boot loader */
#define MODEL_NUMBER 10

/* define this if you have recording possibility */
#define HAVE_RECORDING 1

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP 1

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR 1

/* define this if you can invert the colours on your LCD */
#define HAVE_LCD_INVERT

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* define this if you have access to the pitchscreen */
#define HAVE_PITCHSCREEN

/* LCD dimensions */
#define LCD_WIDTH  160
#define LCD_HEIGHT 128
#define LCD_DEPTH  16   /* pseudo 262.144 colors */
#define LCD_PIXELFORMAT RGB565 /* rgb565 */

/* remote LCD */
#define LCD_REMOTE_WIDTH  128
#define LCD_REMOTE_HEIGHT 96
#define LCD_REMOTE_DEPTH  2

#define LCD_REMOTE_PIXELFORMAT VERTICAL_INTERLEAVED

#define CONFIG_KEYPAD IAUDIO_X5_PAD

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* Define this if you have an remote lcd */
#define HAVE_REMOTE_LCD

#define CONFIG_LCD LCD_X5

/* Main LCD contrast range and defaults */
#define MIN_CONTRAST_SETTING        1
#define MAX_CONTRAST_SETTING        30
#define DEFAULT_CONTRAST_SETTING    19 /* Match boot contrast */

/* Remote LCD contrast range and defaults */
#define MIN_REMOTE_CONTRAST_SETTING     10
#define MAX_REMOTE_CONTRAST_SETTING     35
#define DEFAULT_REMOTE_CONTRAST_SETTING 24 /* Match boot contrast */

/* Define this for LCD backlight available */
#define CONFIG_BACKLIGHT BL_X5 /* PCF50606 I2C */
#define HAVE_BACKLIGHT_BRIGHTNESS

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x80000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

/* FM Tuner */
#define CONFIG_TUNER       TEA5767
#define CONFIG_TUNER_XTAL  32768


#define BATTERY_CAPACITY_DEFAULT 950 /* default battery capacity */

#define HAVE_TLV320

#ifndef SIMULATOR

/* Define this if your LCD can set contrast */
#define HAVE_LCD_CONTRAST

/* Define this if your LCD can be enabled/disabled */
#define HAVE_LCD_ENABLE

/* Define this if your LCD can be put to sleep. HAVE_LCD_ENABLE
   should be defined as well. */
#define HAVE_LCD_SLEEP

/* Define this if you have a Motorola SCF5250 */
#define CONFIG_CPU MCF5250

/* Define this if you want to use coldfire's i2c interface */
#define CONFIG_I2C I2C_COLDFIRE

/* Hardware controlled charging? FIXME */
#define CONFIG_CHARGING CHARGING_SIMPLE

/* define this if the hardware can be powered off while charging */
#define HAVE_POWEROFF_WHILE_CHARGING

/* The size of the flash ROM */
#define FLASH_SIZE 0x400000

/* Define this to the CPU frequency */
#define CPU_FREQ      11289600

/* Type of mobile power */
#define CONFIG_BATTERY BATT_LIPOL1300
#define BATTERY_CAPACITY_MIN 950   /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 2250  /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 50    /* capacity increment */
#define BATTERY_TYPES_COUNT  1     /* only one type */
#define BATTERY_SCALE_FACTOR 23437 /* FIX: this value is picked at random */

/* define this if you have a real-time clock */
#define CONFIG_RTC RTC_PCF50606

/* Define this if you have ATA power-off control */
#define HAVE_ATA_POWER_OFF

/* Virtual LED (icon) */
#define CONFIG_LED LED_VIRTUAL

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

#define USB_X5STYLE

/* USB On-the-go */
#define CONFIG_USBOTG USBOTG_M5636

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

#define BOOTFILE_EXT "iaudio"
#define BOOTFILE "rockbox." BOOTFILE_EXT

#define BOOTLOADER_ENTRYPOINT  0x001F0000
#define FLASH_ENTRYPOINT       0x00001000
#define FLASH_MAGIC            0xfbfbfbf1

#endif /* SIMULATOR */

/* Define this for FM radio input available */
#define HAVE_FMRADIO_IN
