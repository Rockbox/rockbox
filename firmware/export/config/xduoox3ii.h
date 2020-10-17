/*
 * This config file is for the xDuoo X3ii
 */

/* For Rolo and boot loader */
#define MODEL_NUMBER 110

#define MODEL_NAME   "xDuoo X3ii"

/* LCD dimensions */
#define LCD_WIDTH  240
#define LCD_HEIGHT 320
/* sqrt(240^2 + 320^2) / 2.4 = 166 */
#define LCD_DPI 166


/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

#define HAVE_HEADPHONE_DETECTION
#define HAVE_LINEOUT_DETECTION

/* KeyPad configuration for plugins */
#define CONFIG_KEYPAD XDUOO_X3II_PAD

/* define this if the target has volume keys which can be used in the lists */
#define HAVE_VOLUME_IN_LIST

#define CONFIG_BATTERY_MEASURE VOLTAGE_MEASURE

/* Define this to the CPU frequency */
#define CPU_FREQ           1008000000

/* Battery */
#define BATTERY_TYPES_COUNT  1

/* Audio codec */
#define HAVE_XDUOO_LINUX_CODEC

/* We don't have hardware controls */
#define HAVE_SW_TONE_CONTROLS

/* HW codec is flexible */
#define HW_SAMPR_CAPS SAMPR_CAP_ALL_192

/* Battery */
#define BATTERY_CAPACITY_DEFAULT 2000 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 2000  /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 2000 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 0   /* capacity increment */

#define CURRENT_NORMAL 167      // 2.78mA * 60s
#define CURRENT_BACKLIGHT 180
#define CURRENT_MAX_CHG 500     // bursts higher if needed

/* ROLO */
#define BOOTFILE_EXT "x3ii"
#define BOOTFILE     "rockbox." BOOTFILE_EXT
#define BOOTDIR      "/.rockbox"

/* USB */
#define USB_VID_STR "C502"
#define USB_PID_STR "003C"

/* Generic HiBy stuff */
#include "hibylinux.h"
