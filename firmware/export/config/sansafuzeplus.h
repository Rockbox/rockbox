/*
 * This config file is for the Sandisk Sansa Fuze+
 */
#define IMX233_SUBTARGET    3780
#define IMX233_PACKAGE      IMX233_BGA169
#define IMX233_PARTITIONS   IMX233_FREESCALE

/* For Rolo and boot loader */
#define MODEL_NUMBER 72
#define MODEL_NAME   "Sandisk Sansa Fuze+"
/* Define if boot data from bootloader has been enabled for the target */
#define HAVE_BOOTDATA
/* define boot redirect file name allows booting from external drives */
#define BOOT_REDIR "rockbox_main.fuze+"

// HW can do it but we don't have the IRAM for mix buffers
//#define HW_SAMPR_CAPS       SAMPR_CAP_ALL_192
#define HW_SAMPR_CAPS       SAMPR_CAP_ALL_48

/* define this if you have recording possibility */
#define HAVE_RECORDING

#define REC_SAMPR_CAPS      SAMPR_CAP_ALL_96

/* Default recording levels */
#define DEFAULT_REC_MIC_GAIN    23
#define DEFAULT_REC_LEFT_GAIN   23
#define DEFAULT_REC_RIGHT_GAIN  23

/* Define bitmask of input sources - recordable bitmask can be defined
   explicitly if different */
#define INPUT_SRC_CAPS (SRC_CAP_MIC | SRC_CAP_FMRADIO)



/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR
#define HAVE_LCD_FLIP
#define HAVE_LCD_INVERT

#ifndef BOOTLOADER
#define HAVE_ALBUMART

/* define this to enable bitmap scaling */
#define HAVE_BMP_SCALING

/* define this to enable JPEG decoding */
#define HAVE_JPEG

/* Define this if a programmable hotkey is mapped */
#define HAVE_HOTKEY

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* define this if the target has volume keys which can be used in the lists */
#define HAVE_VOLUME_IN_LIST

/* define this if you have LCD enable function */
#define HAVE_LCD_ENABLE

/* define this if you have a real-time clock */
#define CONFIG_RTC RTC_IMX233

/* define this if you have a real-time clock with alarm facilities */
#define HAVE_RTC_ALARM

#endif /* !BOOTLOADER */

/* define this if you have an i.MX23 codec */
#define HAVE_IMX233_CODEC

#define CONFIG_TUNER SI4700
#define HAVE_RDS_CAP

/* There is no hardware tone control */
#define HAVE_SW_TONE_CONTROLS

#define CONFIG_KEYPAD SANSA_FUZEPLUS_PAD
#define HAVE_TOUCHPAD
#define HAVE_TOUCHPAD_SENSITIVITY_SETTING
#define MIN_TOUCHPAD_SENSITIVITY_SETTING        -25
#define MAX_TOUCHPAD_SENSITIVITY_SETTING        25
#define DEFAULT_TOUCHPAD_SENSITIVITY_SETTING    13

/* Define this to enable dead zone on touchpad */
#ifndef SIMULATOR
#define HAVE_TOUCHPAD_DEADZONE
#define DEFAULT_TOUCHPAD_DEADZONE_SETTING    30
#define MIN_TOUCHPAD_DEADSPACE_SETTING       0
#define MAX_TOUCHPAD_DEADSPACE_SETTING       100
#endif

/* Define this to enable morse code input */
#define HAVE_MORSE_INPUT




/* LCD dimensions */
#define LCD_WIDTH  240
#define LCD_HEIGHT 320
/* sqrt(240^2 + 320^2) / 2.4 = 166.7 */
#define LCD_DPI 167
#define LCD_DEPTH  16   /* 65536 colours */
#define LCD_PIXELFORMAT RGB565 /* rgb565 */

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* Some Sansa Fuzes seem to be FAT16 formatted */
#define HAVE_FAT16SUPPORT

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

//#define AB_REPEAT_ENABLE

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS

/* Main LCD backlight brightness range and defaults */
#define MIN_BRIGHTNESS_SETTING      0
#define MAX_BRIGHTNESS_SETTING      32
#define DEFAULT_BRIGHTNESS_SETTING  16

/* Which backlight fading type? */
#define CONFIG_BACKLIGHT_FADING BACKLIGHT_FADING_SW_SETTING

/* define this if you have a flash memory storage */
#define HAVE_FLASH_STORAGE

/* define this if the flash memory uses the SecureDigital Memory Card protocol */
#define CONFIG_STORAGE (STORAGE_SD | STORAGE_MMC)
#define NUM_DRIVES 2
#define HAVE_HOTSWAP

/* Extra threads: touchpad and rds */
#define TARGET_EXTRA_THREADS 2

/* todo */
#define BATTERY_CAPACITY_DEFAULT 550    /* default battery capacity */
#define BATTERY_CAPACITY_MIN 550        /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 550        /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 0          /* capacity increment */
#define BATTERY_TYPES_COUNT  1          /* only one type */

#define CONFIG_BATTERY_MEASURE VOLTAGE_MEASURE

/* Charging implemented in a target-specific algorithm */
#define CONFIG_CHARGING CHARGING_TARGET

/* define this if the unit can be powered or charged via USB */
#define HAVE_USB_POWER

/* Define this if you have an IMX233*/
#define CONFIG_CPU IMX233

/* Define this if you want to use the IMX233 i2c interface */
#define CONFIG_I2C I2C_IMX233

/* define this if the hardware can be powered off while charging */
#define HAVE_POWEROFF_WHILE_CHARGING

/* define current usage levels (based on battery bench) */
#define CURRENT_NORMAL     35
#define CURRENT_BACKLIGHT  30
#define CURRENT_RECORD     CURRENT_NORMAL

/* maximum charging current */
#define CURRENT_MAX_CHG   200

/* Define this to the CPU frequency */
#define CPU_FREQ      454000000

/* Type of LCD */
#define CONFIG_LCD LCD_FUZEPLUS

/* Offset ( in the firmware file's header ) to the file CRC and data. These are
   only used when loading the old format rockbox.e200 file */
#define FIRMWARE_OFFSET_FILE_CRC    0x0
#define FIRMWARE_OFFSET_FILE_DATA   0x8

/* USB On-the-go */
#define CONFIG_USBOTG USBOTG_ARC

/* enable these for the experimental usb stack */
#define HAVE_USBSTACK
#define USB_EXTRA_STACK 0x80
//#define USB_HANDLED_BY_OF
#define USB_VENDOR_ID 0x0781
#define USB_PRODUCT_ID 0x74e1
#define HAVE_USB_HID_MOUSE
#define HAVE_BOOTLOADER_USB_MODE

/* The fuze+ actually interesting partition table does not use 512-byte sector
 * (usually 2048 logical sector size) */
#define MAX_LOG_SECTOR_SIZE 2048

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

/* Virtual LED (icon) */
#define CONFIG_LED LED_VIRTUAL

/* Define this on iMX233 if the boot process uses a dualboot stub */
#define HAVE_DUALBOOT_STUB

#define BOOTFILE_EXT    "sansa"
#define BOOTFILE        "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"

#define INCLUDE_TIMEOUT_API
