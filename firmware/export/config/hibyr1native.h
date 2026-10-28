/* Copyright (C) 2026 Michael McAllister */

/* HiBy R1 native target: Ingenic X1600E, 64 MiB LPDDR2, 16 KiB instruction
 * and data caches with 32-byte lines. */

/* RoLo-related defines */
#define MODEL_NAME      "HiBy R1 Native"
#define MODEL_NUMBER    125
#define BOOTFILE_EXT    "r1"
#define BOOTFILE        "rockbox." BOOTFILE_EXT

/* CPU defines */
#define CONFIG_CPU          X1600

/* External crystal frequency. */
#define X1600_EXCLK_FREQ    24000000

/* Vendor CPU frequency. Valid after clk_init() configures APLL and CPCCR. */
#define CPU_FREQ            1104000000

#ifndef SIMULATOR
/* OST input clock; the free-running counter uses a divide-by-four
 * prescaler. */
#define TIMER_FREQ          X1600_EXCLK_FREQ
#endif

/* Kernel defines */
#define INCLUDE_TIMEOUT_API
#define HAVE_SEMAPHORE_OBJECTS

/* Hardware I2C: PMU and gauge on I2C0, touchscreen on I2C1. The DAC uses a
 * separate software bus. */
#define HAVE_I2C_ASYNC

/* Buffer for plugins and codecs. */
#define PLUGIN_BUFFER_SIZE  0x200000 /* 2 MiB */
#define CODEC_SIZE          0x100000 /* 1 MiB */

/* 480x800 RGB666 TFT with ST7701S panel controller, driven by the X1600
 * DPU. */
#define CONFIG_LCD          LCD_HIBY_R1
#define LCD_WIDTH           480
#define LCD_HEIGHT          800
#define LCD_DEPTH           16
#define LCD_PIXELFORMAT     RGB565
#define LCD_DPI             311 /* sqrt(480^2 + 800^2) / 3.0" diagonal */
#define HAVE_LCD_COLOR
#define HAVE_LCD_BITMAP

/* The DPU clock and SRAM power domain can be disabled when the panel is
 * off. */
#define HAVE_LCD_ENABLE

/* LCD sleep/resume is not yet enabled. */

/* PC00 is driven as a GPIO; PWM brightness control is not implemented. */
#define HAVE_BACKLIGHT
#define CONFIG_BACKLIGHT_FADING BACKLIGHT_NO_FADING

/* CS43131 DAC with a software I2C control bus. The measured clock settings
 * support 44.1 and 48 kHz at 512fs MCLK and 64fs BCLK. */
#define HAVE_CS43131
#define HW_SAMPR_CAPS   (SAMPR_CAP_44 | SAMPR_CAP_48)

/* The CS43131 has hardware volume, but no tone controls. */
#define HAVE_SW_TONE_CONTROLS

/* Five physical keys and a touchscreen; see button-target.h for the key
 * bits. */
/* Reuse the hosted HiBy keymap and plugin mappings. */
#define CONFIG_KEYPAD   HIBY_R3PROII_PAD
#define HAVE_TOUCHSCREEN
#define HAVE_BUTTON_DATA
#define HAVE_KBD_POINT_MODE

/* CST8xx on I2C1: PA16 interrupt, PA17 reset, ALDO2 supply. Single touch,
 * no axis swap or inversion. */
#define HAVE_CST8XX
#define CST8XX_NUM_POINTS 1

/* SADC channel 1 is headphone detect; in-range is 2800-3300 mV (verified). */
#define HAVE_HEADPHONE_DETECTION

/* microSD on MSC1, with card detect on PB22 and power on PC25. MSC0 is
 * wired to Wi-Fi. Internal SPI-NAND is not exposed as Rockbox storage. */
#define CONFIG_STORAGE  STORAGE_SD
#define HAVE_HOTSWAP
#define HAVE_HOTSWAP_STORAGE_AS_MAIN
#define HAVE_MULTIVOLUME
#define HAVE_FAT16SUPPORT

/* MSC DMA requires aligned buffers; use the storage bounce buffer for
 * unaligned transfers. */
#define STORAGE_WANTS_ALIGN
#define STORAGE_NEEDS_BOUNCE_BUFFER

/* One SD card slot */
#define SDMMC_HOST_NUM_SD_CONTROLLERS 1

/* RTC register definitions are supplied through ingenic-soc.h to the
 * shared driver. */
#define CONFIG_RTC      RTC_X1600
/* RTC alarm support is not implemented. */

/* AXP2101 PMU and CW2015 gauge on I2C0. Battery: 1600 mAh single-cell Li-ion, rated to 4.35 V. */
#define CONFIG_BATTERY_MEASURE \
    (VOLTAGE_MEASURE|PERCENTAGE_MEASURE|TIME_MEASURE)
#define CONFIG_CHARGING        CHARGING_MONITOR
#define HAVE_SW_POWEROFF

#ifndef SIMULATOR
#define HAVE_AXP2101
/* CW2015 fuel gauge. The vendor's battery profile (1600 mAh, 4.35 V) is
 * recorded in docs/hibyr1-RESUME.md; cw2015.c has no consumer for it yet. */
#define HAVE_CW2015
#define HAVE_POWEROFF_WHILE_CHARGING
#endif

/* Only one battery type */
#define BATTERY_CAPACITY_DEFAULT 1600
#define BATTERY_CAPACITY_MIN     1600
#define BATTERY_CAPACITY_MAX     1600
#define BATTERY_CAPACITY_INC     0

/* Multiboot */
#define HAVE_BOOTDATA
#define BOOT_REDIR "rockbox_main.hiby_r1"

#ifndef SIMULATOR

/* DesignWare USB device controller. */
#define CONFIG_USBOTG USBOTG_DESIGNWARE
#define USB_DW_ARCH_SLAVE
#define USB_DW_TURNAROUND 5
#define HAVE_USBSTACK

/* CDC serial console alongside mass storage. USB_ENABLE_SERIAL also
 * enables USB output from logf(). */
#define USB_ENABLE_SERIAL
/* Vendor mass-storage USB IDs, from usb_dev_mass_storage.sh. */
#define USB_VENDOR_ID  0x32BB       /* HiBy */
#define USB_PRODUCT_ID 0x0004       /* R1 */
#define USB_DEVBSS_ATTR __attribute__((aligned(32)))
#define HAVE_USB_POWER
/* Only the volume keys toggle charge-only mode on USB insertion. */
#define USBPOWER_BTN_IGNORE (~(BUTTON_VOL_UP|BUTTON_VOL_DOWN))
#define HAVE_USB_CHARGING_ENABLE
#define HAVE_USB_CHARGING_IN_THREAD
#define TARGET_USB_CHARGING_DEFAULT USB_CHARGING_FORCE
/* Include USB mass storage in the bootloader recovery menu. */
#define HAVE_BOOTLOADER_USB_MODE

/* Use 16 KiB buffers for USB slave mode. */
#define USB_READ_BUFFER_SIZE    (16 * 1024)
#define USB_WRITE_BUFFER_SIZE   (16 * 1024)

/* Use the SoC SADC driver. */
#define X1600_HAVE_SADC_DRIVER

#endif

/* Rockbox capabilities */
#define HAVE_ALBUMART
#define HAVE_BMP_SCALING
#define HAVE_JPEG
#define HAVE_TAGCACHE
#define HAVE_VOLUME_IN_LIST
#define HAVE_QUICKSCREEN
#define HAVE_HOTKEY
#define AB_REPEAT_ENABLE

/* Bluetooth, Wi-Fi, recording and general-purpose LED support are not
 * implemented. */
