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
#include "untools2_3d.xpm"
#include "themes_3d.xpm"

#include "bootloaders.h"
#include "install_dialogs.h"


//----------------------------------------------------------------------------
// rbutilFrm
//----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(rbutilFrm,wxFrame)
	EVT_BUTTON      (ID_INSTALL_BTN, rbutilFrm::OnInstallBtn)
	EVT_BUTTON      (ID_REMOVE_BTN, rbutilFrm::OnRemoveBtn)
	EVT_BUTTON      (ID_FONT_BTN, rbutilFrm::OnFontBtn)
	EVT_BUTTON      (ID_THEMES_BTN, rbutilFrm::OnThemesBtn)
	EVT_BUTTON      (ID_BOOTLOADER_BTN, rbutilFrm::OnBootloaderBtn)
	EVT_BUTTON      (ID_BOOTLOADERREMOVE_BTN, rbutilFrm::OnBootloaderRemoveBtn)

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

	wxBoxSizer* WxBoxSizer1 = new wxBoxSizer(wxVERTICAL);
	this->SetSizer(WxBoxSizer1);
	this->SetAutoLayout(TRUE);
	
	wxPanel* mainPanel = new wxPanel(this,wxID_ANY);
	WxBoxSizer1->Add(mainPanel,0,wxALL,0);
	wxBoxSizer* WxBoxSizer0 = new wxBoxSizer(wxVERTICAL);
	mainPanel->SetSizer(WxBoxSizer0);
	mainPanel->SetAutoLayout(TRUE);

   wxBitmap rockboxbmp(rblogo_xpm);
   ImageCtrl* rockboxbmpCtrl = new ImageCtrl(mainPanel,wxID_ANY);
   rockboxbmpCtrl->SetBitmap(rockboxbmp);
   WxBoxSizer0->Add(rockboxbmpCtrl,0,wxALIGN_CENTER_HORIZONTAL | wxALL,5);

	myDeviceSelector = new DeviceSelectorCtrl(mainPanel,wxID_ANY);
	myDeviceSelector->setDefault();
	WxBoxSizer0->Add(myDeviceSelector,0,wxALL,5);

   wxNotebook* tabwindow = new wxNotebook(mainPanel,wxID_ANY); 
   WxBoxSizer0->Add(tabwindow,0,wxALL,5);
     
   wxPanel* installpage = new wxPanel(tabwindow,wxID_ANY);
   wxPanel* themepage = new wxPanel(tabwindow,wxID_ANY);
   wxPanel* uninstallpage = new wxPanel(tabwindow,wxID_ANY);
   tabwindow->AddPage(installpage,wxT("Installation"),true);
   tabwindow->AddPage(themepage,wxT("Themes"));
   tabwindow->AddPage(uninstallpage,wxT("Uninstallation"));
   
	/*********************
	Install Page
	***********************/   
   
	wxBoxSizer* WxBoxSizer2 = new wxBoxSizer(wxVERTICAL);
	installpage->SetSizer(WxBoxSizer2);
	installpage->SetAutoLayout(TRUE);
	
	wxStaticBox* WxStaticBoxSizer3_StaticBoxObj = new wxStaticBox(installpage,
        wxID_ANY, wxT("Please choose an option"));
	wxStaticBoxSizer* WxStaticBoxSizer3 =
        new wxStaticBoxSizer(WxStaticBoxSizer3_StaticBoxObj,wxHORIZONTAL);
	WxBoxSizer2->Add(WxStaticBoxSizer3,1,wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

	wxFlexGridSizer* WxFlexGridSizer1 = new wxFlexGridSizer(2,2,0,0);
	WxStaticBoxSizer3->Add(WxFlexGridSizer1,0,wxGROW | wxALL,0);

   wxBitmap BootloaderInstallButton (tools2_3d_xpm);
   WxBitmapButton4 = new wxBitmapButton(installpage, ID_BOOTLOADER_BTN,
        BootloaderInstallButton, wxPoint(0,0), wxSize(64,54),
        wxRAISED_BORDER | wxBU_AUTODRAW);
   WxBitmapButton4->SetToolTip(wxT("Instructions for installing the "
        "Rockbox bootloader on your audio device"));
   WxFlexGridSizer1->Add(WxBitmapButton4, 0,
        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxALL,5);

   wxStaticText* WxStaticText5 =  new wxStaticText(installpage, wxID_ANY,
        wxT("Bootloader installation instructions\n\n"
        "Before Rockbox can be installed on your audio player, you "
        "may have to\ninstall a bootloader.  This can not currently "
        "be done automatically, but is\nonly necessary the first time "
        "Rockbox is installed."));
	WxFlexGridSizer1->Add(WxStaticText5, 0,
        wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxALL,5);

	wxBitmap WxBitmapButton1_BITMAP (install_3d_xpm);
	WxBitmapButton1 = new wxBitmapButton(installpage, ID_INSTALL_BTN,
        WxBitmapButton1_BITMAP, wxPoint(0,0), wxSize(64,54),
        wxRAISED_BORDER | wxBU_AUTODRAW, wxDefaultValidator,
        wxT("WxBitmapButton1"));
	WxBitmapButton1->SetToolTip(wxT("Install Rockbox"));
	WxFlexGridSizer1->Add(WxBitmapButton1,0,
        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxALL,5);

	WxStaticText2 = new wxStaticText(installpage, ID_WXSTATICTEXT2,
        wxT("Install Rockbox on your audio player"));
	WxFlexGridSizer1->Add(WxStaticText2,0,
        wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxALL,5);

/* ********************+
	Theme Page
	***********************/

	wxBoxSizer* WxBoxSizer3 = new wxBoxSizer(wxVERTICAL);
	themepage->SetSizer(WxBoxSizer3);
	themepage->SetAutoLayout(TRUE);
	
	wxStaticBox* WxStaticBoxSizer4_StaticBoxObj = new wxStaticBox(themepage,
        wxID_ANY, wxT("Please choose an option"));
	wxStaticBoxSizer* WxStaticBoxSizer4 =
        new wxStaticBoxSizer(WxStaticBoxSizer4_StaticBoxObj,wxHORIZONTAL);
	WxBoxSizer3->Add(WxStaticBoxSizer4,1,wxALIGN_CENTER_HORIZONTAL | wxALL, 5);
	
	wxFlexGridSizer* WxFlexGridSizer2 = new wxFlexGridSizer(2,2,0,0);
	WxStaticBoxSizer4->Add(WxFlexGridSizer2,0,wxGROW | wxALL,0);

    wxBitmap FontInstallButton (fonts_3d_xpm);
    WxBitmapButton3 = new wxBitmapButton(themepage, ID_FONT_BTN,
        FontInstallButton, wxPoint(0,0), wxSize(64,54),
        wxRAISED_BORDER | wxBU_AUTODRAW);
    WxBitmapButton3->SetToolTip(wxT("Download the most up to date "
        "Rockbox fonts."));
    WxFlexGridSizer2->Add(WxBitmapButton3, 0,
        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxALL,5);

    wxStaticText* WxStaticText4 =  new wxStaticText(themepage, wxID_ANY,
        wxT("Install the Rockbox fonts package\n\n"
        "This step is only needed if you have installed "
        "a daily build and want\nthe full set of Rockbox fonts.  You "
        "will not need to download these\nagain unless you uninstall "
        "Rockbox."));
	WxFlexGridSizer2->Add(WxStaticText4, 0,
        wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxALL,5);

    wxBitmap ThemesInstallButton (themes_3d_xpm);
    WxBitmapButton5 = new wxBitmapButton(themepage, ID_THEMES_BTN,
        ThemesInstallButton, wxPoint(0,0), wxSize(64,54),
        wxRAISED_BORDER | wxBU_AUTODRAW);
    WxBitmapButton5->SetToolTip(wxT("Download other Themes for Rockbox."));
    WxFlexGridSizer2->Add(WxBitmapButton5, 0,
        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxALL,5);

    wxStaticText* WxStaticText6 =  new wxStaticText(themepage, wxID_ANY,
        wxT("Install more Themes for Rockbox.\n\n"));
	WxFlexGridSizer2->Add(WxStaticText6, 0,
        wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxALL,5);
        
   /* ********************+
	Uninstall Page
	***********************/

	wxBoxSizer* WxBoxSizer4 = new wxBoxSizer(wxVERTICAL);
	uninstallpage->SetSizer(WxBoxSizer4);
	uninstallpage->SetAutoLayout(TRUE);	
	
	wxStaticBox* WxStaticBoxSizer5_StaticBoxObj = new wxStaticBox(uninstallpage,
        wxID_ANY, wxT("Please choose an option"));
	wxStaticBoxSizer* WxStaticBoxSizer5 =
        new wxStaticBoxSizer(WxStaticBoxSizer5_StaticBoxObj,wxHORIZONTAL);
	WxBoxSizer4->Add(WxStaticBoxSizer5,1,wxALIGN_CENTER_HORIZONTAL | wxALL, 5);
	
	wxFlexGridSizer* WxFlexGridSizer3 = new wxFlexGridSizer(2,2,0,0);
	WxStaticBoxSizer5->Add(WxFlexGridSizer3,0,wxGROW | wxALL,0);
        
	wxBitmap WxBitmapButton2_BITMAP (uninstall_3d_xpm);
	WxBitmapButton2 = new wxBitmapButton(uninstallpage, ID_REMOVE_BTN,
        WxBitmapButton2_BITMAP, wxPoint(0,0), wxSize(64,54),
        wxRAISED_BORDER | wxBU_AUTODRAW, wxDefaultValidator,
        wxT("WxBitmapButton2"));
	WxBitmapButton2->SetToolTip(wxT("Uninstall Rockbox"));
	WxFlexGridSizer3->Add(WxBitmapButton2,0,
        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxALL,5);

	WxStaticText3 = new wxStaticText(uninstallpage, ID_WXSTATICTEXT3,
        wxT("Remove Rockbox from your audio player"));
	WxFlexGridSizer3->Add(WxStaticText3,0,
        wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxALL,5);

    wxBitmap WxBitmapButton4_BITMAP (untools2_3d_xpm);
	WxBitmapButton4 = new wxBitmapButton(uninstallpage, ID_BOOTLOADERREMOVE_BTN,
        WxBitmapButton4_BITMAP, wxPoint(0,0), wxSize(64,54),
        wxRAISED_BORDER | wxBU_AUTODRAW, wxDefaultValidator,
        wxT("WxBitmapButton4"));
	WxBitmapButton4->SetToolTip(wxT("Uninstall Bootloader"));
	WxFlexGridSizer3->Add(WxBitmapButton4,0,
        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxALL,5);

	WxStaticText4 = new wxStaticText(uninstallpage, ID_WXSTATICTEXT4,
        wxT("Remove Rockbox Bootloader from your audio player"));
	WxFlexGridSizer3->Add(WxStaticText4,0,
        wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxALL,5);

	WxMenuBar1 =  new wxMenuBar();
	wxMenu *ID_FILE_MENU_Mnu_Obj = new wxMenu(0);
	WxMenuBar1->Append(ID_FILE_MENU_Mnu_Obj, wxT("&File"));

    ID_FILE_MENU_Mnu_Obj->Append(ID_FILE_WIPECACHE,
        wxT("&Empty local download cache"), wxT(""), wxITEM_NORMAL);
    if (! gv->portable )
    {
        ID_FILE_MENU_Mnu_Obj->Append(ID_PORTABLE_INSTALL,
            wxT("&Install Rockbox Utility on device"), wxT(""), wxITEM_NORMAL);
    }
    ID_FILE_MENU_Mnu_Obj->Append(ID_FILE_ABOUT, wxT("&About"), wxT(""),
        wxITEM_NORMAL);
    ID_FILE_MENU_Mnu_Obj->Append(ID_FILE_EXIT, wxT("E&xit\tCtrl+X"), wxT(""),
        wxITEM_NORMAL);

	this->SetMenuBar(WxMenuBar1);
	Layout();
	GetSizer()->Fit(this);
	GetSizer()->SetSizeHints(this);
	if (gv->portable)
	{
	    this->SetTitle(wxT("Rockbox Utility (portable)"));
	} else
	{
        this->SetTitle(wxT("Rockbox Utility"));
	}
	this->Center();
	wxIcon rbutilFrm_ICON (rbutilFrm_XPM);
	this->SetIcon(rbutilFrm_XPM);
	this->SetToolTip(wxT("Install Rockbox"));
	
    wxLogVerbose(wxT("=== end rbutilFrm::CreateGUIControls"));
}

