/*
 * This config file is for the Sandisk Sansa e200
 */

/* For Rolo and boot loader */
#define MODEL_NUMBER 16
#define MODEL_NAME   "Sandisk Sansa e200 series"

#define HW_SAMPR_CAPS       (SAMPR_CAP_48 | SAMPR_CAP_44 | SAMPR_CAP_32)

/* Define if boot data from bootloader has been enabled for the target */
#define HAVE_BOOTDATA

/* define boot redirect file name allows booting from external drives */
#define BOOT_REDIR "rockbox_main.e200"
#define MULTIBOOT_MIN_VOLUME 1

/* define this if you have recording possibility */
#define HAVE_RECORDING

#define REC_SAMPR_CAPS      (SAMPR_CAP_24 | SAMPR_CAP_22 | SAMPR_CAP_16)
#define REC_FREQ_DEFAULT    REC_FREQ_22 /* Default is not 44.1kHz */
#define REC_SAMPR_DEFAULT   SAMPR_22

/* because the samplerates don't match at each point, we must be able to
 * tell PCM which set of rates to use. not needed if recording rates are
 * a simple subset of playback rates and are equal values. */
#define CONFIG_SAMPR_TYPES

/* Define bitmask of input sources - recordable bitmask can be defined
   explicitly if different */
#define INPUT_SRC_CAPS (SRC_CAP_MIC | SRC_CAP_FMRADIO)




/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR

/* define this if you want album art for this target */
#define HAVE_ALBUMART

/* define this to enable bitmap scaling */
#define HAVE_BMP_SCALING

/* define this to enable JPEG decoding */
#define HAVE_JPEG

/* define this if you have a light associated with the buttons */
#define HAVE_BUTTON_LIGHT

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* LCD dimensions */
#define LCD_WIDTH  176
#define LCD_HEIGHT 220
/* sqrt(176^2 + 220^2) / 1.8 = 156.5 */
#define LCD_DPI 157
#define LCD_DEPTH  16   /* 65536 colours */
#define LCD_PIXELFORMAT RGB565 /* rgb565 */

#ifndef BOOTLOADER
/* define this if you have LCD enable function */
#define HAVE_LCD_ENABLE

/* Define this if your LCD can be put to sleep. HAVE_LCD_ENABLE
   should be defined as well. */
#define HAVE_LCD_SLEEP
#define HAVE_LCD_SLEEP_SETTING
#endif

/* define this if you can flip your LCD */
#define HAVE_LCD_FLIP

/* define this if you can invert the colours on your LCD */
#define HAVE_LCD_INVERT

/* #define IRAM_LCDFRAMEBUFFER IDATA_ATTR *//* put the lcd frame buffer in IRAM */

#define CONFIG_KEYPAD SANSA_E200_PAD

/* Define this to have CPU boosted while scrolling in the UI */
#define HAVE_GUI_BOOST

/* Define this to enable morse code input */
#define HAVE_MORSE_INPUT



/* There is no hardware tone control */
#define HAVE_SW_TONE_CONTROLS
/* The PP5024 has a built-in AustriaMicrosystems AS3514 */
#define HAVE_AS3514

/* define this if you have a real-time clock */
#ifndef BOOTLOADER
#define CONFIG_RTC RTC_AS3514
#endif

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* Some Sansa E200s seem to be FAT16 formatted */
#define HAVE_FAT16SUPPORT

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

#define AB_REPEAT_ENABLE

/* FM Tuner */
#define CONFIG_TUNER LV24020LP
#define HAVE_TUNER_PWR_CTRL

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS

/* Which backlight fading type? */
#define CONFIG_BACKLIGHT_FADING BACKLIGHT_FADING_SW_SETTING

/* define this if the unit uses a scrollwheel for navigation */
#define HAVE_SCROLLWHEEL
/* define to activate advanced wheel acceleration code */
#define HAVE_WHEEL_ACCELERATION
/* define from which rotation speed [degree/sec] on the acceleration starts */
#define WHEEL_ACCEL_START 540
/* define type of acceleration (1 = ^2, 2 = ^3, 3 = ^4) */
#define WHEEL_ACCELERATION 1

/* define this if you have a flash memory storage */
#define HAVE_FLASH_STORAGE

/* define this if the flash memory uses the SecureDigital Memory Card protocol */
#define CONFIG_STORAGE STORAGE_SD

#define BATTERY_CAPACITY_DEFAULT 750    /* default battery capacity */
#define BATTERY_CAPACITY_MIN 750        /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 750        /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 0          /* capacity increment */


#define CONFIG_BATTERY_MEASURE VOLTAGE_MEASURE

/* Charging implemented in a target-specific algorithm */
#define CONFIG_CHARGING CHARGING_TARGET
#define HAVE_POWEROFF_WHILE_CHARGING

/* define current usage levels */
#define CURRENT_NORMAL     30  /* Toni's measurements in Nov 2008  */
#define CURRENT_BACKLIGHT  40  /* Screen is about 20, blue LEDs are another 20, so 40 if both */
#define CURRENT_RECORD     30  /* flash player, so this is just unboosted current*/

/* define this if the unit can be powered or charged via USB */
#define HAVE_USB_POWER

/* Define this if you have a PortalPlayer PP5024 */
#define CONFIG_CPU PP5024

/* Define this if you want to use the PP5024 i2c interface */
#define CONFIG_I2C I2C_PP5024

/* define this if the hardware can be powered off while charging */
/* Sansa can't be powered off while charging */
/* #define HAVE_POWEROFF_WHILE_CHARGING */

/* The start address index for ROM builds */
#define ROM_START 0x00000000

/* Define this to the CPU frequency */
#define CPU_FREQ      75000000

/* Type of LCD TODO: hopefully the same as the x5 but check this*/
#define CONFIG_LCD LCD_X5

#define HAVE_MULTIDRIVE
#define NUM_DRIVES 2
#define HAVE_HOTSWAP /* required to access sd from bootloader */

/* USB On-the-go */
#define CONFIG_USBOTG USBOTG_ARC

/* enable these for the experimental usb stack */
#define HAVE_USBSTACK
#define USB_VENDOR_ID 0x0781
#define USB_PRODUCT_ID 0x7421
#define HAVE_USB_HID_MOUSE
#ifdef BOOTLOADER
#define HAVE_BOOTLOADER_USB_MODE
#endif

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

#define MI4_FORMAT
#define BOOTFILE_EXT    "mi4"
#define BOOTFILE        "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"

#define ICODE_ATTR_TREMOR_NOT_MDCT

#define INCLUDE_TIMEOUT_API

/** Port-specific settings **/

/* Main LCD backlight brightness range and defaults */
#define MIN_BRIGHTNESS_SETTING      1
#define MAX_BRIGHTNESS_SETTING     12
#define DEFAULT_BRIGHTNESS_SETTING  6

/* Default recording levels */
#define DEFAULT_REC_MIC_GAIN    23
#define DEFAULT_REC_LEFT_GAIN   23
#define DEFAULT_REC_RIGHT_GAIN  23

#ifdef E200R_INSTALLER
#define IRAMORIG 0x40004000
#endif

/* Define this if a programmable hotkey is mapped */
#define HAVE_HOTKEY
