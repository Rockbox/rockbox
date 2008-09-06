/*
 * This config file is for the Iaudio7 series
 */
#define TARGET_TREE /* this target is using the target tree system */

/* For Rolo and boot loader */
#define MODEL_NUMBER 32

/* define this if you have recording possibility */
#define HAVE_RECORDING

/* Define bitmask of input sources - recordable bitmask can be defined
   explicitly if different */
#define INPUT_SRC_CAPS (SRC_CAP_MIC | SRC_CAP_LINEIN | SRC_CAP_FMRADIO)

/* define hardware samples rate caps mask */                                                                                                  
#define HW_SAMPR_CAPS   (/*SAMPR_CAP_88 | */SAMPR_CAP_44/* | SAMPR_CAP_22 | SAMPR_CAP_11*/)

/* define the bitmask of recording sample rates */
#define REC_SAMPR_CAPS  (SAMPR_CAP_44/* | SAMPR_CAP_22 | SAMPR_CAP_11*/)

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR

/* define this if you can flip your LCD */
//#define HAVE_LCD_FLIP

/* define this if you can invert the colours on your LCD */
//#define HAVE_LCD_INVERT

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* define this if you have access to the pitchscreen */
#define HAVE_PITCHSCREEN

/* define this if you have LCD enable function */
#define HAVE_LCD_ENABLE

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

#define HAVE_FAT16SUPPORT

#if 0 /* Enable for USB driver test */
#define HAVE_USBSTACK
#define USE_HIGH_SPEED
#define USB_VENDOR_ID   0x0e21
#define USB_PRODUCT_ID  0x0750

#define USB_STORAGE
#define USB_SERIAL
#endif

/* define this if you have a flash memory storage */
#define HAVE_FLASH_STORAGE

/* LCD dimensions */
#define LCD_WIDTH  160
#define LCD_HEIGHT 128
/* 16bits for now... */
#define LCD_DEPTH  16   /* 262144 colours */
#define LCD_PIXELFORMAT RGB565   /*rgb565*/

/*#define LCD_PIXELFORMAT VERTICAL_PACKING*/

/* define this to indicate your device's keypad */
#define CONFIG_KEYPAD IAUDIO67_PAD

/* #define HAVE_BUTTON_DATA */

/* define this if you have a real-time clock */
#define CONFIG_RTC RTC_PCF50606

/* define this if you have RTC RAM available for settings */
//#define HAVE_RTC_RAM

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* Reduce Tremor's ICODE usage */
#define ICODE_ATTR_TREMOR_NOT_MDCT

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x80000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

#define AB_REPEAT_ENABLE 1

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* The iaudio7 uses built-in WM8731 codec */
#define HAVE_WM8731
/* Codec is slave on serial bus */
#define CODEC_SLAVE

/* Define this if you have the TLV320 audio codec */
//#define HAVE_TLV320

/* TLV320 has no tone controls, so we use the software ones */
//#define HAVE_SW_TONE_CONTROLS

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT

#define CONFIG_I2C I2C_TCC77X

#define BATTERY_CAPACITY_DEFAULT 540 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 540 /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 540 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 50   /* capacity increment */
#define BATTERY_TYPES_COUNT  1    /* only one type */

/* define this if the unit should not shut down on low battery. */
#define NO_LOW_BATTERY_SHUTDOWN
#define CONFIG_CHARGING CHARGING_SIMPLE

#ifndef SIMULATOR

/* Define this if you have a TCC770 */
#define CONFIG_CPU TCC770

/* Define this if you have ATA power-off control */
#define HAVE_ATA_POWER_OFF

/* Define this to the CPU frequency */
#define CPU_FREQ      120000000

/* Offset ( in the firmware file's header ) to the file length */
//#define FIRMWARE_OFFSET_FILE_LENGTH 0

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

/* Software controlled LED */
#define CONFIG_LED LED_VIRTUAL

#define CONFIG_LCD LCD_IAUDIO67

/* FM Tuner */                                                                                                                                
#define CONFIG_TUNER LV24020LP
#define HAVE_TUNER_PWR_CTRL

/* Define this for FM radio input available */                                                                                                
#define HAVE_FMRADIO_IN                                                                                                                       

#define BOOTFILE_EXT "iaudio"
#define BOOTFILE "rockbox." BOOTFILE_EXT
#define BOOTDIR "/"

#ifdef BOOTLOADER
#define TCCBOOT
#endif
#endif /* SIMULATOR */
