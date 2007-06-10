
#include "rbutilCtrls.h"
#include "bootloaders.h"

/////////////////////////////////////////////////////////////
//// Controls
////////////////////////////////////////////////////////////////

/////////////////////////////////////////////
//// Image Ctrl
//////////////////////////////////////////////

BEGIN_EVENT_TABLE(ImageCtrl, wxControl)
    EVT_PAINT(ImageCtrl::OnPaint)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(ImageCtrl, wxControl)

bool ImageCtrl::Create(wxWindow* parent, wxWindowID id,
    const wxPoint& pos, const wxSize& size, long style,
    const wxValidator& validator)
{
    if (!wxControl::Create(parent, id, pos, size, style, validator)) return false;

return true;
}

void ImageCtrl::OnPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);
    dc.DrawBitmap(m_bitmap,0,0,false);
}

void ImageCtrl::SetBitmap(wxBitmap bmp)
{
    m_bitmap = bmp;
    Refresh();

}

wxSize ImageCtrl::DoGetBestSize() const
{
    wxSize bestsize;
    bestsize.x = m_bitmap.GetWidth();
    bestsize.y = m_bitmap.GetHeight();
    return bestsize;
}



/////////////////////////////////////////////
//// Theme Control
//////////////////////////////////////////////

BEGIN_EVENT_TABLE(ThemeCtrl, wxPanel)
    EVT_LISTBOX(ID_THEME_LST, ThemeCtrl::OnThemesLst)
    EVT_BUTTON(ID_THEME_SELECT_ALL, ThemeCtrl::OnSelectAll)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(ThemeCtrl, wxPanel)

bool ThemeCtrl::Create(wxWindow* parent, wxWindowID id,
    const wxPoint& pos, const wxSize& size, long style,
    const wxString title )
{
    if (!wxPanel::Create(parent, id, pos, size, style, title)) return false;

    CreateControls();

    GetSizer()->Fit(this);

    GetSizer()->SetSizeHints(this);
return true;
}

void ThemeCtrl::CreateControls()
{
    // A top-level sizer
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(topSizer);

    wxBoxSizer* horizontalSizer = new wxBoxSizer(wxHORIZONTAL);
    topSizer->Add(horizontalSizer, 0, wxALIGN_LEFT|wxALL, 5);

    //Device Selection
    wxBoxSizer* wxBoxSizer7 = new wxBoxSizer(wxVERTICAL);
    horizontalSizer->Add(wxBoxSizer7,0,wxGROW | wxALL,0);

    wxStaticText* m_desc = new wxStaticText( this, wxID_STATIC,
        wxT("Select one or more Themes to install"), wxDefaultPosition,
        wxDefaultSize, 0 );
    wxBoxSizer7->Add(m_desc, 0, wxALIGN_LEFT|wxALL, 5);

    m_themeList = new wxListBox(this,ID_THEME_LST,wxDefaultPosition,
            wxDefaultSize,0,NULL,wxLB_EXTENDED);
    wxBoxSizer7->Add(m_themeList, 0, wxALIGN_LEFT|wxALL, 5);

    m_selectAllThemes = new wxButton(this, ID_THEME_SELECT_ALL,
            wxT("Select All"));
    wxBoxSizer7->Add(m_selectAllThemes, 0, wxALIGN_LEFT|wxALL, 5);

    // Preview Picture
    wxBoxSizer* wxBoxSizer9 = new wxBoxSizer(wxVERTICAL);
    horizontalSizer->Add(wxBoxSizer9,0,wxGROW | wxALL,0);

    wxStaticText* preview_desc= new wxStaticText(this,wxID_ANY,wxT("Preview:"));
    wxBoxSizer9->Add(preview_desc,0,wxGROW | wxALL,5);

    m_PreviewBitmap = new ImageCtrl(this,ID_PREVIEW_BITMAP );
    wxBoxSizer9->Add(m_PreviewBitmap,0,wxALIGN_LEFT | wxALL,5);

    wxStaticBox* groupbox= new wxStaticBox(this,wxID_ANY,wxT("Selected Theme:"));
    wxStaticBoxSizer* styleSizer = new wxStaticBoxSizer( groupbox, wxVERTICAL );
    topSizer->Add(styleSizer,0,wxGROW|wxALL,0);

    // horizontal sizer
    wxBoxSizer* wxBoxSizer8 = new wxBoxSizer(wxHORIZONTAL);
    styleSizer->Add(wxBoxSizer8,0,wxGROW | wxALL,0);

    // File size
    wxStaticText* size_desc= new wxStaticText(this,wxID_ANY,wxT("Filesize:"));
    wxBoxSizer8->Add(size_desc,0,wxGROW | wxALL,5);

    m_size= new wxStaticText(this,ID_FILESIZE,wxT(""));
    wxBoxSizer8->Add(m_size,0,wxGROW | wxALL,5);

    // Description
    wxStaticText* desc_desc= new wxStaticText(this,wxID_ANY,wxT("Description:"));
    styleSizer->Add(desc_desc,0,wxGROW | wxALL,5);

    m_themedesc= new wxTextCtrl(this,ID_DESC,wxT(""),wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY);
    styleSizer->Add(m_themedesc,0,wxGROW | wxALL,5);

    topSizer->Fit(this);
    topSizer->SetSizeHints(this);
    Layout();

}

