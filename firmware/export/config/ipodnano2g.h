/*
 * This config file is for iPod Nano 2nd Generation
 */

#define IPOD_ARCH 1

/* For Rolo and boot loader */
#define MODEL_NUMBER 62

#define MODEL_NAME   "Apple iPod Nano 2g"

/* define this if you have recording possibility */
//#define HAVE_RECORDING

/* Define bitmask of input sources - recordable bitmask can be defined
   explicitly if different */
#define INPUT_SRC_CAPS (SRC_CAP_LINEIN|SRC_CAP_FMRADIO)

/* define the bitmask of hardware sample rates */
#define HW_SAMPR_CAPS   (SAMPR_CAP_88 | SAMPR_CAP_44 | SAMPR_CAP_22 | SAMPR_CAP_11 \
                       | SAMPR_CAP_96 | SAMPR_CAP_48 | SAMPR_CAP_24 | SAMPR_CAP_12 \
                       | SAMPR_CAP_64 | SAMPR_CAP_32 | SAMPR_CAP_16 | SAMPR_CAP_8)

/* define the bitmask of recording sample rates */
#define REC_SAMPR_CAPS  (SAMPR_CAP_88 | SAMPR_CAP_44 | SAMPR_CAP_22 | SAMPR_CAP_11 \
                       | SAMPR_CAP_96 | SAMPR_CAP_48 | SAMPR_CAP_24 | SAMPR_CAP_12 \
                       | SAMPR_CAP_64 | SAMPR_CAP_32 | SAMPR_CAP_16 | SAMPR_CAP_8)




/* define this if you can flip your LCD */
//#define HAVE_LCD_FLIP

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR

/* define this if you want album art for this target */
#define HAVE_ALBUMART

/* define this to enable bitmap scaling */
#define HAVE_BMP_SCALING

/* define this to enable JPEG decoding */
#define HAVE_JPEG

/* define this if you can invert the colours on your LCD */
//#define HAVE_LCD_INVERT

/* LCD stays visible without backlight - simulator hint */
#define HAVE_TRANSFLECTIVE_LCD

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* define this if the unit uses a scrollwheel for navigation */
#define HAVE_SCROLLWHEEL
#define HAVE_WHEEL_ACCELERATION
#define WHEEL_ACCEL_START 270
#define WHEEL_ACCELERATION 3

/* Define this if you can detect headphones */
#define HAVE_HEADPHONE_DETECTION

/* define this if you have a flash memory storage */
#define HAVE_FLASH_STORAGE

#define CONFIG_STORAGE STORAGE_NAND

#define CONFIG_NAND NAND_SAMSUNG

/* define this if at least one storage driver
   needs to do cleanup on shutdown */
#define HAVE_STORAGE_FLUSH

/* The NAND flash has 2048-byte sectors, and is our only storage */
#define SECTOR_SIZE 2048

/* LCD dimensions */
#define LCD_WIDTH  176
#define LCD_HEIGHT 132
/* sqrt(176^2 + 132^2) / 1.5 = 146.7 */
#define LCD_DPI 147
#define LCD_DEPTH  16   /* pseudo 262.144 colors */
#define LCD_PIXELFORMAT RGB565 /* rgb565 */

/* Define this if the LCD can shut down */
#define HAVE_LCD_SHUTDOWN

/* Define this if your LCD can be enabled/disabled */
#define HAVE_LCD_ENABLE

/* Define this if your LCD can be put to sleep. HAVE_LCD_ENABLE
   should be defined as well. */
#ifndef BOOTLOADER
#define HAVE_LCD_SLEEP
#define HAVE_LCD_SLEEP_SETTING
#endif

#define CONFIG_KEYPAD IPOD_4G_PAD

/* Define this to have CPU boosted while scrolling in the UI */
#define HAVE_GUI_BOOST

#define AB_REPEAT_ENABLE
#define ACTION_WPSAB_SINGLE ACTION_WPS_BROWSE

/* Define this to enable morse code input */
#define HAVE_MORSE_INPUT




