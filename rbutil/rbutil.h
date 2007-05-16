/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * Module: rbutil
 * File: rbutil.h
 *
 * Copyright (C) 2005 Christi Alice Scarborough
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <wx/wxprec.h>
#ifdef __BORLANDC__
        #pragma hdrstop
#endif
#ifndef WX_PRECOMP
        #include <wx/wx.h>
#endif

#ifndef __rbutil_HPP_
#define __rbutil_HPP_

#include <wx/config.h>
#include <wx/confbase.h>
#include <wx/fileconf.h>
#include <wx/stdpaths.h>
#include <wx/wfstream.h>
#include <wx/filesys.h>
#include <wx/fs_inet.h>
#include <wx/fs_zip.h>
#include <wx/progdlg.h>
#include <wx/protocol/http.h>
#include <wx/string.h>
#include <wx/url.h>
#include <wx/ptr_scpd.h>
#include <wx/zipstrm.h>
#include <wx/dir.h>
#include <wx/filefn.h>
#include <wx/sstream.h>
#include <wx/msgdlg.h>
#include <wx/log.h>
#include <wx/file.h>
#include <wx/wizard.h>
#include <wx/event.h>
#include <wx/statline.h>
#include <wx/valgen.h>
#include <wx/thread.h>
#include <wx/regex.h>
#include <wx/tokenzr.h>
#include <wx/notebook.h>
#include <wx/html/htmlwin.h>
#include <wx/hyperlink.h>

#ifdef __WXMSW__
#define PATH_SEP "\\"
#define PATH_SEP_CHR '\\'
#define EXE_NAME wxT("rbutil.exe")
#else
#define PATH_SEP wxT("/")
#define PATH_SEP_CHR '/'
#define EXE_NAME wxT("rbutil")
#endif

#define UNINSTALL_FILE wxT(".rockbox" PATH_SEP ".rbutil_install_data")
#define MAX_PLATFORMS 50
#define SYSTEM_CONFIG rbutil.ini
#define FILE_BUFFER_SIZE 1024

//WX_DEFINE_OBJARRAY(bool, wxArrayBool);

class GlobalVars
{
public:
    // Program configuration data (rbutil.ini and environment)
    wxFileConfig*           GlobalConfig;
    wxFileConfig*           UserConfig;
    wxString                UserConfigFile;
    wxString                GlobalConfigFile;
    wxString                AppDir;
    wxString                ResourceDir;

    wxString*               ErrStr;
    wxStandardPaths*        stdpaths;
    wxArrayString           plat_id;
    wxArrayString           plat_name;
    wxArrayInt              plat_released;
    wxArrayInt              plat_needsbootloader;
    wxArrayString           plat_bootloadermethod;
    wxArrayString           plat_bootloadername;
    wxArrayString           plat_resolution;
    wxString                download_url;
    wxString                daily_url;
    wxString                bleeding_url;
    wxString                server_conf_url;
    wxString                font_url;
    wxString                last_release;
    wxString                prog_name;
    wxString                bootloader_url;
    wxString                themes_url;
    wxString                manual_url;
    wxString                doom_url;
    wxString                proxy_url;


    // User configuration data.
    wxString                curplat;
   // unsigned int            curplatnum;
    wxString                curdestdir;
    wxString                curfirmware;
    unsigned int            curbuild;
    bool                    curisfull;
    bool                    nocache;
    bool                    portable;
    wxString                curresolution;
    wxArrayString           themesToInstall;

    // Global system variables
    wxFFile*                 logfile;
    wxLogStderr*             logstderr;
    wxLogChain*              logchain;
    wxLogGui*                loggui;
};

extern GlobalVars* gv;

wxString wxFindAppPath(const wxString& argv0, const wxString& cwd,
    const wxString& appVariableName);
int DownloadURL(wxString src, wxString dest);
int UnzipFile(wxString src, wxString destdir, bool isInstall = false);
int Uninstall(const wxString dir, bool isFullUninstall = false);
bool InstallRbutil(wxString dest);
bool InstallTheme(wxString src);
bool checkZip(wxString zipname);
wxString stream_err_str(int errnum);
bool rm_rf(wxString file);


#define ERR_DIALOG(msg, title) \
    wxLogError(wxT("%s: %s"), ((wxString) title).c_str(), ((wxString) msg).c_str())

#define WARN_DIALOG(msg, title) \
    wxLogWarning(wxT("%s: %s"), ((wxString) title).c_str(), ((wxString) msg).c_str())

#define MESG_DIALOG(msg) \
   wxLogMessage(msg)

#define INFO_DIALOG(msg) \
    wxLogInfo(msg)

#define BUILD_RELEASE       0
#define BUILD_DAILY         1
#define BUILD_BLEEDING      2

#define BOOTLOADER_ADD      0
#define BOOTLOADER_REM      1


#endif
