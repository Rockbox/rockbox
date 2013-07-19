#include "config.h"

/* Can't link all Sansa PP devices the same way at this time */
#ifdef HAVE_BOOTLOADER_USB_MODE
#include "../pp/boot-pp502x-bl-usb.lds"
#else
#include "../pp/boot-pp.lds"
#endif