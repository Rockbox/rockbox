#ifndef JZTOOL_PRIVATE_H
#define JZTOOL_PRIVATE_H

#include "jztool.h"
#include <libusb.h>

struct jz_context {
    void* user_data;
    jz_log_cb log_cb;
    jz_log_level log_level;
    libusb_context* usb_ctx;
    int usb_ctxref;
};

struct jz_usbdev {
    jz_context* jz;
    libusb_device_handle* handle;
};

int jz_context_ref_libusb(jz_context* jz);
void jz_context_unref_libusb(jz_context* jz);

#endif /* JZTOOL_H */
