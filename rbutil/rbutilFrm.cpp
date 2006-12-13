/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * Module: rbutil
 * File: rbutilFrm.cpp
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

#include "rbutilFrm.h"
#include "credits.h"

#include "rbutilFrm_XPM.xpm"
#include "install_3d.xpm"
#include "uninstall_3d.xpm"
#include "fonts_3d.xpm"
#include "tools2_3d.xpm"
#include "rblogo.xpm"

#include "wizard.xpm"

//----------------------------------------------------------------------------
// rbutilFrm
//----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(rbutilFrm,wxFrame)
	EVT_BUTTON      (ID_INSTALL_BTN, rbutilFrm::OnInstallBtn)
	EVT_BUTTON      (ID_REMOVE_BTN, rbutilFrm::OnRemoveBtn)
	EVT_BUTTON      (ID_FONT_BTN, rbutilFrm::OnFontBtn)
	EVT_BUTTON      (ID_BOOTLOADER_BTN, rbutilFrm::OnBootloaderBtn)

	EVT_CLOSE(rbutilFrm::rbutilFrmClose)
	EVT_MENU(ID_FILE_EXIT, rbutilFrm::OnFileExit)
	EVT_MENU(ID_FILE_ABOUT, rbutilFrm::OnFileAbout)
	EVT_MENU(ID_FILE_WIPECACHE, rbutilFrm::OnFileWipeCache)
	EVT_MENU(ID_PORTABLE_INSTALL, rbutilFrm::OnPortableInstall)
END_EVENT_TABLE()

rbutilFrm::rbutilFrm( wxWindow *parent, wxWindowID id, const wxString &title,
    const wxPoint &position, const wxSize& size, long style )
    : wxFrame( parent, id, title, position, size, style)
{
    wxLogVerbose(wxT("=== begin rbutilFrm::rbutilFrm(...)"));
    CreateGUIControls();
    wxLogVerbose(wxT("=== end rbutilFrm::rbutilFrm"));
}

rbutilFrm::~rbutilFrm() {}

