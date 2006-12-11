//---------------------------------------------------------------------------
//
// Name:        rbutil.h
// Author:      Christi Scarborough
// Created:     03/12/2005 00:35:02
//
//---------------------------------------------------------------------------

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


#ifdef __WXMSW__
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

#define UNINSTALL_FILE ".rockbox" PATH_SEP ".rbutil_install_data"
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
    wxString*               ErrStr;
    wxStandardPaths*        stdpaths;
    wxArrayString           plat_id;
    wxArrayString           plat_name;
    wxArrayInt              plat_released;
    wxString                download_url;
    wxString                daily_url;
    wxString                bleeding_url;
    wxString                server_conf_url;
    wxString                font_url;
    wxString                last_release;
    wxString                prog_name;

    // User configuration data.
    wxString                 curplat;
    unsigned int             curplatnum;
    wxString                 curdestdir;
    unsigned int             curbuild;
    bool                     curisfull;
    bool                     nocache;

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
wxString stream_err_str(int errnum);
bool rm_rf(wxString file);

#define ERR_DIALOG(msg, title) \
    wxLogError("%s: %s", ((wxString) title).c_str(), ((wxString) msg).c_str())

#define WARN_DIALOG(msg, title) \
    wxLogWarning("%s: %s", ((wxString) title).c_str(), ((wxString) msg).c_str())

#define MESG_DIALOG(msg) \
   wxLogMessage(msg)

#define INFO_DIALOG(msg) \
    wxLogInfo(msg)

#define BUILD_RELEASE       0
#define BUILD_DAILY         1
#define BUILD_BLEEDING      2

#endif
