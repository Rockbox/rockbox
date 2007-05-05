
#include "install_dialogs.h"
#include "wizard.xpm"



////////////////////////////////////////////////
//// Bootloader Installation
/////////////////////////////////////////////////

IMPLEMENT_CLASS( bootloaderInstallDlg, wxDialog )

BEGIN_EVENT_TABLE( bootloaderInstallDlg, wxDialog )

END_EVENT_TABLE()

bootloaderInstallDlg::bootloaderInstallDlg( )
{
    Init();
}

bootloaderInstallDlg::bootloaderInstallDlg( wxWindow* parent,
    wxWindowID id, const wxString& caption,
    const wxPoint& pos, const wxSize& size, long style )
{
    Init();
    Create(parent, id, caption, pos, size, style);
}

void bootloaderInstallDlg::CreateControls()
{
    // A top-level sizer
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(topSizer);

    wxBoxSizer* wxBoxSizer2 = new wxBoxSizer(wxHORIZONTAL);
    topSizer->Add(wxBoxSizer2, 0, wxALIGN_LEFT|wxALL, 5);

    // bitmap
    wxBitmap sidebmp(wizard_xpm);
    ImageCtrl* sideimage = new ImageCtrl(this,wxID_ANY);
    sideimage->SetBitmap(sidebmp);
    wxBoxSizer2->Add(sideimage,0,wxALIGN_LEFT | wxALL,5);

    wxBoxSizer* wxBoxSizer3 = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer2->Add(wxBoxSizer3, 0, wxALIGN_LEFT|wxALL, 5);

    m_devicepos = new DevicePositionCtrl(this,ID_DEVICEPOS);
    wxBoxSizer3->Add(m_devicepos, 0, wxALIGN_LEFT|wxALL, 5);

    m_firmwarepos = new FirmwarePositionCtrl(this,ID_FIRMWARE);
    wxBoxSizer3->Add(m_firmwarepos, 0, wxALIGN_LEFT|wxALL, 5);

    OkCancelCtrl* okCancel = new OkCancelCtrl(this,wxID_ANY);
    topSizer->Add(okCancel, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

}

//init the local variables
void bootloaderInstallDlg::Init()
{

}

// create the window
bool bootloaderInstallDlg::Create( wxWindow* parent,
    wxWindowID id, const wxString& caption,
    const wxPoint& pos, const wxSize& size, long style )
{

    if (!wxDialog::Create( parent, id, caption, pos, size, style ))
        return false;
    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
    return true;
}



// tranver data from the controls
bool bootloaderInstallDlg::TransferDataFromWindow()
{

    if( m_devicepos->IsShown())
    {
        gv->curdestdir = m_devicepos->getDevicePos();
        if(!wxDirExists(gv->curdestdir))
        {
            WARN_DIALOG(wxT("The Devicepostion is not valid"),
                wxT("Select a Deviceposition"));
            gv->curdestdir = wxT("");
            return false;
        }
    }

    if(m_firmwarepos->IsShown())
    {
       gv->curfirmware = m_firmwarepos->getFirmwarePos();
        if(!wxFileExists(gv->curfirmware))
        {
            WARN_DIALOG(wxT("The Firmware position is not valid"),
                wxT("Select a original Firmware"));
            gv->curfirmware = wxT("");
            return false;
        }
    }


}

// tranver data to the controls
bool bootloaderInstallDlg::TransferDataToWindow()
{
    if(gv->curplat == wxT(""))
    {
        WARN_DIALOG(wxT("You have not selected a audio device"),
                wxT("Select a Device"));
        return false;
    }
	 int index = gv->plat_id.Index(gv->curplat);

    if(!gv->plat_needsbootloader[index])
    {
    	 WARN_DIALOG(wxT("This Device doesnt need a Bootloader"),
                wxT("Bootloader"));
        return false;
    }

    if(gv->plat_bootloadermethod[index] != wxT("ipodpatcher") && gv->plat_bootloadermethod[index] != wxT("sansapatcher"))
    {
        m_devicepos->Show(true);
    }else
    {
        m_devicepos->Show(false);
    }
    if(gv->plat_bootloadermethod[index] == wxT("fwpatcher"))
    {
        m_firmwarepos->Show(true);
    }else
    {
        m_firmwarepos->Show(false);
    }

    m_devicepos->setDefault();
    m_firmwarepos->setDefault();
    return true;
}


////////////////////////////////////////////////
//// Font Installation
/////////////////////////////////////////////////


IMPLEMENT_CLASS( fontInstallDlg, wxDialog )

BEGIN_EVENT_TABLE( fontInstallDlg, wxDialog )

END_EVENT_TABLE()

fontInstallDlg::fontInstallDlg( )
{
    Init();
}

fontInstallDlg::fontInstallDlg( wxWindow* parent,
    wxWindowID id, const wxString& caption,
    const wxPoint& pos, const wxSize& size, long style )
{
    Init();
    Create(parent, id, caption, pos, size, style);
}

void fontInstallDlg::CreateControls()
{
    // A top-level sizer
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(topSizer);

    wxBoxSizer* wxBoxSizer2 = new wxBoxSizer(wxHORIZONTAL);
    topSizer->Add(wxBoxSizer2, 0, wxALIGN_LEFT|wxALL, 5);

    // bitmap
    wxBitmap sidebmp(wizard_xpm);

    ImageCtrl* sideimage = new ImageCtrl(this,wxID_ANY);
    sideimage->SetBitmap(sidebmp);
    wxBoxSizer2->Add(sideimage,0,wxALIGN_LEFT | wxALL,5);

    wxBoxSizer* wxBoxSizer3 = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer2->Add(wxBoxSizer3, 0, wxALIGN_LEFT|wxALL, 5);

    m_devicepos = new DevicePositionCtrl(this,ID_DEVICEPOS);
    wxBoxSizer3->Add(m_devicepos, 0, wxALIGN_LEFT|wxALL, 5);


    OkCancelCtrl* okCancel = new OkCancelCtrl(this,wxID_ANY);
    topSizer->Add(okCancel, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    // controls at the bottom
    wxBoxSizer* wxBoxSizer7 = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(wxBoxSizer7, 0, wxGROW | wxALL, 5);

}

//init the local variables
void fontInstallDlg::Init()
{

}

// create the window
bool fontInstallDlg::Create( wxWindow* parent,
    wxWindowID id, const wxString& caption,
    const wxPoint& pos, const wxSize& size, long style )
{

    if (!wxDialog::Create( parent, id, caption, pos, size, style ))
        return false;
    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
    return true;
}


// tranver data from the controls
bool fontInstallDlg::TransferDataFromWindow()
{
    gv->curdestdir = m_devicepos->getDevicePos();
    if(!wxDirExists(gv->curdestdir))
    {
        WARN_DIALOG(wxT("The Devicepostion is not valid"),
            wxT("Select a Deviceposition"));
        gv->curdestdir = wxT("");
        return false;
    }

    return true;
}

// tranver data to the controls
bool fontInstallDlg::TransferDataToWindow()
{
   m_devicepos->setDefault();
   return true;
}

////////////////////////////////////////////////
//// Rockbox DeInstallation
/////////////////////////////////////////////////

IMPLEMENT_CLASS( rockboxDeInstallDlg, wxDialog )

BEGIN_EVENT_TABLE( rockboxDeInstallDlg, wxDialog )

END_EVENT_TABLE()

rockboxDeInstallDlg::rockboxDeInstallDlg( )
{
    Init();
}

rockboxDeInstallDlg::rockboxDeInstallDlg( wxWindow* parent,
    wxWindowID id, const wxString& caption,
    const wxPoint& pos, const wxSize& size, long style )
{
    Init();
    Create(parent, id, caption, pos, size, style);
}

void rockboxDeInstallDlg::CreateControls()
{
    // A top-level sizer
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(topSizer);

    wxBoxSizer* wxBoxSizer2 = new wxBoxSizer(wxHORIZONTAL);
    topSizer->Add(wxBoxSizer2, 0, wxALIGN_LEFT|wxALL, 5);

     // bitmap
    wxBitmap sidebmp(wizard_xpm);

    ImageCtrl* sideimage = new ImageCtrl(this,wxID_ANY);
    sideimage->SetBitmap(sidebmp);
    wxBoxSizer2->Add(sideimage,0,wxALIGN_LEFT | wxALL,5);

    wxBoxSizer* wxBoxSizer3 = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer2->Add(wxBoxSizer3, 0, wxALIGN_LEFT|wxALL, 5);

    m_devicepos = new DevicePositionCtrl(this,ID_DEVICEPOS);
    wxBoxSizer3->Add(m_devicepos, 0, wxALIGN_LEFT|wxALL, 5);

    // Full deinstallation ?
    wxStaticText* WxStaticText1 = new wxStaticText(this, wxID_ANY,
        wxT("Rockbox Utility normally uninstalls Rockbox using an uninstall\n"
          "file created during installation.  This means that when Rockbox is\n"
          "uninstalled all your configuration files are preserved.  However,\n"
          "you can also perform a full uninstall, which will completely\n"
          "remove all traces of Rockbox from your system, and can be used\n"
          "even if Rockbox was previously installed manually."));
    wxBoxSizer3->Add(WxStaticText1,0,wxGROW | wxALL,5);

    wxCheckBox* FullCheckBox = new wxCheckBox(this, ID_FULL_CHCKBX,
        wxT("Perform a full uninstall"));
    wxBoxSizer3->Add(FullCheckBox, 0, wxALL, 5);

   // controls at the bottom
    OkCancelCtrl* okCancel = new OkCancelCtrl(this,wxID_ANY);
    topSizer->Add(okCancel, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

}

//init the local variables
void rockboxDeInstallDlg::Init()
{

}

// create the window
bool rockboxDeInstallDlg::Create( wxWindow* parent,
    wxWindowID id, const wxString& caption,
    const wxPoint& pos, const wxSize& size, long style )
{
    if (!wxDialog::Create( parent, id, caption, pos, size, style ))
        return false;
    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
    return true;
}


// tranver data from the controls
bool rockboxDeInstallDlg::TransferDataFromWindow()
{

    gv->curdestdir = m_devicepos->getDevicePos();
    if(!wxDirExists(gv->curdestdir))
    {
        WARN_DIALOG(wxT("The Devicepostion is not valid"),
            wxT("Select a Deviceposition"));
        gv->curdestdir = wxT("");
        return false;
    }

    wxCheckBox* fullchkbx = (wxCheckBox*) FindWindow(ID_FULL_CHCKBX);
    gv->curisfull = fullchkbx->IsChecked();

    return true;
}

// tranver data to the controls
bool rockboxDeInstallDlg::TransferDataToWindow()
{
    m_devicepos->setDefault();
    return true;
}

////////////////////////////////////////////////
//// Themes Installation
/////////////////////////////////////////////////

IMPLEMENT_CLASS( themesInstallDlg, wxDialog )

BEGIN_EVENT_TABLE( themesInstallDlg, wxDialog )

END_EVENT_TABLE()

themesInstallDlg::themesInstallDlg( )
{

}

themesInstallDlg::themesInstallDlg( wxWindow* parent,
    wxWindowID id, const wxString& caption,
    const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

void themesInstallDlg::CreateControls()
{
    // A top-level sizer
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(topSizer);

    wxBoxSizer* topHoriSizer = new wxBoxSizer(wxHORIZONTAL);
    topSizer->Add(topHoriSizer, 0, wxALIGN_LEFT|wxALL, 5);

      // bitmap
    wxBitmap sidebmp(wizard_xpm);

    ImageCtrl* sideimage = new ImageCtrl(this,wxID_ANY);
    sideimage->SetBitmap(sidebmp);
    topHoriSizer->Add(sideimage,0,wxALIGN_LEFT | wxALL,5);

    wxBoxSizer* mainVertiSizer = new wxBoxSizer(wxVERTICAL);
    topHoriSizer->Add(mainVertiSizer, 0, wxGROW|wxALL, 5);

    wxBoxSizer* wxBoxSizer4 = new wxBoxSizer(wxHORIZONTAL);
    mainVertiSizer->Add(wxBoxSizer4, 0, wxGROW|wxALL, 0);

    wxBoxSizer* wxBoxSizer5 = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer4->Add(wxBoxSizer5, 0, wxGROW|wxALL, 0);

    m_devicepos = new DevicePositionCtrl(this,ID_DEVICEPOS);
    wxBoxSizer5->Add(m_devicepos, 0, wxALIGN_LEFT|wxALL, 5);

    m_theme = new ThemeCtrl(this,ID_THEME);
    wxBoxSizer5->Add(m_theme, 0, wxALIGN_LEFT|wxALL, 5);

     // controls at the bottom
    OkCancelCtrl* okCancel = new OkCancelCtrl(this,wxID_ANY);
    topSizer->Add(okCancel, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);


}


// create the window
bool themesInstallDlg::Create( wxWindow* parent,
    wxWindowID id, const wxString& caption,
    const wxPoint& pos, const wxSize& size, long style )
{

    if (!wxDialog::Create( parent, id, caption, pos, size, style ))
        return false;
    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
    return true;
}


// tranver data from the controls
bool themesInstallDlg::TransferDataFromWindow()
{

    gv->curdestdir = m_devicepos->getDevicePos();
    if(!wxDirExists(gv->curdestdir))
    {
        WARN_DIALOG(wxT("The Devicepostion is not valid"),
            wxT("Select a Deviceposition"));
        gv->curdestdir = wxT("");
        return false;
    }

    gv->themesToInstall.Clear();
    gv->themesToInstall = m_theme->getThemesToInstall();

    if(gv->themesToInstall.GetCount() == 0)
    {
         WARN_DIALOG(wxT("You have not selected a Theme to Install"), wxT("Select a Theme"));
         return false;
    }

    return true;
}

// tranver data to the controls
bool themesInstallDlg::TransferDataToWindow()
{
    if(gv->curplat == wxT(""))
    {
        WARN_DIALOG(wxT("You have not selected a audio device"),
                wxT("Select a Device"));
        return false;
    }

    m_devicepos->setDefault();
    m_theme->setDevice(gv->curplat);
    return true;
}
////////////////////////////////////////////////
//// Rockbox Installation
/////////////////////////////////////////////////

IMPLEMENT_CLASS( rockboxInstallDlg, wxDialog )

BEGIN_EVENT_TABLE( rockboxInstallDlg, wxDialog )
    EVT_RADIOBOX(ID_BUILD_BOX, rockboxInstallDlg::OnBuildBox)
END_EVENT_TABLE()

rockboxInstallDlg::rockboxInstallDlg( )
{
}

rockboxInstallDlg::rockboxInstallDlg( wxWindow* parent,
    wxWindowID id, const wxString& caption,
    const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

void rockboxInstallDlg::CreateControls()
{
    // A top-level sizer
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(topSizer);

    wxBoxSizer* wxBoxSizer2 = new wxBoxSizer(wxHORIZONTAL);
    topSizer->Add(wxBoxSizer2, 0, wxALIGN_LEFT|wxALL, 5);

       // bitmap
    wxBitmap sidebmp(wizard_xpm);

    ImageCtrl* sideimage = new ImageCtrl(this,wxID_ANY);
    sideimage->SetBitmap(sidebmp);
    wxBoxSizer2->Add(sideimage,0,wxALIGN_LEFT | wxALL,5);

    wxBoxSizer* wxBoxSizer3 = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer2->Add(wxBoxSizer3, 0, wxALIGN_LEFT|wxALL, 5);

    m_devicepos = new DevicePositionCtrl(this,ID_DEVICEPOS);
    wxBoxSizer3->Add(m_devicepos, 0, wxALIGN_LEFT|wxALL, 5);

    // Build information
    wxStaticText* WxStaticText1 = new wxStaticText(this, wxID_ANY,
    wxT("Please select the Rockbox version you would like "
        "to install on your audio\ndevice:"));
    wxBoxSizer3->Add(WxStaticText1,0,wxGROW | wxALL,5);

    wxArrayString array;
    wxString buf;
    buf.Printf(wxT("Rockbox stable version (%s)") , gv->last_release.c_str());
    array.Add(buf);
    array.Add(wxT("Archived Build"));
    array.Add(wxT("Current Build "));

    wxRadioBox* BuildRadioBox = new wxRadioBox(this, ID_BUILD_BOX, wxT("Version"),
        wxDefaultPosition, wxDefaultSize, array, 0, wxRA_SPECIFY_ROWS);
    wxBoxSizer3->Add(BuildRadioBox, 0, wxGROW | wxALL, 5);

    wxStaticBox* WxStaticBox1 = new wxStaticBox(this, wxID_ANY, wxT("Details:"));
    wxStaticBoxSizer* WxStaticBoxSizer2 = new wxStaticBoxSizer(WxStaticBox1,
        wxVERTICAL);
    wxStaticText* DetailText = new wxStaticText(this, ID_DETAIL_TXT, wxT(""));
    wxBoxSizer3->Add(WxStaticBoxSizer2, 1, wxGROW | wxALL, 5);
    WxStaticBoxSizer2->Add(DetailText, 1, wxGROW | wxALL, 5);

    wxStaticText* WxStaticText2 = new wxStaticText(this, wxID_ANY,
        wxT("Rockbox Utility stores copies of Rockbox it has downloaded on the\n"
        "local hard disk to save network traffic.  If your local copy is\n"
        "no longer working, tick this box to download a fresh copy.") );
    wxBoxSizer3->Add(WxStaticText2, 0 , wxALL, 5);

    wxCheckBox* NoCacheCheckBox = new wxCheckBox(this, ID_NOCACHE_CHCKBX,
        wxT("Don't use locally cached copies of Rockbox") );
    wxBoxSizer3->Add(NoCacheCheckBox, 0, wxALL, 5);
    // controls at the bottom
    OkCancelCtrl* okCancel = new OkCancelCtrl(this,wxID_ANY);
    topSizer->Add(okCancel, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

}

// create the window
bool rockboxInstallDlg::Create( wxWindow* parent,
    wxWindowID id, const wxString& caption,
    const wxPoint& pos, const wxSize& size, long style )
{

    if (!wxDialog::Create( parent, id, caption, pos, size, style ))
        return false;
    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
    return true;
}

void rockboxInstallDlg::OnBuildBox(wxCommandEvent& event)
{
    wxString str;
    wxRadioBox* BuildRadioBox = (wxRadioBox*) FindWindow(ID_BUILD_BOX);
    wxCheckBox* NoCacheCheckBox = (wxCheckBox*) FindWindow(ID_NOCACHE_CHCKBX);
    wxStaticText* DetailText = (wxStaticText*)FindWindow(ID_DETAIL_TXT);

    switch(BuildRadioBox->GetSelection() )
    {
         case BUILD_RELEASE:
            str = _("This is the last released version of Rockbox, and is the\n"
               "recommended version to install.");
            NoCacheCheckBox->Enable();
            break;
         case BUILD_DAILY:
            str = _("These are automatically built each day from the current\n"
                "development source code.  This generally has more features\n"
                "than the last release but may be much less stable.  Features\n"
                "may change regularly.");
            NoCacheCheckBox->Enable();
            break;
        case BUILD_BLEEDING:
            str = _("This is the absolute up to the minute Rockbox built after\n"
                "the last change was made.\n\n"
                "Note:  This option will always download a fresh copy from the\n"
                "web server.\n");
            NoCacheCheckBox->Enable(false);
            break;
        default:
            break;
    }

    DetailText->SetLabel(str);

    this->GetSizer()->Layout();
    this->GetSizer()->Fit(this);
    this->GetSizer()->SetSizeHints(this);
    Refresh();
}


// tranver data from the controls
bool rockboxInstallDlg::TransferDataFromWindow()
{
    wxRadioBox* BuildRadioBox = (wxRadioBox*) FindWindow(ID_BUILD_BOX);
    wxCheckBox* NoCacheCheckBox = (wxCheckBox*) FindWindow(ID_NOCACHE_CHCKBX);

    gv->curdestdir = m_devicepos->getDevicePos();
    if(!wxDirExists(gv->curdestdir))
    {
       WARN_DIALOG(wxT("The Devicepostion is not valid"),
           wxT("Select a Deviceposition"));
       gv->curdestdir = wxT("");
       return false;
    }

    gv->curbuild = BuildRadioBox->GetSelection();
    gv->nocache = (gv->curbuild == BUILD_BLEEDING) ? true :
                  NoCacheCheckBox->IsChecked();

    return true;
}

// tranver data to the controls
bool rockboxInstallDlg::TransferDataToWindow()
{
    m_devicepos->setDefault();

	 if(gv->curplat == wxT(""))
    {
        WARN_DIALOG(wxT("You have not selected a audio device"),
                wxT("Select a Device"));
        return false;
    }

    wxRadioBox* BuildRadioBox = (wxRadioBox*) FindWindow(ID_BUILD_BOX);

    int index =gv->plat_id.Index(gv->curplat);

    wxCommandEvent updateradiobox(wxEVT_COMMAND_RADIOBOX_SELECTED,
        ID_BUILD_BOX);

    if (gv->plat_released[index] )
    {
        BuildRadioBox->Enable(BUILD_RELEASE, true);
        BuildRadioBox->SetSelection(BUILD_RELEASE);
    } else {
        BuildRadioBox->Enable(BUILD_RELEASE, false);
        BuildRadioBox->SetSelection(BUILD_DAILY);

    }
    wxPostEvent(this, updateradiobox);
    return true;
}
