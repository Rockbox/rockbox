/*
 * This config file is for the HiBy R3 Pro II based on the x1600E soc
 */

/* For Rolo and boot loader */
#define MODEL_NUMBER 123
#define MODEL_NAME   "HIBY R3 PRO II"

#define PIVOT_ROOT "/data/mnt/sd_0"
#define MULTIDRIVE_DIR "/data/mnt/usb"

/* LCD dimensions */
/* sqrt(width^2 + height^2) / 4 = 216 */
#define LCD_WIDTH 480
#define LCD_HEIGHT 720
#define LCD_DPI 216

#define HAVE_LCD_SLEEP
#define LCD_SLEEP_TIMEOUT (2*HZ)

#define LCD_DEPTH  16   /* 65536 colours */
#define LCD_PIXELFORMAT RGB565 /* rgb565 */

#define CPU_FREQ           1008000000

#ifndef SIMULATOR
#define HAVE_GENERAL_PURPOSE_LED
#endif

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN
#define HAVE_HOTKEY

#define HAVE_HEADPHONE_DETECTION
#define NO_BUTTON_LR

#ifndef BOOTLOADER
#define HAVE_BUTTON_DATA
#define HAVE_TOUCHSCREEN
#endif

#ifndef CONFIG_BACKLIGHT_FADING
#undef CONFIG_BACKLIGHT_FADING
#endif

/* KeyPad configuration for plugins */
#define CONFIG_KEYPAD HIBY_R3PROII_PAD

/* define this if the target has volume keys which can be used in the lists */
#define HAVE_VOLUME_IN_LIST

/* Battery doesn't update its charging status, but the charger does. */
#define POWER_DEV_NAME "mp2731-charger"

/* Battery */
#define BATTERY_TYPES_COUNT  1

/* Audio codec */
#define HAVE_HIBY_LINUX_CODEC

/* We don't have hardware controls */
#define HAVE_SW_TONE_CONTROLS

/* HW codec is flexible */
#define HW_SAMPR_CAPS SAMPR_CAP_ALL_192

/* Battery */
#define CONFIG_BATTERY_MEASURE (VOLTAGE_MEASURE|PERCENTAGE_MEASURE|TIME_MEASURE)

#define BATTERY_CAPACITY_DEFAULT 100 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 100  /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 100 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 0   /* capacity increment */

/* Special backlight paths */
#define BACKLIGHT_HIBY

#define MIN_BRIGHTNESS_SETTING      1
#define MAX_BRIGHTNESS_SETTING      100
#define BRIGHTNESS_STEP             5
#define DEFAULT_BRIGHTNESS_SETTING  100

/* ROLO */
#define BOOTFILE_EXT "r3proii"
#define BOOTFILE     "rockbox." BOOTFILE_EXT
#define BOOTDIR      "/.rockbox"

/* USB */
#define USB_VID_STR "32BB"
#define USB_PID_STR "0101"

/* Generic HiBy stuff */
#include "hibylinux.h"