void rbutilFrm::rbutilFrmClose(wxCloseEvent& event)
{
    wxLogVerbose(wxT("=== begin rbutilFrm::rbutilFrmClose(event)"));
    Destroy();
    wxLogVerbose(wxT("=== end rbutilFrm::rbutilFrmClose"));
}



/*
 * OnFileExit
 */
void rbutilFrm::OnFileExit(wxCommandEvent& event)
{
    wxLogVerbose(wxT("=== begin rbutilFrm::OnFileExit(event)"));
	Close();
	wxLogVerbose(wxT("=== end rbutilFrm::OnFileExit"));
}

// The routines this code uses are in the wxWidgets documentation, but
// not yet the library (2.7.0-1).  This code can be re-enabled later.

void rbutilFrm::OnFileAbout(wxCommandEvent& event)
{
/*
    wxAboutDialogInfo *info = new wxAboutDialogInfo();

    info->SetName(wxT(RBUTIL_FULLNAME));
    info->SetVersion(wxT(RBUTIL_VERSION));
    info->SetCopyright(wxT(RBUTIL_COPYRIGHT));
    info->SetDescription(wxT(RBUTIL_DESCRIPTION));
    info->SetWebSite(wxT(RBUTIL_WEBSITE));

    long i = 0;
    while (rbutil_developers[i] != "")
    {
        info->AddDeveloper(wxT(rbutil_developers[i++]));
    }

    wxAboutBox(*info);
    delete info;
*/

    AboutDlg(this).ShowModal();
}

