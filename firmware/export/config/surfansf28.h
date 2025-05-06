/*
 * This config file is for the Surfans F28
 */

/* For Rolo and boot loader */
#define MODEL_NUMBER 120

#define MODEL_NAME   "Surfans F28"

/* LCD dimensions */
#define LCD_WIDTH  320
#define LCD_HEIGHT 480
/* sqrt(320^2 + 480^2) / 3.5 = 165 */
#define LCD_DPI 165

/* define this if you have access to the quickscreen */
//#define HAVE_QUICKSCREEN
//#define HAVE_HOTKEY

#define HAVE_HEADPHONE_DETECTION
#define HAVE_SCROLLWHEEL
#define NO_BUTTON_LR

#ifndef BOOTLOADER
#define HAVE_BUTTON_DATA
#define HAVE_TOUCHSCREEN
#endif

/* KeyPad configuration for plugins */
#define CONFIG_KEYPAD SURFANS_F28_PAD

/* define this if the target has volume keys which can be used in the lists */
#define HAVE_VOLUME_IN_LIST

#define CONFIG_BATTERY_MEASURE VOLTAGE_MEASURE

/* Define this to the CPU frequency */
#define CPU_FREQ           1008000000

/* Battery */
#define BATTERY_TYPES_COUNT  1

/* Audio codec */
#define HAVE_SURFANS_LINUX_CODEC

/* We don't have hardware controls */
#define HAVE_SW_TONE_CONTROLS

/* HW codec is flexible */
#define HW_SAMPR_CAPS SAMPR_CAP_ALL_192

/* Battery */
#define BATTERY_CAPACITY_DEFAULT 1500 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 1500  /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 1500 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 0   /* capacity increment */

#define CURRENT_NORMAL 100      // 1.7mA * 60s
#define CURRENT_BACKLIGHT 180
#define CURRENT_MAX_CHG 500     // bursts higher if needed

/* ROLO */
#define BOOTFILE_EXT "f28"
#define BOOTFILE     "rockbox." BOOTFILE_EXT
#define BOOTDIR      "/.rockbox"

/* USB */
#define USB_VID_STR "18D1"
#define USB_PID_STR "0D02"

/* Generic HiBy stuff */
#include "hibylinux.h"
