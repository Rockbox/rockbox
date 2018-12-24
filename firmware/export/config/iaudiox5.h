/*
 * This config file is for iAudio X5
 */

/* For Rolo and boot loader */
#define MODEL_NUMBER 10

#define MODEL_NAME   "iAudio X5"

/* define this if you use an ATA controller */
#define CONFIG_STORAGE STORAGE_ATA

/*define this if the ATA controller and method of USB access support LBA48 */
#define HAVE_LBA48

/* define this if you have recording possibility */
#define HAVE_RECORDING

/* Define bitmask of input sources - recordable bitmask can be defined
   explicitly if different */
#define INPUT_SRC_CAPS (SRC_CAP_MIC | SRC_CAP_LINEIN | SRC_CAP_FMRADIO)

/* define the bitmask of hardware sample rates */
#define HW_SAMPR_CAPS   (SAMPR_CAP_88 | SAMPR_CAP_44 | SAMPR_CAP_22 | SAMPR_CAP_11)

/* define the bitmask of recording sample rates */
#define REC_SAMPR_CAPS  (SAMPR_CAP_88 | SAMPR_CAP_44 | SAMPR_CAP_22 | SAMPR_CAP_11)

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

/* define this if you can flip your LCD */
#define HAVE_LCD_FLIP

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR

/* define this if you want album art for this target */
#define HAVE_ALBUMART

/* define this to enable bitmap scaling */
#define HAVE_BMP_SCALING

/* define this to enable JPEG decoding */
#define HAVE_JPEG

/* define this if you can invert the colours on your LCD */
#define HAVE_LCD_INVERT

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* LCD dimensions */
#define LCD_WIDTH  160
#define LCD_HEIGHT 128
/* sqrt(160^2 + 128^2) / 1.8 = 113.8 */
#define LCD_DPI 114
#define LCD_DEPTH  16   /* pseudo 262.144 colors */
#define LCD_PIXELFORMAT RGB565 /* rgb565 */

/* remote LCD */
#define LCD_REMOTE_WIDTH  128
#define LCD_REMOTE_HEIGHT 96
#define LCD_REMOTE_DEPTH  2

#define LCD_REMOTE_PIXELFORMAT VERTICAL_INTERLEAVED

/* Remote display colours, for screenshots and sim (0xRRGGBB) */
#define LCD_REMOTE_DARKCOLOR       0x000000
#define LCD_REMOTE_BRIGHTCOLOR     0x5a915a
#define LCD_REMOTE_BL_DARKCOLOR    0x000000
#define LCD_REMOTE_BL_BRIGHTCOLOR  0x82b4fa

#ifndef BOOTLOADER
/* Define this if your LCD can be enabled/disabled */
#define HAVE_LCD_ENABLE

/* Define this if your LCD can be put to sleep. HAVE_LCD_ENABLE
   should be defined as well. */
#define HAVE_LCD_SLEEP
#define HAVE_LCD_SLEEP_SETTING
#endif

#define CONFIG_KEYPAD IAUDIO_X5M5_PAD

#define CONFIG_REMOTE_KEYPAD IAUDIO_REMOTE

/* Define this to enable morse code input */
#define HAVE_MORSE_INPUT

#define AB_REPEAT_ENABLE
#define ACTION_WPSAB_SINGLE ACTION_WPS_BROWSE

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* define this if you have a real-time clock */
#define CONFIG_RTC RTC_PCF50606

/* define this if you have a real-time clock alarm */
#define HAVE_RTC_ALARM

/* Define this if you have an remote lcd */
#define HAVE_REMOTE_LCD

#define CONFIG_LCD LCD_X5

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS
/* Which backlight fading type? */
#define CONFIG_BACKLIGHT_FADING BACKLIGHT_FADING_SW_HW_REG

/* define this if you have a disk storage, i.e. something
   that needs spinups and can cause skips when shaked */
#define HAVE_DISK_STORAGE

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

/* FM Tuner */
#define CONFIG_TUNER       TEA5767
#define CONFIG_TUNER_XTAL  32768

#define HAVE_TLV320

/* TLV320 has no tone controls, so we use the software ones */
#define HAVE_SW_TONE_CONTROLS

#define BATTERY_CAPACITY_DEFAULT 950 /* default battery capacity */
#define BATTERY_CAPACITY_MIN    950   /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX    2250  /* max. capacity selectable */
#define BATTERY_CAPACITY_INC    50    /* capacity increment */
#define BATTERY_TYPES_COUNT     1     /* only one type */

#define CONFIG_BATTERY_MEASURE VOLTAGE_MEASURE

/* Hardware controlled charging? FIXME */
#define CONFIG_CHARGING CHARGING_SIMPLE

/* define current usage levels */
#define CURRENT_NORMAL     65  /*2250mah/35h = 65 ma*/  
#define CURRENT_BACKLIGHT  25  
#define CURRENT_REMOTE      8  /* additional current when remote connected */

/* Define this if your LCD can set contrast */
#define HAVE_LCD_CONTRAST

/* Define this if you have a Motorola SCF5250 */
#define CONFIG_CPU MCF5250

/* Define this if you want to use coldfire's i2c interface */
#define CONFIG_I2C I2C_COLDFIRE

/* define this if the hardware can be powered off while charging */
#define HAVE_POWEROFF_WHILE_CHARGING

/* The size of the flash ROM */
#define FLASH_SIZE 0x400000

/* Define this to the CPU frequency */
#define CPU_FREQ      11289600

/* Define this if you have ATA power-off control */
#define HAVE_ATA_POWER_OFF

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

/* USB On-the-go */
#define CONFIG_USBOTG USBOTG_M5636

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

#define BOOTFILE_EXT "iaudio"
#define BOOTFILE "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"

#define BOOTLOADER_ENTRYPOINT  0x001F0000
#define FLASH_ENTRYPOINT       0x00001000
#define FLASH_MAGIC            0xfbfbfbf1

/* Define this for FM radio input available */
#define HAVE_FMRADIO_IN

/** Port-specific settings **/

/* Main LCD contrast range and defaults */
#define MIN_CONTRAST_SETTING        1
#define MAX_CONTRAST_SETTING        30
#define DEFAULT_CONTRAST_SETTING    19 /* Match boot contrast */

/* Main LCD backlight brightness range and defaults */
/* PCF50506 can output 0%-100% duty cycle but D305A expects %15-100%. */
#define MIN_BRIGHTNESS_SETTING      1  /* 15/16 (93.75%) */
#define MAX_BRIGHTNESS_SETTING      13 /*  3/16 (18.75%) */
#define DEFAULT_BRIGHTNESS_SETTING  8  /*  8/16 (50.00%) = x5 boot default */

/* Remote LCD contrast range and defaults */
#define MIN_REMOTE_CONTRAST_SETTING     10
#define MAX_REMOTE_CONTRAST_SETTING     35
#define DEFAULT_REMOTE_CONTRAST_SETTING 24 /* Match boot contrast */

/* Define this if a programmable hotkey is mapped */
//#define HAVE_HOTKEY
