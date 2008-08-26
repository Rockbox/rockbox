/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include "system.h"
#include "usb_ch9.h"
#include "usb_drv.h"
#include "usb_core.h"
#include "jz4740.h"
#include "thread.h"

#if 0

#define EP1_INTR_BIT       2
#define EP_FIFO_NOEMPTY    2

#define GPIO_UDC_DETE_PIN  (32 * 3 + 6)
#define GPIO_UDC_DETE      GPIO_UDC_DETE_PIN
#define IRQ_GPIO_UDC_DETE  (IRQ_GPIO_0 + GPIO_UDC_DETE)

#define IS_CACHE(x)        (x < 0xa0000000)

bool usb_drv_connected(void)
{
    return (__gpio_get_pin(GPIO_UDC_DETE)==1);
}

int usb_detect(void)
{
    if(__gpio_get_pin(GPIO_UDC_DETE)==1)
        return USB_INSERTED;
    else
        return USB_EXTRACTED;
}

void usb_init_device(void)
{
    system_enable_irq(IRQ_UDC);
    __gpio_as_input(GPIO_UDC_DETE_PIN);
    return;
}

void usb_enable(bool on)
{
    if(on)
        usb_core_init();
    else
        usb_core_exit();
}

void usb_drv_init(void)
{
	/* Set this bit to allow the UDC entering low-power mode when
	 * there are no actions on the USB bus.
	 * UDC still works during this bit was set.
	 */
	//__cpm_stop_udc();
    
    __cpm_start_udc();

	/* Enable the USB PHY */
	REG_CPM_SCR |= CPM_SCR_USBPHY_ENABLE;

	/* Disable interrupts */
	REG_USB_REG_INTRINE = 0;
	REG_USB_REG_INTROUTE = 0;
	REG_USB_REG_INTRUSBE = 0;

	/* Enable interrupts */
	REG_USB_REG_INTRINE |= USB_INTR_EP0;
	REG_USB_REG_INTRUSBE |= USB_INTR_RESET;

	/* Enable SUSPEND */
	/* usb_setb(USB_REG_POWER, USB_POWER_SUSPENDM); */

	/* Enable HS Mode */
    REG_USB_REG_POWER |= USB_POWER_HSENAB;

	/* Let host detect UDC:
	 * Software must write a 1 to the PMR:USB_POWER_SOFTCONN bit to turn this
	 * transistor on and pull the USBDP pin HIGH.
	 */
	REG_USB_REG_POWER |= USB_POWER_SOFTCONN;
}

void usb_drv_exit(void)
{
	/* Disable interrupts */
	REG_USB_REG_INTRINE = 0;
	REG_USB_REG_INTROUTE = 0;
	REG_USB_REG_INTRUSBE = 0;

	/* Disable DMA */
	REG_USB_REG_CNTL1 = 0;
	REG_USB_REG_CNTL2 = 0;
    
    /* Disconnect from usb */
    REG_USB_REG_POWER &= ~USB_POWER_SOFTCONN;
    
    /* Disable the USB PHY */
    REG_CPM_SCR &= ~CPM_SCR_USBPHY_ENABLE;
    
    __cpm_stop_udc();
}

void usb_drv_set_address(int address)
{
    REG_USB_REG_FADDR = address;
}

/* Interrupt handler */
void UDC(void)
{

}

#else

//------------------------------------------
#ifndef u8
#define u8	unsigned char
#endif

#ifndef u16
#define u16	unsigned short
#endif

#ifndef u32
#define u32	unsigned int
#endif

#ifndef s8
#define s8	char
#endif

#ifndef s16
#define s16	short
#endif

#ifndef s32
#define s32	int
#endif

extern int usbdebug;

enum USB_ENDPOINT_TYPE
{
	ENDPOINT_TYPE_CONTROL,
	/* Typically used to configure a device when attached to the host.
	 * It may also be used for other device specific purposes, including
	 * control of other pipes on the device.
	 */
	ENDPOINT_TYPE_ISOCHRONOUS,
	/* Typically used for applications which need guaranteed speed.
	 * Isochronous transfer is fast but with possible data loss. A typical
	 * use is audio data which requires a constant data rate.
	 */
	ENDPOINT_TYPE_BULK,
	/* Typically used by devices that generate or consume data in relatively
	 * large and bursty quantities. Bulk transfer has wide dynamic latitude
	 * in transmission constraints. It can use all remaining available bandwidth,
	 * but with no guarantees on bandwidth or latency. Since the USB bus is
	 * normally not very busy, there is typically 90% or more of the bandwidth
	 * available for USB transfers.
	 */
	ENDPOINT_TYPE_INTERRUPT
	/* Typically used by devices that need guaranteed quick responses
	 * (bounded latency).
	 */
};