void ThemeCtrl::Init()
{
    m_Themes.Clear();
    m_Themes_path.Clear();
    m_Themes_size.Clear();
    m_Themes_image.Clear();
    m_Themes_desc.Clear();

}

void ThemeCtrl::setDevice(wxString device)
{

    int index = gv->plat_id.Index(device);
    if(index  == -1) return;

    if(gv->plat_resolution[index] == m_currentResolution)
        return;
    else
        m_currentResolution = gv->plat_resolution[index];

    // load the themelist
    Init();
    m_size->SetLabel(wxT(""));
    m_themedesc->SetValue(wxT(""));
    m_themeList->Clear();

     //get correct Themes list
    wxString src,dest,err;

    src.Printf(wxT("%srbutil.php?res=%s"),gv->themes_url.c_str(),m_currentResolution.c_str());
    dest.Printf(wxT("%s" PATH_SEP "download" PATH_SEP "%s.list"),
        gv->stdpaths->GetUserDataDir().c_str(),m_currentResolution.c_str());

    if(DownloadURL(src, dest))
    {
        MESG_DIALOG(wxT("Unable to download themes list."));
        return;
    }

     //read and parse Themes list
    wxString themelistraw;
    wxFFile themefile;
    if(!themefile.Open(dest))       //open file
    {
        MESG_DIALOG(wxT("Unable to open themes list."));
        return;
    }
    if(!themefile.ReadAll(&themelistraw))  //read complete file
    {
        MESG_DIALOG(wxT("Unable to read themes list."));
        return;
    }
    wxRegEx reAll(wxT("<body >(.+)</body>"));  //extract body part
    if(! reAll.Matches(themelistraw))
    {
        MESG_DIALOG(wxT("Themes list is in wrong Format."));
        return;
    }
    wxString lines = reAll.GetMatch(themelistraw,1);

    // prepare text
    lines.Replace(wxT("<br />"),wxT(""),true);   //replace <br /> with nothing
    lines.Replace(wxT("\n"),wxT(""),true);   //replace \n with nothing
    lines.Trim(true);                         //strip WS at end
    lines.Trim(false);                         //strip WS at beginning
    wxStringTokenizer tkz(lines,wxT("|"));   //tokenize it

    while ( tkz.HasMoreTokens() )           // read all entrys
    {
        m_Themes.Add(tkz.GetNextToken());         //Theme name
        m_Themes_path.Add(tkz.GetNextToken());    //Theme path
        m_Themes_size.Add(tkz.GetNextToken());    //File size
        m_Themes_image.Add(tkz.GetNextToken());   //Screenshot
        m_Themes_desc.Add(tkz.GetNextToken());    //Description

        m_themeList->Append(m_Themes.Last());
    }

    this->GetSizer()->Layout();
    this->GetSizer()->Fit(this);
    this->GetSizer()->SetSizeHints(this);
    m_parent->GetSizer()->Layout();
    m_parent->GetSizer()->Fit(m_parent);
    m_parent->GetSizer()->SetSizeHints(m_parent);
}


void ThemeCtrl::OnThemesLst(wxCommandEvent& event)
{
    ThemePreview();
}

