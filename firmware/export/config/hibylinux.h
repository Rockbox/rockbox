/* This file is for Hiby-based Linux targets */

/* For Great Justice! */
#define HIBY_LINUX

#ifndef SIMULATOR
#define CONFIG_PLATFORM (PLATFORM_HOSTED)
#define PIVOT_ROOT "/mnt/sd_0"
#endif

#define HAVE_FPU

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR

/* Define this if the LCD can be toggled */
#define HAVE_LCD_ENABLE

/* Define this if the LCD can shut down */
#define HAVE_LCD_SHUTDOWN

#ifdef HAVE_LCD_COLOR
/* define this if you want album art for this target */
#define HAVE_ALBUMART

/* define this to enable bitmap scaling */
#define HAVE_BMP_SCALING

/* define this to enable JPEG decoding */
#define HAVE_JPEG
#endif /* HAVE_LCD_COLOR */

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x100000

#define CONFIG_LCD LCD_INGENIC_LINUX

#define LCD_DEPTH  32
/* Check that but should not matter */
#define LCD_PIXELFORMAT XRGB8888

#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS

/* Main LCD backlight brightness range and defaults: the backlight driver
 * has levels from 0 to 255. But 0 is off so start at 1.
 */
#define MIN_BRIGHTNESS_SETTING      1
#define MAX_BRIGHTNESS_SETTING      255
#define BRIGHTNESS_STEP             5
#define DEFAULT_BRIGHTNESS_SETTING  70

/* Which backlight fading type? */
#define CONFIG_BACKLIGHT_FADING BACKLIGHT_FADING_SW_SETTING

/* define this if you have a real-time clock */
#define CONFIG_RTC APPLICATION

#ifndef SIMULATOR
/* We have usb power and can detect usb but it is handled by Linux */
#define HAVE_USB_POWER
#endif

/* Linux controlls charging, we can monitor */
#define CONFIG_CHARGING CHARGING_MONITOR

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* Supports attaching to external USB drive */
#define CONFIG_STORAGE (STORAGE_HOSTFS|STORAGE_USB)
#define HOSTFS_VOL_DEC "microSD"
#define HAVE_STORAGE_FLUSH
#define HAVE_MULTIDRIVE  /* But _not_ CONFIG_STORAGE_MULTI */
#define NUM_DRIVES 2
#define HAVE_HOTSWAP
#define HAVE_HOTSWAP_STORAGE_AS_MAIN
#define MULTIDRIVE_DIR "/mnt/usb"
#define MULTIDRIVE_DEV "/sys/block/sda"
#define ROOTDRIVE_DEV "/sys/block/mmcblk0"

/* More common stuff */
#ifndef EROS_Q
#define BATTERY_DEV_NAME "battery"
#endif
#define POWER_DEV_NAME "usb"
