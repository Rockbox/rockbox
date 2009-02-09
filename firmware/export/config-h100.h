#if defined(MEM) && (MEM > 16)
#error "re-run configure this just so wrong"
#endif

#define TARGET_TREE /* this target is using the target tree system */

/*
 * This config file is for iriver iHP-100, iHP-110, iHP-115
 */
#define IRIVER_H100_SERIES 1

#define MODEL_NAME "iriver iHP-100 series"

/* For Rolo and boot loader */
#define MODEL_NUMBER 1

/* define this if you use an ATA controller */
#define CONFIG_STORAGE STORAGE_ATA

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

/* define this if you want album art for this target */
#define HAVE_ALBUMART

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
#define LCD_WIDTH  160
#define LCD_HEIGHT 128
#define LCD_DEPTH  2

#define LCD_PIXELFORMAT VERTICAL_PACKING

/* Display colours, for screenshots and sim (0xRRGGBB) */
#define LCD_DARKCOLOR       0x000000
#define LCD_BRIGHTCOLOR     0x7e917e
#define LCD_BL_DARKCOLOR    0x000000
#define LCD_BL_BRIGHTCOLOR  0xadd8e6

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

#define CONFIG_KEYPAD IRIVER_H100_PAD

#define CONFIG_REMOTE_KEYPAD H100_REMOTE

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* Define this if you have an remote lcd */
#define HAVE_REMOTE_LCD

/* Define if we have a hardware defect that causes ticking on the audio line */
#define HAVE_REMOTE_LCD_TICKING

#define CONFIG_LCD LCD_S1D15E06

/* define this if you have a disk storage, i.e. something
   that needs spinups and can cause skips when shaked */
#define HAVE_DISK_STORAGE

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT

/* We can fade the backlight by using PWM */
#define CONFIG_BACKLIGHT_FADING BACKLIGHT_FADING_PWM

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

#define AB_REPEAT_ENABLE 1

#define CONFIG_TUNER TEA5767
#define CONFIG_TUNER_XTAL 32768

#define HAVE_UDA1380

/* define this if you have recording possibility */
#define HAVE_RECORDING

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

/* Hardware controlled charging */
#define CONFIG_CHARGING CHARGING_SIMPLE

/* define current usage levels */
#define CURRENT_NORMAL     80  /* 16h playback on 1300mAh battery */
#define CURRENT_BACKLIGHT  23  /* from IriverBattery twiki page */
#define CURRENT_SPDIF_OUT  10  /* optical SPDIF output on */
#define CURRENT_RECORD    105  /* additional current while recording */
#define CURRENT_REMOTE      8  /* additional current when remote connected */

#ifndef SIMULATOR

/* Define this if you have a Motorola SCF5249 */
#define CONFIG_CPU MCF5249

/* Define this if you want to use coldfire's i2c interface */
#define CONFIG_I2C I2C_COLDFIRE

/* Define this if you can run rockbox from flash memory */
/* In theory we can, but somebody needs to verify there are no issues. */
#define HAVE_FLASHED_ROCKBOX

/* define this if the hardware can be powered off while charging */
#define HAVE_POWEROFF_WHILE_CHARGING

/* The size of the flash ROM */
#define FLASH_SIZE 0x200000

/* Define this to the CPU frequency */
#define CPU_FREQ      11289600

/* Define this if you have ATA power-off control */
#define HAVE_ATA_POWER_OFF

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

#define HAVE_ATA_LED_CTRL

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

#define BOOTFILE_EXT "iriver"
#define BOOTFILE "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"

#define BOOTLOADER_ENTRYPOINT  0x001F0000
#define FLASH_RAMIMAGE_ENTRY   0x00001000
#define FLASH_ROMIMAGE_ENTRY   0x00100000
#define FLASH_MAGIC            0xfbfbfbf2

/* Define this if there is an EEPROM chip */
#define HAVE_EEPROM

/* Define this if the EEPROM chip is used */
#define HAVE_EEPROM_SETTINGS

#endif /* !SIMULATOR */

/* Define bitmask of input sources - recordable bitmask can be defined
   explicitly if different */
#define INPUT_SRC_CAPS (SRC_CAP_MIC | SRC_CAP_LINEIN | \
                        SRC_CAP_FMRADIO | SRC_CAP_SPDIF)

/* Define this for S/PDIF output available */
#define HAVE_SPDIF_OUT

/* Define this if you can control the S/PDIF power */
#define HAVE_SPDIF_POWER
#define SPDIF_POWER_INVERTED

/* Define this for FM radio input available */
#define HAVE_FMRADIO_IN

/** Port-specific settings **/

#define HAVE_LCD_CONTRAST

/* Main LCD backlight brightness range and defaults */
#define MIN_CONTRAST_SETTING        14 /* White screen a bit higher than this */
#define MAX_CONTRAST_SETTING        63 /* Black screen a bit lower than this */
#define DEFAULT_CONTRAST_SETTING    27

/* Remote LCD contrast range and defaults */
#define MIN_REMOTE_CONTRAST_SETTING     5
#define MAX_REMOTE_CONTRAST_SETTING     63
#define DEFAULT_REMOTE_CONTRAST_SETTING 42
