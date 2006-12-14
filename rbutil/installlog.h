/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * Module: rbutil
 * File: installlog.h
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


#ifndef INSTALLLOG_H_INCLUDED
#define INSTALLLOG_H_INCLUDED

#include <wx/wxprec.h>
#ifdef __BORLANDC__
        #pragma hdrstop
#endif
#ifndef WX_PRECOMP
        #include <wx/wx.h>
#endif

#include <wx/confbase.h>
#include <wx/fileconf.h>

#define LOGFILE_VERSION 1
#define DIRECTORY_KLUDGE "_DIRECTORY_MARKER_RECORD_KLUDGE_"
class InstallLog
{
    // Class variables
    wxFileConfig*        logfile;

    // Methods
    public:
    InstallLog(wxString logname, bool CreateLog = true);
    ~InstallLog();
    unsigned int        WriteFile(wxString filepath, bool isDir = false);
    unsigned int        WriteFile(wxArrayString filepaths);
    wxArrayString*      GetInstalledFiles();

    private:
    bool                dirtyflag;
    wxArrayString       workingAS;
//    long                dummy;

    private:
    void                EnumerateCurDir(wxString curdir);
}; // InstallLog


#endif // INSTALLLOG_H_INCLUDED
