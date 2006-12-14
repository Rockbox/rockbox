/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * Module: rbutil
 * File: rbutilApp.cpp
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

#include "rbutilApp.h"

GlobalVars* gv = new GlobalVars();

IMPLEMENT_APP(rbutilFrmApp)

bool rbutilFrmApp::OnInit()
{
    wxString buf = "";

    wxLogVerbose(wxT("=== begin rbutilFrmApp::Oninit()"));

    gv->stdpaths = new wxStandardPaths();

    // Get application directory
    // DANGER!  GetDataDir() doesn't portably return the application directory
    // We want to use the form below instead, but not until wxWidgets 2.8 is
    // released.
    //    gv->AppDir = gv->stdpaths->GetExecutablePath()->BeforeLast(&pathsep);
    buf = gv->stdpaths->GetDataDir(); buf.Append(PATH_SEP);
    gv->AppDir = buf.BeforeLast(PATH_SEP_CHR).c_str();

    buf = gv->stdpaths->GetUserDataDir();
    if (! wxDirExists(buf) )
    {
        wxLogNull lognull;
        if (! wxMkdir(buf, 0777))
        {
            wxLogFatalError(_("Can't create data directory %s"),
                buf.c_str());
        }
    }

    buf += wxT(PATH_SEP "rbutil.log");
    gv->logfile = new wxFFile(buf, "w");
    if (! gv->logfile->IsOpened() )
        wxLogFatalError(_("Unable to open log file"));

    gv->loggui = new wxLogGui();
    gv->loggui->SetActiveTarget(gv->loggui);
    gv->loggui->SetLogLevel(wxLOG_Message);
    gv->logchain = new wxLogChain(
        gv->logstderr = new wxLogStderr(gv->logfile->fp() ) );

    buf = buf.Left(buf.Len() - 10);
    buf.Append(wxT("download"));
    if (! wxDirExists(buf) ) wxMkdir(buf, 0777);

    wxFileSystem::AddHandler(new wxInternetFSHandler);
    wxFileSystem::AddHandler(new wxZipFSHandler);

    if (!ReadGlobalConfig(NULL))
    {
        ERR_DIALOG(gv->ErrStr->GetData(), _("Rockbox Utility"));
        return FALSE;
    }
    ReadUserConfig();

    rbutilFrm *myFrame = new  rbutilFrm(NULL);
    SetTopWindow(myFrame);
    myFrame->Show(TRUE);

    wxLogVerbose(wxT("=== end rbUtilFrmApp::OnInit()"));
    return TRUE;
}

int rbutilFrmApp::OnExit()
{
    wxLogVerbose(wxT("=== begin rbUtilFrmApp::OnExit()"));

    WriteUserConfig();

    gv->logfile->Close();
    /* Enabling this code causes the program to crash.  I
     * have no idea why.
    wxLog::DontCreateOnDemand();
    // Free a bunch of structures.
    delete gv->GlobalConfig;
    delete gv->ErrStr;
    delete gv->stdpaths;
    delete gv->platform;

    delete gv->logstderr;
    delete gv->logchain;
    delete gv->logfile;
    delete gv->loggui;
*/
    wxLogVerbose(wxT("=== end rbUtilFrmApp::OnExit()"));
	return 0;
}

