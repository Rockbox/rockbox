/*
 * This config file is for MPIO HD200
 */
#define TARGET_TREE /* this target is using the target tree system */

/* For Rolo and boot loader */
#define MODEL_NUMBER 69

#define MODEL_NAME   "MPIO HD200"

/* define this if you use an ATA controller */
#define CONFIG_STORAGE STORAGE_ATA
#define HAVE_LBA48

/* define this if you have recording possibility */
/* not implemented yet 
 * #define HAVE_RECORDING
 */


/* Define bitmask of input sources - recordable bitmask can be defined
 *  explicitly if different
 * not implemented yet
 */

#define INPUT_SRC_CAPS (SRC_CAP_MIC | SRC_CAP_LINEIN | SRC_CAP_FMRADIO)


/* define the bitmask of hardware sample rates */
#define HW_SAMPR_CAPS   (SAMPR_CAP_88 | SAMPR_CAP_44 | SAMPR_CAP_22 | SAMPR_CAP_11)

/* define the bitmask of recording sample rates
 * not implemented yet
 *#define REC_SAMPR_CAPS  (SAMPR_CAP_88 | SAMPR_CAP_44 | SAMPR_CAP_22 | SAMPR_CAP_11)
 */

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

/* define this if you have access to the pitchscreen */
#define HAVE_PITCHSCREEN

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* LCD dimensions */
#define LCD_WIDTH  128
#define LCD_HEIGHT 128
#define LCD_DEPTH  2

#define LCD_PIXELFORMAT VERTICAL_INTERLEAVED

/* Display colours, for screenshots and sim (0xRRGGBB) */
#define LCD_DARKCOLOR       0x000000
#define LCD_BRIGHTCOLOR     0x5a915a
#define LCD_BL_DARKCOLOR    0x000000
#define LCD_BL_BRIGHTCOLOR  0x82b4fa

#define CONFIG_KEYPAD MPIO_HD200_PAD

#define AB_REPEAT_ENABLE 1
#define ACTION_WPSAB_SINGLE ACTION_WPS_BROWSE

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

#define CONFIG_LCD LCD_TL0350A
#define HAVE_LCD_SHUTDOWN

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


/* we have WM8750 codec in I2S slave mode */
#define HAVE_WM8750
#define CODEC_SLAVE

#define BATTERY_CAPACITY_DEFAULT 950 /* default battery capacity */
#define BATTERY_CAPACITY_MIN    950   /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX    2250  /* max. capacity selectable */
#define BATTERY_CAPACITY_INC    50    /* capacity increment */
#define BATTERY_TYPES_COUNT     1     /* only one type */

#define CONFIG_CHARGING CHARGING_MONITOR

/* define current usage levels */
/* additional current when remote connected */
/*
#define CURRENT_REMOTE      8 
*/
#ifndef SIMULATOR

/* Define this if your LCD can set contrast */
#define HAVE_LCD_CONTRAST

/* Define this if you have a Motorola SCF5249 */
#define CONFIG_CPU MCF5249

/* Define this if you want to use coldfire's i2c interface */
#define CONFIG_I2C I2C_COLDFIRE

/* OF resets device instead of poweroff while charging
 * this triggers bootloader code which takes care of charging.
 * I have feeling that powering off while charging may cause
 * partition table corruption I am experiencing from time to time
 */

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

/* Main LCD contrast range and defaults  taken from OF*/
#define MIN_CONTRAST_SETTING        24
#define MAX_CONTRAST_SETTING        63
#define DEFAULT_CONTRAST_SETTING    27 /* 0x1B */

#define IRAM_LCDFRAMEBUFFER IBSS_ATTR /* put the lcd frame buffer in IRAM */
