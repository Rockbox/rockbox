/*
 * This config file is for the Samsung YP-CP3 (rk27xx)
 */

/* For Rolo and boot loader */
#define MODEL_NUMBER 132

#define MODEL_NAME   "Samsung YP-CP3"

/* define this if you have recording possibility */
#define HAVE_RECORDING

/* define the bitmask of hardware sample rates */
#define HW_SAMPR_CAPS   (SAMPR_CAP_44 | SAMPR_CAP_22 | SAMPR_CAP_11 \
                       | SAMPR_CAP_48 | SAMPR_CAP_24 | SAMPR_CAP_12 \
                       | SAMPR_CAP_32 | SAMPR_CAP_16 | SAMPR_CAP_8)

/* define the bitmask of recording sample rates */
#define REC_SAMPR_CAPS  (SAMPR_CAP_44 | SAMPR_CAP_22 | SAMPR_CAP_11 \
                       | SAMPR_CAP_48 | SAMPR_CAP_24 | SAMPR_CAP_12 \
                       | SAMPR_CAP_32 | SAMPR_CAP_16 | SAMPR_CAP_8)

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

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* define this if you have a flash memory storage */
#define HAVE_FLASH_STORAGE

#define CONFIG_STORAGE (STORAGE_SD | STORAGE_NAND)

#define CONFIG_NAND NAND_RK27XX

/* commented for now */
/* #define HAVE_HOTSWAP */

/* The NAND holds two volumes, SYS and USER; SYS is hidden - see
 * config/rk27generic.h for why and for what HAVE_RK27XX_NAND_SYS does.
 * storage.c numbers drives by driver, SD first:
 *
 *                      default             HAVE_RK27XX_NAND_SYS
 *   drive 0            SD                  SD
 *   drive 1            NAND USER           NAND SYS
 *   drive 2                                NAND USER
 */
/* #define HAVE_RK27XX_NAND_SYS */

#ifdef HAVE_RK27XX_NAND_SYS
#define NUM_DRIVES 3
#else
#define NUM_DRIVES 2
#endif
#define SECTOR_SIZE 512

/* for small(ish) SD cards */
#define HAVE_FAT16SUPPORT

/* LCD dimensions */
#define LCD_WIDTH  400
#define LCD_HEIGHT 240
/* sqrt(400^2 + 240^2) / 3.0 = 155.5 */
#define LCD_DPI 155
#define LCD_DEPTH  16   /* pseudo 262.144 colors */
#define LCD_PIXELFORMAT RGB565 /* rgb565 */

/* Define this if your LCD can be put to sleep. HAVE_LCD_ENABLE
   should be defined as well. */
#ifndef BOOTLOADER
/* TODO: #define HAVE_LCD_SLEEP */
/* TODO: #define HAVE_LCD_SLEEP_SETTING */
#endif

/* the YP-R0's key set - see ypcp3/button-target.h */
#define CONFIG_KEYPAD SAMSUNG_YPR0_PAD

/* Define this to enable morse code input */
#define HAVE_MORSE_INPUT

#define CONFIG_LCD LCD_SPFD5420A

/* Wolfson WM8750 on I2C, headphones on its OUT2. The codec is the I2S
 * slave: the rk27xx drives the bus and clocks the codec from its codec PLL
 * at 256 fs for every rate (pcm-rk27xx.c), so the codec's CLOCKING register
 * is its normal-mode 256 fs setting throughout. The original firmware runs
 * the codec as master off a fixed 12 MHz instead, in USB mode, which puts
 * 44.1 kHz at 44.118. */
#define HAVE_WM8750
#define CODEC_SLAVE
#define CODEC_SRCTRL_11025HZ 0
#define CODEC_SRCTRL_22050HZ 0
#define CODEC_SRCTRL_44100HZ 0

/* Seiko S-35390A real-time clock on I2C, left in 12-hour mode by the
 * original firmware */
#define CONFIG_RTC RTC_S35390A

/* Silicon Labs Si4703 FM tuner on I2C, with RDS. It has no power control -
 * the original firmware has none either - and its audio reaches the codec
 * on LINPUT1/RINPUT1. RDS is polled, which needs no interrupt line. */
#define CONFIG_TUNER SI4700
#define HAVE_RDS_CAP
#define CONFIG_RDS (RDS_CFG_POLL | RDS_CFG_PROCESS)
#define CONFIG_RDS_POLL_TICKS 4
/* inputs: the microphone (mono, on RINPUT2) and the tuner */
#define INPUT_SRC_CAPS (SRC_CAP_MIC | SRC_CAP_FMRADIO)

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS
#define MIN_BRIGHTNESS_SETTING      0
#define MAX_BRIGHTNESS_SETTING      31
#define DEFAULT_BRIGHTNESS_SETTING   20
#define CONFIG_BACKLIGHT_FADING BACKLIGHT_FADING_SW_HW_REG

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

/* TODO: Figure out real values */
#define BATTERY_CAPACITY_DEFAULT 400 /* default battery capacity */
#define BATTERY_CAPACITY_MIN     300 /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX     500 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC      10 /* capacity increment */

#define CONFIG_BATTERY_MEASURE VOLTAGE_MEASURE

/* Hardware controlled charging with monitoring */
#define CONFIG_CHARGING CHARGING_MONITOR

/* define this if the unit can be powered or charged via USB */
#define HAVE_USB_POWER

/* USB On-the-go */
#define CONFIG_USBOTG USBOTG_RK27XX

/* enable these for the experimental usb stack */
#define HAVE_USBSTACK

#define USB_VENDOR_ID 0x071b
#define USB_PRODUCT_ID 0x3202
#define HAVE_BOOTLOADER_USB_MODE

/* The exact type of CPU */
#define CONFIG_CPU RK27XX

/* I2C interface */
#define CONFIG_I2C I2C_RK27XX

/* Define this to the CPU frequency */
#define CPU_FREQ        200000000

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

/* Virtual LED (icon) */
#define CONFIG_LED LED_VIRTUAL

#define RKW_FORMAT
#define BOOTFILE_EXT "rkw"
#define BOOTFILE "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"
