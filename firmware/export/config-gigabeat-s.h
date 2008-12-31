/*
 * This config file is for toshiba Gigabeat S
 */

#define TARGET_TREE /* this target is using the target tree system */

#define TOSHIBA_GIGABEAT_S 1

#define MODEL_NAME "Toshiba Gigabeat S"

/* For Rolo and boot loader */
#define MODEL_NUMBER 21

/* define this if you use an ATA controller */
#define CONFIG_STORAGE STORAGE_ATA

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

#define CONFIG_KEYPAD GIGABEAT_S_PAD

/* Define keyboard features */
#define KBD_CURSOR_KEYS /* allow non-line edit cursor movement */
#define KBD_MODES       /* enable line edit */
#define KBD_MORSE_INPUT /* enable morse code input */

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* define this if you have a real-time clock */
#define CONFIG_RTC RTC_MC13783

/* Define if the device can wake from an RTC alarm */
#define HAVE_RTC_ALARM

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT

/* define this if you have a disk storage, i.e. something
   that needs spinups and can cause skips when shaked */
#define HAVE_DISK_STORAGE

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

/* Define this if you have a SI4700 fm radio tuner */
#define CONFIG_TUNER SI4700

/* Define this if you have the WM8978 audio codec */
#define HAVE_WM8978

/* Define bitmask of input sources - recordable bitmask can be defined
   explicitly if different */
#define INPUT_SRC_CAPS SRC_CAP_FMRADIO

/* define the bitmask of hardware sample rates */
#define HW_SAMPR_CAPS (SAMPR_CAP_48 | SAMPR_CAP_44 | SAMPR_CAP_32 | \
                       SAMPR_CAP_24 | SAMPR_CAP_22 | SAMPR_CAP_16 | \
                       SAMPR_CAP_12 | SAMPR_CAP_11 | SAMPR_CAP_8)

/* define the bitmask of recording sample rates */
#define REC_SAMPR_CAPS HW_SAMPR_CAPS /* Same as playback */

/* define default recording levels */
#define DEFAULT_REC_LEFT_GAIN 0
#define DEFAULT_REC_RIGHT_GAIN 0

/* Define this if you have recording capability */
#define HAVE_RECORDING

/* Define this if your LCD can be put to sleep. */
#define HAVE_LCD_SLEEP
/* We don't use a setting but a fixed delay after the backlight has
 * turned off */
#define LCD_SLEEP_TIMEOUT (2*HZ)

#ifndef BOOTLOADER
#if 0
/* Define this if your LCD can be enabled/disabled */
#define HAVE_LCD_ENABLE
#endif

#define HAVE_BACKLIGHT_BRIGHTNESS

/* Main LCD backlight brightness range and defaults */
#define MIN_BRIGHTNESS_SETTING          1
#define MAX_BRIGHTNESS_SETTING          24
#define DEFAULT_BRIGHTNESS_SETTING      12


#define HAVE_HEADPHONE_DETECTION
#endif /* BOOTLOADER */

#ifndef SIMULATOR

/* Implementation-defined fading type with bool settings */
#define USE_BACKLIGHT_CUSTOM_FADING_BOOL

/* The LCD on a Gigabeat is 240x320 - it is portrait */
#define HAVE_PORTRAIT_LCD

#define CONFIG_CPU IMX31L

/* Define this if you want to use imx31l's i2c interface */
#define CONFIG_I2C I2C_IMX31L

/* Define the bitmask of modules used */
#define SPI_MODULE_MASK (USE_CSPI2_MODULE)
#define I2C_MODULE_MASK (USE_I2C1_MODULE | USE_I2C2_MODULE)
#define GPIO_EVENT_MASK (USE_GPIO1_EVENTS)

/* Define this if target has an additional number of threads specific to it */
#define TARGET_EXTRA_THREADS 1

/* Type of mobile power - check this out */
#define BATTERY_CAPACITY_DEFAULT 700 /* default battery capacity */
#define BATTERY_CAPACITY_MIN     700 /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX    1200 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC      25 /* capacity increment */
#define BATTERY_TYPES_COUNT        1 /* only one type */

/* TODO: have a proper status displayed in the bootloader and have it
 * work! */
/* Charing implemented in a target-specific algorithm */
#define CONFIG_CHARGING CHARGING_TARGET

/* define this if the hardware can be powered off while charging */
#define HAVE_POWEROFF_WHILE_CHARGING

/* The size of the flash ROM */
#define FLASH_SIZE 0x400000

/* Define this to the CPU frequency */
/* TODO */
#define CPU_FREQ 264000000 /* Set by retailOS loader */

/* define this if the unit can be powered or charged via USB */
#define HAVE_USB_POWER
#define USBPOWER_BUTTON     BUTTON_MENU
#define USBPOWER_BTN_IGNORE BUTTON_POWER

/* define this if the unit has a battery switch or battery can be removed
 * when running */
#define HAVE_BATTERY_SWITCH

/* USB On-the-go */
#define CONFIG_USBOTG USBOTG_ARC

/* enable these for the experimental usb stack */
#define USE_ROCKBOX_USB
#define HAVE_USBSTACK
#define USB_STORAGE
#define USB_VENDOR_ID 0x0930
#define USB_PRODUCT_ID 0x0010

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
#define HAVE_VOLUME_IN_LIST

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
