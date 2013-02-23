/* This file originates from the linux kernel provided in Samsung's YP-R0 Open
 * Source package.
 */

/*
* Bigbang project
* Copyright (c) 2009 VPS R&D Group, Samsung Electronics, Inc.
* All rights reserved.
*/

/**
* This file defines data structures and APIs for Freescale SC900776
*
* @name    sc900776.h
* @author  Eung Chan Kim (eungchan.kim@samsung.com)
* @version 0.1
* @see
*/

#ifndef __SC900776_H__
#define __SC900776_H__


typedef enum
{
	SC900776_DEVICE_ID = 0x01,		/* 01h		R   */
	SC900776_CONTROL,				/* 02h		R/W */
	SC900776_INTERRUPT1,			/* 03h		R/C */
	SC900776_INTERRUPT2,			/* 04h		R/C */
	SC900776_INTERRUPT_MASK1,		/* 05h		R/W */
	SC900776_INTERRUPT_MASK2,		/* 06h		R/W */
	SC900776_ADC_RESULT,			/* 07h		R   */
	SC900776_TIMING_SET1,			/* 08h		R/W */
	SC900776_TIMING_SET2,			/* 09h		R/W */
	SC900776_DEVICE_TYPE1,			/* 0Ah		R   */
	SC900776_DEVICE_TYPE2,			/* 0Bh		R   */
	SC900776_BUTTON1,				/* 0Ch		R/C */
	SC900776_BUTTON2,				/* 0Dh		R/C */
	/* 0Eh ~ 12h : reserved */
	SC900776_MANUAL_SWITCH1 = 0x13,	/* 13h		R/W */
	SC900776_MANUAL_SWITCH2, 		/* 14h		R/W */
	/* 15h ~ 1Fh : reserved */
	SC900776_FSL_STATUS = 0x20, 	/* 20h		R   */
	SC900776_FSL_CONTROL,			/* 21h		R/W */
	SC900776_TIME_DELAY,			/* 22h		R/W */
	SC900776_DEVICE_MODE,			/* 23h		R/W */

	SC900776_REG_MAX
} eSc900776_register_t;

typedef enum
{
	DEVICETYPE1_UNDEFINED = 0,
	DEVICETYPE1_USB,				// 0x04 0x00	// normal usb cable & ad200
	DEVICETYPE1_DEDICATED, 			// 0x40 0x00	// dedicated charger cable
	DEVICETYPE2_JIGUARTON,			// 0x00 0x08	// Anygate_UART jig
	DEVICETYPE2_JIGUSBOFF,			// 0x00 0x01	// USB jig(AS center)
	DEVICETYPE2_JIGUSBON,			// 0x00 0x02    // Anygate_USB jig with boot-on, not tested
} eMinivet_device_t;

/*
 * sc900776 register bit definitions
 */
#define MINIVET_DEVICETYPE1_USBOTG 		0x80	/* 1: a USBOTG device is attached */
#define MINIVET_DEVICETYPE1_DEDICATED 	0x40	/* 1: a dedicated charger is attached */
#define MINIVET_DEVICETYPE1_USBCHG 		0x20	/* 1: a USB charger is attached */
#define MINIVET_DEVICETYPE1_5WCHG 		0x10	/* 1: a 5-wire charger (type 1 or 2) is attached */
#define MINIVET_DEVICETYPE1_UART 		0x08	/* 1: a UART cable is attached */
#define MINIVET_DEVICETYPE1_USB 		0x04	/* 1: a USB host is attached */
#define MINIVET_DEVICETYPE1_AUDIO2 		0x02	/* 1: an audio accessory type 2 is attached */
#define MINIVET_DEVICETYPE1_AUDIO1 		0x01	/* 1: an audio accessory type 1 is attached */

#define MINIVET_DEVICETYPE2_AV			0x40	/* 1: an audio/video cable is attached */
#define MINIVET_DEVICETYPE2_TTY			0x20	/* 1: a TTY converter is attached */
#define MINIVET_DEVICETYPE2_PPD			0x10	/* 1: a phone powered device is attached */
#define MINIVET_DEVICETYPE2_JIGUARTON 	0x08	/* 1: a UART jig cable with the BOOT-on option is attached */
#define MINIVET_DEVICETYPE2_JIGUARTOFF 	0x04	/* 1: a UART jig cable with the BOOT-off option is attached */
#define MINIVET_DEVICETYPE2_JIGUSBON 	0x02	/* 1: a USB jig cable with the BOOT-on option is attached */
#define MINIVET_DEVICETYPE2_JIGUSBOFF 	0x01	/* 1: a USB jig cable with the BOOT-off option is attached */

#define MINIVET_FSLSTATUS_FETSTATUS		0x40	/* 1: The on status of the power MOSFET */
#define MINIVET_FSLSTATUS_IDDETEND		0x20	/* 1: ID resistance detection finished */
#define MINIVET_FSLSTATUS_VBUSDETEND	0x10	/* 1: VBUS power supply type identification completed */
#define MINIVET_FSLSTATUS_IDGND			0x08	/* 1: ID pin is shorted to ground */
#define MINIVET_FSLSTATUS_IDFLOAT		0x04	/* 1: ID line is floating */
#define MINIVET_FSLSTATUS_VBUSDET		0x02	/* 1: VBUS voltage is higher than the POR */
#define MINIVET_FSLSTATUS_ADCSTATUS		0x01	/* 1: ADC conversion completed */


#define SC900776_I2C_SLAVE_ADDR			0x25

typedef struct {
	unsigned char addr;
	unsigned char value;
}__attribute__((packed)) sMinivet_t;


#define DRV_IOCTL_MINIVET_MAGIC		'M'


typedef enum
{
	E_IOCTL_MINIVET_INIT = 0,
	E_IOCTL_MINIVET_WRITE_BYTE,
	E_IOCTL_MINIVET_READ_BYTE,
	E_IOCTL_MINIVET_DET_VBUS,
	E_IOCTL_MINIVET_MANUAL_USB,
	E_IOCTL_MINIVET_MANUAL_UART,

	E_IOCTL_MINIVET_MAX
} eSc900776_ioctl_t;

#define IOCTL_MINIVET_INIT 			_IO(DRV_IOCTL_MINIVET_MAGIC, E_IOCTL_MINIVET_INIT)
#define IOCTL_MINIVET_WRITE_BYTE 	_IOW(DRV_IOCTL_MINIVET_MAGIC, E_IOCTL_MINIVET_WRITE_BYTE, sMinivet_t)
#define IOCTL_MINIVET_READ_BYTE 	_IOR(DRV_IOCTL_MINIVET_MAGIC, E_IOCTL_MINIVET_READ_BYTE, sMinivet_t)
#define IOCTL_MINIVET_DET_VBUS 		_IO(DRV_IOCTL_MINIVET_MAGIC, E_IOCTL_MINIVET_DET_VBUS)
#define IOCTL_MINIVET_MANUAL_USB 	_IO(DRV_IOCTL_MINIVET_MAGIC, E_IOCTL_MINIVET_MANUAL_USB)
#define IOCTL_MINIVET_MANUAL_UART 	_IO(DRV_IOCTL_MINIVET_MAGIC, E_IOCTL_MINIVET_MANUAL_UART)


#ifndef __MINIVET_ENUM__
#define __MINIVET_ENUM__
enum
{
	EXT_PWR_UNPLUGGED = 0,
	EXT_PWR_PLUGGED,
	EXT_PWR_NOT_OVP,
	EXT_PWR_OVP,
};

#endif /* __MINIVET_ENUM__ */


#endif /* __MINIVET_IOCTL_H__ */