void ThemeCtrl::ThemePreview()
{
  //  wxCriticalSectionLocker locker(m_ThemeSelectSection);

    wxArrayInt selected;
    int numSelected = m_themeList->GetSelections(selected);
    if(numSelected == 0) return;

    int index = selected[0];

    m_size->SetLabel(m_Themes_size[index]);
    m_themedesc->SetValue(m_Themes_desc[index]);
   // m_themedesc->Wrap(200);                       // wrap desc

    wxString src,dest;

    int pos = m_Themes_image[index].Find('/',true);
    wxString filename = m_Themes_image[index](pos+1,m_Themes_image[index].Length());

    dest.Printf(wxT("%s" PATH_SEP "download" PATH_SEP "%s"),
            gv->stdpaths->GetUserDataDir().c_str(),m_currentResolution.c_str());

    if(!wxDirExists(dest))
        wxMkdir(dest);

    //this is a URL no PATH_SEP
    src.Printf(wxT("%s/data/%s/%s"),gv->themes_url.c_str(),m_currentResolution.c_str(),filename.c_str());
    dest.Printf(wxT("%s" PATH_SEP "download" PATH_SEP "%s" PATH_SEP "%s"),
        gv->stdpaths->GetUserDataDir().c_str(),m_currentResolution.c_str(),filename.c_str());

    if(DownloadURL(src, dest))
    {
        MESG_DIALOG(wxT("Unable to download image."));
        return;
    }

    m_currentimage = dest;
    wxBitmap bmp;
    bmp.LoadFile(m_currentimage,wxBITMAP_TYPE_PNG);
    m_PreviewBitmap->SetBitmap(bmp);

    Refresh();
    this->GetSizer()->Layout();
    this->GetSizer()->Fit(this);
    this->GetSizer()->SetSizeHints(this);

    m_parent->GetSizer()->Layout();
    m_parent->GetSizer()->Fit(m_parent);
    m_parent->GetSizer()->SetSizeHints(m_parent);

}

void ThemeCtrl::OnSelectAll(wxCommandEvent& event)
{
    for(unsigned int i=0; i < m_themeList->GetCount(); i++)
        m_themeList->Select(i);
    ThemePreview();
}

wxArrayString ThemeCtrl::getThemesToInstall()
{
    wxArrayString themes;
    wxArrayInt selected;
    int numSelected = m_themeList->GetSelections(selected);

    for(int i=0; i < numSelected; i++)
    {
        themes.Add(m_Themes_path[selected[i]]);
    }
    return themes;

 }

/////////////////////////////////////////////
//// Ok Cancel Control
//////////////////////////////////////////////

BEGIN_EVENT_TABLE(OkCancelCtrl, wxPanel)

END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(OkCancelCtrl, wxPanel)

bool OkCancelCtrl::Create(wxWindow* parent, wxWindowID id,
    const wxPoint& pos, const wxSize& size, long style,
    const wxString title)
{
    if (!wxPanel::Create(parent, id, pos, size, style, title)) return false;

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
return true;
}