void rbutilFrm::OnFileWipeCache(wxCommandEvent& event)
{
    wxString cacheloc, datadir;

    datadir = gv->stdpaths->GetUserDataDir();
    if (datadir == wxT(""))
    {
        ERR_DIALOG(wxT("Can't locate user data directory.  Unable to delete "
            "cache."), wxT("Delete download cache.") );
        return;
    }

    cacheloc.Printf(wxT("%s" PATH_SEP "download"),
        datadir.c_str());

    if (! rm_rf(cacheloc) )
    {
        wxMessageDialog* msg = new wxMessageDialog(this, wxT("Local download cache has been deleted.")
                            , wxT("Cache deletion"), wxOK |wxICON_INFORMATION);
        msg->ShowModal();
        delete msg;
    }
    else {
        MESG_DIALOG(wxT("Errors occured deleting the local download cache."));
    }

    wxMkdir(cacheloc, 0777);
}

void rbutilFrm::OnBootloaderRemoveBtn(wxCommandEvent& event)
{
    wxLogVerbose(wxT("=== begin rbutilFrm::OnBootloaderRemoveBtn(event)"));

    bootloaderDeInstallDlg dialog(NULL, wxID_ANY,
        wxT("Bootloader Deinstallation"));

    if (dialog.ShowModal() == wxID_OK)
    {
        int index = gv->plat_id.Index(gv->curplat);
        wxString bootloadermethod = gv->plat_bootloadermethod[index];

        if(bootloadermethod == wxT("ipodpatcher"))
        {
            wxString bootloadername = wxT("bootloader-");
            bootloadername.Append(gv->plat_bootloadername[index] );
            if(ipodpatcher(BOOTLOADER_REM,bootloadername))
            {
                MESG_DIALOG(wxT("The Bootloader has been uninstalled.") );
            }
            else
            {
                MESG_DIALOG(wxT("The Uninstallation failed.") );
            }
        }
        else if(bootloadermethod== wxT("gigabeatf"))
        {

            if(gigabeatf(BOOTLOADER_REM,gv->plat_bootloadername[index],gv->curdestdir))
            {
               MESG_DIALOG(wxT("The Bootloader has been uninstalled."));
            }
            else
                MESG_DIALOG(wxT("The Uninstallation failed.") );
        }
        else if(bootloadermethod == wxT("iaudio") )
        {
           MESG_DIALOG(wxT("To uninstall the Bootloader on this Device,\n"
                    "you need to download and install an Original Firmware from the Manufacturer."));
        }
        else if(bootloadermethod == wxT("fwpatcher"))
        {
           MESG_DIALOG(wxT("To uninstall the Bootloader on this Device,\n"
                         "you need to download and install an original Firmware from the Manufacturer.\n"
                         "To do this, you need to boot into the original Firmware."));
        }
        else if(bootloadermethod == wxT("h10"))
        {
            if(h10(BOOTLOADER_REM,gv->plat_bootloadername[index],gv->curdestdir))
            {
               MESG_DIALOG(wxT("The Bootloader has been uninstalled."));
            }
            else
              MESG_DIALOG(wxT("The Uninstallation failed.") );
        }
        else
        {
            MESG_DIALOG(wxT("Unsupported Bootloader Uninstall method.") );
        }
    }


    wxLogVerbose(wxT("=== end rbutilFrm::OnBootloaderRemoveBtn"));
}

