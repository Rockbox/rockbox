/*
 * This config file is for the Agptek Rocker (native port)
 */

/* For Rolo and boot loader */
#define MODEL_NAME   "Agptek Rocker Native"
#define MODEL_NUMBER 133
#define BOOTFILE_EXT "rockern"
#define BOOTFILE     "rockbox." BOOTFILE_EXT
#define BOOTDIR      "/.rockbox"

/* CPU defines */
#define CONFIG_CPU      X1000
/* TODO: verify, depends on the BOOT_SEL2 strap (PB30) */
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

/* Buffers for plugins and codecs */
#define PLUGIN_BUFFER_SIZE 0x200000 /* 2 MiB */
#define CODEC_SIZE         0x100000 /* 1 MiB */

/* LCD defines */
#define CONFIG_LCD   LCD_AGPTEK_ROCKER
#define LCD_WIDTH  128
#define LCD_HEIGHT 160
#define LCD_DEPTH  16
#define LCD_PIXELFORMAT  RGB565
/* sqrt(128^2 + 160^2) / 2 = 102. */
#define LCD_DPI 102
#define HAVE_LCD_COLOR
#define HAVE_LCD_BITMAP
#define HAVE_LCD_ENABLE
#define HAVE_LCD_SHUTDOWN

#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS
#define MIN_BRIGHTNESS_SETTING      1
#define MAX_BRIGHTNESS_SETTING      255
#define BRIGHTNESS_STEP             5
#define DEFAULT_BRIGHTNESS_SETTING  120
#define CONFIG_BACKLIGHT_FADING BACKLIGHT_FADING_SW_SETTING

/* RTC settings */
#define CONFIG_RTC      RTC_X1000

/* Codec / audio hardware defines */
#define HW_SAMPR_CAPS SAMPR_CAP_ALL_96
#define HAVE_CS42L51
#define HAVE_SW_TONE_CONTROLS

/* Button defines */
#define CONFIG_KEYPAD   AGPTEK_ROCKER_PAD
#define HAVE_HEADPHONE_DETECTION

/* Storage defines */
#define CONFIG_STORAGE  STORAGE_SD
#define HAVE_HOTSWAP
#define HAVE_HOTSWAP_STORAGE_AS_MAIN
#define HAVE_MULTIVOLUME
#define STORAGE_WANTS_ALIGN
#define STORAGE_NEEDS_BOUNCE_BUFFER

/* One SD card slot */
#define SDMMC_HOST_NUM_SD_CONTROLLERS 1

/* Power management */
#define CONFIG_BATTERY_MEASURE (VOLTAGE_MEASURE | CURRENT_MEASURE)
#define CONFIG_CHARGING        CHARGING_MONITOR
#define HAVE_SW_POWEROFF

#ifndef SIMULATOR
#define HAVE_AXP_PMU 192
#define HAVE_POWEROFF_WHILE_CHARGING
#endif

/* Battery */
#define BATTERY_CAPACITY_DEFAULT 600 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 600  /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 600 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 0   /* capacity increment */

/* Multiboot */
#define HAVE_BOOTDATA
#define BOOT_REDIR "rockbox_main.agptek_rockern"

/* DeviceData */
#define HAVE_DEVICEDATA

/* USB support */
#ifndef SIMULATOR
#define CONFIG_USBOTG USBOTG_DESIGNWARE
#define USB_DW_TURNAROUND 5
#define HAVE_USBSTACK
#define USB_VENDOR_ID 0xc502
#define USB_PRODUCT_ID 0x0029
#define USB_DEVBSS_ATTR __attribute__((aligned(32)))
#define HAVE_USB_POWER
#define HAVE_USB_CHARGING_ENABLE
#define HAVE_USB_CHARGING_IN_THREAD
#define TARGET_USB_CHARGING_DEFAULT USB_CHARGING_FORCE
#define HAVE_BOOTLOADER_USB_MODE
#define USB_READ_BUFFER_SIZE    (128 * 1024)
#define USB_WRITE_BUFFER_SIZE   (128 * 1024)
/* No DRVVBUS pin on this board */
#define X1000_USB_NO_DRVVBUS
#endif

#ifdef BOOTLOADER
/* Ignore on any key can cause surprising USB issues in the bootloader */
# define USBPOWER_BTN_IGNORE (~(BUTTON_UP|BUTTON_DOWN))
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
