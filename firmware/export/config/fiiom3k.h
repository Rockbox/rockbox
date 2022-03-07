/* RoLo-related defines */
#define MODEL_NAME      "FiiO M3K"
#define MODEL_NUMBER    114
#define BOOTFILE_EXT    "m3k"
#define BOOTFILE        "rockbox." BOOTFILE_EXT
#define BOOTDIR         "/.rockbox"
#define FIRMWARE_OFFSET_FILE_CRC    0
#define FIRMWARE_OFFSET_FILE_DATA   8

/* CPU defines */
#define CONFIG_CPU          X1000
#define X1000_EXCLK_FREQ    24000000
#define CPU_FREQ            1008000000

#ifndef SIMULATOR
#define TIMER_FREQ          X1000_EXCLK_FREQ
#endif

/* Kernel defines */
#define INCLUDE_TIMEOUT_API
#define HAVE_SEMAPHORE_OBJECTS

/* Drivers */
#define HAVE_I2C_ASYNC
#define HAVE_FT6x06
#define FT6x06_SWAP_AXES
#define FT6x06_NUM_POINTS 1

/* Buffer for plugins and codecs. */
#define PLUGIN_BUFFER_SIZE  0x200000 /* 2 MiB */
#define CODEC_SIZE          0x100000 /* 1 MiB */

/* LCD defines */
#define CONFIG_LCD          LCD_FIIOM3K
#define LCD_WIDTH           240
#define LCD_HEIGHT          320
#define LCD_DEPTH           16
#define LCD_PIXELFORMAT     RGB565
#define LCD_DPI             200
#define HAVE_LCD_COLOR
#define HAVE_LCD_BITMAP
#define HAVE_LCD_ENABLE
#define LCD_X1000_FASTSLEEP

/* Backlight defines */
#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS
#define HAVE_BUTTON_LIGHT
#define HAVE_BUTTONLIGHT_BRIGHTNESS
#define MIN_BRIGHTNESS_SETTING      1
#define MAX_BRIGHTNESS_SETTING      100
#define BRIGHTNESS_STEP             5
#define DEFAULT_BRIGHTNESS_SETTING  70
#define CONFIG_BACKLIGHT_FADING BACKLIGHT_FADING_SW_SETTING

/* Codec / audio hardware defines */
#define HW_SAMPR_CAPS   SAMPR_CAP_ALL_192
#define REC_SAMPR_CAPS  (SAMPR_CAP_ALL_96 & ~SAMPR_CAP_64)
#define INPUT_SRC_CAPS  SRC_CAP_MIC
#define AUDIOHW_CAPS    (FILTER_ROLL_OFF_CAP|POWER_MODE_CAP|MIC_GAIN_CAP)
#define HAVE_RECORDING
#define HAVE_RECORDING_WITHOUT_MONITORING
#define HAVE_AK4376
#define HAVE_X1000_ICODEC_REC
#define HAVE_SW_TONE_CONTROLS
#define HAVE_SW_VOLUME_CONTROL
#define DEFAULT_REC_MIC_GAIN  12

/* Button defines */
#define CONFIG_KEYPAD   FIIO_M3K_PAD
#define HAVE_HEADPHONE_DETECTION
#define HAVE_TOUCHPAD
#define HAVE_TOUCHPAD_SENSITIVITY_SETTING
#define MIN_TOUCHPAD_SENSITIVITY_SETTING        (-25)
#define MAX_TOUCHPAD_SENSITIVITY_SETTING        (25)
#define DEFAULT_TOUCHPAD_SENSITIVITY_SETTING    (0)
#define HAVE_SCROLLWHEEL
/* #define HAVE_WHEEL_ACCELERATION */
/* #define WHEEL_ACCELERATION */
/* #define WHEEL_ACCEL_START */

/* Storage defines */
#define CONFIG_STORAGE  STORAGE_SD
#define HAVE_HOTSWAP
#define HAVE_HOTSWAP_STORAGE_AS_MAIN
#define HAVE_MULTIDRIVE
#define HAVE_MULTIVOLUME
#define NUM_DRIVES 1
#define STORAGE_WANTS_ALIGN
#define STORAGE_NEEDS_BOUNCE_BUFFER

/* RTC settings */
#define CONFIG_RTC      RTC_X1000
/* TODO: implement HAVE_RTC_ALARM */

/* Power management */
#define CONFIG_BATTERY_MEASURE (VOLTAGE_MEASURE|CURRENT_MEASURE)
#define CONFIG_CHARGING        CHARGING_MONITOR
#define HAVE_SW_POWEROFF

#ifndef SIMULATOR
#define HAVE_AXP_PMU 192
#define HAVE_POWEROFF_WHILE_CHARGING
#endif

/* Only one battery type */
#define BATTERY_CAPACITY_DEFAULT 1100
#define BATTERY_CAPACITY_MIN     1100
#define BATTERY_CAPACITY_MAX     1100
#define BATTERY_CAPACITY_INC     0
#define BATTERY_TYPES_COUNT      1

/* Multiboot */
#define HAVE_BOOTDATA
#define BOOT_REDIR "rockbox_main.fiio_m3k"

/* USB support */
#ifndef SIMULATOR
#define CONFIG_USBOTG USBOTG_DESIGNWARE
#define USB_DW_TURNAROUND 5
#define HAVE_USBSTACK
#define USB_VENDOR_ID 0x2972
#define USB_PRODUCT_ID 0x0003
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
# define USBPOWER_BTN_IGNORE (~(BUTTON_VOL_UP|BUTTON_VOL_DOWN))
#endif

/* Rockbox capabilities */
#define HAVE_FAT16SUPPORT
#define HAVE_ALBUMART
#define HAVE_BMP_SCALING
#define HAVE_JPEG
#define HAVE_TAGCACHE
#define HAVE_VOLUME_IN_LIST
#define HAVE_QUICKSCREEN
#define HAVE_HOTKEY
#define HAVE_LOCKED_ACTIONS
#define AB_REPEAT_ENABLE
#define HAVE_BOOTLOADER_SCREENDUMP