void rbutilFrm::OnBootloaderBtn(wxCommandEvent& event)
{
    wxLogVerbose(wxT("=== begin rbutilFrm::OnBootloaderBtn(event)"));

    bootloaderInstallDlg dialog(NULL, wxID_ANY,
        wxT("Bootloader Installation"));

    if (dialog.ShowModal() == wxID_OK)
    {
        int index = gv->plat_id.Index(gv->curplat);
        wxString bootloadermethod = gv->plat_bootloadermethod[index];

        if(bootloadermethod == wxT("ipodpatcher"))
        {
            wxString bootloadername = wxT("bootloader-");
            bootloadername.Append(gv->plat_bootloadername[index] );
            if(ipodpatcher(BOOTLOADER_ADD,bootloadername))
            {
                MESG_DIALOG(wxT("The Bootloader has been installed on your device.") );
            }
            else
            {
                MESG_DIALOG(wxT("The installation has failed.") );
            }
        }
        else if(bootloadermethod== wxT("gigabeatf"))
        {

            if(gigabeatf(BOOTLOADER_ADD,gv->plat_bootloadername[index],gv->curdestdir))
            {
               MESG_DIALOG(wxT("The Bootloader has been installed on your device."));
            }
            else
                MESG_DIALOG(wxT("The installation has failed.") );
        }
        else if(bootloadermethod == wxT("iaudio") )
        {
            if(iaudiox5(BOOTLOADER_ADD,gv->plat_bootloadername[index],gv->curdestdir))
            {
                MESG_DIALOG(wxT("The Bootloader has been installed on your device.\n"
                             "Now turn OFF your Device, unplug USB,and insert Charger\n"
                             "Your Device will automatically upgrade the flash with the Rockbox bootloader"));
            }
            else
                MESG_DIALOG(wxT("The installation has failed.") );
        }
        else if(bootloadermethod == wxT("fwpatcher"))
        {
            if(fwpatcher(BOOTLOADER_ADD,gv->plat_bootloadername[index],gv->curdestdir,gv->curfirmware))
            {
              MESG_DIALOG(wxT("The Bootloader has been patched and copied on your device.\n"
                             "Now use the Firmware upgrade option of your Device\n"));
            }
            else
                MESG_DIALOG(wxT("The installation has failed.") );
        }
        else if(bootloadermethod == wxT("h10"))
        {
            if(h10(BOOTLOADER_ADD,gv->plat_bootloadername[index],gv->curdestdir))
            {
               MESG_DIALOG(wxT("The Bootloader has been patched and copied on your device.\n"));
            }
            else
              MESG_DIALOG(wxT("The installation has failed.") );
        }
        else
        {
            MESG_DIALOG(wxT("Unsupported Bootloader Install method.") );
        }
    }


    wxLogVerbose(wxT("=== end rbutilFrm::OnBootloaderBtn"));

}

