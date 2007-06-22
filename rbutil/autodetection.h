/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * Module: rbutil
 * File: autodetection.h
 *
 * Copyright (C) 2008 Dominik Wenger
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef AUTODETECTION_H_INCLUDED
#define AUTODETECTION_H_INCLUDED


/**************************************
* General code for USB Device detection
***************************************/
#include "rbutil.h"

#define TOMANYDEVICES 2
#define NODEVICE 1
#define DEVICEFOUND 0

struct UsbDeviceInfo
{
  int device_index;
  wxString path;
  int status;
};


bool detectDevices(UsbDeviceInfo* tempdevice);

wxArrayString getPossibleMountPoints(); /* this funktion has to be implemented for every OS


/********************************
* Windows header for USB Device detection and information
**************************************/

#if defined( __WXMSW__ )


#endif /*__WXMSW__ */


/************************************************************************+
*Linux header for autodetection
**************************************************************************/


#if !(defined( __WXMSW__ ) || defined( __DARWIN__))

wxString resolve_mount_point( const wxString device );


#endif /* Linux Code */




/************************************************************************+
*MAc header for autodetection
**************************************************************************/


#if defined( __DARWIN__)



#endif /* MAc Code */








#endif