void OkCancelCtrl::CreateControls()
{
    // A top-level sizer
    wxBoxSizer* topSizer = new wxBoxSizer(wxHORIZONTAL);
    this->SetSizer(topSizer);

    // The OK button
    m_OkBtn = new wxButton ( this, wxID_OK, wxT("&OK"),
        wxDefaultPosition, wxDefaultSize, 0 );
    topSizer->Add(m_OkBtn, 0, wxALL, 5);
    // The Cancel button
    m_CancelBtn = new wxButton ( this, wxID_CANCEL,
        wxT("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    topSizer->Add(m_CancelBtn, 0, wxALL, 5);

    Layout();

}


/////////////////////////////////////////////
//// Device Selector
//////////////////////////////////////////////

BEGIN_EVENT_TABLE(DeviceSelectorCtrl, wxPanel)
    EVT_BUTTON(ID_AUTODETECT_BTN, DeviceSelectorCtrl::OnAutoDetect)
    EVT_COMBOBOX(ID_DEVICE_CBX,DeviceSelectorCtrl::OnComboBox)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(DeviceSelectorCtrl, wxPanel)

bool DeviceSelectorCtrl::Create(wxWindow* parent, wxWindowID id,
    const wxPoint& pos, const wxSize& size, long style,
    const wxString title)
{
    if (!wxPanel::Create(parent, id, pos, size, style, title)) return false;

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    return true;
}

void DeviceSelectorCtrl::CreateControls()
{
    // A top-level sizer
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(topSizer);

     //Device Selection
    wxBoxSizer* horizontalSizer = new wxBoxSizer(wxHORIZONTAL);
    topSizer->Add(horizontalSizer, 0, wxALIGN_LEFT|wxALL, 5);
    m_desc = new wxStaticText( this, wxID_STATIC,
        wxT("Device:"), wxDefaultPosition,
        wxDefaultSize, 0 );
    horizontalSizer->Add(m_desc, 0, wxALIGN_LEFT|wxALL, 5);

    m_deviceCbx = new wxComboBox(this, ID_DEVICE_CBX,wxT("Select your Device"),
            wxDefaultPosition,wxDefaultSize,gv->plat_name,wxCB_READONLY);

    m_deviceCbx->SetToolTip(wxT("Select your Device."));
    m_deviceCbx->SetHelpText(wxT("Select your Device."));

    horizontalSizer->Add(m_deviceCbx, 0, wxALIGN_LEFT|wxALL, 5);

    wxButton* m_autodetectBtn = new wxButton(this, ID_AUTODETECT_BTN, wxT("Autodetect"),
         wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator,
         wxT("AutodetectBtn"));

    m_autodetectBtn->SetToolTip(wxT("Click here to autodetect your Device."));
    m_autodetectBtn->SetHelpText(wxT("Autodetection of the Device."));
   // m_autodetectBtn->SetFocus();

    horizontalSizer->Add(m_autodetectBtn,0,wxGROW | wxALL,5);
    Layout();

}

wxString DeviceSelectorCtrl::getDevice()
{
    return m_currentDevice;
}

void DeviceSelectorCtrl::setDefault()
{
    int index = gv->plat_id.Index(gv->curplat);
    if(index == -1) return;
    m_deviceCbx->SetValue(gv->plat_name[index]);
}

void DeviceSelectorCtrl::OnComboBox(wxCommandEvent& event)
{
    int index = gv->plat_name.Index(m_deviceCbx->GetValue());

    if(index == -1)
    {
        m_currentDevice = wxT("");
        return;
    }

    gv->curplat = gv->plat_id[index];
}

void DeviceSelectorCtrl::OnAutoDetect(wxCommandEvent& event)
{
    AutoDetect();
}

void DeviceSelectorCtrl::AutoDetect()
{
    struct ipod_t ipod;
    int n = ipod_scan(&ipod);
    if(n == 1)
    {
        wxString temp(ipod.targetname,wxConvUTF8);
        int index = gv->plat_bootloadername.Index(temp);   // use the bootloader names..
        m_deviceCbx->SetValue(gv->plat_name[index]);
        gv->curplat=gv->plat_id[index];
        return;
    }
    else if (n > 1)
    {
        WARN_DIALOG(wxT("More then one Ipod device detected, please connect only One"),
                wxT("Detecting a Device"));
        return;
    }

    struct sansa_t sansa;
    int n2 = sansa_scan(&sansa);
    if(n2==1)
    {
        int index = gv->plat_id.Index(wxT("sansae200"));
        m_deviceCbx->SetValue(gv->plat_name[index]);
        gv->curplat=gv->plat_id[index];
        return;
    }
    else if (n2 > 1)
    {
        WARN_DIALOG(wxT("More then one Sansa device detected, please connect only One"),
                wxT("Detecting a Device"));
        return;
    }

    WARN_DIALOG(wxT("No Device detected. (This function currently only works for Ipods and Sansas)."),
                wxT("Detecting a Device"));
    return;

}

/////////////////////////////////////////////
//// DevicePosition Selector
//////////////////////////////////////////////

BEGIN_EVENT_TABLE(DevicePositionCtrl, wxPanel)
    EVT_BUTTON(ID_BROWSE_BTN, DevicePositionCtrl::OnBrowseBtn)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(DevicePositionCtrl, wxPanel)

bool DevicePositionCtrl::Create(wxWindow* parent, wxWindowID id,
    const wxPoint& pos, const wxSize& size, long style,
    const wxString title)
{
    if (!wxPanel::Create(parent, id, pos, size, style, title)) return false;

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
return true;
}

void DevicePositionCtrl::CreateControls()
{
    // A top-level sizer
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(topSizer);

    //Device Selection
    m_desc = new wxStaticText( this, wxID_STATIC,
        wxT("Select your Device in the Filesystem"), wxDefaultPosition,
        wxDefaultSize, 0 );
    topSizer->Add(m_desc, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* horizontalSizer = new wxBoxSizer(wxHORIZONTAL);
    topSizer->Add(horizontalSizer, 0, wxGROW|wxALL, 5);

    m_devicePos = new wxTextCtrl(this,wxID_ANY,gv->curdestdir);
    m_devicePos->SetToolTip(wxT("Type the folder where your Device is here"));
    m_devicePos->SetHelpText(wxT("Type the folder where your Device is here"));
    horizontalSizer->Add(m_devicePos,0,wxGROW | wxALL,5);

    m_browseBtn = new wxButton(this, ID_BROWSE_BTN, wxT("Browse"),
         wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator,
         wxT("BrowseBtn"));
    m_browseBtn->SetToolTip(wxT("Browse for your Device"));
    m_browseBtn->SetHelpText(wxT("Browse for your Device"));
    horizontalSizer->Add(m_browseBtn,0,wxGROW | wxALL,5);

    topSizer->Fit(this);
    Layout();

}

wxString DevicePositionCtrl::getDevicePos()
{
    return m_devicePos->GetValue();

}

void DevicePositionCtrl::setDefault()
{
    m_devicePos->SetValue(gv->curdestdir);
}

void DevicePositionCtrl::OnBrowseBtn(wxCommandEvent& event)
{
     const wxString& temp = wxDirSelector(
        wxT("Please select the location of your audio device"), gv->curdestdir);

    if (!temp.empty())
    {
        m_devicePos->SetValue(temp);
    }

}

/////////////////////////////////////////////
//// FirmwarePosition Selector
//////////////////////////////////////////////

BEGIN_EVENT_TABLE(FirmwarePositionCtrl, wxPanel)
    EVT_BUTTON(ID_BROWSE_BTN, FirmwarePositionCtrl::OnBrowseBtn)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(FirmwarePositionCtrl, wxControl)

bool FirmwarePositionCtrl::Create(wxWindow* parent, wxWindowID id,
    const wxPoint& pos, const wxSize& size, long style,
    const wxString title)
{
    if (!wxPanel::Create(parent, id, pos, size, style, title)) return false;

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
return true;
}

void FirmwarePositionCtrl::CreateControls()
{
    // A top-level sizer
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(topSizer);

    //Device Selection
    m_desc = new wxStaticText( this, wxID_STATIC,
        wxT("Select original Firmware from the Manufacturer"), wxDefaultPosition,
        wxDefaultSize, 0 );
    topSizer->Add(m_desc, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* horizontalSizer = new wxBoxSizer(wxHORIZONTAL);
    topSizer->Add(horizontalSizer, 0, wxALIGN_LEFT|wxALL, 5);

    m_firmwarePos = new wxTextCtrl(this,wxID_ANY,gv->curdestdir);
    m_firmwarePos->SetToolTip(wxT("Type the folder where the original Firmware is here"));
    m_firmwarePos->SetHelpText(wxT("Type the folder where the original Firmware is here"));
    horizontalSizer->Add(m_firmwarePos,0,wxGROW | wxALL,5);

    m_browseBtn = new wxButton(this, ID_BROWSE_BTN, wxT("Browse"),
         wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator,
         wxT("BrowseBtn"));
    m_browseBtn->SetToolTip(wxT("Browse for the original Firmware"));
    m_browseBtn->SetHelpText(wxT("Browse for the original Firmware"));
    horizontalSizer->Add(m_browseBtn,0,wxGROW | wxALL,5);

    Layout();

}

wxString FirmwarePositionCtrl::getFirmwarePos()
{
    return m_firmwarePos->GetValue();

}

void FirmwarePositionCtrl::setDefault()
{
    m_firmwarePos->SetValue(gv->curfirmware);
}

void FirmwarePositionCtrl::OnBrowseBtn(wxCommandEvent& event)
{
     wxString temp = wxFileSelector(
        wxT("Please select the location of the original Firmware"), gv->curdestdir,wxT(""),wxT(""),wxT("*.hex"));

    if (!temp.empty())
    {
        m_firmwarePos->SetValue(temp);
    }

}