void rbutilFrm::OnInstallBtn(wxCommandEvent& event)
{
    wxString src, dest, buf;
    wxDateTime date;
    wxTimeSpan day(24);
    wxLogVerbose(wxT("=== begin rbutilFrm::OnInstallBtn(event)"));
    wxFileSystem fs;
    wxFileConfig* buildinfo;
    wxDateSpan oneday;

    rockboxInstallDlg dialog(NULL, wxID_ANY,
        wxT("Rockbox Installation"));

    if (dialog.ShowModal() == wxID_OK)
    {
        switch (gv->curbuild)
        {
            case BUILD_RELEASE:
                // This is a URL - don't use PATH_SEP
                src.Printf(wxT("%s%s-%s-%s.zip"),
                    gv->download_url.c_str(), gv->prog_name.c_str(),
                    gv->last_release.c_str(), gv->curplat.c_str());
                dest.Printf(wxT("%s" PATH_SEP "download" PATH_SEP "%s-%s-%s.zip"),
                    gv->stdpaths->GetUserDataDir().c_str(),
                    gv->prog_name.c_str(), gv->last_release.c_str(),
                    gv->curplat.c_str());
                break;
            case BUILD_DAILY:
                dest.Printf(wxT("%s" PATH_SEP "download" PATH_SEP "build-info"),
                    gv->stdpaths->GetUserDataDir().c_str());
                if (DownloadURL(gv->server_conf_url, dest)) {
                    WARN_DIALOG(wxT("Unable to download build status."),
                    wxT("Install"));
                    buf = wxT("");
                } else
                {
                    buildinfo = new wxFileConfig(wxEmptyString,
                        wxEmptyString, dest);
                    buf = buildinfo->Read(wxT("/dailies/date"));
                    buildinfo->DeleteAll();

                    if (buf.Len() != 8) {
                        dest.Printf(wxT("Invalid build date: %s"), buf.c_str());
                        WARN_DIALOG(dest, wxT("Install"));
                        buf = wxT("");
                    }
                }

                if (buf == wxT("")) {
                    WARN_DIALOG(wxT("Can't get date of latest build from "
                        "server.  Using yesterday's date."), wxT("Install") );
                    date = wxDateTime::Now();
                    date.Subtract(oneday.Day());
                    buf = date.Format(wxT("%Y%m%d")); // yes, we want UTC
                }

                src.Printf(wxT("%s%s/%s-%s-%s.zip"),
                    gv->daily_url.c_str(), gv->curplat.c_str(),
                    gv->prog_name.c_str(), gv->curplat.c_str(),
                    buf.c_str() );

                dest.Printf(wxT("%s" PATH_SEP "download" PATH_SEP "%s-%s-%s.zip"),
                    gv->stdpaths->GetUserDataDir().c_str(),
                    gv->prog_name.c_str(),
                    gv->curplat.c_str(), buf.c_str() );
                break;
            case BUILD_BLEEDING:
                 src.Printf(wxT("%s%s/%s.zip"),
                    gv->bleeding_url.c_str(), gv->curplat.c_str(),
                    gv->prog_name.c_str() );
                dest.Printf(wxT("%s" PATH_SEP "download" PATH_SEP "%s.zip"),
                    gv->stdpaths->GetUserDataDir().c_str(),
                    gv->prog_name.c_str() );
                break;
            default:
                ERR_DIALOG(wxT("Something seriously odd has happened."),
                    wxT("Install"));
                return;
                break;
        }

        if (gv->nocache || ( ! wxFileExists(dest) ) )
        {
            if ( DownloadURL(src, dest) )
            {
                wxRemoveFile(dest);
                buf.Printf(wxT("Unable to download %s"), src.c_str() );
                ERR_DIALOG(buf, wxT("Install"));
                return;
            }
        }

        if ( !UnzipFile(dest, gv->curdestdir, true) )
        {
            wxMessageDialog* msg = new wxMessageDialog(this, wxT("Rockbox has been installed on your device.")
                            ,wxT("Installation"), wxOK |wxICON_INFORMATION);
            msg->ShowModal();
            delete msg;
        } else
        {
            wxRemoveFile(dest);
            buf.Printf(wxT("Unable to unzip %s"), dest.c_str() );
            ERR_DIALOG(buf, wxT("Install"));
        }
    }

    wxLogVerbose(wxT("=== end rbutilFrm::OnInstallBtn"));
}

