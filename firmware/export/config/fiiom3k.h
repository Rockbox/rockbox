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
#define TIMER_FREQ          X1000_EXCLK_FREQ
#define CPU_FREQ            1000000000
#define CPUFREQ_MAX         CPU_FREQ
#define CPUFREQ_DEFAULT     (CPUFREQ_MAX/5)
#define CPUFREQ_NORMAL      (CPUFREQ_MAX/5)
#define HAVE_ADJUSTABLE_CPU_FREQ

/* Kernel defines */
#define INCLUDE_TIMEOUT_API
#define HAVE_SEMAPHORE_OBJECTS

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
#define HAVE_LCD_SLEEP
#define HAVE_LCD_SLEEP_SETTING

/* Backlight defines */
#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS
#define HAVE_BUTTON_LIGHT
#define HAVE_BUTTONLIGHT_BRIGHTNESS
#define MIN_BRIGHTNESS_SETTING      1
#define MAX_BRIGHTNESS_SETTING      100
#define BRIGHTNESS_STEP             5
#define DEFAULT_BRIGHTNESS_SETTING  70

/* Codec / audio hardware defines */
#define CONFIG_CODEC    SWCODEC
#define HW_SAMPR_CAPS   SAMPR_CAP_ALL_192
#define HAVE_FIIOM3K_CODEC
#define HAVE_AK4376
#define HAVE_SW_TONE_CONTROLS

/* Button defines */
#define CONFIG_KEYPAD   FIIO_M3K_PAD
#define HAVE_TOUCHPAD
#define HAVE_VOLUME_IN_LIST

/* Storage defines */
#define CONFIG_STORAGE  STORAGE_SD
/* DMA requires cache aligned buffers */
#define STORAGE_WANTS_ALIGN

/* RTC settings */
#define CONFIG_RTC      RTC_X1000
#define HAVE_RTC_ALARM

/* Power management */
#define HAVE_SW_POWEROFF
/* #define HAVE_POWEROFF_WHILE_CHARGING */

/* USB is still TODO. */
#define USB_NONE
