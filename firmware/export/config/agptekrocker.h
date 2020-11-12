/*
 * This config file is for the Agptek Rocket
 */

/* For Rolo and boot loader */
#define MODEL_NUMBER 105

#define MODEL_NAME   "Agptek Rocker"

/* LCD dimensions */
#define LCD_WIDTH  128
#define LCD_HEIGHT 160
/* sqrt(128^2 + 160^2) / 2 = 102. */
#define LCD_DPI 102

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* define this if the target has volume keys which can be used in the lists */
#define HAVE_VOLUME_IN_LIST

/* Define if the device can wake from an RTC alarm */
//#define HAVE_RTC_ALARM

#define HAVE_HEADPHONE_DETECTION

/* KeyPad configuration for plugins */
#define CONFIG_KEYPAD AGPTEK_ROCKER_PAD

#define CONFIG_BATTERY_MEASURE VOLTAGE_MEASURE

/* Define this to the CPU frequency */
#define CPU_FREQ            1008000000

/* Battery */
#define BATTERY_TYPES_COUNT  1

/* Audio codec */
#define HAVE_ROCKER_CODEC

/* We don't have hardware controls */
#define HAVE_SW_TONE_CONTROLS

/* HW codec is flexible */
#define HW_SAMPR_CAPS SAMPR_CAP_ALL_192

/* Battery */
#define BATTERY_CAPACITY_DEFAULT 600 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 600  /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 600 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 0   /* capacity increment */

/* ROLO */
#define BOOTFILE_EXT "rocker"
#define BOOTFILE     "rockbox." BOOTFILE_EXT
#define BOOTDIR      "/.rockbox"

/* USB */
#define USB_VID_STR "C502"
#define USB_PID_STR "0029"

/* Generic HiBy stuff */
#include "hibylinux.h"

/* Define this if a programmable hotkey is mapped */
#define HAVE_HOTKEY