void rbutilFrm::OnFontBtn(wxCommandEvent& event)
{
    wxString src, dest, buf;
    wxDateTime date;
    wxTimeSpan day(24);
    wxLogVerbose(wxT("=== begin rbutilFrm::OnFontBtn(event)"));
    wxFileSystem fs;
    wxFileConfig* buildinfo;
    wxDateSpan oneday;

    fontInstallDlg dialog(NULL, wxID_ANY,
        wxT("Font Installation"));

    if (dialog.ShowModal() == wxID_OK)
    {

        buf.Printf(wxT("%s" PATH_SEP ".rockbox"), gv->curdestdir.c_str()) ;
        if (! wxDirExists(buf) )
        {
            buf.Printf(wxT("Rockbox is not yet installed on %s - install "
                "Rockbox first."), buf.c_str() );
            WARN_DIALOG(buf, wxT("Can't install fonts") );
            return;
        }

        dest.Printf(wxT("%s" PATH_SEP "download" PATH_SEP "build-info"),
        gv->stdpaths->GetUserDataDir().c_str());
        if (DownloadURL(gv->server_conf_url, dest))
        {
            WARN_DIALOG(wxT("Unable to download build status."),
                wxT("Font Install"));
                buf = wxT("");
        } else
        {
            buildinfo = new wxFileConfig(wxEmptyString,
                wxEmptyString, dest);
            buf = buildinfo->Read(wxT("/dailies/date"));
                buildinfo->DeleteAll();

            if (buf.Len() != 8) {
                dest.Printf(wxT("Invalid build date: %s"), buf.c_str());
                WARN_DIALOG(dest, wxT("Font Install"));
                buf = wxT("");
            }
        }

        if (buf == wxT("")) {
            WARN_DIALOG(wxT("Can't get date of latest build from "
                "server.  Using yesterday's date."),
                wxT("Font Install") );
            date = wxDateTime::Now();
            date.Subtract(oneday.Day());
             buf = date.Format(wxT("%Y%m%d")); // yes, we want UTC
        }

        src.Printf(wxT("%s%s.zip"), gv->font_url.c_str(), buf.c_str() );

        dest.Printf(wxT("%s" PATH_SEP "download" PATH_SEP
            "rockbox-fonts-%s.zip"), gv->stdpaths->GetUserDataDir().c_str(),
            buf.c_str() );

        if ( ! wxFileExists(dest)  )
        {
            if ( DownloadURL(src, dest) )
            {
                wxRemoveFile(dest);
                buf.Printf(wxT("Unable to download %s"), src.c_str() );
                ERR_DIALOG(buf, wxT("Font Install"));
                return;
            }
        }

        if ( !UnzipFile(dest, gv->curdestdir, true) )
        {
            wxMessageDialog* msg = new wxMessageDialog(this, wxT("The Rockbox fonts have been installed on your device.")
                            ,wxT("Installation"), wxOK |wxICON_INFORMATION);
            msg->ShowModal();
            delete msg;
        } else
        {
            wxRemoveFile(dest);
            buf.Printf(wxT("Unable to unzip %s"), dest.c_str() );
            ERR_DIALOG(buf, wxT("Font Install"));
        }
    }
    wxLogVerbose(wxT("=== end rbutilFrm::OnFontBtn"));
}


