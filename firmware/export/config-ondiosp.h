/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP 1

/* LCD dimensions */
#define LCD_WIDTH  112
#define LCD_HEIGHT 64

/* define this if you have an Ondio style 6-key keyboard */
#define CONFIG_KEYPAD ONDIO_PAD

#ifndef SIMULATOR

/* Define this if you have a SH7034 */
#define CONFIG_CPU SH7034

/* Define this if you have a MAS3539F */
#define CONFIG_HWCODEC MAS3539F

/* Define this to the CPU frequency */
#define CPU_FREQ      12000000

/* Type of mobile power */
#define CONFIG_BATTERY BATT_3AAA_ALKALINE

/* Battery scale factor (measured from Jörg's FM) */
#define BATTERY_SCALE_FACTOR 4785 /* 4.890V read as 0x3FE */

/* Define this if you control power on PB5 (instead of the OFF button) */
#define HAVE_POWEROFF_ON_PB5

/* Offset ( in the firmware file's header ) to the file length */
#define FIRMWARE_OFFSET_FILE_LENGTH 20

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 6

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 24

/* How to detect USB */
#define USB_FMRECORDERSTYLE 1 /* like FM, on AN1 */

/* How to enable USB */
#define USB_ENABLE_ONDIOSTYLE 1 /* with PA5 */

/* The start address index for ROM builds */
#define ROM_START 0x12010

/* Define this if the display is mounted upside down */
#define HAVE_DISPLAY_FLIPPED

/* Define this for different I2C pinout */
#define HAVE_ONDIO_I2C

/* Define this for different ADC channel assignment */
#define HAVE_ONDIO_ADC

/* Define this for MMC support instead of ATA harddisk */
#define HAVE_MMC

/* Define this to support mounting FAT16 partitions */
#define HAVE_FAT16SUPPORT

/* Define this if the MAS SIBI line can be controlled via PB8 */
#define HAVE_MAS_SIBI_CONTROL

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* define this if more than one device/partition can be used */
#define HAVE_MULTIVOLUME

#endif /* SIMULATOR */
