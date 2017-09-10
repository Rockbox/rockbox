/*
 * This config file is for the Sony NWZ-A860 series
 */

/* For Rolo and boot loader */
#define MODEL_NUMBER 107

#define MODEL_NAME   "Sony NWZ-A860 Series"

/* LCD dimensions */
#define LCD_WIDTH  240
#define LCD_HEIGHT 400
/* sqrt(240^2 + 400^2) / 2.8 = 166 */
#define LCD_DPI 166

/* this device has a touchscreen */
#define HAVE_TOUCHSCREEN
#define HAVE_BUTTON_DATA

#include "sonynwzlinux.h"

/* override keypad */
#undef CONFIG_KEYPAD
#define CONFIG_KEYPAD SONY_NWZA860_PAD
