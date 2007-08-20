
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

	return true;

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
//// Talk file creation
/////////////////////////////////////////////////


IMPLEMENT_CLASS( talkInstallDlg, wxDialog )

BEGIN_EVENT_TABLE( talkInstallDlg, wxDialog )
    EVT_BUTTON(ID_BROWSE_ENC_BTN, talkInstallDlg::OnBrowseEncBtn)
    EVT_BUTTON(ID_BROWSE_TTS_BTN, talkInstallDlg::OnBrowseTtsBtn)
END_EVENT_TABLE()

talkInstallDlg::talkInstallDlg(TalkFileCreator* talkcreator )
{
    m_talkCreator = talkcreator;
    Init();
}

talkInstallDlg::talkInstallDlg(TalkFileCreator* talkcreator, wxWindow* parent,
    wxWindowID id, const wxString& caption,
    const wxPoint& pos, const wxSize& size, long style )
{
    m_talkCreator = talkcreator;
    Init();
    Create(parent, id, caption, pos, size, style);
}

void talkInstallDlg::CreateControls()
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

    // Device position
    m_devicepos = new DevicePositionCtrl(this,ID_DEVICEPOS);
    wxBoxSizer3->Add(m_devicepos, 0, wxALIGN_LEFT|wxALL, 5);

    // Encoder
    wxStaticBox* WxStaticBoxSizer2_StaticBoxObj = new wxStaticBox(this,
        wxID_ANY, wxT("Encoder"));
	wxStaticBoxSizer* WxStaticBoxSizer2 =
        new wxStaticBoxSizer(WxStaticBoxSizer2_StaticBoxObj,wxVERTICAL);
	wxBoxSizer3->Add(WxStaticBoxSizer2,0,wxALIGN_CENTER_HORIZONTAL|wxGROW | wxALL, 5);

    m_Enc = new wxComboBox(this,ID_ENC_CBX,wxT("lame"),
            wxDefaultPosition,wxDefaultSize,m_talkCreator->getSupportedEnc(),wxCB_READONLY);
    m_Enc->SetToolTip(wxT("Select your Encoder."));
    m_Enc->SetHelpText(wxT("Select your Encoder."));
    WxStaticBoxSizer2->Add(m_Enc,0,wxALIGN_CENTER_HORIZONTAL|wxGROW | wxALL, 5);

    wxStaticText* enc_desc = new wxStaticText( this, wxID_STATIC,
        wxT("Select the Encoder executable"), wxDefaultPosition,
        wxDefaultSize, 0 );
    WxStaticBoxSizer2->Add(enc_desc, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* horizontalSizer = new wxBoxSizer(wxHORIZONTAL);
    WxStaticBoxSizer2->Add(horizontalSizer, 0, wxGROW|wxALL, 5);

    m_EncExe = new wxTextCtrl(this,wxID_ANY,gv->pathToEnc);
    m_EncExe->SetToolTip(wxT("Type the folder where your Encoder exe is"));
    m_EncExe->SetHelpText(wxT("Type the folder where your Encoder exe is"));
    horizontalSizer->Add(m_EncExe,0,wxGROW | wxALL,5);

    m_browseEncBtn = new wxButton(this, ID_BROWSE_ENC_BTN, wxT("Browse"),
         wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator,
         wxT("BrowseEncBtn"));
    m_browseEncBtn->SetToolTip(wxT("Browse for your Encoder Exe"));
    m_browseEncBtn->SetHelpText(wxT("Browse for your Encoder exe"));
    horizontalSizer->Add(m_browseEncBtn,0,wxGROW | wxALL,5);

    wxStaticText* enc_desc_opt = new wxStaticText( this, wxID_STATIC,
        wxT("Encoder Options"), wxDefaultPosition,
        wxDefaultSize, 0 );
    WxStaticBoxSizer2->Add(enc_desc_opt, 0, wxALIGN_LEFT|wxALL, 5);

    m_EncOpts = new wxTextCtrl(this,wxID_ANY,m_talkCreator->getEncOpts(wxT("lame")));
    m_EncOpts->SetToolTip(wxT("Encoder Options"));
    m_EncOpts->SetHelpText(wxT("Encoder Options"));
    WxStaticBoxSizer2->Add(m_EncOpts, 0, wxALIGN_LEFT|wxALL, 5);

    // TTS
    wxStaticBox* WxStaticBoxSizer3_StaticBoxObj = new wxStaticBox(this,
        wxID_ANY, wxT("Text to Speach"));
	wxStaticBoxSizer* WxStaticBoxSizer3 =
        new wxStaticBoxSizer(WxStaticBoxSizer3_StaticBoxObj,wxVERTICAL);
	wxBoxSizer3->Add(WxStaticBoxSizer3,0,wxALIGN_CENTER_HORIZONTAL|wxGROW | wxALL, 5);

    m_Tts = new wxComboBox(this,ID_TTS_CBX,wxT("espeak"),
            wxDefaultPosition,wxDefaultSize,m_talkCreator->getSupportedTTS(),wxCB_READONLY);
    m_Tts->SetToolTip(wxT("Select your TTS."));
    m_Tts->SetHelpText(wxT("Select your TTS."));
    WxStaticBoxSizer3->Add(m_Tts,0,wxALIGN_CENTER_HORIZONTAL|wxGROW | wxALL, 5);

    wxStaticText* tts_desc = new wxStaticText( this, wxID_STATIC,
        wxT("Select the TTS executable"), wxDefaultPosition,
        wxDefaultSize, 0 );
    WxStaticBoxSizer3->Add(tts_desc, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* horizontalSizer2 = new wxBoxSizer(wxHORIZONTAL);
    WxStaticBoxSizer3->Add(horizontalSizer2, 0, wxGROW|wxALL, 5);

    m_TtsExe = new wxTextCtrl(this,wxID_ANY,gv->pathToTts);
    m_TtsExe->SetToolTip(wxT("Type the folder where your TTS exe is"));
    m_TtsExe->SetHelpText(wxT("Type the folder where your TTS exe is"));
    horizontalSizer2->Add(m_TtsExe,0,wxGROW | wxALL,5);

    m_browseTtsBtn = new wxButton(this, ID_BROWSE_TTS_BTN, wxT("Browse"),
         wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator,
         wxT("BrowseEncBtn"));
    m_browseTtsBtn->SetToolTip(wxT("Browse for your Encoder Exe"));
    m_browseTtsBtn->SetHelpText(wxT("Browse for your Encoder exe"));
    horizontalSizer2->Add(m_browseTtsBtn,0,wxGROW | wxALL,5);

    wxStaticText* tts_desc_opt = new wxStaticText( this, wxID_STATIC,
        wxT("TTS Options"), wxDefaultPosition,
        wxDefaultSize, 0 );
    WxStaticBoxSizer3->Add(tts_desc_opt, 0, wxALIGN_LEFT|wxALL, 5);

    m_TtsOpts = new wxTextCtrl(this,wxID_ANY,m_talkCreator->getTTsOpts(wxT("espeak")));
    m_TtsOpts->SetToolTip(wxT("TTS Options"));
    m_TtsOpts->SetHelpText(wxT("TTS Options"));
    WxStaticBoxSizer3->Add(m_TtsOpts, 0, wxALIGN_LEFT|wxALL, 5);

    m_OverwriteWave = new wxCheckBox(this,wxID_ANY,wxT("Overwrite Wav"));
    m_OverwriteWave->SetToolTip(wxT("Overwrite Wavefiles"));
    m_OverwriteWave->SetHelpText(wxT("Overwrite Wavefiles"));
    wxBoxSizer3->Add(m_OverwriteWave,0,wxALIGN_CENTER_HORIZONTAL|wxGROW | wxALL, 5);

    m_OverwriteTalk = new wxCheckBox(this,wxID_ANY,wxT("Overwrite Talk"));
    m_OverwriteTalk->SetToolTip(wxT("Overwrite Talkfiles"));
    m_OverwriteTalk->SetHelpText(wxT("Overwrite Talkfiles"));
    wxBoxSizer3->Add(m_OverwriteTalk,0,wxALIGN_CENTER_HORIZONTAL|wxGROW | wxALL, 5);

    m_RemoveWave = new wxCheckBox(this,wxID_ANY,wxT("Remove Wav"));
    m_RemoveWave->SetToolTip(wxT("Remove Wavfiles"));
    m_RemoveWave->SetHelpText(wxT("Remove Wavfiles"));
    wxBoxSizer3->Add(m_RemoveWave,0,wxALIGN_CENTER_HORIZONTAL|wxGROW | wxALL, 5);

    m_Recursive = new wxCheckBox(this,wxID_ANY,wxT("Recursive"));
    m_Recursive->SetToolTip(wxT("Recursive"));
    m_Recursive->SetHelpText(wxT("Recursive"));
    wxBoxSizer3->Add(m_Recursive,0,wxALIGN_CENTER_HORIZONTAL|wxGROW | wxALL, 5);

    m_StripExtensions = new wxCheckBox(this,wxID_ANY,wxT("Strip Extensions"));
    m_StripExtensions->SetToolTip(wxT("Strip Extensions"));
    m_StripExtensions->SetHelpText(wxT("Strip Extensions"));
    wxBoxSizer3->Add(m_StripExtensions,0,wxALIGN_CENTER_HORIZONTAL|wxGROW | wxALL, 5);

    OkCancelCtrl* okCancel = new OkCancelCtrl(this,wxID_ANY);
    topSizer->Add(okCancel, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    // controls at the bottom
    wxBoxSizer* wxBoxSizer7 = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(wxBoxSizer7, 0, wxGROW | wxALL, 5);

}

//init the local variables
void talkInstallDlg::Init()
{

}

// create the window
bool talkInstallDlg::Create( wxWindow* parent,
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

void talkInstallDlg::OnBrowseEncBtn(wxCommandEvent& event)
{
    const wxString& temp = wxFileSelector(
        wxT("Please select the location of your encoder"), wxT(""),
        wxT(""),wxT(""),wxT("*.*"),0, this);

    if (!temp.empty())
    {
        m_EncExe->SetValue(temp);
    }
}

void talkInstallDlg::OnBrowseTtsBtn(wxCommandEvent& event)
{
    const wxString& temp = wxFileSelector(
        wxT("Please select the location of your TTS engine"), wxT(""),
        wxT(""),wxT(""),wxT("*.*"),0, this);


    if (!temp.empty())
    {
        m_TtsExe->SetValue(temp);
    }
}

// tranver data from the controls
bool talkInstallDlg::TransferDataFromWindow()
{
    gv->curdestdir = m_devicepos->getDevicePos();
    if(!wxDirExists(gv->curdestdir))
    {
        WARN_DIALOG(wxT("The Devicepostion is not valid"),
            wxT("Select a Deviceposition"));
        gv->curdestdir = wxT("");
        return false;
    }
    m_talkCreator->setDir(gv->curdestdir);

    gv->pathToEnc = m_EncExe->GetValue();
    if(!wxFileExists(gv->pathToEnc))
    {
        WARN_DIALOG(wxT("The Encoder exe is not valid"),
            wxT("Select an Encoder"));
        gv->pathToEnc = wxT("");
        return false;
    }
    m_talkCreator->setEncexe(gv->pathToEnc);

    gv->pathToTts = m_TtsExe->GetValue();
    if(!wxFileExists(gv->pathToTts))
    {
        WARN_DIALOG(wxT("The TTs exe is not valid"),
            wxT("Select an TTS engine"));
        gv->pathToTts = wxT("");
        return false;
    }
    m_talkCreator->setTTSexe(gv->pathToTts);

    m_talkCreator->setTTsType(m_Tts->GetValue());
    m_talkCreator->setEncType(m_Enc->GetValue());


    m_talkCreator->setOverwriteTalk(m_OverwriteWave->IsChecked());
    m_talkCreator->setOverwriteWav(m_OverwriteTalk->IsChecked());
    m_talkCreator->setRemoveWav(m_RemoveWave->IsChecked());
    m_talkCreator->setRecursive(m_Recursive->IsChecked());
    m_talkCreator->setStripExtensions(m_StripExtensions->IsChecked());

    m_talkCreator->setEncOpts(m_EncOpts->GetValue());
    m_talkCreator->setTTsOpts(m_TtsOpts->GetValue());

    return true;
}

// tranver data to the controls
bool talkInstallDlg::TransferDataToWindow()
{
   m_devicepos->setDefault();

   m_OverwriteWave->SetValue(true);
   m_OverwriteTalk->SetValue(true);
   m_RemoveWave->SetValue(true);
   m_Recursive->SetValue(true);
   m_StripExtensions->SetValue(false);



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
    array.Add(wxT("Rockbox stable version (") + gv->last_release + wxT(")"));
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
        BuildRadioBox->SetSelection(BUILD_BLEEDING);

    }
    wxPostEvent(this, updateradiobox);
    return true;
}
