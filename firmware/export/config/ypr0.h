/*
 * This config file is for the RockBox as application on the Samsung YP-R0 player.
 * The target name for ifdefs is: SAMSUNG_YPR0; or CONFIG_PLATFORM & PLAFTORM_YPR0
 */

#define TARGET_TREE /* this target is using the target tree system */

/* We don't run on hardware directly */
/* YP-R0 need it too of course */
#define CONFIG_PLATFORM (PLATFORM_HOSTED)

/* For Rolo and boot loader */
#define MODEL_NUMBER 100

#define MODEL_NAME   "Samsung YP-R0"

/* Indeed to check that */
/*TODO: R0 should charge battery automatically, no software stuff to manage that. Just to know about some as3543 registers, that should be set after loading samsung's afe.ko module 
 */
/*TODO: implement USB data transfer management -> see safe mode script and think a way to implemtent it in the code */
#define USB_NONE

/* Hardware controlled charging with monitoring */
//#define CONFIG_CHARGING CHARGING_MONITOR

/* There is only USB charging */
//#define HAVE_USB_POWER

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR

/* define this if the LCD needs to be shutdown */
/* TODO: Our framebuffer must be closed... */
#define HAVE_LCD_SHUTDOWN

/* define this if you want album art for this target */
#define HAVE_ALBUMART

/* define this to enable bitmap scaling */
#define HAVE_BMP_SCALING

/* define this to enable JPEG decoding */
#define HAVE_JPEG

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* define this if you have access to the pitchscreen */
#define HAVE_PITCHSCREEN

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* LCD dimensions
 *
 * overriden by configure for application builds */
#ifndef LCD_WIDTH
#define LCD_WIDTH  240
#endif

#ifndef LCD_HEIGHT
#define LCD_HEIGHT 320
#endif

#define LCD_DEPTH  16
/* Check that but should not matter */
#define LCD_PIXELFORMAT 565

/* YP-R0 has the backlight */
#define HAVE_BACKLIGHT

/* Define this for LCD backlight brightness available */
#define HAVE_BACKLIGHT_BRIGHTNESS

/* Main LCD backlight brightness range and defaults */
/* 0 is turned off. 31 is the real maximum for the ASCODEC DCDC but samsung doesn't use any value over 15, so it safer to don't go up too much */
#define MIN_BRIGHTNESS_SETTING          1
#define MAX_BRIGHTNESS_SETTING          15
#define DEFAULT_BRIGHTNESS_SETTING      4

/* Which backlight fading type? */
/* TODO: ASCODEC has an auto dim feature, so disabling the supply to leds should do the trick. But for now I tested SW fading only */
#define CONFIG_BACKLIGHT_FADING BACKLIGHT_FADING_SW_SETTING

/* define this if you have RTC RAM available for settings */
/* TODO: in theory we could use that, ascodec offers us such a ram. we have also a small device, part of the nand of 1 MB size, that Samsung uses to store region code etc and it's almost unused space */
//#define HAVE_RTC_RAM

/* define this if you have a real-time clock */
//#define CONFIG_RTC APPLICATION
#define CONFIG_RTC RTC_AS3514
#define HAVE_RTC_ALARM

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x80000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x100000

/* We can do AB-repeat -> we use User key, our hotkey */
#define AB_REPEAT_ENABLE
#define ACTION_WPSAB_SINGLE ACTION_WPS_HOTKEY

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* R0 KeyPad configuration for plugins */
#define CONFIG_KEYPAD SAMSUNG_YPR0_PAD
/* It's better to close /dev/r0Btn at shutdown */
#define BUTTON_DRIVER_CLOSE

/* The YPR0 has a as3534 codec and we use that to control the volume */
#define HAVE_AS3514
#define HAVE_AS3543

#define HAVE_SW_TONE_CONTROLS

/* TODO: Make use of the si4703 tuner hardware */
/* #define CONFIG_TUNER SI4700 */
/* #define HAVE_TUNER_PWR_CTRL*/

/*TODO: In R0 there is an interrupt for this (figure out ioctls)*/
/* #define HAVE_HEADPHONE_DETECTION */

/* Define current usage levels. */
/* TODO: to be filled with correct values after implementing power management */
#define CURRENT_NORMAL     88 /* 18 hours from a 1600 mAh battery */
#define CURRENT_BACKLIGHT  30 /* TBD */
#define CURRENT_RECORD     0  /* no recording yet */

/* TODO: We need to do battery handling */
//#define BATTERY_CAPACITY_DEFAULT 600 /* default battery capacity */
//#define BATTERY_CAPACITY_MIN 600  /* min. capacity selectable */
//#define BATTERY_CAPACITY_MAX 700 /* max. capacity selectable */
//#define BATTERY_CAPACITY_INC 50   /* capacity increment */
//#define BATTERY_TYPES_COUNT  1    /* only one type */

/* TODO: We possibly can only watch linux charging */
//#define CONFIG_CHARGING CHARGING_TARGET
//#define HAVE_RESET_BATTERY_FILTER

/* same dimensions as gigabeats */
#define CONFIG_LCD LCD_YPR0

/* Define this if a programmable hotkey is mapped */
#define HAVE_HOTKEY

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* Define this if you have adjustable CPU frequency
 * NOTE: We could do that on this device, but it's probably better
 * to let linux do it (we set ondemand governor before loading Rockbox) */
/* #define HAVE_ADJUSTABLE_CPU_FREQ */
/* Define this to the CPU frequency */
#define CPU_FREQ            532000000
/* 0.8Vcore using 200 MHz */
/* #define CPUFREQ_DEFAULT     200000000 */
/* This is 400 MHz -> not so powersaving-ful */
/* #define CPUFREQ_NORMAL      400000000 */
/* Max IMX37 Cpu Frequency */
/* #define CPUFREQ_MAX         CPU_FREQ */

/* TODO: my idea is to create a folder in the cramfs [/.rockbox], mounting it by the starter script as the current working directory, so no issues of any type keeping the rockbox folder as in all other players */
#define BOOTDIR "/.rockbox"
