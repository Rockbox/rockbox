/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:  $
 *
 * Copyright (C) 2014 by Amaury Pouly
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef USB_AUDIO_H
#define USB_AUDIO_H

#include "usb_ch9.h"
#include "usb_class_driver.h"

/* NOTE
 *
 * This is USBAudio 1.0. USBAudio 2.0 is notably _not backwards compatible!_
 * USBAudio 1.0 over _USB_ 2.0 is perfectly valid!
 *
 * Relevant specifications are USB 2.0 and USB Audio Class 1.0.
 */

/*
 * usb_audio_request_endpoints():
 *
 * Calls usb_core_request_endpoint() to request one IN and one OUT
 * isochronous endpoint.
 *
 * Called by allocate_interfaces_and_endpoints().
 *
 * Returns -1 if either request fails, returns 0 if success.
 *
 * Also requests buffer allocations. If allocation fails,
 * returns -1 so that the driver will be disabled by the USB core.
 */
int usb_audio_request_endpoints(struct usb_class_driver *);

/*
 * usb_audio_set_first_interface():
 *
 * Required function for the class driver.
 *
 * Called by allocate_interfaces_and_endpoints() to
 * tell the class driver what its first interface number is.
 * Returns the number of the interface available for the next
 * class driver to use.
 *
 * We need 2 interfaces, AudioControl and AudioStreaming.
 * Return interface+2.
 */
int usb_audio_set_first_interface(int interface);

/*
 * usb_audio_get_config_descriptor():
 *
 * Required function for the class driver.
 *
 * Called by request_handler_device_get_descriptor(), which expects
 * this function to fill *dest with the configuration descriptor for this
 * class driver.
 *
 * Return the size of this descriptor in bytes.
 */
int usb_audio_get_config_descriptor(unsigned char *dest,int max_packet_size);

/*
 * usb_audio_init_connection():
 *
 * Called by usb_core_do_set_config() when the
 * connection is ready to be used. Currently just sets
 * the audio sample rate to default.
 */
void usb_audio_init_connection(void);

/*
 * usb_audio_init():
 *
 * Initialize the driver. Called by usb_core_init().
 * Currently initializes the sampling frequency values available
 * to the AudioStreaming interface.
 */
void usb_audio_init(void);

/*
 * usb_audio_disconnect():
 *
 * Called by usb_core_exit() AND usb_core_do_set_config().
 *
 * Indicates to the Class driver that the connection is no
 * longer active. Currently just calls usb_audio_stop_playback().
 */
void usb_audio_disconnect(void);

/*
 * usb_audio_get_playing():
 *
 * Returns playing/not playing status of usbaudio.
 */
bool usb_audio_get_playing(void);

/*
 * usb_audio_get_alloc_failed():
 *
 * Return whether the buffer allocation succeeded (0)
 * or failed (1).
 */
bool usb_audio_get_alloc_failed(void);

/*
 * usb_audio_transfer_complete():
 *
 * Dummy function.
 *
 * The fast_transfer_complete() function needs to be used instead.
 */
void usb_audio_transfer_complete(int ep,int dir, int status, int length);

/*
 * usb_audio_fast_transfer_complete():
 *
 * Called by usb_core_transfer_complete().
 * The normal transfer complete handler system is too slow to deal with
 * ISO data at the rate required, so this is required.
 *
 * Return true if the transfer is handled, false otherwise.
 */
bool usb_audio_fast_transfer_complete(int ep,int dir, int status, int length);

/*
 * usb_audio_control_request():
 *
 * Called by control_request_handler_drivers().
 * Pass control requests down to the appropriate functions.
 *
 * Return true if this driver handles the request, false otherwise.
 */
bool usb_audio_control_request(struct usb_ctrlrequest* req, void* reqdata, unsigned char* dest);

/*
 * usb_audio_set_interface():
 *
 * Called by control_request_handler_drivers().
 * Deal with changing the interface between control and streaming.
 *
 * Return 0 for success, -1 otherwise.
 */
int usb_audio_set_interface(int intf, int alt);

/*
 * usb_audio_get_interface():
 *
 * Called by control_request_handler_drivers().
 * Get the alternate of the given interface.
 *
 * Return the alternate of the given interface, -1 if unknown.
 */
int usb_audio_get_interface(int intf);

/*
 * usb_audio_get_playback_sampling_frequency():
 *
 * Return the sample rate currently set.
 */
unsigned long usb_audio_get_playback_sampling_frequency(void);

/*
 * usb_audio_get_main_intf():
 *
 * Return the main usb interface
 */
int usb_audio_get_main_intf(void);

/*
 * usb_audio_get_alt_intf():
 *
 * Return the alternate usb interface
 */
int usb_audio_get_alt_intf(void);

/*
 * usb_audio_get_samplesperframe():
 *
 * Return the samples per frame over the last two feedback cycles
 * This is the samples sent to the mixer.
 *
 * This is returned in floating point 16.16 type. To convert to float,
 * do ((double)result / (1<<16))
 */
int32_t usb_audio_get_samplesperframe(void);

/*
 * usb_audio_get_samplesperframe():
 *
 * Return the samples per frame over the last two feedback cycles
 * This is the samples received from USB.
 *
 * This is returned in floating point 16.16 type. To convert to float,
 * do ((double)result / (1<<16))
 */
int32_t usb_audio_get_samples_rx_perframe(void);

/*
 * usb_audio_get_out_ep():
 *
 * Return the out (to device) endpoint
 */
unsigned int usb_audio_get_out_ep(void);

/*
 * usb_audio_get_in_ep():
 *
 * Return the in (to host) endpoint
 */
unsigned int usb_audio_get_in_ep(void);

/*
 * usb_audio_get_prebuffering():
 *
 * Return number of buffers filled ahead of playback
 */
int usb_audio_get_prebuffering(void);

/*
 * usb_audio_get_prebuffering_avg():
 *
 * Return the average number of buffers filled ahead of playback
 * over the last two feedback cycles
 *
 * This is returned in floating point 16.16 type. To convert to float,
 * do ((double)result / (1<<16))
 */
int32_t usb_audio_get_prebuffering_avg(void);

/*
 * usb_audio_get_prebuffering_maxmin():
 *
 * Return the max or min number of buffers filled ahead of playback
 * over the last feedback cycle
 */
int usb_audio_get_prebuffering_maxmin(bool max);

/*
 * usb_audio_get_underflow():
 *
 * Return whether playback is in "underflow" state
 */
bool usb_audio_get_underflow(void);

/*
 * usb_audio_get_overflow():
 *
 * Return whether usb is in "overflow" state
 */
bool usb_audio_get_overflow(void);

/*
 * usb_audio_get_frames_dropped():
 *
 * Return the number of frames which have been dropped during playback
 */
int usb_audio_get_frames_dropped(void);

/*
 * usb_audio_get_cur_volume():
 *
 * Return current audio volume in db
 */
int usb_audio_get_cur_volume(void);

bool usb_audio_get_active(void);

#endif