/* define this if you have a real-time clock */
#define CONFIG_RTC RTC_NANO2G

/* Define if the device can wake from an RTC alarm */
//#define HAVE_RTC_ALARM

#define CONFIG_LCD LCD_NANO2G

/* Define the type of audio codec */
#define HAVE_WM8975

#define HAVE_PCM_DMA_ADDRESS

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

#define BATTERY_CAPACITY_DEFAULT 400 /* default battery capacity */
#define BATTERY_CAPACITY_MIN     300 /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX     500 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC      10 /* capacity increment */


#define CONFIG_BATTERY_MEASURE VOLTAGE_MEASURE

/* Hardware controlled charging with monitoring */
#define CONFIG_CHARGING CHARGING_MONITOR

/* define current usage levels */
#define CURRENT_NORMAL     17  /* playback @48MHz clock, lcd sleep*/
#define CURRENT_BACKLIGHT  23  /* maximum brightness */

/* define this if the unit can be powered or charged via USB */
#define HAVE_USB_POWER

/* Define this if your LCD can set contrast */
//#define HAVE_LCD_CONTRAST

/* Define Apple remote tuner */
#define CONFIG_TUNER IPOD_REMOTE_TUNER
#define HAVE_RDS_CAP
#define CONFIG_RDS RDS_CFG_PUSH

/* The exact type of CPU */
#define CONFIG_CPU S5L8701

/* I2C interface */
#define CONFIG_I2C I2C_S5L8700

#define HAVE_USB_CHARGING_ENABLE

/* The size of the flash ROM */
#define FLASH_SIZE 0x400000

/* Define this to the CPU frequency */
#define CPU_FREQ        191692800

/* define this if the hardware can be powered off while charging */
#define HAVE_POWEROFF_WHILE_CHARGING

/* Define this if you can read an absolute wheel position */
#define HAVE_WHEEL_POSITION

#define HAVE_HARDWARE_CLICK

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

#define BOOTFILE_EXT "ipod"
#define BOOTFILE "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"

/* Alternative bootfile extension - this is for encrypted images */
#define BOOTFILE_EXT2 "ipodx"

/* Define this for FM radio input available */
#define HAVE_FMRADIO_IN

/** Port-specific settings **/

#if 0
/* Main LCD contrast range and defaults */
#define MIN_CONTRAST_SETTING        1
#define MAX_CONTRAST_SETTING        30
#define DEFAULT_CONTRAST_SETTING    19 /* Match boot contrast */
#endif

/* Main LCD backlight brightness range and defaults */
#define MIN_BRIGHTNESS_SETTING      1
#define MAX_BRIGHTNESS_SETTING      0x2e
#define DEFAULT_BRIGHTNESS_SETTING  0x20

/* USB defines */
#define CONFIG_USBOTG USBOTG_DESIGNWARE
#define USB_DW_CLOCK 0
#define USB_DW_TURNAROUND 5
/* logf() over USB serial (http://www.rockbox.org/wiki/PortalPlayerUsb) */
//#define USB_ENABLE_SERIAL
#define HAVE_USBSTACK
#define HAVE_USB_HID_MOUSE
#define USB_VENDOR_ID 0x05AC
#define USB_PRODUCT_ID 0x1260
#define USB_DEVBSS_ATTR __attribute__((aligned(16)))

/* Define this if you can switch on/off the accessory power supply */
#define HAVE_ACCESSORY_SUPPLY

/* Serial */
#ifndef BOOTLOADER
#define HAVE_SERIAL
/* Disable iAP when LOGF_SERIAL is enabled to avoid conflicts */
#ifndef LOGF_SERIAL
#define IPOD_ACCESSORY_PROTOCOL
#endif
#endif

/* Define this, if you can switch on/off the lineout */
#define HAVE_LINEOUT_POWEROFF

#define USB_WRITE_BUFFER_SIZE (1024*64)

/* Define this if a programmable hotkey is mapped */
#define HAVE_HOTKEY