enum USB_STANDARD_REQUEST_CODE {
	GET_STATUS,
	CLEAR_FEATURE,
	SET_FEATURE = 3,
	SET_ADDRESS = 5,
	GET_DESCRIPTOR,
	SET_DESCRIPTOR,
	GET_CONFIGURATION,
	SET_CONFIGURATION,
	GET_INTERFACE,
	SET_INTERFACE,
	SYNCH_FRAME
};


enum USB_DESCRIPTOR_TYPE {
	DEVICE_DESCRIPTOR = 1,
	CONFIGURATION_DESCRIPTOR,
	STRING_DESCRIPTOR,
	INTERFACE_DESCRIPTOR,
	ENDPOINT_DESCRIPTOR,
	DEVICE_QUALIFIER_DESCRIPTOR,
	OTHER_SPEED_CONFIGURATION_DESCRIPTOR,
	INTERFACE_POWER1_DESCRIPTOR
};


enum USB_FEATURE_SELECTOR {
	ENDPOINT_HALT,
	DEVICE_REMOTE_WAKEUP,
	TEST_MODE
};

enum USB_CLASS_CODE {
	CLASS_DEVICE,
	CLASS_AUDIO,
	CLASS_COMM_AND_CDC_CONTROL,
	CLASS_HID,
	CLASS_PHYSICAL = 0x05,
	CLASS_STILL_IMAGING,
	CLASS_PRINTER,
	CLASS_MASS_STORAGE,
	CLASS_HUB,
	CLASS_CDC_DATA,
	CLASS_SMART_CARD,
	CLASS_CONTENT_SECURITY = 0x0d,
	CLASS_VIDEO,
	CLASS_DIAGNOSTIC_DEVICE = 0xdc,
	CLASS_WIRELESS_CONTROLLER = 0xe0,
	CLASS_MISCELLANEOUS = 0xef,
	CLASS_APP_SPECIFIC = 0xfe,
	CLASS_VENDOR_SPECIFIC = 0xff
};


typedef struct {
	u8 bmRequestType;
	u8 bRequest;
	u16 wValue;
	u16 wIndex;
	u16 wLength;
} __attribute__ ((packed)) USB_DeviceRequest;


typedef struct {
	u8 bLength;
	u8 bDescriptorType;
	u16 bcdUSB;
	u8 bDeviceClass;
	u8 bDeviceSubClass;
	u8 bDeviceProtocol;
	u8 bMaxPacketSize0;
	u16 idVendor;
	u16 idProduct;
	u16 bcdDevice;
	u8 iManufacturer;
	u8 iProduct;
	u8 iSerialNumber;
	u8 bNumConfigurations;
} __attribute__ ((packed)) USB_DeviceDescriptor;


typedef struct {
	u8 bLength;
	u8 bDescriptorType;
	u16 bcdUSB;
	u8 bDeviceClass;
	u8 bDeviceSubClass;
	u8 bDeviceProtocol;
	u8 bMaxPacketSize0;
	u8 bNumConfigurations;
	u8 bReserved;
} __attribute__ ((packed)) USB_DeviceQualifierDescriptor;


typedef struct {
	u8 bLength;
	u8 bDescriptorType;
	u16 wTotalLength;
	u8 bNumInterfaces;
	u8 bConfigurationValue;
	u8 iConfiguration;
	u8 bmAttributes;
	u8 MaxPower;
} __attribute__ ((packed)) USB_ConfigDescriptor;


typedef struct {
	u8 bLength;
	u8 bDescriptorType;
	u16 wTotalLength;
	u8 bNumInterfaces;
	u8 bConfigurationValue;
	u8 iConfiguration;
	u8 bmAttributes;
	u8 bMaxPower;
} __attribute__ ((packed)) USB_OtherSpeedConfigDescriptor;


typedef struct {
	u8 bLength;
	u8 bDescriptorType;
	u8 bInterfaceNumber;
	u8 bAlternateSetting;
	u8 bNumEndpoints;
	u8 bInterfaceClass;
	u8 bInterfaceSubClass;
	u8 bInterfaceProtocol;
	u8 iInterface;
} __attribute__ ((packed)) USB_InterfaceDescriptor;


