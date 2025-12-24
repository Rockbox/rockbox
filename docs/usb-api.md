Handling USB control requests
=============================

API overview
------------
A control request goes through from device driver to class drivers, via usb core.
Each step is done by those functions:

    # from device driver to usb core
    void usb_core_setup_received(struct usb_ctrlrequest* req);
    # from usb core to class drivers
    bool usb_class_driver::control_request(struct usb_ctrlrequest* req);

Also control response goes through the inverse steps, and the following
functions are used:

    # from class drivers to usb core
    void usb_core_control_response(enum usb_control_response response, const void* data, size_t size);
    # from usb core to device drivers
    int usb_drv_{send,recv}_nonblocking(int endpoint, void *ptr, int length);

Request handling process
------------------------

The driver submits control requests to the USB core one at a time. Once a
request is submitted, it must be completed before the next request can be
submitted. This mirrors normal USB operation.

When the USB driver receives a setup packet from the host, it submits it
to the core to begin handling the control transfer. The driver calls
`usb_core_setup_received(req)`, passing the setup packet in `req`.

If the request was a Non-data transfer, the core or recipient class driver will
call `usb_core_control_response(USB_CONTROL_ACK, NULL, 0)` for ACK or
`usb_core_control_response(USB_CONTROL_STALL, NULL, 0)` for NAK.

If the request was a Control read transfer, the core or recipient class driver
will call `usb_core_control_response(USB_CONTROL_ACK, data, size)` to send data packets,
or `usb_core_control_response(false, USB_CONTROL_STALL, 0)` for NAK.

If the request was a Control write transfer, the core firstly receive data packets
using `usb_drv_recv_nonblocking`. Once it's done, the core or recipient class
driver will process it, then `usb_core_control_response(USB_CONTROL_ACK, NULL, 0)` for ACK
or `usb_core_control_response(USB_CONTROL_STALL, NULL, 0)` for NAK.