bool rbutilFrmApp::ReadGlobalConfig(rbutilFrm* myFrame)
{
    wxString buf, tmpstr, stack;
    wxLogVerbose(wxT("=== begin rbutilFrmApp::ReadGlobalConfig(%p)"),
        (void*) myFrame);

    // Cross-platform compatibility: look for rbutil.ini in the same dir as the
    // executable before trying the standard data directory.  On Windows these
    // are of course the same directory.
    buf.Printf(wxT("%s" PATH_SEP "rbutil.ini"), gv->AppDir.c_str() );

    if (! wxFileExists(buf) )
    {
        gv->ResourceDir = gv->stdpaths->GetResourcesDir();
        buf.Printf(wxT("%s" PATH_SEP "rbutil.ini"),
            gv->ResourceDir.c_str() );
    } else
    {
        gv->ResourceDir = gv->AppDir;
    }

    wxFileInputStream* cfgis = new wxFileInputStream(buf);

    if (!cfgis->CanRead()) {
        gv->ErrStr = new wxString(_("Unable to open configuration file"));
        return false;
    }

    gv->GlobalConfig = new wxFileConfig(*cfgis);
    gv->GlobalConfigFile = buf;

    unsigned int i = 0;

    stack = gv->GlobalConfig->GetPath();
    gv->GlobalConfig->SetPath(wxT("/platforms"));
    while(gv->GlobalConfig->Read(buf.Format(wxT("platform%d"), i + 1),
        &tmpstr)) {
        gv->plat_id.Add(tmpstr);
        gv->GlobalConfig->Read(buf.Format(wxT("/%s/name"),
            gv->plat_id[i].c_str()), &tmpstr);
        gv->plat_name.Add(tmpstr);
        gv->GlobalConfig->Read(buf.Format(wxT("/%s/released"),
            gv->plat_id[i].c_str()), &tmpstr);
        gv->plat_released.Add( (tmpstr == wxT("yes")) ? true : false ) ;
        i++;
    }

    gv->GlobalConfig->SetPath(wxT("/general"));
    gv->GlobalConfig->Read(wxT("default_platform"), &tmpstr, wxT("cthulhu"));

    for (i=0; i< gv->plat_id.GetCount(); i++) {
        if (gv->plat_id[i] == tmpstr) gv->curplatnum  = i;
    }

    gv->GlobalConfig->Read(wxT("last_release"), &tmpstr);
    gv->last_release = tmpstr;

    gv->GlobalConfig->Read(wxT("download_url"), &tmpstr);
    gv->download_url = tmpstr;

    gv->GlobalConfig->Read(wxT("daily_url"), &tmpstr);
    gv->daily_url = tmpstr;

    gv->GlobalConfig->Read(wxT("bleeding_url"), &tmpstr);
    gv->bleeding_url = tmpstr;

    gv->GlobalConfig->Read(wxT("server_conf_url"), &tmpstr);
    gv->server_conf_url = tmpstr;

    gv->GlobalConfig->Read(wxT("font_url"), &tmpstr);
    gv->font_url = tmpstr;

    gv->GlobalConfig->Read(wxT("prog_name"), &tmpstr);
    gv->prog_name = tmpstr;

#ifdef __WXMSW__
    gv->curdestdir = wxT("D:\\");
#else
    gv->curdestdir = wxT("/mnt");
#endif
    gv->GlobalConfig->SetPath(stack);

    delete buf; delete tmpstr;
    wxLogVerbose(wxT("=== end rbutilFrmApp::ReadGlobalConfig()"));
    return true;
}

void rbutilFrmApp::ReadUserConfig()
{
    wxString buf, str, stack;

    buf.Printf(wxT("%s" PATH_SEP "RockboxUtility.cfg"),
         gv->AppDir.c_str());

    if (wxFileExists(buf) )
    {
        gv->portable = true;
    }
    else
    {
        gv->portable = false;
        buf.Printf(wxT("%s" PATH_SEP "%s"),
            gv->stdpaths->GetUserDataDir().c_str(), wxT("RockboxUtility.cfg"));
    }

    gv->UserConfig = new wxFileConfig(wxEmptyString, wxEmptyString, buf);
    gv->UserConfigFile = buf;
    gv->UserConfig->Set(gv->UserConfig); // Store wxWidgets internal settings
    stack = gv->UserConfig->GetPath();

    gv->UserConfig->SetPath(wxT("/defaults"));
    if (gv->UserConfig->Read(wxT("curdestdir"), &str) ) gv->curdestdir = str;
    if (gv->UserConfig->Read(wxT("curplatform"), &str) ) gv->curplat = str;
    gv->UserConfig->SetPath(stack);
}

void rbutilFrmApp::WriteUserConfig()
{
    gv->UserConfig->SetPath(wxT("/defaults"));
    gv->UserConfig->Write(wxT("curdestdir"), gv->curdestdir);
    gv->UserConfig->Write(wxT("curplatform"), gv->curplat);

    delete gv->UserConfig;

}

