/* RoLo-related defines */
#define MODEL_NAME      "Echo R1"
#define MODEL_NUMBER    119
#define BOOTFILE_EXT    "echo"
#define BOOTFILE        "rockbox." BOOTFILE_EXT
#define BOOTDIR         "/.rockbox"

/* CPU defines */
#define CONFIG_CPU          STM32H743
#define STM32_LSE_FREQ      32768
#define STM32_HSE_FREQ      24000000
#define CPU_FREQ            480000000

#ifndef SIMULATOR
#define TIMER_FREQ          STM32_HSE_FREQ
#endif

/* Kernel defines */
#define INCLUDE_TIMEOUT_API
#define HAVE_SEMAPHORE_OBJECTS

/* Buffer for plugins and codecs. */
#define PLUGIN_BUFFER_SIZE  0x200000 /* 2 MiB */
#define CODEC_SIZE          0x100000 /* 1 MiB */

/* LCD defines */
#define CONFIG_LCD          LCD_ECHO_R1
#define LCD_WIDTH           320
#define LCD_HEIGHT          240
#define LCD_DEPTH           16
#define LCD_PIXELFORMAT     RGB565
#define LCD_DPI             240 // FIXME: review this.
#define HAVE_LCD_COLOR
#define HAVE_LCD_BITMAP
#define HAVE_LCD_ENABLE

/* Backlight defines */
#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS
#define MIN_BRIGHTNESS_SETTING      1
#define MAX_BRIGHTNESS_SETTING      100
#define BRIGHTNESS_STEP             5
#define DEFAULT_BRIGHTNESS_SETTING  70
#define CONFIG_BACKLIGHT_FADING     BACKLIGHT_FADING_SW_SETTING

/* Codec / audio hardware defines */
#define HW_SAMPR_CAPS   SAMPR_CAP_ALL_96 // FIXME: check this section
#define REC_SAMPR_CAPS  SAMPR_CAP_ALL_96
#define INPUT_SRC_CAPS  SRC_CAP_MIC
#define AUDIOHW_CAPS    MIC_GAIN_CAP
#define HAVE_RECORDING
#define HAVE_TLV320AIC3104 // TODO: Sansa connect uses the AIC3106, possible code sharing?
#define HAVE_SW_TONE_CONTROLS
#define HAVE_SW_VOLUME_CONTROL
#define DEFAULT_REC_MIC_GAIN  12

/* Button defines */
#define CONFIG_KEYPAD ECHO_R1_PAD
#define HAVE_HEADPHONE_DETECTION
#define HAVE_LINEOUT_DETECTION

/* Storage defines */
#define CONFIG_STORAGE  STORAGE_SD
#define HAVE_HOTSWAP
#define HAVE_HOTSWAP_STORAGE_AS_MAIN
#define HAVE_MULTIVOLUME
#define STORAGE_WANTS_ALIGN
#define STORAGE_NEEDS_BOUNCE_BUFFER

/* RTC settings */
#define CONFIG_RTC      RTC_STM32H743
#define HAVE_RTC_ALARM

/* Power management */
#define CONFIG_BATTERY_MEASURE VOLTAGE_MEASURE
#define CONFIG_CHARGING        CHARGING_MONITOR
#define HAVE_SW_POWEROFF

/* Only one battery type */
#define BATTERY_CAPACITY_DEFAULT 1100
#define BATTERY_CAPACITY_MIN     1100
#define BATTERY_CAPACITY_MAX     1100
#define BATTERY_CAPACITY_INC     0

/* Multiboot */
#define HAVE_BOOTDATA
#define BOOT_REDIR "rockbox_main.echor1"

/* USB support */
#ifndef SIMULATOR
#define CONFIG_USBOTG USBOTG_DESIGNWARE
#define USB_DW_TURNAROUND 5
#define HAVE_USBSTACK
#define USB_VENDOR_ID 0x1
#define USB_PRODUCT_ID 0x2
#define USB_DEVBSS_ATTR __attribute__((aligned(32)))
#define HAVE_USB_POWER
//#define HAVE_USB_CHARGING_ENABLE
//#define HAVE_USB_CHARGING_IN_THREAD
//#define TARGET_USB_CHARGING_DEFAULT USB_CHARGING_FORCE
#define HAVE_BOOTLOADER_USB_MODE
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
#define AB_REPEAT_ENABLE
#define HAVE_BOOTLOADER_SCREENDUMP
