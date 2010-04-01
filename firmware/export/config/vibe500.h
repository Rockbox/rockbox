/*
 * This config file is for Packard Bell Vibe 500
 */
#define TARGET_TREE /* this target is using the target tree system */

/* For Rolo and boot loader */
#define MODEL_NUMBER 67

#define MODEL_NAME   "Packard Bell Vibe 500"

#define CONFIG_STORAGE STORAGE_ATA

/* define this if you have recording possibility */
#define HAVE_RECORDING

/* Define bitmask of input sources - recordable bitmask can be defined
   explicitly if different */
#define INPUT_SRC_CAPS (SRC_CAP_MIC | SRC_CAP_LINEIN)

/* define the bitmask of hardware sample rates */
#define HW_SAMPR_CAPS   (SAMPR_CAP_88 | SAMPR_CAP_44 | SAMPR_CAP_22 | SAMPR_CAP_11)

/* define the bitmask of recording sample rates */
#define REC_SAMPR_CAPS  (SAMPR_CAP_88 | SAMPR_CAP_44 | SAMPR_CAP_22 | SAMPR_CAP_11)

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

/* define this if you can flip your LCD */
#define HAVE_LCD_FLIP

/* define this if you can ivert the colours on your LCD */
#define HAVE_LCD_INVERT

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR

/* define this if you want album art for this target */
#define HAVE_ALBUMART

/* define this to enable bitmap scaling */
#define HAVE_BMP_SCALING

/* define this to enable JPEG decoding */
#define HAVE_JPEG

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* define this if you have access to the pitchscreen */
#define HAVE_PITCHSCREEN

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* LCD dimensions */
#define LCD_WIDTH  160
#define LCD_HEIGHT 128
#define LCD_DEPTH  16   /* 65536 colors */
#define LCD_PIXELFORMAT RGB565SWAPPED /* rgb565 */

#ifndef BOOTLOADER
/* Define this if your LCD can be enabled/disabled */
#define HAVE_LCD_ENABLE

/* Define this if your LCD can be put to sleep. HAVE_LCD_ENABLE
   should be defined as well. */
#define HAVE_LCD_SLEEP
#define HAVE_LCD_SLEEP_SETTING
#endif

#define CONFIG_KEYPAD PBELL_VIBE500_PAD

/* Define this to enable morse code input */
#define HAVE_MORSE_INPUT

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* define this if you have a real-time clock */
/* Philips 8563T - E8564 is a clone of it */
#define CONFIG_RTC RTC_E8564
#define HAVE_RTC_ALARM

/* define this if you have a disk storage, i.e. something
   that needs spinups and can cause skips when shaked */
#define HAVE_DISK_STORAGE

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

/* Define the type of audio codec */
#define HAVE_WM8731

/* WM8731 has no tone controls, so we use the software ones */
#define HAVE_SW_TONE_CONTROLS

#define AB_REPEAT_ENABLE 1
#define ACTION_WPSAB_SINGLE ACTION_WPS_BROWSE

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS

/* Main LCD backlight brightness range and defaults */
#define MIN_BRIGHTNESS_SETTING      1
#define MAX_BRIGHTNESS_SETTING     16
#define DEFAULT_BRIGHTNESS_SETTING 10

/* define this if you have a light associated with the buttons */
#define HAVE_BUTTON_LIGHT
#define HAVE_BUTTONLIGHT_BRIGHTNESS /* for compatibility */

#define BATTERY_CAPACITY_DEFAULT 1000 /* default battery capacity */
#define BATTERY_CAPACITY_MIN    500   /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX    1500  /* max. capacity selectable */
#define BATTERY_CAPACITY_INC    50    /* capacity increment */
#define BATTERY_TYPES_COUNT     1     /* only one type */

/* Hardware controlled charging, software can monitor plug and charge state */
#define CONFIG_CHARGING CHARGING_SIMPLE

/* define this if the unit can be powered or charged via USB */
#define HAVE_USB_POWER

/* Define this if you have a PortalPlayer PP5020 */
#define CONFIG_CPU PP5020

/* Define this if you want to use the PP5022 i2c interface */
#define CONFIG_I2C I2C_PP5020

/* define this if the hardware can be powered off while charging */
#define HAVE_POWEROFF_WHILE_CHARGING

/* The start address index for ROM builds */
#define ROM_START 0x00000000

/* The size of the flash ROM */
#define FLASH_SIZE 0x80000

/* Define this to the CPU frequency */
#define CPU_FREQ      75000000

#define CONFIG_LCD LCD_VIBE500

/* Define this if your LCD can set contrast */
#define HAVE_LCD_CONTRAST
#define MIN_CONTRAST_SETTING 0
#define MAX_CONTRAST_SETTING 16
#define DEFAULT_CONTRAST_SETTING 10 /* OF */

/* We're able to shut off power to the HDD */
#define HAVE_ATA_POWER_OFF

/* Offset ( in the firmware file's header ) to the file length */
#define FIRMWARE_OFFSET_FILE_LENGTH 0

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

/* USB On-the-go */
#define CONFIG_USBOTG USBOTG_ARC

/* enable these for the experimental usb stack */
#define HAVE_USBSTACK
#define USE_ROCKBOX_USB
#define USB_VENDOR_ID 0x0409
#define USB_PRODUCT_ID 0x8038
#define HAVE_USB_HID_MOUSE

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

/* define this if the device has larger sectors when accessed via USB */
/* (only relevant in disk.c, fat.c now always supports large virtual sectors) */
/* #define MAX_LOG_SECTOR_SIZE 2048 */

/* define this if the hard drive uses large physical sectors (ATA-7 feature) */
/* and doesn't handle them in the drive firmware */
/* #define MAX_PHYS_SECTOR_SIZE 1024 */

#define MI4_FORMAT
#define BOOTFILE_EXT "mi4"
#define BOOTFILE "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"

#define ICODE_ATTR_TREMOR_NOT_MDCT

/* DMA is used only for reading on PP502x because although reads are ~8x faster
 * writes appear to be ~25% slower.
 */
#define HAVE_ATA_DMA

/* Define this if a programmable hotkey is mapped */
//#define HAVE_HOTKEY