typedef struct {
	u8 bLegth;
	u8 bDescriptorType;
	u8 bEndpointAddress;
	u8 bmAttributes;
	u16 wMaxPacketSize;
	u8 bInterval;
} __attribute__ ((packed)) USB_EndPointDescriptor;


typedef struct {
	u8 bLength;
	u8 bDescriptorType;
	u16 SomeDesriptor[1];
} __attribute__ ((packed)) USB_StringDescriptor;
//------------------------------------------
#define MAX_EP0_SIZE	64
#define MAX_EP1_SIZE	512

#define USB_HS      0
#define USB_FS      1
#define USB_LS      2

//definitions of EP0
#define USB_EP0_IDLE	0
#define USB_EP0_RX	1
#define USB_EP0_TX	2
/* Define maximum packet size for endpoint 0 */
#define M_EP0_MAXP	64
/* Endpoint 0 status structure */

static __inline__ void usb_setb(u32 port, u8 val)
{
	volatile u8 *ioport = (volatile u8 *)(port);
	*ioport = (*ioport) | val;
}

static __inline__ void usb_clearb(u32 port, u8 val)
{
	volatile u8 *ioport = (volatile u8 *)(port);
	*ioport = (*ioport) & ~val;
}

static __inline__ void usb_setw(u32 port, u16 val)
{
	volatile u16 *ioport = (volatile u16 *)(port);
	*ioport = (*ioport) | val;
}

static __inline__ void usb_clearw(u32 port, u16 val)
{
	volatile u16 *ioport = (volatile u16 *)(port);
	*ioport = (*ioport) & ~val;
}
//---------------------------------
#define BULK_OUT_BUF_SIZE 0x20000      //buffer size :
#define BULK_IN_BUF_SIZE 0x20000       // too

enum UDC_STATE
{
	IDLE,
	BULK_IN,
	BULK_OUT
};

enum USB_JZ4740_REQUEST            //add for USB_BOOT
{
	VR_GET_CUP_INFO = 0,
	VR_SET_DATA_ADDERSS,
	VR_SET_DATA_LENGTH,
	VR_FLUSH_CACHES,
	VR_PROGRAM_START1,
	VR_PROGRAM_START2,
	VR_NOR_OPS,
	VR_NAND_OPS,
	VR_SDRAM_OPS,
	VR_CONFIGRATION
};

enum NOR_OPS_TYPE
{
	NOR_INIT = 0,
	NOR_QUERY,
	NOR_WRITE,
	NOR_ERASE_CHIP,
	NOR_ERASE_SECTOR
};

enum NOR_FLASH_TYPE
{
	NOR_AM29 = 0,
	NOR_SST28,
	NOR_SST39x16,
	NOR_SST39x8
};

enum NAND_OPS_TYPE
{
	NAND_QUERY = 0,
	NAND_INIT,
	NAND_MARK_BAD,
	NAND_READ_OOB,
	NAND_READ_RAW,
	NAND_ERASE,
	NAND_READ,
	NAND_PROGRAM,
	NAND_READ_TO_RAM
};

enum SDRAM_OPS_TYPE
{
	SDRAM_LOAD,

};

enum DATA_STRUCTURE_OB
{
	DS_flash_info ,
	DS_hand
};


/*typedef enum _USB_BOOT_STATUS 
{
	USB_NO_ERR =0 ,
	GET_CPU_INFO_ERR,
	SET_DATA_ADDRESS_ERR,
	SET_DATA_LENGTH_ERR,
	FLUSH_CAHCES_ERR,
	PROGRAM_START1_ERR,
	PROGRAM_START2_ERR,
	NOR_OPS_ERR,
	NAND_OPS_ERR,
	NOR_FLASHTYPE_ERR,
	OPS_NOTSUPPORT_ERR
}USB_BOOT_STATUS;*/

enum OPTION
{
	OOB_ECC,
	OOB_NO_ECC,
	NO_OOB,
};
//-------------------------
static inline void jz_writeb(u32 address, u8 value)
{
	*((volatile u8 *)address) = value;
}

static inline void jz_writew(u32 address, u16 value)
{
	*((volatile u16 *)address) = value;
}

static inline void jz_writel(u32 address, u32 value)
{
	*((volatile u32 *)address) = value;
}

static inline u8 jz_readb(u32 address)
{
	return *((volatile u8 *)address);
}

static inline u16 jz_readw(u32 address)
{
	return *((volatile u16 *)address);
}

static inline u32 jz_readl(u32 address)
{
	return *((volatile u32 *)address);
}
//---------------------------

#define TXFIFOEP0 USB_FIFO_EP0

u32 Bulk_in_buf[BULK_IN_BUF_SIZE];
u32 Bulk_out_buf[BULK_OUT_BUF_SIZE];
u32 Bulk_in_size,Bulk_in_finish,Bulk_out_size;
u16 handshake_PKT[4]={0,0,0,0};
u8 udc_state;

static u32 rx_buf[32];
static u32 tx_buf[32];
static u32 tx_size, rx_size, finished,fifo;
static u8 ep0state,USB_Version;

static u32 fifoaddr[] = 
{
	TXFIFOEP0, TXFIFOEP0+4 ,TXFIFOEP0+8
};

static u32 fifosize[] = {
	MAX_EP0_SIZE, MAX_EP1_SIZE
};

static void udcReadFifo(u8 *ptr, int size)
{
	u32 *d = (u32 *)ptr;
	int s;
	s = (size + 3) >> 2;
	while (s--)
		*d++ = REG32(fifo);
}

static void udcWriteFifo(u8 *ptr, int size)
{
	u32 *d = (u32 *)ptr;
	u8 *c;
	int s, q;

	if (size > 0) {
		s = size >> 2;
		while (s--)
			REG32(fifo) = *d++;
		q = size & 3;
		if (q) {
			c = (u8 *)d;
			while (q--)
				REG8(fifo) = *c++;
		}
	} 
}

void HW_SendPKT(int ep, const u8 *buf, int size)
{
	fifo = fifoaddr[ep];

	if (ep!=0)
	{
		Bulk_in_size = size;
		Bulk_in_finish = 0;
		jz_writeb(USB_REG_INDEX, ep);
		if (Bulk_in_size - Bulk_in_finish <= fifosize[ep]) 
		{
			udcWriteFifo((u8 *)((u32)buf+Bulk_in_finish),
				     Bulk_in_size - Bulk_in_finish);
			usb_setb(USB_REG_INCSR, USB_INCSR_INPKTRDY);
			Bulk_in_finish = Bulk_in_size;
		} else 
		{
			udcWriteFifo((u8 *)((u32)buf+Bulk_in_finish),
				     fifosize[ep]);
			usb_setb(USB_REG_INCSR, USB_INCSR_INPKTRDY);
			Bulk_in_finish += fifosize[ep];
		}
	}
	else  //EP0
	{
		tx_size = size;
		finished = 0;
		memcpy((void *)tx_buf, buf, size);
		ep0state = USB_EP0_TX;		
	}
}

void HW_GetPKT(int ep, const u8 *buf, int size)
{
	memcpy((void *)buf, (u8 *)rx_buf, size);
	fifo = fifoaddr[ep];
	if (rx_size > size)
		rx_size -= size;
	else {
		size = rx_size;
		rx_size = 0;
	}
	memcpy((u8 *)rx_buf, (u8 *)((u32)rx_buf+size), rx_size);
}

static USB_DeviceDescriptor devDesc = 
{
	sizeof(USB_DeviceDescriptor),
	DEVICE_DESCRIPTOR,	//1
	0x0200,     //Version 2.0
	0xff,    //Vendor spec class
	0xff,
	0xff,
	64,	/* Ep0 FIFO size */
	0x601a,  //vendor ID
	0xDEAD,  //Product ID
	0xffff,
	0x00,
	0x00,
	0x00,
	0x01
};

#define	CONFIG_DESCRIPTOR_LEN	(sizeof(USB_ConfigDescriptor) + \
				 sizeof(USB_InterfaceDescriptor) + \
				 sizeof(USB_EndPointDescriptor) * 2)

static struct {
	USB_ConfigDescriptor    configuration_descriptor;
	USB_InterfaceDescriptor interface_descritor;
	USB_EndPointDescriptor  endpoint_descriptor[2];
} __attribute__ ((packed)) confDesc = {
	{
		sizeof(USB_ConfigDescriptor),
		CONFIGURATION_DESCRIPTOR,
		CONFIG_DESCRIPTOR_LEN,
		0x01,
		0x01,
		0x00,
		0xc0,	// Self Powered, no remote wakeup
		0x64	// Maximum power consumption 2000 mA
	},
	{
		sizeof(USB_InterfaceDescriptor),
		INTERFACE_DESCRIPTOR,
		0x00,
		0x00,
		0x02,	/* ep number */
		0xff,
		0xff,
		0xff,
		0x00
	},
	{
		{
			sizeof(USB_EndPointDescriptor),
			ENDPOINT_DESCRIPTOR,
			(1 << 7) | 1,// endpoint 2 is IN endpoint
			2, /* bulk */
			512,
			16
		},
		{
			sizeof(USB_EndPointDescriptor),
			ENDPOINT_DESCRIPTOR,
			(0 << 7) | 1,// endpoint 5 is OUT endpoint
			2, /* bulk */
			512, /* OUT EP FIFO size */
			16
		}
	}
};

void sendDevDescString(int size)
{
	u16 str_ret[13] = {
		   0x031a,//0x1a=26 byte
		   0x0041,
		   0x0030,
		   0x0030,
		   0x0041,
		   0x0030,
		   0x0030,
		   0x0041,
		   0x0030,
		   0x0030,
		   0x0041,
		   0x0030,
		   0x0030
		  };
	if(size >= 26)
		size = 26;
	str_ret[0] = (0x0300 | size);
	HW_SendPKT(0, (u8 *)str_ret,size);
}

void sendDevDesc(int size)
{
    switch (size) {
	case 18:
		HW_SendPKT(0, (u8 *)&devDesc, sizeof(devDesc));
		break;
	default:
		HW_SendPKT(0, (u8 *)&devDesc, 8);
		break;
	}
}

void sendConfDesc(int size)
{
	switch (size) {
	case 9:
		HW_SendPKT(0, (u8 *)&confDesc, 9);
		break;
	case 8:
		HW_SendPKT(0, (u8 *)&confDesc, 8);
		break;
	default:
		HW_SendPKT(0, (u8 *)&confDesc, sizeof(confDesc));
		break;
	}
}

void EP0_init(u32 out, u32 out_size, u32 in, u32 in_size)
{
	confDesc.endpoint_descriptor[0].bEndpointAddress = (1<<7) | in;
	confDesc.endpoint_descriptor[0].wMaxPacketSize = in_size;
	confDesc.endpoint_descriptor[1].bEndpointAddress = (0<<7) | out;
	confDesc.endpoint_descriptor[1].wMaxPacketSize = out_size;
}

static void udc_reset(void)
{
	//data init
	ep0state = USB_EP0_IDLE;
	Bulk_in_size = 0;
	Bulk_in_finish = 0;
	Bulk_out_size = 0;
	udc_state = IDLE;
	tx_size = 0;
	rx_size = 0;
	finished = 0;
	/* Enable the USB PHY */
//	REG_CPM_SCR |= CPM_SCR_USBPHY_ENABLE;
	/* Disable interrupts */
	jz_writew(USB_REG_INTRINE, 0);
	jz_writew(USB_REG_INTROUTE, 0);
	jz_writeb(USB_REG_INTRUSBE, 0);
	jz_writeb(USB_REG_FADDR,0);
	jz_writeb(USB_REG_POWER,0x60);   //High speed
	jz_writeb(USB_REG_INDEX,0);
	jz_writeb(USB_REG_CSR0,0xc0);
	jz_writeb(USB_REG_INDEX,1);
	jz_writew(USB_REG_INMAXP,512);
	jz_writew(USB_REG_INCSR,0x2048);
	jz_writeb(USB_REG_INDEX,1);
	jz_writew(USB_REG_OUTMAXP,512);
	jz_writew(USB_REG_OUTCSR,0x0090);
	jz_writew(USB_REG_INTRINE,0x3);   //enable intr
	jz_writew(USB_REG_INTROUTE,0x2);
	jz_writeb(USB_REG_INTRUSBE,0x4);

	if ((jz_readb(USB_REG_POWER)&0x10)==0) 
	{
		jz_writeb(USB_REG_INDEX,1);
		jz_writew(USB_REG_INMAXP,64);
		jz_writew(USB_REG_INCSR,0x2048);
		jz_writeb(USB_REG_INDEX,1);
		jz_writew(USB_REG_OUTMAXP,64);
		jz_writew(USB_REG_OUTCSR,0x0090);
		USB_Version=USB_FS;
		fifosize[1]=64;
		EP0_init(1,64,1,64);
	}
	else
	{
		jz_writeb(USB_REG_INDEX,1);
		jz_writew(USB_REG_INMAXP,512);
		jz_writew(USB_REG_INCSR,0x2048);
		jz_writeb(USB_REG_INDEX,1);
		jz_writew(USB_REG_OUTMAXP,512);
		jz_writew(USB_REG_OUTCSR,0x0090);
		USB_Version=USB_HS;
		fifosize[1]=512;
		EP0_init(1,512,1,512);
	}

}

void usbHandleStandDevReq(u8 *buf)
{
	USB_DeviceRequest *dreq = (USB_DeviceRequest *)buf;
	switch (dreq->bRequest) {
	case GET_DESCRIPTOR:
		if (dreq->bmRequestType == 0x80)	/* Dev2Host */
			switch(dreq->wValue >> 8) 
			{
			case DEVICE_DESCRIPTOR:
				sendDevDesc(dreq->wLength);
				break;
			case CONFIGURATION_DESCRIPTOR:
				sendConfDesc(dreq->wLength);
				break;
			case STRING_DESCRIPTOR:
				if (dreq->wLength == 0x02)
					HW_SendPKT(0, "\x04\x03", 2);
				else
					sendDevDescString(dreq->wLength);
				//HW_SendPKT(0, "\x04\x03\x09\x04", 2);
				break;
			}
		ep0state=USB_EP0_TX;
		
		break;
	case SET_ADDRESS:
		jz_writeb(USB_REG_FADDR,dreq->wValue);
		break;
	case GET_STATUS:
		switch (dreq->bmRequestType) {
		case 80:	/* device */
			HW_SendPKT(0, "\x01\x00", 2);
			break;
		case 81:	/* interface */
		case 82:	/* ep */
			HW_SendPKT(0, "\x00\x00", 2);
			break;
		}
		ep0state=USB_EP0_TX;
		break;
	case CLEAR_FEATURE:
	case SET_CONFIGURATION:
	case SET_INTERFACE:
	case SET_FEATURE:
		break;
	}
}

extern char printfbuf[256];

int GET_CUP_INFO_Handle()
{
	HW_SendPKT(0, printfbuf, 64);
	udc_state = IDLE;
	return 0;
}

void usbHandleVendorReq(u8 *buf)
{
	int ret_state;
	USB_DeviceRequest *dreq = (USB_DeviceRequest *)buf;
	switch (dreq->bRequest) {
	case 0xAB:
		ret_state=GET_CUP_INFO_Handle();
		break;
    case 0x12:
    	HW_SendPKT(0, "TEST", 4);
    	udc_state = IDLE;
        break;
	}
}

void Handshake_PKT(void)
{
	if (udc_state!=IDLE)
	{
		HW_SendPKT(1,(u8 *)handshake_PKT,sizeof(handshake_PKT));
		udc_state = IDLE;
	}
}

void usbHandleDevReq(u8 *buf)
{
	switch ((buf[0] & (3 << 5)) >> 5) {
	case 0: /* Standard request */
		usbHandleStandDevReq(buf);
		break;
	case 1: /* Class request */
		break;
	case 2: /* Vendor request */
		usbHandleVendorReq(buf);
		break;
	}
}

void EP0_Handler (void)
{
	u8			byCSR0;

/* Read CSR0 */
	jz_writeb(USB_REG_INDEX, 0);
	byCSR0 = jz_readb(USB_REG_CSR0);

/* Check for SentStall 
   if sendtall is set ,clear the sendstall bit*/
	if (byCSR0 & USB_CSR0_SENTSTALL) 
	{
		jz_writeb(USB_REG_CSR0, (byCSR0 & ~USB_CSR0_SENDSTALL));
		ep0state = USB_EP0_IDLE;
		return;
	}

/* Check for SetupEnd */
	if (byCSR0 & USB_CSR0_SETUPEND) 
	{
		jz_writeb(USB_REG_CSR0, (byCSR0 | USB_CSR0_SVDSETUPEND));
		ep0state = USB_EP0_IDLE;
		return;
	}
/* Call relevant routines for endpoint 0 state */
	if (ep0state == USB_EP0_IDLE) 
	{
		if (byCSR0 & USB_CSR0_OUTPKTRDY)   //There are datas in fifo
		{
			USB_DeviceRequest *dreq;
			fifo=fifoaddr[0];
			udcReadFifo((u8 *)rx_buf, sizeof(USB_DeviceRequest));
			usb_setb(USB_REG_CSR0, 0x48);//clear OUTRD bit
			dreq = (USB_DeviceRequest *)rx_buf;
			usbHandleDevReq((u8 *)rx_buf);
		}
		rx_size = 0;
	}
	
	if (ep0state == USB_EP0_TX) 
	{
		fifo=fifoaddr[0];
		if (tx_size - finished <= 64) 
		{
			udcWriteFifo((u8 *)((u32)tx_buf+finished),
				     tx_size - finished);
			finished = tx_size;
			usb_setb(USB_REG_CSR0, USB_CSR0_INPKTRDY);
			usb_setb(USB_REG_CSR0, USB_CSR0_DATAEND); //Set dataend!
			ep0state=USB_EP0_IDLE;
		} else 
		{
			udcWriteFifo((u8 *)((u32)tx_buf+finished), 64);
			usb_setb(USB_REG_CSR0, USB_CSR0_INPKTRDY);
			finished += 64;
		}
	}
	return;
}

void EPIN_Handler(u8 EP)
{
	jz_writeb(USB_REG_INDEX, EP);
	fifo = fifoaddr[EP];

	if (Bulk_in_size-Bulk_in_finish==0) 
	{
		Handshake_PKT();
		return;
	}

	if (Bulk_in_size - Bulk_in_finish <= fifosize[EP]) 
	{
		udcWriteFifo((u8 *)((u32)Bulk_in_buf+Bulk_in_finish),
			     Bulk_in_size - Bulk_in_finish);
		usb_setw(USB_REG_INCSR, USB_INCSR_INPKTRDY);
		Bulk_in_finish = Bulk_in_size;
	} else 
	{
		udcWriteFifo((u8 *)((u32)Bulk_in_buf+Bulk_in_finish),
			    fifosize[EP]);
		usb_setw(USB_REG_INCSR, USB_INCSR_INPKTRDY);
		Bulk_in_finish += fifosize[EP];
	}
}

void EPOUT_Handler(u8 EP)
{
	u32 size;
	jz_writeb(USB_REG_INDEX, EP);
	size = jz_readw(USB_REG_OUTCOUNT);
	fifo = fifoaddr[EP];
	udcReadFifo((u8 *)((u32)Bulk_out_buf+Bulk_out_size), size);
	usb_clearb(USB_REG_OUTCSR,USB_OUTCSR_OUTPKTRDY);
	Bulk_out_size += size;
}

void UDC(void)
{
	u8	IntrUSB;
	u16	IntrIn;
	u16	IntrOut;
/* Read interrupt registers */
	IntrUSB = jz_readb(USB_REG_INTRUSB);
	IntrIn  = jz_readw(USB_REG_INTRIN);
	IntrOut = jz_readw(USB_REG_INTROUT);

	if ( IntrUSB == 0 && IntrIn == 0 && IntrOut == 0)
		return;

	if (IntrIn & 2) 
	{
		EPIN_Handler(1);	     
	}
	if (IntrOut & 2) 
	{
		EPOUT_Handler(1);
	}
	if (IntrUSB & USB_INTR_RESET) 
	{
		udc_reset();
	}

/* Check for endpoint 0 interrupt */
	if (IntrIn & USB_INTR_EP0) 
	{
		EP0_Handler();
	}

	IntrIn  = jz_readw(USB_REG_INTRIN);
	return;
}

void __udc_start(void)
{
    udc_reset();
    
	ep0state = USB_EP0_IDLE;
	Bulk_in_size = 0;
	Bulk_in_finish = 0;
	Bulk_out_size = 0;
	udc_state = IDLE;
	tx_size = 0;
	rx_size = 0;
	finished = 0;

	if ((jz_readb(USB_REG_POWER)&0x10)==0) 
	{
		USB_Version=USB_FS;
		fifosize[1]=64;
		EP0_init(1,64,1,64);
	}
	else
	{
		USB_Version=USB_HS;
		fifosize[1]=512;
		EP0_init(1,512,1,512);
	}

	USB_Version=USB_HS;
    system_enable_irq(IRQ_UDC);
}

void usb_init_device(void){}

#endif