void rbutilFrm::OnThemesBtn(wxCommandEvent& event)
{
    wxString src, dest, buf;
    wxLogVerbose(wxT("=== begin rbutilFrm::OnThemesBtn(event)"));


    themesInstallDlg dialog(NULL, wxID_ANY,
        wxT("Theme Installation"));

    if (dialog.ShowModal() == wxID_OK)
    {
        bool success=true;
        for(unsigned int i=0 ;i < gv->themesToInstall.GetCount();i++)
        {
            if(!InstallTheme(gv->themesToInstall[i]))
            {
                MESG_DIALOG(wxT("The Themes installation has failed") );
                success=false;
                break;
            }
        }
        if(success)
        {
            MESG_DIALOG(wxT("The Theme installation completed successfully.") );
        }
    }


     wxLogVerbose(wxT("=== end rbutilFrm::OnThemesBtn(event)"));
}


void rbutilFrm::OnRemoveBtn(wxCommandEvent& event)
{
    wxLogVerbose(wxT("=== begin rbutilFrm::OnRemoveBtn(event)"));

    rockboxDeInstallDlg dialog(NULL, wxID_ANY,
        wxT("Rockbox Deinstallation"));

    if (dialog.ShowModal() == wxID_OK)
    {
        if (Uninstall(gv->curdestdir, gv->curisfull) )
        {
           MESG_DIALOG(
           wxT("The uninstallation wizard was cancelled or completed with "
             "some errors.") );
        } else {
            wxMessageDialog* msg = new wxMessageDialog(this, wxT("The uninstall wizard completed successfully\n"
                          "Depending on which Device you own, you also have to uninstall the Bootloader")
                            ,wxT("Uninstallation"), wxOK |wxICON_INFORMATION);
            msg->ShowModal();
            delete msg;
        }
    }

    wxLogVerbose(wxT("=== end rbutilFrm::OnRemoveBtn"));
}

