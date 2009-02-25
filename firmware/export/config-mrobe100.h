/*
 * This config file is for the Olympus m:robe MR-100
 */
 
#define TARGET_TREE

/* For Rolo and boot loader */
#define MODEL_NUMBER 23
#define MODEL_NAME   "Olympus m:robe MR-100"

/* define this if you use an ATA controller */
#define CONFIG_STORAGE STORAGE_ATA

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* define this if you have access to the pitchscreen */
#define HAVE_PITCHSCREEN

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* LCD dimensions */
#define LCD_WIDTH  160
#define LCD_HEIGHT 128
#define LCD_DEPTH  1

#define LCD_PIXELFORMAT VERTICAL_PACKING
#define HAVE_NEGATIVE_LCD /* bright on dark */

/* Display colours, for screenshots and sim (0xRRGGBB) */
#define LCD_DARKCOLOR       0x000000
#define LCD_BRIGHTCOLOR     0x323232
#define LCD_BL_DARKCOLOR    0x5e0202
#define LCD_BL_BRIGHTCOLOR  0xf10603

/* define this if you can flip your LCD */
#define HAVE_LCD_FLIP

/* define this if you can invert the colours on your LCD */
#define HAVE_LCD_INVERT

/*#define IRAM_LCDFRAMEBUFFER IDATA_ATTR */ /* put the lcd frame buffer in IRAM */

#define CONFIG_KEYPAD MROBE100_PAD

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* define this if you have a real-time clock */
#ifndef BOOTLOADER
#define CONFIG_RTC RTC_MR100
#endif

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

/* Wolfsom audio codec */
#define HAVE_WM8751
#define CODEC_SRCTRL_44100HZ     (0x40|(0x11 << 1)|1)

#define AB_REPEAT_ENABLE 1

/* define this if you have a disk storage, i.e. something
   that needs spinups and can cause skips when shaked */
#define HAVE_DISK_STORAGE

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT

/* We can fade the backlight by using PWM */
#define CONFIG_BACKLIGHT_FADING BACKLIGHT_FADING_PWM

/* Main LCD backlight brightness range and defaults */
#define MIN_BRIGHTNESS_SETTING      1
#define MAX_BRIGHTNESS_SETTING     12
#define DEFAULT_BRIGHTNESS_SETTING  6

/* define this if you have a light associated with the buttons */
#define HAVE_BUTTON_LIGHT

#define HAVE_BUTTONLIGHT_BRIGHTNESS

#define BATTERY_CAPACITY_DEFAULT 720 /* default battery capacity */

#ifndef SIMULATOR

/* Define this if you have a PortalPlayer PP5020 */
#define CONFIG_CPU PP5020

/* Define this if you want to use the PP5020 i2c interface */
#define CONFIG_I2C I2C_PP5020

/* Type of mobile power */
#define CONFIG_BATTERY BATT_LIION750
#define BATTERY_CAPACITY_MIN 750        /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 750        /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 0          /* capacity increment */
#define BATTERY_TYPES_COUNT  1          /* only one type */

/* Hardware controlled charging */
#define CONFIG_CHARGING CHARGING_SIMPLE

/* define this if the hardware can be powered off while charging */
#define HAVE_POWEROFF_WHILE_CHARGING

/* The start address index for ROM builds */
#define ROM_START   0x00000000

/* The size of the flash ROM */
#define FLASH_SIZE  0x100000

/* Define this to the CPU frequency */
#define CPU_FREQ    75000000

/* Type of LCD */
#define CONFIG_LCD  LCD_MROBE100

/* Define this if your LCD can set contrast */
#define HAVE_LCD_CONTRAST

#define MIN_CONTRAST_SETTING        0
#define MAX_CONTRAST_SETTING        40
#define DEFAULT_CONTRAST_SETTING    20

/* Define this if your LCD can be enabled/disabled */
/* TODO: #define HAVE_LCD_ENABLE */

/* Define this if your LCD can be put to sleep. HAVE_LCD_ENABLE
 * should be defined as well.
 * We can currently put the lcd to sleep but it won't wake up properly */
/*TODO: #define HAVE_LCD_SLEEP*/
/*TODO: #define HAVE_LCD_SLEEP_SETTING <= optional */

/* We're able to shut off power to the HDD */
#define HAVE_ATA_POWER_OFF

/* USB On-the-go */
#define CONFIG_USBOTG USBOTG_ARC

/* define this if the unit can be powered or charged via USB */
#define HAVE_USB_POWER

/* enable these for the experimental usb stack */
#define HAVE_USBSTACK
#define USE_ROCKBOX_USB
#define USB_VENDOR_ID 0x07B4
#define USB_PRODUCT_ID 0x0280

/* Virtual LED (icon) */
#define CONFIG_LED LED_VIRTUAL

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

/* Define this if you can detect headphones */
#define HAVE_HEADPHONE_DETECTION

#define MI4_FORMAT
#define BOOTFILE_EXT    "mi4"
#define BOOTFILE        "rockbox." BOOTFILE_EXT
#define BOOTDIR         "/.rockbox"

/* Offset ( in the firmware file's header ) to the file CRC and data.
   Not used for the mrobe 100, since it boots an mi4 file, but needed
   for compatibility. */
#define FIRMWARE_OFFSET_FILE_CRC    0x0
#define FIRMWARE_OFFSET_FILE_DATA   0x0

#define ICODE_ATTR_TREMOR_NOT_MDCT

#endif
