/* define this if you have a charcell LCD display */
#define HAVE_LCD_CHARCELLS 1

/* define this if you have a neo-style LCD */
#define HAVE_NEO_LCD 1

/* define this if you have the Player's keyboard */
#define HAVE_NEO_KEYPAD 1

/* Define this if you have a SH7034 */
#define HAVE_SH7034

/* Define this if you have a MAS3507D */
#define HAVE_MAS3507D

/* Define this if you have a DAC3550A */
#define HAVE_DAC3550A

/* Define this to the CPU frequency */
#define CPU_FREQ 12000000 /* cycle time ~83.3ns */

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
#define USB_NONE 1

/* If this is a Neo-style memory architecture platform */
#define NEO_MEMORY 1

/* Define this for programmable LED available */
#define HAVE_LED

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT

