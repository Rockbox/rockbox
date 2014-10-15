#ifndef _USB_DX50_H_
#define _USB_DX50_H_


/* Supported usb modes. */
enum
{
    USB_MODE_MASS_STORAGE = 0,
    USB_MODE_CHARGE_ONLY,
    USB_MODE_ADB
};

/*
    Set the usb mode
    mode: Either USB_MODE_MASS_STORAGE, USB_MODE_CHARGE_ONLY or USB_MODE_ADB.
*/
void set_usb_mode( int mode );


#endif
