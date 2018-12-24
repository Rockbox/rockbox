/*
 * This config file is for iriver H320, H340
 */
#define IRIVER_H300_SERIES 1

#define MODEL_NAME "iriver H300 series"

/* For Rolo and boot loader */
#define MODEL_NUMBER 2

/* define this if you use an ATA controller */
#define CONFIG_STORAGE STORAGE_ATA

/*define this if the ATA controller and method of USB access support LBA48 */
#define HAVE_LBA48

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR

/* define this if you want album art for this target */
#define HAVE_ALBUMART

/* define this to enable bitmap scaling */
#define HAVE_BMP_SCALING

/* define this to enable JPEG decoding */
#define HAVE_JPEG

/* define this if you can flip your LCD */
#define HAVE_LCD_FLIP

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* LCD dimensions */
#define LCD_WIDTH  220
#define LCD_HEIGHT 176
/* sqrt(220^2 + 176^2) / 2.0 = 140.9 */
#define LCD_DPI 141
#define LCD_DEPTH  16   /* 65k colours */
#define LCD_PIXELFORMAT RGB565 /* rgb565 */

/* remote LCD */
#define LCD_REMOTE_WIDTH  128
#define LCD_REMOTE_HEIGHT 64
#define LCD_REMOTE_DEPTH  1

#define LCD_REMOTE_PIXELFORMAT VERTICAL_PACKING

/* Remote display colours, for screenshots and sim (0xRRGGBB) */
#define LCD_REMOTE_DARKCOLOR       0x000000
#define LCD_REMOTE_BRIGHTCOLOR     0x5a915a
#define LCD_REMOTE_BL_DARKCOLOR    0x000000
#define LCD_REMOTE_BL_BRIGHTCOLOR  0x82b4fa

#define CONFIG_KEYPAD IRIVER_H300_PAD

#define CONFIG_REMOTE_KEYPAD H300_REMOTE

/* Define this to enable morse code input */
#define HAVE_MORSE_INPUT

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* define this if you have a real-time clock */
#define CONFIG_RTC RTC_PCF50606

/* define this if you have a disk storage, i.e. something
   that needs spinups and can cause skips when shaked */
#define HAVE_DISK_STORAGE

/* Define if the device can wake from an RTC alarm */
#define HAVE_RTC_ALARM

/* Define this if you have an remote lcd */
#define HAVE_REMOTE_LCD

/* Define if we have a hardware defect that causes ticking on the audio line */
#define HAVE_REMOTE_LCD_TICKING

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS
/* Which backlight fading type? */
#define CONFIG_BACKLIGHT_FADING BACKLIGHT_FADING_SW_HW_REG
/* Define this if your LCD can be enabled/disabled */
#define HAVE_LCD_ENABLE

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

#define AB_REPEAT_ENABLE

#define CONFIG_TUNER TEA5767
#define CONFIG_TUNER_XTAL 32768

#define HAVE_UDA1380

/* define this if you have recording possibility */
#define HAVE_RECORDING

#define HAVE_HISTOGRAM

/* Define bitmask of input sources - recordable bitmask can be defined
   explicitly if different */
#define INPUT_SRC_CAPS (SRC_CAP_MIC | SRC_CAP_LINEIN | SRC_CAP_FMRADIO)

/* define hardware samples rate caps mask */
#define HW_SAMPR_CAPS   (SAMPR_CAP_88 | SAMPR_CAP_44 | SAMPR_CAP_22 | SAMPR_CAP_11)

/* define the bitmask of recording sample rates */
#define REC_SAMPR_CAPS  (SAMPR_CAP_44 | SAMPR_CAP_22 | SAMPR_CAP_11)

#define HAVE_AGC

#define BATTERY_CAPACITY_DEFAULT 1300 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 1300 /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 3200 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 50   /* capacity increment */
#define BATTERY_TYPES_COUNT  1    /* only one type */

#define CONFIG_BATTERY_MEASURE VOLTAGE_MEASURE

/* Hardware controlled charging with monitoring */
#define CONFIG_CHARGING CHARGING_MONITOR

/* define this if the unit can be powered or charged via USB */
#define HAVE_USB_POWER

/* define current usage levels */
#define CURRENT_NORMAL     80  /* 16h playback on 1300mAh battery from IriverRuntime wiki page */
#define CURRENT_BACKLIGHT  23  /* FIXME: This needs to be measured, copied from H100 */
#define CURRENT_RECORD    110  /* additional current while recording */
#define CURRENT_MAX_CHG   650  /* maximum charging current */
#define CURRENT_REMOTE      8  /* additional current when remote connected */

/* define this if the unit can have USB charging disabled by user -
 * if USB/MAIN power is discernable and hardware doesn't compel charging */
#define HAVE_USB_CHARGING_ENABLE

/* Define this if you have a Motorola SCF5249 */
#define CONFIG_CPU MCF5249

/* Define this if you want to use coldfire's i2c interface */
#define CONFIG_I2C I2C_COLDFIRE

/* The size of the flash ROM */
#define FLASH_SIZE 0x400000

/* Define this to the CPU frequency */
#define CPU_FREQ      11289600

/* Define this if you have ATA power-off control */
#define HAVE_ATA_POWER_OFF

#define CONFIG_LCD LCD_H300

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

/* USB On-the-go */
#define CONFIG_USBOTG USBOTG_ISP1362

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

#define BOOTFILE_EXT "iriver"
#define BOOTFILE "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"

#define BOOTLOADER_ENTRYPOINT  0x001F0000
#define FLASH_ENTRYPOINT       0x00001000
#define FLASH_MAGIC            0xfbfbfbf1

/* Define this if there is an EEPROM chip */
#define HAVE_EEPROM

/* Define this for FM radio input available */
#define HAVE_FMRADIO_IN

/** Port-specific settings **/

/* Main LCD contrast range and defaults */
#define MIN_CONTRAST_SETTING        5
#define MAX_CONTRAST_SETTING        63
#define DEFAULT_CONTRAST_SETTING    40

/* Main LCD backlight brightness range and defaults */
/* accepts 0..15 but note that 0 and 1 give a black display on H300! */
#define MIN_BRIGHTNESS_SETTING      2  /*  2/16 (12.50%) */
#define MAX_BRIGHTNESS_SETTING      15 /* 15/16 (93.75%) */
#define DEFAULT_BRIGHTNESS_SETTING  9  /*  9/16 (56.25%) */

/* Remote LCD contrast range and defaults */
#define MIN_REMOTE_CONTRAST_SETTING     5
#define MAX_REMOTE_CONTRAST_SETTING     63
#define DEFAULT_REMOTE_CONTRAST_SETTING 42 

/* Define this if a programmable hotkey is mapped */
#define HAVE_HOTKEY
