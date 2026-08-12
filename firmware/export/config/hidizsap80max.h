/*
 * This config file is for the Hidizs AP80 Pro Max based on the x1600E soc
 */

/* For Rolo and boot loader */
#define MODEL_NUMBER 125
#define MODEL_NAME   "HIDIZS AP80 PRO MAX"

#define PIVOT_ROOT "/data/mnt/sd_0"
#define MULTIDRIVE_DIR "/data/mnt/usb"

/* LCD dimensions. The framebuffer is not the panel: /dev/fb0 is 480x640 but
   only the center 360 columns are scanned out - see hiby/lcd-target.h.
   sqrt(360^2 + 640^2) / 2.95" = 249 */
#define LCD_WIDTH 360
#define LCD_HEIGHT 640
#define LCD_DPI 249

#define HAVE_LCD_SLEEP
#define LCD_SLEEP_TIMEOUT (2*HZ)

#define LCD_DEPTH  16   /* 65536 colours */
#define LCD_PIXELFORMAT RGB565 /* rgb565 */

/* /dev/fb0 is 480 columns wide but only the center 360 display */
#define FB_STRIDE_MISMATCH   480
#define FB_STRIDE_XOFFSET ((FB_STRIDE_MISMATCH - LCD_WIDTH) / 2)

#define CPU_FREQ           1008000000

#ifndef SIMULATOR
#define HAVE_GENERAL_PURPOSE_LED
#endif

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN
#define HAVE_HOTKEY

#define HAVE_HEADPHONE_DETECTION

#ifndef BOOTLOADER
#define HAVE_BUTTON_DATA
#define HAVE_TOUCHSCREEN
#endif

/* Identity, as the digitiser already reports in panel coordinates. EVIOCGABS
   declares twice that range but the hardware does not use it */
#define DEFAULT_TOUCHSCREEN_CALIBRATION { .A=1, .B=0, .C=0, \
                                          .D=0, .E=1, .F=0, \
                                          .divider=1 }

#define HAVE_SCROLLWHEEL

#ifndef CONFIG_BACKLIGHT_FADING
#undef CONFIG_BACKLIGHT_FADING
#endif

/* KeyPad configuration for plugins */
#define CONFIG_KEYPAD HIDIZS_AP80MAX_PAD

/* There are no volume buttons, so HAVE_VOLUME_IN_LIST is not defined - the
   scroll wheel handles volume instead */

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
#define BOOTFILE_EXT "ap80max"
#define BOOTFILE     "rockbox." BOOTFILE_EXT
#define BOOTDIR      "/.rockbox"

/* USB */
#define HAVE_USB_ADB
#define HAVE_HOST_USB_AUDIO
#define HAVE_USB_POWER
#define USB_VID_STR "C502"
#define USB_PID_STR "008C"

/* Generic HiBy stuff */
#include "hibylinux.h"
