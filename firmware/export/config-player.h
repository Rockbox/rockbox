/* define this if you have a charcell LCD display */
#define HAVE_LCD_CHARCELLS 1

/* LCD dimensions (for the simulator) */
#define LCD_WIDTH       (4*11*6)    /* Display width in pixels */
#define LCD_HEIGHT      (4*16+2*24) /* 4*char + 2*icons */
#define LCD_DEPTH       1

/* define this if you have the Player's keyboard */
#define CONFIG_KEYPAD PLAYER_PAD

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x8000

#define AB_REPEAT_ENABLE 1

#ifndef SIMULATOR

/* Define this if you have a SH7034 */
#define CONFIG_CPU SH7034

/* Define this if you have a MAS3507D */
#define CONFIG_CODEC MAS3507D

/* Define this if you have a DAC3550A */
#define HAVE_DAC3550A

/* Define this if you need to power on ATA */
#define NEEDS_ATA_POWER_ON

/* Define this if you control ata power player style
   (with PB4, new player only) */
#define ATA_POWER_PLAYERSTYLE

/* Define this to the CPU frequency */
#define CPU_FREQ 12000000 /* cycle time ~83.3ns */

/* Type of mobile power */
#define CONFIG_BATTERY BATT_4AA_NIMH

/* Battery scale factor (?) */
#define BATTERY_SCALE_FACTOR 6546

/* Define this if you must discharge the data line by driving it low
   and then set it to input to see if it stays low or goes high */
#define HAVE_I2C_LOW_FIRST

/* Define this if you control power on PADR (instead of PBDR) */
#define HAVE_POWEROFF_ON_PADR

/* Offset ( in the firmware file's header ) to the file length */
#define FIRMWARE_OFFSET_FILE_LENGTH 0

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 4

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 6

/* How to detect USB */
#define USB_PLAYERSTYLE 1

/* Define this if the platform can charge batteries */
#define HAVE_CHARGING 1

/* The start address index for ROM builds */
/* #define ROM_START 0xD010 for behind original Archos */
#define ROM_START 0x6010 /* for behind BootBox */

/* Software controlled LED */
#define CONFIG_LED LED_REAL

/* Define this for LCD backlight available */
#define CONFIG_BACKLIGHT BL_PA14_LO /* port PA14, low active */

#define CONFIG_LCD LCD_SSD1801

#define BOOTFILE_EXT "mod"
#define BOOTFILE "archos." BOOTFILE_EXT

#endif /* SIMULATOR */
