/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2007 by Christian Gmeiner
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <string.h>
#include "usbstack/core.h"

void into_usb_ctrlrequest(struct usb_ctrlrequest* request) {

	char* type = "";
	char* req = "";
	char* extra = 0;
	
	logf("-usb request-");
    /* check if packet is okay */
	if (request->bRequestType == 0 &&
        request->bRequest == 0 &&
        request->wValue == 0 &&
        request->wIndex == 0 &&
        request->wLength == 0) {
        logf(" -> INVALID <-");
        return;
	}	
	
    switch (request->bRequestType & USB_TYPE_MASK) {
	case USB_TYPE_STANDARD:
		type = "standard";
		
        switch (request->bRequest) {
        case USB_REQ_GET_STATUS:
        	req  = "get status";
            break;
        case USB_REQ_CLEAR_FEATURE:
        	req = "clear feature";
            break;
        case USB_REQ_SET_FEATURE:
        	req = "set feature";
            break;
        case USB_REQ_SET_ADDRESS:
        	req = "set address";
            break;
        case USB_REQ_GET_DESCRIPTOR:
        	req = "get descriptor";
        	
    		switch (request->wValue >> 8) {
    		case USB_DT_DEVICE:
    			extra = "get device descriptor";
    			break;
    		case USB_DT_DEVICE_QUALIFIER:
    			extra = "get device qualifier";
    			break;
    		case USB_DT_OTHER_SPEED_CONFIG:
    			extra = "get other-speed config descriptor";
    		case USB_DT_CONFIG:
    			extra = "get configuration descriptor";
    			break;
    		case USB_DT_STRING:
    			extra = "get string descriptor";
    			break;
    		case USB_DT_DEBUG:
    			extra = "debug";
    			break;
    		}
    		break;        	
        	
            break;
        case USB_REQ_SET_DESCRIPTOR:
        	req = "set descriptor";
            break;
        case USB_REQ_GET_CONFIGURATION:
        	req = "get configuration";
            break;
        case USB_REQ_SET_CONFIGURATION:
        	req = "set configuration";
            break;
        case USB_REQ_GET_INTERFACE:
        	req = "get interface";
            break;
        case USB_REQ_SET_INTERFACE:
        	req = "set interface";
            break;
        case USB_REQ_SYNCH_FRAME:
        	req = "sync frame";
            break;
        default:
        	req = "unkown";
        	break;
        }

        break;
    case USB_TYPE_CLASS:
    	type = "class";
        break;

    case USB_TYPE_VENDOR:
    	type = "vendor";
        break;
    }
        
    logf(" -b 0x%x", request->bRequestType);
    logf(" -b 0x%x", request->bRequest);
    logf(" -b 0x%x", request->wValue);
    logf(" -b 0x%x", request->wIndex);
    logf(" -b 0x%x", request->wLength);
    logf(" -> t: %s", type);
    logf(" -> r: %s", req);
    if (extra != 0) {
    	logf(" -> e: %s", extra);
    }
}