void rbutilFrm::OnPortableInstall(wxCommandEvent& event)
{
    wxString src, dest, buf;
    wxLogVerbose(wxT("=== begin rbutilFrm::OnPortableInstall(event)"));
    wxFileSystem fs;
    wxDateSpan oneday;

    fontInstallDlg dialog(NULL, wxID_ANY,
        wxT("Rockbox Utility Portable Installation"));

    if (dialog.ShowModal() == wxID_OK)
    {
        if ( InstallRbutil(gv->curdestdir) )
        {
            MESG_DIALOG(wxT("The Rockbox Utility has been installed on your device."));

        } else
        {
            ERR_DIALOG(wxT("Installation failed"), wxT("Portable Install"));
        }
    }

    wxLogVerbose(wxT("=== end rbutilFrm::OnUnstallPortable"));
}

AboutDlg::AboutDlg(rbutilFrm* parent)
    : wxDialog(parent, -1, wxT("About"), wxDefaultPosition, wxDefaultSize,
    wxDEFAULT_DIALOG_STYLE)
{
    wxBoxSizer* WxBoxSizer1 = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(WxBoxSizer1);
    this->SetAutoLayout(TRUE);

    wxBoxSizer* WxBoxSizer2 = new wxBoxSizer(wxHORIZONTAL);

    wxBitmap WxBitmap1 = wxBitmap(rbutilFrm_XPM);
    wxStaticBitmap* WxStaticBitmap1 = new wxStaticBitmap(this, wxID_ANY,
        WxBitmap1);
    WxBoxSizer2->Add(WxStaticBitmap1, 0, wxALL | wxCENTER, 5);

    wxStaticText* WxStaticText1 = new wxStaticText(this, wxID_ANY,
        wxT(RBUTIL_FULLNAME), wxDefaultPosition, wxDefaultSize,
        wxALIGN_CENTER | wxST_NO_AUTORESIZE );
    WxBoxSizer2->Add(WxStaticText1, 0, wxALL | wxCENTER, 5);
    WxBoxSizer1->Add(WxBoxSizer2, 0, wxALL, 5);

    wxStaticText* WxStaticText2 = new wxStaticText(this, wxID_ANY,
        wxT(RBUTIL_VERSION "\n" RBUTIL_DESCRIPTION "\n\n" RBUTIL_COPYRIGHT));
    WxStaticText2->Wrap(400);
    WxBoxSizer1->Add(WxStaticText2, 0, wxALL, 5);

    wxHyperlinkCtrl* WxHyperlink1 = new wxHyperlinkCtrl(this, wxID_ANY,
        wxT(RBUTIL_WEBSITE), wxT(RBUTIL_WEBSITE) );
    WxBoxSizer1->Add(WxHyperlink1, 0, wxALL, 5);

    wxStaticBox* WxStaticBox1 = new wxStaticBox(this, wxID_ANY, wxT("Contributors:"));
    wxStaticBoxSizer* WxStaticBoxSizer2 = new wxStaticBoxSizer(WxStaticBox1,
        wxVERTICAL);
    wxTextCtrl* WxTextCtrl1 = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);

    long i = 0;
    while ( rbutil_developers[i] != wxT(""))
    {
        WxTextCtrl1->AppendText(rbutil_developers[i++]);
        WxTextCtrl1->AppendText(wxT("\n"));
    }

    WxBoxSizer1->Add(WxStaticBoxSizer2, 1, wxGROW | wxALL, 5);
    WxStaticBoxSizer2->Add(WxTextCtrl1, 1, wxGROW | wxALL, 0);

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
