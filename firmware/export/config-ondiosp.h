/* define this if you have recording possibility */
#define HAVE_RECORDING 1

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP 1

/* define this if you have an Ondio style 6-key keyboard */
#define HAVE_ONDIO_KEYPAD

/* Define this if you have a MAS3587F */
#define HAVE_MAS3587F

/* Define this if you have a LiIon battery */
/* #define HAVE_LIION */

/* Define this if you need to power on ATA */
/* #define NEEDS_ATA_POWER_ON */

/* Define this if battery voltage can only be measured with ATA powered */
/* #define NEED_ATA_POWER_BATT_MEASURE */

/* Define this to the CPU frequency */
#define CPU_FREQ      12000000

/* Battery scale factor (measured from Jörg's FM) */
#define BATTERY_SCALE_FACTOR 4785 /* 4.890V read as 0x3FE */

/* Define this if you control power on PB5 (instead of the OFF button) */
#define HAVE_POWEROFF_ON_PB5 /* don't know yet */

/* Offset ( in the firmware file's header ) to the file length */
#define FIRMWARE_OFFSET_FILE_LENGTH 20

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 6

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 24

/* FM recorders can wake up from RTC alarm */
/* #define HAVE_ALARM_MOD 1 */

/* Define this if you have an FM Radio */
#define HAVE_FMRADIO 0

/* How to detect USB */
#define USB_FMRECORDERSTYLE 1 /* like FM, on AN1 */

/* How to enable USB */
#define USB_ENABLE_ONDIOSTYLE 1 /* with PA5 */

/* The start address index for ROM builds */
#define ROM_START 0x12010 /* don't know yet */

/* Define this if the display is mounted upside down */
#define HAVE_DISPLAY_FLIPPED

/* Define this for different I2C pinout */
#define HAVE_ONDIO_I2C

/* Define this for different ADC channel assignment */
#define HAVE_ONDIO_ADC

/* Define this for MMC support instead of ATA harddisk */
#define HAVE_MMC

