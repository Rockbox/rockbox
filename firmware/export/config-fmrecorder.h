/* define this if you have recording possibility */
#define HAVE_RECORDING 1

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP 1

/* LCD dimensions */
#define LCD_WIDTH  112
#define LCD_HEIGHT 64
#define LCD_DEPTH  1

/* define this if you have a Recorder style 10-key keyboard */
#define CONFIG_KEYPAD RECORDER_PAD

/* define this if you have a real-time clock */
#define HAVE_RTC 1

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x8000

/* Define this if you have an FM Radio */
#define CONFIG_TUNER S1A0903X01

#define AB_REPEAT_ENABLE 1

#ifndef SIMULATOR

/* Define this if you have a MAS3587F */
#define CONFIG_CODEC MAS3587F

/* Define this if you have a SH7034 */
#define CONFIG_CPU SH7034

/* Define this if you have a FM Recorder key system */
#define HAVE_FMADC 1

/* Define this if you need to power on ATA */
#define NEEDS_ATA_POWER_ON

/* Define this if battery voltage can only be measured with ATA powered */
#define NEED_ATA_POWER_BATT_MEASURE

/* Define this to the CPU frequency */
#define CPU_FREQ      11059200

/* Type of mobile power */
#define CONFIG_BATTERY BATT_LIION2200

/* Battery scale factor (guessed, seems to be 1,25 * value from recorder) */
#define BATTERY_SCALE_FACTOR 8081

/* Define this if you control power on PB5 (instead of the OFF button) */
#define HAVE_POWEROFF_ON_PB5

/* Offset ( in the firmware file's header ) to the file length */
#define FIRMWARE_OFFSET_FILE_LENGTH 20

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 6

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 24

/* FM recorders can wake up from RTC alarm */
#define HAVE_ALARM_MOD 1

/* How to detect USB */
#define USB_FMRECORDERSTYLE 1

/* Define this if the platform can charge batteries */
#define HAVE_CHARGING 1

/* The start address index for ROM builds */
/* #define ROM_START 0x14010 for behind original Archos */
#define ROM_START 0x7010 /* for behind BootBox */

/* Software controlled LED */
#define CONFIG_LED LED_REAL

/* Define this for LCD backlight available */
#define CONFIG_BACKLIGHT BL_RTC /* on I2C controlled RTC port */

/* define this if the unit can be powered or charged via USB */
#define HAVE_USB_POWER

/* Define this for S/PDIF input available */
#define HAVE_SPDIF_IN

#define CONFIG_LCD LCD_SSD1815

#define BOOTFILE_EXT "ajz"
#define BOOTFILE "ajbrec." BOOTFILE_EXT

#endif /* SIMULATOR */
