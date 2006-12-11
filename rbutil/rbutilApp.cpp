//---------------------------------------------------------------------------
//
// Name:        rbutilApp.cpp
// Author:      Christi Scarborough
// Created:     03/12/2005 00:35:02
//
//---------------------------------------------------------------------------

#include "rbutilApp.h"

GlobalVars* gv = new GlobalVars();

IMPLEMENT_APP(rbutilFrmApp)

bool rbutilFrmApp::OnInit()
{
    wxString buf = "";

    wxLogVerbose(wxT("=== begin rbutilFrmApp::Oninit()"));

    gv->stdpaths = new wxStandardPaths();
    buf = gv->stdpaths->GetUserDataDir();
    if (! wxDirExists(buf) )
    {
        wxLogNull lognull;
        if (! wxMkdir(buf))
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
    if (! wxDirExists(buf) ) wxMkdir(buf);

    wxFileSystem::AddHandler(new wxInternetFSHandler);
    wxFileSystem::AddHandler(new wxZipFSHandler);

    rbutilFrm *myFrame = new  rbutilFrm(NULL);
    SetTopWindow(myFrame);

    if (!ReadGlobalConfig(myFrame))
    {
        ERR_DIALOG(gv->ErrStr->GetData(), _("Rockbox Utility"));
        return FALSE;
    }

    ReadUserConfig();

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

    buf.Printf(wxT("%s" PATH_SEP "rbutil.ini"),
        gv->stdpaths->GetDataDir().c_str() );
    wxFileInputStream* cfgis = new wxFileInputStream(buf);

    if (!cfgis->CanRead()) {
        gv->ErrStr = new wxString(_("Unable to open configuration file"));
        return false;
    }

    gv->GlobalConfig = new wxFileConfig(*cfgis);

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

    buf.Printf(wxT("%s" PATH_SEP "%s"), gv->stdpaths->GetUserDataDir().c_str(),
        wxT("RockboxUtility.cfg"));
    gv->UserConfig = new wxFileConfig(wxEmptyString, wxEmptyString, buf);
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

