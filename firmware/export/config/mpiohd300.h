/*
 * This config file is for MPIO HD200
 */

/* For Rolo and boot loader */
#define MODEL_NUMBER 70

#define MODEL_NAME   "MPIO HD300"

/* define this if you use an ATA controller */
#define CONFIG_STORAGE STORAGE_ATA
#define HAVE_LBA48
#define ATA_SWAP_WORDS

/* define this if you have recording possibility */
#define HAVE_RECORDING

/* Define bitmask of input sources - recordable bitmask can be defined
 *  explicitly if different
 */
#define INPUT_SRC_CAPS (SRC_CAP_MIC | SRC_CAP_LINEIN | SRC_CAP_FMRADIO)

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
/* #define HAVE_QUICKSCREEN */

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* LCD dimensions */
#define LCD_WIDTH  160
#define LCD_HEIGHT 128
#define LCD_DEPTH  2

#define LCD_PIXELFORMAT VERTICAL_PACKING

/* Display colours, for screenshots and sim (0xRRGGBB) */
#define LCD_DARKCOLOR       0x000000
#define LCD_BRIGHTCOLOR     0x5a915a
#define LCD_BL_DARKCOLOR    0x000000
#define LCD_BL_BRIGHTCOLOR  0x82b4fa

#define CONFIG_KEYPAD MPIO_HD300_PAD

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* Define this if you have RTC */
#define CONFIG_RTC RTC_S35380A
#define HAVE_RTC_ALARM

#define CONFIG_LCD LCD_S1D15E06

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS
#define MIN_BRIGHTNESS_SETTING      0 
#define MAX_BRIGHTNESS_SETTING      31
#define DEFAULT_BRIGHTNESS_SETTING  20


/* define this if you have a disk storage, i.e. something
   that needs spinups and can cause skips when shaked */
#define HAVE_DISK_STORAGE

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

/* FM Tuner
 * turn off for now
 */
#define CONFIG_TUNER       TEA5767
#define CONFIG_TUNER_XTAL  32768


/* we have WM8750 codec in I2S master mode */
#define HAVE_WM8750

/* clocking setup based on 11.2896 MHz master clock
 * provided to the codec by MCU
 * WM8750L Datasheet Table 40, page 46
 */
#define CODEC_SRCTRL_11025HZ (0x18 << 1)
#define CODEC_SRCTRL_22050HZ (0x1A << 1)
#define CODEC_SRCTRL_44100HZ (0x10 << 1)
#define CODEC_SRCTRL_88200HZ (0x1E << 1)

#define BATTERY_TYPES_COUNT 1

#define CONFIG_BATTERY_MEASURE VOLTAGE_MEASURE
#define BATTERY_CAPACITY_DEFAULT  1200  /* this is wild guess */
#define BATTERY_CAPACITY_MIN      800  /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX     2500  /* max. capacity selectable */
#define BATTERY_CAPACITY_INC       50  /* capacity increment */

#define CURRENT_NORMAL     68  /* measured during playback unboosted */
#define CURRENT_BACKLIGHT  24  /* measured */
#define CURRENT_RECORD     40  /* additional current while recording */
#define CURRENT_ATA       100  /* additional current when ata system is ON */
/* #define CURRENT_REMOTE      0  additional current when remote connected */

#define CONFIG_CHARGING CHARGING_MONITOR

#ifndef SIMULATOR

/* Define this if your LCD can set contrast */
#define HAVE_LCD_CONTRAST

/* Define this if you have a Motorola SCF5249 */
#define CONFIG_CPU MCF5249

/* Define this if you want to use coldfire's i2c interface */
#define CONFIG_I2C I2C_COLDFIRE

/* define this if the hardware can be powered off while charging */
/* #define HAVE_POWEROFF_WHILE_CHARGING */

/* The size of the flash ROM */
#define FLASH_SIZE 0x200000

/* Define this to the CPU frequency */
#define CPU_FREQ      11289600

/* Define this if you have ATA power-off control */
#define HAVE_ATA_POWER_OFF

/* Offset ( in the firmware file's header ) to the file length */
#define FIRMWARE_OFFSET_FILE_LENGTH 0

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

#define BOOTFILE_EXT "mpio"
#define BOOTFILE "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"

#define BOOTLOADER_ENTRYPOINT  0x001F0000
#define FLASH_ENTRYPOINT       0x00001000
#define FLASH_MAGIC            0xfbfbfbf1

#endif /* SIMULATOR */

/** Port-specific settings **/

#define MIN_CONTRAST_SETTING        20
#define MAX_CONTRAST_SETTING        32
#define DEFAULT_CONTRAST_SETTING    24

#define IRAM_LCDFRAMEBUFFER IBSS_ATTR /* put the lcd frame buffer in IRAM */
