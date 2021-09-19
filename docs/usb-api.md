Handling USB control requests
=============================

API overview
------------

    enum usb_control_response {
        USB_CONTROL_ACK,
        USB_CONTROL_STALL,
        USB_CONTROL_RECEIVE,
    };

    void usb_core_control_request(struct usb_ctrlrequest* req, void* reqdata);
    void usb_core_control_complete(int status);
    void usb_drv_control_response(enum usb_control_response resp,
                                  void* data, int length);

The two `usb_core` functions are common to all targets with a USB stack and
are implemented in `usb_core.c`. The USB driver calls them to inform the core
when a control request arrives or is completed.

Each USB driver implements `usb_drv_control_response()`. The core calls this
to let the driver know how to respond to each control request.

### Legacy API

    void usb_core_legacy_control_request(struct usb_ctrlrequest* req);

The old control request API is available through this function. Drivers which
don't yet implement the new API can use the legacy API instead. To support
legacy drivers, the USB core implements all functions in the new API and
emulates the old control request handling behavior, bugs included.

This is intended as a stopgap measure so that old drivers keep working as-is.
The core can start using the new API right away, and drivers can be ported
one-by-one as time allows. Once all drivers are ported to the new API, all
legacy driver support can be removed.

Request handling process
------------------------

The driver submits control requests to the USB core one at a time. Once a
request is submitted, it must be completed before the next request can be
submitted. This mirrors normal USB operation.

When the USB driver receives a setup packet from the host, it submits it
to the core to begin handling the control transfer. The driver calls
`usb_core_control_request(req, NULL)`, passing the setup packet in `req`.
The second argument, `reqdata`, is not used at this time and is passed
as `NULL`.

The core processes the setup packet and calls `usb_drv_control_response()`
when it's done. The allowed responses depend on the type of control transfer
being processed.

### Non-data transfers

- `USB_CONTROL_ACK`, to indicate the request was processed successfully.
- `USB_CONTROL_STALL`, if the request is unsupported or cannot be processed.

### Control read transfers

- `USB_CONTROL_ACK`, to indicate the request was processed successfully.
  The core must provide a valid `data` buffer with `length` not exceeding
  the `wLength` field in the setup packet; otherwise, driver behavior is
  undefined. The driver will transfer this data to the host during the
  data phase of the control transfer, and then acknowledge the host's OUT
  packet to complete the transfer successfully.
- `USB_CONTROL_STALL`, if the request is unsupported or cannot be processed.

### Control write transfers

The driver calls `usb_core_control_request()` twice to handle control writes.
The first call allows the core to handle the setup packet, and if the core
decides to accept the data phase, the second call is made when the data has
been received without error.

#### Setup phase

The first call is made at the end of the setup phase, after receiving the
setup packet. The driver passes `reqdata = NULL` to indicate this.

The core can decide whether it wants to receive the data phase:

- `USB_CONTROL_RECEIVE`, if the core wishes to continue to the data phase.
  The core must provide a valid `data` buffer with `length` greater than or
  equal to the `wLength` specified in the setup packet; otherwise, driver
  behavior is undefined. The driver will proceed to the data phase and store
  received data into the provided buffer.
- `USB_CONTROL_STALL`, if the request is unsupported or cannot be processed.

If the core accepts the data phase, the driver will re-submit the request
when the data phase is completed correctly. If any error occurs during the
data phase, the driver will not re-submit the request; instead, it will
call `usb_core_control_complete()` with a non-zero status code.

#### Status phase

The second call to `usb_core_control_request()` is made at the end of the data
phase. The `reqdata` passed by the driver is the same one that the core passed
in its `USB_CONTROL_RECEIVE` response.

The core's allowed responses are:

- `USB_CONTROL_ACK`, to indicate the request was processed successfully.
- `USB_CONTROL_STALL`, if the request is unsupported or cannot be processed.

### Request completion

The driver will notify the core when a request has completed by calling
`usb_core_control_complete()`. A status code of zero means the request was
completed successfully; a non-zero code means it failed. Note that failure
can occur even if the request was successful from the core's perspective.

If the core response is `USB_CONTROL_STALL` at any point, the request is
considered complete. In this case, the driver won't deliver a completion
notification because it would be redundant.

The driver may only complete a request after the core has provided a response
to any pending `usb_core_control_request()` call. Specifically, if the core
has not yet responded to a request, the driver needs to defer the completion
notification until it sees the core's response. If the core's response is a
stall, then the notification should be silently dropped.

### Notes

- Driver behavior is undefined if the core makes an inappropriate response
  to a request, for example, responding with `USB_CONTROL_ACK` in the setup
  phase of a control write or `USB_CONTROL_RECEIVE` to a non-data request.
  The only permissible responses are the documented ones.

- If a response requires a buffer, then `data` must be non-NULL unless the
  `length` is also zero. If a buffer is not required, the core must pass
  `data = NULL` and `length = 0`. Otherwise, driver behavior is undefined.
  There are two responses which require a buffer:
  + `USB_CONTROL_ACK` to a control read
  + `USB_CONTROL_RECEIVE` to the setup phase of a control write

- Drivers must be prepared to accept a setup packet at any time, including
  in the middle of a control request. In such a case, devices are required
  to abort the ongoing request and start handling the new request. (This is
  intended as an error recovery mechanism and should not be abused by hosts
  in normal operation.) The driver must take care to notify the core of the
  current request's failure, and then submit the new request.
