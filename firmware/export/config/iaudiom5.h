/*
 * This config file is for iAudio M5
 */

/* For Rolo and boot loader */
#define MODEL_NUMBER 17

#define MODEL_NAME   "iAudio M5"

/* define this if you use an ATA controller */
#define CONFIG_STORAGE STORAGE_ATA

/*define this if the ATA controller and method of USB access support LBA48 */
#define HAVE_LBA48

/* define this if you have recording possibility */
#define HAVE_RECORDING

/* Define bitmask of input sources - recordable bitmask can be defined
   explicitly if different */
#ifdef HAVE_FMRADIO_IN /* FM modded M5 */
#define INPUT_SRC_CAPS (SRC_CAP_MIC | SRC_CAP_LINEIN | SRC_CAP_FMRADIO)
#else /* stock M5 */
#define INPUT_SRC_CAPS (SRC_CAP_MIC | SRC_CAP_LINEIN)
#endif

/* define the bitmask of hardware sample rates */
#define HW_SAMPR_CAPS   (SAMPR_CAP_88 | SAMPR_CAP_44 | SAMPR_CAP_22 | SAMPR_CAP_11)

/* define the bitmask of recording sample rates */
#define REC_SAMPR_CAPS  (SAMPR_CAP_88 | SAMPR_CAP_44 | SAMPR_CAP_22 | SAMPR_CAP_11)

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

/* define this if you want album art for this target */
#define HAVE_ALBUMART

/* define this to enable bitmap scaling */
#define HAVE_BMP_SCALING

/* define this to enable JPEG decoding */
#define HAVE_JPEG

/* define this if you can flip your LCD */
#define HAVE_LCD_FLIP

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
#define LCD_DEPTH  2

#define LCD_PIXELFORMAT VERTICAL_PACKING

/* Display colours, for screenshots and sim (0xRRGGBB) */
#define LCD_DARKCOLOR       0x000000
#define LCD_BRIGHTCOLOR     0x648764
#define LCD_BL_DARKCOLOR    0x000000
#define LCD_BL_BRIGHTCOLOR  0xdfd8ff

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

#define CONFIG_LCD LCD_S1D15E06

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

#ifdef HAVE_FMRADIO_IN /* FM modded M5 */
/* FM Tuner */
#define CONFIG_TUNER       TEA5767
#define CONFIG_TUNER_XTAL  32768
#endif

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

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

#define BOOTFILE_EXT "iaudio"
#define BOOTFILE "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"

#define BOOTLOADER_ENTRYPOINT  0x001F0000
#define FLASH_ENTRYPOINT       0x00001000
#define FLASH_MAGIC            0xfbfbfbf1

/** Port-specific settings **/

/* Main LCD contrast range and defaults */
#define MIN_CONTRAST_SETTING        24
#define MAX_CONTRAST_SETTING        63
#define DEFAULT_CONTRAST_SETTING    40

/* Remote LCD contrast range and defaults */
#define MIN_REMOTE_CONTRAST_SETTING     10
#define MAX_REMOTE_CONTRAST_SETTING     35
#define DEFAULT_REMOTE_CONTRAST_SETTING 24 /* Match boot contrast */

#define IRAM_LCDFRAMEBUFFER IBSS_ATTR /* put the lcd frame buffer in IRAM */

/* Define this if a programmable hotkey is mapped */
//#define HAVE_HOTKEY
