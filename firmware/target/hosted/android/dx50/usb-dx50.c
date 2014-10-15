#include "usb-dx50.h"


void usb_enable_adb( void )
{
    system( "setprop persist.sys.usb.config adb" );
    system( "setprop persist.usb.debug 1" );
}


void usb_enable_mass_storage( void )
{
    system( "setprop persist.sys.usb.config mass_storage" );
    system( "setprop persist.usb.debug 0" );
}


void set_usb_mode( int mode )
{
    switch( mode )
    {
        case USB_MODE_MASS_STORAGE:
        {
            usb_enable_mass_storage();
            break;
        }

        case USB_MODE_CHARGE_ONLY: /* Work around. */
        case USB_MODE_ADB:
        {
            usb_enable_adb();
            break;
        }
    }
}