void rbutilFrm::CreateGUIControls(void)
{
    wxLogVerbose(wxT("=== begin rbutilFrm::CreateGUIControls()"));

	wxBoxSizer* WxBoxSizer1 = new wxBoxSizer(wxHORIZONTAL);
	this->SetSizer(WxBoxSizer1);
	this->SetAutoLayout(TRUE);

	WxPanel1 = new wxPanel(this, ID_WXPANEL1, wxPoint(0,0), wxSize(400,600));
	WxBoxSizer1->Add(WxPanel1,1,wxGROW | wxALL,0);

	wxBoxSizer* WxBoxSizer2 = new wxBoxSizer(wxVERTICAL);
	WxPanel1->SetSizer(WxBoxSizer2);
	WxPanel1->SetAutoLayout(TRUE);

	wxBitmap WxStaticBitmap1_BITMAP(rblogo_xpm);
	WxStaticBitmap1 = new wxStaticBitmap(WxPanel1, ID_WXSTATICBITMAP1,
        WxStaticBitmap1_BITMAP, wxPoint(0,0), wxSize(400,123));
	WxStaticBitmap1->Enable(false);
	WxBoxSizer2->Add(WxStaticBitmap1,0,wxALIGN_CENTER_HORIZONTAL | wxALL,5);

	wxStaticBox* WxStaticBoxSizer3_StaticBoxObj = new wxStaticBox(WxPanel1,
        wxID_ANY, _("Please choose an option"));
	wxStaticBoxSizer* WxStaticBoxSizer3 =
        new wxStaticBoxSizer(WxStaticBoxSizer3_StaticBoxObj,wxHORIZONTAL);
	WxBoxSizer2->Add(WxStaticBoxSizer3,1,wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

	wxFlexGridSizer* WxFlexGridSizer1 = new wxFlexGridSizer(2,2,0,0);
	WxStaticBoxSizer3->Add(WxFlexGridSizer1,0,wxGROW | wxALL,0);

    wxBitmap BootloaderInstallButton (tools2_3d_xpm);
    WxBitmapButton4 = new wxBitmapButton(WxPanel1, ID_BOOTLOADER_BTN,
        BootloaderInstallButton, wxPoint(0,0), wxSize(64,54),
        wxRAISED_BORDER | wxBU_AUTODRAW);
    WxBitmapButton4->SetToolTip(_("Instructions for installing the "
        "Rockbox bootloader on your audio device"));
    WxFlexGridSizer1->Add(WxBitmapButton4, 0,
        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxALL,5);

    wxStaticText* WxStaticText5 =  new wxStaticText(WxPanel1, wxID_ANY,
        _("Bootloader installation instructions\n\n"
        "Before Rockbox can be installed on your audio player, you "
        "may have to\ninstall a bootloader.  This can not currently "
        "be done automatically, but is\nonly necessary the first time "
        "Rockbox is installed."));
	WxFlexGridSizer1->Add(WxStaticText5, 0,
        wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxALL,5);

	wxBitmap WxBitmapButton1_BITMAP (install_3d_xpm);
	WxBitmapButton1 = new wxBitmapButton(WxPanel1, ID_INSTALL_BTN,
        WxBitmapButton1_BITMAP, wxPoint(0,0), wxSize(64,54),
        wxRAISED_BORDER | wxBU_AUTODRAW, wxDefaultValidator,
        wxT("WxBitmapButton1"));
	WxBitmapButton1->SetToolTip(_("Install Rockbox"));
	WxFlexGridSizer1->Add(WxBitmapButton1,0,
        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxALL,5);

	WxStaticText2 = new wxStaticText(WxPanel1, ID_WXSTATICTEXT2,
        _("Install Rockbox on your audio player"), wxPoint(70,16),
        wxSize(175,17), 0, wxT("WxStaticText2"));
	WxFlexGridSizer1->Add(WxStaticText2,0,
        wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxALL,5);

    wxBitmap FontInstallButton (fonts_3d_xpm);
    WxBitmapButton3 = new wxBitmapButton(WxPanel1, ID_FONT_BTN,
        FontInstallButton, wxPoint(0,0), wxSize(64,54),
        wxRAISED_BORDER | wxBU_AUTODRAW);
    WxBitmapButton3->SetToolTip(_("Download the most up to date "
        "Rockbox fonts."));
    WxFlexGridSizer1->Add(WxBitmapButton3, 0,
        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxALL,5);

    wxStaticText* WxStaticText4 =  new wxStaticText(WxPanel1, wxID_ANY,
        _("Install the Rockbox fonts package\n\n"
        "This step is only needed if you have installed "
        "a daily build and want\nthe full set of Rockbox fonts.  You "
        "will not need to download these\nagain unless you uninstall "
        "Rockbox."));
	WxFlexGridSizer1->Add(WxStaticText4, 0,
        wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxALL,5);

	wxBitmap WxBitmapButton2_BITMAP (uninstall_3d_xpm);
	WxBitmapButton2 = new wxBitmapButton(WxPanel1, ID_REMOVE_BTN,
        WxBitmapButton2_BITMAP, wxPoint(0,0), wxSize(64,54),
        wxRAISED_BORDER | wxBU_AUTODRAW, wxDefaultValidator,
        wxT("WxBitmapButton2"));
	WxBitmapButton2->SetToolTip(_("Uninstall Rockbox"));
	WxFlexGridSizer1->Add(WxBitmapButton2,0,
        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxALL,5);

	WxStaticText3 = new wxStaticText(WxPanel1, ID_WXSTATICTEXT3,
        _("Remove Rockbox from your audio player"), wxPoint(60,66),
        wxSize(196,17), 0, wxT("WxStaticText3"));
	WxFlexGridSizer1->Add(WxStaticText3,0,
        wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxALL,5);

	WxMenuBar1 =  new wxMenuBar();
	wxMenu *ID_FILE_MENU_Mnu_Obj = new wxMenu(0);
	WxMenuBar1->Append(ID_FILE_MENU_Mnu_Obj, _("&File"));

    ID_FILE_MENU_Mnu_Obj->Append(ID_FILE_WIPECACHE,
        _("&Empty local download cache"), wxT(""), wxITEM_NORMAL);
    if (! gv->portable )
    {
        ID_FILE_MENU_Mnu_Obj->Append(ID_PORTABLE_INSTALL,
            _("&Install Rockbox Utility on device"), wxT(""), wxITEM_NORMAL);
    }
    ID_FILE_MENU_Mnu_Obj->Append(ID_FILE_ABOUT, _("&About"), wxT(""),
        wxITEM_NORMAL);
    ID_FILE_MENU_Mnu_Obj->Append(ID_FILE_EXIT, _("E&xit\tCtrl+X"), wxT(""),
        wxITEM_NORMAL);

	this->SetMenuBar(WxMenuBar1);

	GetSizer()->Fit(this);
	GetSizer()->SetSizeHints(this);
	if (gv->portable)
	{
	    this->SetTitle(_("Rockbox Utility (portable)"));
	} else
	{
        this->SetTitle(_("Rockbox Utility"));
	}
	this->Center();
	wxIcon rbutilFrm_ICON (rbutilFrm_XPM);
	this->SetIcon(rbutilFrm_XPM);
	this->SetToolTip(wxT("Install Rockbox"));

    wxLogVerbose("=== end rbutilFrm::CreateGUIControls");
}

void rbutilFrm::rbutilFrmClose(wxCloseEvent& event)
{
    wxLogVerbose("=== begin rbutilFrm::rbutilFrmClose(event)");
    Destroy();
    wxLogVerbose("=== end rbutilFrm::rbutilFrmClose");
}


/*
 * OnFileExit
 */
void rbutilFrm::OnFileExit(wxCommandEvent& event)
{
    wxLogVerbose("=== begin rbutilFrm::OnFileExit(event)");
	Close();
	wxLogVerbose("=== end rbutilFrm::OnFileExit");
}

// The routines this code uses are in the wxWidgets documentation, but
// not yet the library (2.7.0-1).  This code can be re-enabled later.

void rbutilFrm::OnFileAbout(wxCommandEvent& event)
{
/*
    wxAboutDialogInfo *info = new wxAboutDialogInfo();

    info->SetName(_(RBUTIL_LONGNAME));
    info->SetVersion(_(RBUTIL_VERSION));
    info->SetCopyright(_(RBUTIL_COPYRIGHT));
    info->SetDescription(_(RBUTIL_DESCRIPTION));
    info->SetWebsite(_(RBUTIL_WEBSITE));
    ind
    wxArrayString *array = new wxArrayString(sizeof(rbutil_developers[]).
        rbutil_developers);
    info->SetDevelopers(array);
    delete array;

//    array = new wxArrayString(sizeof(rbutil_translators[]),
//        rbutil_translators);
//    info->SetTranslators(array);
//    delete array;

    wxAboutBox(info);
*/
    AboutDlg(this).ShowModal();
}

void rbutilFrm::OnFileWipeCache(wxCommandEvent& event)
{
    wxString cacheloc, datadir;

    datadir = gv->stdpaths->GetUserDataDir();
    if (datadir == "")
    {
        ERR_DIALOG(_("Can't locate user data directory.  Unable to delete "
            "cache."), _("Delete download cache.") );
        return;
    }

    cacheloc.Printf(wxT("%s" PATH_SEP "download"),
        datadir.c_str());

    if (! rm_rf(cacheloc) )
    {
        MESG_DIALOG(_("Local download cache has been deleted."));
    }
    else {
        MESG_DIALOG(_("Errors occured deleting the local download cache."));
    }

    wxMkDir(cacheloc, 0777);
}


void rbutilFrm::OnBootloaderBtn(wxCommandEvent& event)
{
    if (!wxLaunchDefaultBrowser(wxT(
        "http://www.rockbox.org/twiki/bin/view/Main/"
        "DocsIndex#Rockbox_Installation_Instruction") ) )
    {
        ERR_DIALOG(_("Unable to launch external browser"),
            _("Boot loader install instructions."));
    }
}

void rbutilFrm::OnInstallBtn(wxCommandEvent& event)
{
    wxString src, dest, buf;
    wxDateTime date;
    wxTimeSpan day(24);
    wxLogVerbose("=== begin rbutilFrm::OnInstallBtn(event)");
    wxFileSystem fs;
    wxFileConfig* buildinfo;
    wxDateSpan oneday;

    wxWizard *wizard = new wxWizard(this, wxID_ANY,
                    _("Rockbox Installation Wizard"),
                    wxBitmap(wizard_xpm),
                    wxDefaultPosition,
                    wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    wxPlatformPage* page1 = new wxPlatformPage(wizard);
    wxLocationPage* page2 = new wxLocationPage(wizard);
    wxBuildPage*    page3 = new wxBuildPage(wizard);
    page1->SetNext(page2);
    page2->SetPrev(page1);
    page2->SetNext(page3);
    page3->SetPrev(page2);

    wizard->GetPageAreaSizer()->Add(page1);

    if (wizard->RunWizard(page1) )
    {
        switch (gv->curbuild)
        {
            case BUILD_RELEASE:
                // This is a URL - don't use PATH_SEP
                src.Printf("%s%s-%s-%s.zip",
                    gv->download_url.c_str(), gv->prog_name.c_str(),
                    gv->last_release.c_str(), gv->curplat.c_str());
                dest.Printf("%s" PATH_SEP "download" PATH_SEP "%s-%s-%s.zip",
                    gv->stdpaths->GetUserDataDir().c_str(),
                    gv->prog_name.c_str(), gv->last_release.c_str(),
                    gv->curplat.c_str());
                break;
            case BUILD_DAILY:
                dest.Printf("%s" PATH_SEP "download" PATH_SEP "build-info",
                    gv->stdpaths->GetUserDataDir().c_str());
                if (DownloadURL(gv->server_conf_url, dest)) {
                    WARN_DIALOG(_("Unable to download build status."),
                    _("Install"));
                    buf = "";
                } else
                {
                    buildinfo = new wxFileConfig(wxEmptyString,
                        wxEmptyString, dest);
                    buf = buildinfo->Read(wxT("/dailies/date"));
                    buildinfo->DeleteAll();

                    if (buf.Len() != 8) {
                        dest.Printf(_("Invalid build date: %s"), buf.c_str());
                        WARN_DIALOG(dest, _("Install"));
                        buf = "";
                    }
                }

                if (buf == "") {
                    WARN_DIALOG(_("Can't get date of latest build from "
                        "server.  Using yesterday's date."), _("Install") );
                    date = wxDateTime::Now();
                    date.Subtract(oneday.Day());
                    buf = date.Format(wxT("%Y%m%d")); // yes, we want UTC
                }

                src.Printf("%s%s/%s-%s-%s.zip",
                    gv->daily_url.c_str(), gv->curplat.c_str(),
                    gv->prog_name.c_str(), gv->curplat.c_str(),
                    buf.c_str() );

                dest.Printf("%s" PATH_SEP "download" PATH_SEP "%s-%s-%s.zip",
                    gv->stdpaths->GetUserDataDir().c_str(),
                    gv->prog_name.c_str(),
                    gv->curplat.c_str(), buf.c_str() );
                break;
            case BUILD_BLEEDING:
                 src.Printf("%s%s/%s.zip",
                    gv->bleeding_url.c_str(), gv->curplat.c_str(),
                    gv->prog_name.c_str() );
                dest.Printf("%s" PATH_SEP "download" PATH_SEP "%s.zip",
                    gv->stdpaths->GetUserDataDir().c_str(),
                    gv->prog_name.c_str() );
                break;
            default:
                ERR_DIALOG(_("Something seriously odd has happened."),
                    _("Install"));
                return;
                break;
        }

        if (gv->nocache || ( ! wxFileExists(dest) ) )
        {
            if ( DownloadURL(src, dest) )
            {
                wxRemoveFile(dest);
                buf.Printf(_("Unable to download %s"), src.c_str() );
                ERR_DIALOG(buf, _("Install"));
                return;
            }
        }

        if ( !UnzipFile(dest, gv->curdestdir, true) )
        {
            MESG_DIALOG(_("Rockbox has been installed on your device.") );
        } else
        {
            wxRemoveFile(dest);
            buf.Printf(_("Unable to unzip %s"), dest.c_str() );
            ERR_DIALOG(buf, _("Install"));
        }
    } else
    {
        MESG_DIALOG(_("The installation wizard was cancelled") );
    }

    wxLogVerbose("=== end rbutilFrm::OnInstallBtn");
}

void rbutilFrm::OnFontBtn(wxCommandEvent& event)
{
    wxString src, dest, buf;
    wxDateTime date;
    wxTimeSpan day(24);
    wxLogVerbose("=== begin rbutilFrm::OnFontBtn(event)");
    wxFileSystem fs;
    wxFileConfig* buildinfo;
    wxDateSpan oneday;

    wxWizard *wizard = new wxWizard(this, wxID_ANY,
                    _("Rockbox Font Installation Wizard"),
                    wxBitmap(wizard_xpm),
                    wxDefaultPosition,
                    wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    wxLocationPage* page1 = new wxLocationPage(wizard);

    wizard->GetPageAreaSizer()->Add(page1);

    if (wizard->RunWizard(page1) )
    {
        dest.Printf("%s" PATH_SEP "download" PATH_SEP "build-info",
        gv->stdpaths->GetUserDataDir().c_str());
        if (DownloadURL(gv->server_conf_url, dest))
        {
            WARN_DIALOG(_("Unable to download build status."),
                _("Font Install"));
                buf = "";
        } else
        {
            buildinfo = new wxFileConfig(wxEmptyString,
                wxEmptyString, dest);
            buf = buildinfo->Read(wxT("/dailies/date"));
                buildinfo->DeleteAll();

            if (buf.Len() != 8) {
                dest.Printf(_("Invalid build date: %s"), buf.c_str());
                WARN_DIALOG(dest, _("Font Install"));
                buf = "";
            }
        }

        if (buf == "") {
            WARN_DIALOG(_("Can't get date of latest build from "
                "server.  Using yesterday's date."),
                _("Font Install") );
            date = wxDateTime::Now();
            date.Subtract(oneday.Day());
             buf = date.Format(wxT("%Y%m%d")); // yes, we want UTC
        }

        src.Printf("%s%s.zip", gv->font_url.c_str(), buf.c_str() );

        dest.Printf("%s" PATH_SEP "download" PATH_SEP
            "rockbox-fonts-%s.zip", gv->stdpaths->GetUserDataDir().c_str(),
            buf.c_str() );

        if ( ! wxFileExists(dest)  )
        {
            if ( DownloadURL(src, dest) )
            {
                wxRemoveFile(dest);
                buf.Printf(_("Unable to download %s"), src.c_str() );
                ERR_DIALOG(buf, _("Font Install"));
                return;
            }
        }

        if ( !UnzipFile(dest, gv->curdestdir) )
        {
            MESG_DIALOG(_("The Rockbox fonts have been installed on your device.") );
        } else
        {
            wxRemoveFile(dest);
            buf.Printf(_("Unable to unzip %s"), dest.c_str() );
            ERR_DIALOG(buf, _("Font Install"));
        }
    } else
    {
        MESG_DIALOG(_("The font installation wizard was cancelled") );
    }

    wxLogVerbose("=== end rbutilFrm::OnFontBtn");
}

void rbutilFrm::OnRemoveBtn(wxCommandEvent& event)
{
     wxLogVerbose("=== begin rbutilFrm::OnRemoveBtn(event)");

   wxWizard *wizard = new wxWizard(this, wxID_ANY,
                    _("Rockbox Uninstallation Wizard"),
                    wxBitmap(wizard_xpm),
                    wxDefaultPosition,
                    wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    wxLocationPage* page1 = new wxLocationPage(wizard);
    wxFullUninstallPage *page2 = new wxFullUninstallPage(wizard);
    page1->SetNext(page2);
    page2->SetPrev(page1);

    wizard->GetPageAreaSizer()->Add(page1);

    if (wizard->RunWizard(page1) )
    {
        if (Uninstall(gv->curdestdir, gv->curisfull) )
        {
           MESG_DIALOG(
           _("The uninstallation wizard was cancelled or completed with "
             "some errors.") );
        } else {
            MESG_DIALOG(_("The uninstall wizard completed successfully") );
        }
    } else
    {
        MESG_DIALOG(_("The uninstallation wizard was cancelled.") );
    }

    wxLogVerbose("=== end rbutilFrm::OnRemoveBtn");
}

void rbutilFrm::OnPortableInstall(wxCommandEvent& event)
{
    wxString src, dest, buf;
    wxLogVerbose("=== begin rbutilFrm::OnPortableInstall(event)");
    wxFileSystem fs;
    wxFileConfig* buildinfo;
    wxDateSpan oneday;

    wxWizard *wizard = new wxWizard(this, wxID_ANY,
                    _("Rockbox Utility Portable Installation Wizard"),
                    wxBitmap(wizard_xpm),
                    wxDefaultPosition,
                    wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    wxLocationPage* page1 = new wxLocationPage(wizard);

    wizard->GetPageAreaSizer()->Add(page1);

    if (wizard->RunWizard(page1) )
    {
        if ( InstallRbutil(gv->curdestdir) )
        {
            MESG_DIALOG(_("The Rockbox Utility has been installed on your device.") );
        } else
        {
            ERR_DIALOG(_("Installation failed"), _("Portable Install"));
        }
    } else
    {
        MESG_DIALOG(_("The portable installation wizard was cancelled") );
    }

    wxLogVerbose("=== end rbutilFrm::OnUnstallPortable");
}

AboutDlg::AboutDlg(rbutilFrm* parent)
    : wxDialog(parent, -1, _("About"), wxDefaultPosition, wxDefaultSize,
    wxDEFAULT_DIALOG_STYLE)
{
    wxBoxSizer* WxBoxSizer1 = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(WxBoxSizer1);
    this->SetAutoLayout(TRUE);

    wxBitmap WxBitmap1 = wxBitmap(rblogo_xpm);
    wxStaticBitmap* WxStaticBitmap1 = new wxStaticBitmap(this, wxID_ANY,
        WxBitmap1);
    WxBoxSizer1->Add(WxStaticBitmap1, 0, wxALL, 5);

    wxStaticText* WxStaticText1 = new wxStaticText(this, wxID_ANY,
        _(RBUTIL_FULLNAME "\n" RBUTIL_VERSION "\n" RBUTIL_DESCRIPTION "\n\n"
            RBUTIL_COPYRIGHT));
    WxBoxSizer1->Add(WxStaticText1, 0, wxALL, 5);

    wxHyperlinkCtrl* WxHyperlink1 = new wxHyperlinkCtrl(this, wxID_ANY,
        wxT(RBUTIL_WEBSITE), wxT(RBUTIL_WEBSITE) );
    WxBoxSizer1->Add(WxHyperlink1, 0, wxALL, 5);

    wxStdDialogButtonSizer* WxStdDialogButtonSizer1 = new wxStdDialogButtonSizer();
    wxButton* WxOKButton = new wxButton(this, wxID_OK);
    WxStdDialogButtonSizer1->AddButton(WxOKButton);
    WxStdDialogButtonSizer1->Realize();

    WxBoxSizer1->Add(WxStdDialogButtonSizer1, 0, wxALL | wxCENTER, 5);

    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);

//this->Center();
    this->Show();

}

AboutDlg::~AboutDlg()
{
}
