/* define this if you have recording possibility */
#define HAVE_RECORDING 1

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP 1

/* LCD dimensions */
#define LCD_WIDTH  112
#define LCD_HEIGHT 64
#define LCD_DEPTH  1

/* define this if you have the Recorder's 10-key keyboard */
#define CONFIG_KEYPAD RECORDER_PAD

/* define this if you have a real-time clock */
#define HAVE_RTC 1

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x8000

#define AB_REPEAT_ENABLE 1

#ifndef SIMULATOR

/* Define this if you have a MAS3587F */
#define CONFIG_CODEC MAS3587F

/* Define this if you have a SH7034 */
#define CONFIG_CPU SH7034

/* Define this if you have charging control */
#define HAVE_CHARGE_CTRL

/* Define this if you have ATA power-off control */
#define HAVE_ATA_POWER_OFF

/* Define this if you need to power on ATA */
#define NEEDS_ATA_POWER_ON

/* Define this to the CPU frequency */
#define CPU_FREQ      11059200

/* Type of mobile power */
#define CONFIG_BATTERY BATT_4AA_NIMH

/* Battery scale factor (?) */
#define BATTERY_SCALE_FACTOR 6465

/* Define this if you control power on PBDR (instead of PADR) */
#define HAVE_POWEROFF_ON_PBDR

/* Offset ( in the firmware file's header ) to the file length */
#define FIRMWARE_OFFSET_FILE_LENGTH 0

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 4

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 6

/* How to detect USB */
#define USB_RECORDERSTYLE 1

/* Define this if the platform can charge batteries */
#define HAVE_CHARGING 1

/* The start address index for ROM builds */
/* #define ROM_START 0x11010 for behind original Archos */
#define ROM_START 0x7010 /* for behind BootBox */

/* Software controlled LED */
#define CONFIG_LED LED_REAL

/* Define this for LCD backlight available */
#define CONFIG_BACKLIGHT BL_RTC /* on I2C controlled RTC port */

/* Define this for S/PDIF output available */
#define HAVE_SPDIF_OUT

/* Define this for S/PDIF input available */
#define HAVE_SPDIF_IN

#define CONFIG_LCD LCD_SSD1815

#define BOOTFILE_EXT "ajz"
#define BOOTFILE "ajbrec." BOOTFILE_EXT

#endif /* SIMULATOR */
