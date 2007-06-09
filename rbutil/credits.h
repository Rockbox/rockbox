/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * Module: rbutil
 * File: credits.h
 *
 * Copyright (C) 2006 Christi Alice Scarborough
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef CREDITS_H_INCLUDED
#define CREDITS_H_INCLUDED

#define RBUTIL_FULLNAME "The Rockbox Utility"
#define RBUTIL_VERSION "Version 0.3.2.4"

static const wxString rbutil_developers[] = {
    wxT("Christi Alice Scarborough"),
    wxT("Dave Chapman"),
    wxT("Dominik Wenger"),
    wxT("Bryan Childs"),
    wxT("")
};

#define RBUTIL_WEBSITE "http://www.rockbox.org/"
#define RBUTIL_COPYRIGHT wxT("(C) 2005-7 The Rockbox Team\n" \
        "released under the GNU Public License v2")
#define RBUTIL_DESCRIPTION wxT("Installer and housekeeping utility for " \
        "the Rockbox open source digital audio player firmware.")


class AboutDlg: public wxDialog
{
	public:
		AboutDlg(rbutilFrm *parent);
		~AboutDlg();
};

#include <wx/hyperlink.h>

#endif // CREDITS_H_INCLUDED
