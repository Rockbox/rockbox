/*
 * This config file is for the AIGO EROS Q / EROS K (and its clones)
 */

/* For Rolo and boot loader */
#define MODEL_NAME   "AIGO Eros Q Native"
#define MODEL_NUMBER 116
#define BOOTFILE_EXT "erosq"
#define BOOTFILE     "rockbox." BOOTFILE_EXT
#define BOOTDIR      "/.rockbox"
#define FIRMWARE_OFFSET_FILE_CRC  0
#define FIRMWARE_OFFSET_FILE_DATA 8

/* CPU defines */
#define CONFIG_CPU      X1000
#define X1000_EXCLK_FREQ   24000000
#define CPU_FREQ           1008000000
#define HAVE_FPU

#ifndef SIMULATOR
#define TIMER_FREQ         X1000_EXCLK_FREQ
#endif

/* kernel defines */
#define INCLUDE_TIMEOUT_API
#define HAVE_SEMAPHORE_OBJECTS

/* drivers */
#define HAVE_I2C_ASYNC

/* Buffers for plugsins and codecs */
#define PLUGIN_BUFFER_SIZE 0x200000 /* 2 MiB */
#define CODEC_SIZE         0x100000 /* 1 MiB */

/* LCD defines */
#define CONFIG_LCD   LCD_EROSQ
#define LCD_WIDTH  320
#define LCD_HEIGHT 240
#define LCD_DEPTH  16 /* Future Improvement: 18 or 24 bpp if display supports it */
#define LCD_PIXELFORMAT  RGB565
/* sqrt(240^2 + 320^2) / 2.0 = 200 */
#define LCD_DPI 200
#define HAVE_LCD_COLOR
#define HAVE_LCD_BITMAP
#define HAVE_LCD_ENABLE
#define HAVE_LCD_SHUTDOWN
#define LCD_X1000_FASTSLEEP
//#define LCD_X1000_DMA_WAITFORFRAME

#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS
#define MIN_BRIGHTNESS_SETTING      1
#define MAX_BRIGHTNESS_SETTING      255
#define BRIGHTNESS_STEP             5
#define DEFAULT_BRIGHTNESS_SETTING  70
#define CONFIG_BACKLIGHT_FADING BACKLIGHT_FADING_SW_SETTING

/* RTC settings */
#define CONFIG_RTC      RTC_X1000

/* Codec / audio hardware defines */
#define HW_SAMPR_CAPS SAMPR_CAP_ALL_192
#define HAVE_EROS_QN_CODEC
#define HAVE_SW_TONE_CONTROLS
#define HAVE_SW_VOLUME_CONTROL

/* use high-bitdepth volume scaling */
#define PCM_NATIVE_BITDEPTH 24

/* Button defines */
#define CONFIG_KEYPAD   EROSQ_PAD
#define HAVE_SCROLLWHEEL
#define HAVE_HEADPHONE_DETECTION
#define HAVE_LINEOUT_DETECTION

/* Storage defines */
#define CONFIG_STORAGE  STORAGE_SD
#define HAVE_HOTSWAP
#define HAVE_HOTSWAP_STORAGE_AS_MAIN
#define HAVE_MULTIDRIVE
#define HAVE_MULTIVOLUME
#define NUM_DRIVES 1
#define STORAGE_WANTS_ALIGN
#define STORAGE_NEEDS_BOUNCE_BUFFER

/* Power management */
#define CONFIG_BATTERY_MEASURE (VOLTAGE_MEASURE/*|CURRENT_MEASURE*/)
#define CONFIG_CHARGING        CHARGING_MONITOR
#define HAVE_SW_POWEROFF

#ifndef SIMULATOR
#define HAVE_AXP_PMU 192
#define HAVE_POWEROFF_WHILE_CHARGING
#endif

/* Battery */
#define BATTERY_TYPES_COUNT  1
#define BATTERY_CAPACITY_DEFAULT 1300 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 1300  /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 1300 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 0   /* capacity increment */

#define CURRENT_NORMAL 100      // 1.7mA * 60s
#define CURRENT_BACKLIGHT 180
#define CURRENT_MAX_CHG 500     // bursts higher if needed

/* Multiboot */
#define HAVE_BOOTDATA
#define BOOT_REDIR "rockbox_main.aigo_erosqn"

/* USB support */
#ifndef SIMULATOR
#define CONFIG_USBOTG USBOTG_DESIGNWARE
#define USB_DW_TURNAROUND 5
#define HAVE_USBSTACK
#define USB_VENDOR_ID 0xc502
#define USB_PRODUCT_ID 0x0023
#define USB_DEVBSS_ATTR __attribute__((aligned(32)))
#define HAVE_USB_POWER
#define HAVE_USB_CHARGING_ENABLE
#define HAVE_USB_CHARGING_IN_THREAD
#define TARGET_USB_CHARGING_DEFAULT USB_CHARGING_FORCE
#define HAVE_BOOTLOADER_USB_MODE
/* This appears to improve transfer performance (the default is 64 KiB).
 * Going any higher doesn't help but we're still slower than the OF. */
#define USB_READ_BUFFER_SIZE    (128 * 1024)
#define USB_WRITE_BUFFER_SIZE   (128 * 1024)
#endif

#ifdef BOOTLOADER
/* Ignore on any key can cause surprising USB issues in the bootloader */
# define USBPOWER_BTN_IGNORE (~(BUTTON_PREV|BUTTON_NEXT))
#endif

/* Rockbox capabilities */
#define HAVE_VOLUME_IN_LIST
#define HAVE_FAT16SUPPORT
#define HAVE_ALBUMART
#define HAVE_BMP_SCALING
#define HAVE_JPEG
#define HAVE_TAGCACHE
#define HAVE_QUICKSCREEN
#define HAVE_HOTKEY
#define HAVE_BOOTLOADER_SCREENDUMP
