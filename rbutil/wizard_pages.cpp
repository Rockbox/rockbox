/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * Module: rbutil
 * File: wizardpages.cpp
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

#include "wizard_pages.h"
#include "bootloaders.h"


#include <wx/regex.h>
#include <wx/tokenzr.h>

/////// Bootplatform page ///////////////////////////////////77
wxBootPlatformPage::wxBootPlatformPage(wxWizard *parent) : wxWizardPageSimple(parent)
{
    wxBoxSizer* WxBoxSizer1 = new wxBoxSizer(wxVERTICAL);

    wxStaticText* WxStaticText1 = new wxStaticText(this, wxID_ANY,
        wxT("Please select the model of audio device that you would like to"
        "\ninstall the Rockbox Bootloader on from the list below:"));
    WxBoxSizer1->Add(WxStaticText1,0,wxGROW | wxALL,5);

    wxArrayString arrayStringFor_WxListBox1;
    for (unsigned int i=0; i< gv->plat_name.GetCount(); i++) {
        if (gv->plat_needsbootloader[i])
        {
           if(gv->plat_autodetect[i])
           {
                if(arrayStringFor_WxListBox1.Index(gv->plat_combinedname[i]) == wxNOT_FOUND)
                    arrayStringFor_WxListBox1.Add(gv->plat_combinedname[i]);
           }
           else
           {
                arrayStringFor_WxListBox1.Add(gv->plat_name[i]);
           }
        }
    }

    BootPlatformListBox = new wxListBox(this, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, arrayStringFor_WxListBox1, wxLB_SINGLE);
    WxBoxSizer1->Add(BootPlatformListBox,0,wxGROW | wxALL,5);

    SetSizer(WxBoxSizer1);
    WxBoxSizer1->Fit(this);

    for (unsigned int i=0; i< gv->plat_id.GetCount(); i++) {
        if (gv->plat_id[i] == gv->curplat) BootPlatformListBox->SetSelection(i);
    }

}

wxWizardPage * wxBootPlatformPage::GetNext() const
{
   if(gv->curbootloadermethod != wxT("fwpatcher")&& gv->curbootloadermethod != wxT("ipodpatcher"))
   {
       if(wxWizardPageSimple::GetNext()->GetNext() != NULL)  // not iriver hx0 and ipod, skip one page
           return wxWizardPageSimple::GetNext()->GetNext();
   }
   else if(gv->curbootloadermethod == wxT("ipodpatcher"))
   {
        if(wxWizardPageSimple::GetNext()->GetNext() != NULL)
            if(wxWizardPageSimple::GetNext()->GetNext()->GetNext() != NULL)
                return wxWizardPageSimple::GetNext()->GetNext()->GetNext(); //ipod, skip 2 pages
            else
                return wxWizardPageSimple::GetNext()->GetNext(); //ipod, skip 1 page (for uninstallation)
   }

   // all others , no skip
   return wxWizardPageSimple::GetNext();
}

bool wxBootPlatformPage::TransferDataFromWindow()
{
    if (BootPlatformListBox->GetSelection() == wxNOT_FOUND )
    {
        WARN_DIALOG(wxT("You must select an audio device type before proceeding"),
            wxT("Select Platform"));
        return false;
    } else
    {
        int idx = gv->plat_name.Index(BootPlatformListBox->GetStringSelection());
        if(idx == wxNOT_FOUND) idx =gv->plat_combinedname.Index(BootPlatformListBox->GetStringSelection());
        gv->curplatnum = idx;
        gv->curplat = gv->plat_id[gv->curplatnum];
        gv->curbootloadermethod = gv->plat_bootloadermethod[gv->curplatnum];
        gv->curbootloader = gv->plat_bootloadername[gv->curplatnum];

        return true;
    }
}
//// Plattfor Page //////////////////////////
wxPlatformPage::wxPlatformPage(wxWizard *parent) : wxWizardPageSimple(parent)
{
    wxBoxSizer* WxBoxSizer1 = new wxBoxSizer(wxVERTICAL);

    wxStaticText* WxStaticText1 = new wxStaticText(this, wxID_ANY,
        wxT("Please select the model of audio device that you would like to"
        "\ninstall Rockbox on from the list below:"));
    WxBoxSizer1->Add(WxStaticText1,0,wxGROW | wxALL,5);

    wxArrayString arrayStringFor_WxListBox1;
    PlatformListBox = new wxListBox(this, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, gv->plat_name, wxLB_SINGLE);
    WxBoxSizer1->Add(PlatformListBox,0,wxGROW | wxALL,5);

    SetSizer(WxBoxSizer1);
    WxBoxSizer1->Fit(this);
    for (unsigned int i=0; i< gv->plat_id.GetCount(); i++) {
        if (gv->plat_id[i] == gv->curplat) PlatformListBox->SetSelection(i);
    }
}

bool wxPlatformPage::TransferDataFromWindow()
{
    if (PlatformListBox->GetSelection() == wxNOT_FOUND )
    {
        WARN_DIALOG(wxT("You must select an audio device type before proceeding"),
            wxT("Select Platform"));
        return false;
    } else
    {
        gv->curplatnum = PlatformListBox->GetSelection();
        gv->curplat = gv->plat_id[gv->curplatnum];
        gv->curresolution = gv->plat_resolution[gv->curplatnum];
        return true;
    }
}


//////////////// ThemeImage Dialog /////////////////
BEGIN_EVENT_TABLE(wxThemeImageDialog,wxDialog)
   EVT_PAINT(wxThemeImageDialog::OnPaint)
END_EVENT_TABLE();
wxThemeImageDialog::wxThemeImageDialog(wxWindow* parent,wxWindowID id,wxString title,wxBitmap bmp)  :
   wxDialog(parent, id, title)
{
    wxBoxSizer *sizerTop = new wxBoxSizer(wxHORIZONTAL);
    m_bitmap = bmp;

    sizerTop->SetMinSize(m_bitmap.GetWidth(),m_bitmap.GetHeight());

    SetSizer(sizerTop);

    sizerTop->SetSizeHints(this);
    sizerTop->Fit(this);

}

void wxThemeImageDialog::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    wxPaintDC dc( this );
    dc.DrawBitmap( m_bitmap, 0, 0, true /* use mask */ );
}

////////////////// Themes page ////////////////////////
BEGIN_EVENT_TABLE(wxThemesPage,wxWizardPageSimple)
	EVT_WIZARD_PAGE_CHANGED          (wxID_ANY, wxThemesPage::OnPageShown)
	EVT_WIZARD_PAGE_CHANGING         (wxID_ANY, wxThemesPage::OnWizardPageChanging)
	EVT_LISTBOX                      (ID_LISTBOX,wxThemesPage::OnListBox)
	EVT_BUTTON                       (ID_PREVIEW_BTN, wxThemesPage::OnPreviewBtn)
	EVT_CHECKBOX                     (ID_INSTALLCHECKBOX, wxThemesPage::OnCheckBox)
END_EVENT_TABLE();

wxThemesPage::wxThemesPage(wxWizard *parent) : wxWizardPageSimple(parent)
{
    m_parent = parent;
    wxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* WxStaticText1 = new wxStaticText(this, wxID_ANY,
        wxT("Please select the Theme you would like to"
        "\ninstall on your Device from the list below:"));
    mainSizer->Add(WxStaticText1,0,wxGROW | wxALL,5);

    // create theme listbox
    wxArrayString list;
    for(int i = 0; i< 35;i++)
        list.Add("");
    ThemesListBox= new wxListBox(this, ID_LISTBOX, wxDefaultPosition,
        wxDefaultSize,list, wxLB_SINGLE);
    mainSizer->Add(ThemesListBox,10,wxGROW | wxALL,5);

    // create groupbox
    wxStaticBox* groupbox= new wxStaticBox(this,wxID_ANY,wxT("Selected Theme:"));
    wxBoxSizer* styleSizer = new wxStaticBoxSizer( groupbox, wxVERTICAL );
    mainSizer->Add(styleSizer,11,wxGROW | wxALL,5);

    // horizontal sizer
    wxBoxSizer* wxBoxSizer2 = new wxBoxSizer(wxHORIZONTAL);
    styleSizer->Add(wxBoxSizer2,0,wxGROW | wxALL,0);

    // preview button
    m_previewBtn = new wxButton(this, ID_PREVIEW_BTN, wxT("Preview"),
         wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator,
        wxT("PreviewBtn"));
    wxBoxSizer2->Add(m_previewBtn,0,wxGROW | wxALL,5);

    // checkbox for Install
    m_InstallCheckBox= new wxCheckBox(this,ID_INSTALLCHECKBOX,wxT("Install")
        ,wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator,
        wxT("InstallCheckbox"));
    wxBoxSizer2->Add(m_InstallCheckBox,0,wxGROW | wxALL,5);

    // horizontal sizer
    wxBoxSizer* wxBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    styleSizer->Add(wxBoxSizer3,0,wxGROW | wxALL,0);

    // File size
    wxStaticText* size= new wxStaticText(this,wxID_ANY,wxT("Filesize:"));
    wxBoxSizer3->Add(size,0,wxGROW | wxALL,5);

    m_size= new wxStaticText(this,wxID_ANY,wxT(""));
    wxBoxSizer3->Add(m_size,0,wxGROW | wxALL,5);

    // Description
    wxStaticText* desc= new wxStaticText(this,wxID_ANY,wxT("Description:"));
    styleSizer->Add(desc,0,wxGROW | wxALL,5);

    m_desc= new wxStaticText(this,wxID_ANY,wxT(""));
    styleSizer->Add(m_desc,0,wxGROW | wxALL,5);

    SetSizer(mainSizer);
    mainSizer->Fit(this);

}

bool wxThemesPage::TransferDataFromWindow()
{
    gv->themesToInstall.Clear();

    for(int i=0; i < m_installTheme.GetCount(); i++)
    {
        if(m_installTheme[i])
        {
            gv->themesToInstall.Add(m_Themes_path[i]);
        }
    }

    return true;

}

void wxThemesPage::OnWizardPageChanging(wxWizardEvent& event)
{
   if(event.GetDirection())  // going forwards in the Wizard
   {
        if(gv->themesToInstall.GetCount() == 0)
        {
            WARN_DIALOG(wxT("You have not selected a Theme to Install"), wxT("Select a Theme"));
            event.Veto();
        }
   }
}

void wxThemesPage::OnCheckBox(wxCommandEvent& event)
{
    int index = ThemesListBox->GetSelection();         //get Index
    if(index == wxNOT_FOUND)
        return;

    m_installTheme[index]= ! m_installTheme[index];    // Toggle install

}

void wxThemesPage::OnListBox(wxCommandEvent& event)
{
    int index = ThemesListBox->GetSelection();  //get Index
    if(index == wxNOT_FOUND)
        return;

    m_desc->SetLabel(m_Themes_desc[index]);     //set Desc
    m_desc->Wrap(270);                       // wrap desc
    m_size->SetLabel(m_Themes_size[index]+wxT(" kb"));  //set file size
    m_InstallCheckBox->SetValue(m_installTheme[index]);  // set the install checkbox

    this->GetSizer()->Layout();

}

void wxThemesPage::OnPreviewBtn(wxCommandEvent& event)
{

    int index = ThemesListBox->GetSelection();
    if(index == wxNOT_FOUND)
        return;

    wxString src,dest;

    int pos = m_Themes_image[index].Find('/',true);
    wxString filename = m_Themes_image[index](pos+1,m_Themes_image[index].Length());

    dest.Printf(wxT("%s" PATH_SEP "download" PATH_SEP "%s"),
            gv->stdpaths->GetUserDataDir().c_str(),gv->curresolution.c_str());

    if(!wxDirExists(dest))
        wxMkdir(dest);

    //this is a URL no PATH_SEP
    src.Printf("%s/data/%s/%s",gv->themes_url.c_str(),gv->curresolution.c_str(),filename.c_str());
    dest.Printf(wxT("%s" PATH_SEP "download" PATH_SEP "%s" PATH_SEP "%s"),
        gv->stdpaths->GetUserDataDir().c_str(),gv->curresolution.c_str(),filename.c_str());

    if(DownloadURL(src, dest))
    {
        MESG_DIALOG(wxT("Unable to download image."));
        return;
    }

    wxBitmap bmp;
    bmp.LoadFile(dest,wxBITMAP_TYPE_PNG);

    wxThemeImageDialog dlg(this,wxID_ANY,wxT("Preview"),bmp);
    dlg.ShowModal();

}

void wxThemesPage::OnPageShown(wxWizardEvent& event)
{
    // clear Theme info
    m_Themes.Clear();
    m_Themes_image.Clear();
    m_Themes_path.Clear();
    m_Themes_desc.Clear();
    m_Themes_size.Clear();
    m_installTheme.Clear();
    m_desc->SetLabel(wxT(""));
    m_size->SetLabel(wxT(""));
    m_InstallCheckBox->SetValue(false);

    //get correct Themes list
    wxString src,dest,err;

    src.Printf("%srbutil.php?res=%s",gv->themes_url.c_str(),gv->curresolution.c_str());
    dest.Printf(wxT("%s" PATH_SEP "download" PATH_SEP "%s.list"),
        gv->stdpaths->GetUserDataDir().c_str(),gv->curresolution.c_str());

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
        m_installTheme.Add(false);                //Theme should be installed ?
    }

    // set ThemeList
    ThemesListBox->Set(m_Themes);
}

 //////////////////// Ipod Locaction Page /////////////////////////////
BEGIN_EVENT_TABLE(wxIpodLocationPage,wxWizardPageSimple)
	EVT_BUTTON   (ID_IPODLOCATION_BTN, wxIpodLocationPage::OnIpodLocationBtn)
	EVT_WIZARD_PAGE_CHANGING(wxID_ANY, wxIpodLocationPage::OnWizardPageChanging)
END_EVENT_TABLE();

wxIpodLocationPage::wxIpodLocationPage(wxWizard* parent) : wxWizardPageSimple(parent)
{

    wxBoxSizer* WxBoxSizer1 = new wxBoxSizer(wxVERTICAL);

    // Header text
    IpodLocationLabel = new wxStaticText(this, wxID_ANY,
        wxT("Rockbox utility needs to know the device where your ipod\n"
            "device is located on your computer. Use the\n"
            "Scan Button:"));
    WxBoxSizer1->Add(IpodLocationLabel,0,wxGROW | wxALL, 5);

    // device location
   	wxBoxSizer* WxBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
   	WxBoxSizer1->Add(WxBoxSizer3,0,
        wxGROW | wxALIGN_CENTER_VERTICAL | wxALL, 5);

  	IpodLocationText = new wxStaticText(this, wxID_ANY, "",
        wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
    WxBoxSizer3->Add(IpodLocationText,1,
        wxGROW | wxALIGN_CENTER_VERTICAL | wxALL, 5);

    IpodLocationBtn = new wxButton(this, ID_IPODLOCATION_BTN, wxT("Scan"),
        wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator,
        wxT("IpodLocationBtn"));
   	IpodLocationBtn->SetToolTip(wxT("Scan for your Ipod"));
   	WxBoxSizer3->Add(IpodLocationBtn,0,wxALIGN_CENTER_VERTICAL | wxALL, 5);

     // Extra text
    IpodLocationExtraText = new wxStaticText(this,wxID_ANY, wxT(""));
    WxBoxSizer1->Add(IpodLocationExtraText,0,wxGROW | wxALL, 5);

   	SetSizer(WxBoxSizer1);
    WxBoxSizer1->Fit(this);
}

wxWizardPage* wxIpodLocationPage::GetPrev() const
{
    if(gv->curbootloadermethod == wxT("ipodpatcher"))   //if ipod, skip previous
    {
        if(wxWizardPageSimple::GetPrev()->GetPrev() != NULL)
            return wxWizardPageSimple::GetPrev()->GetPrev();

    }
}

void wxIpodLocationPage::OnWizardPageChanging(wxWizardEvent& event)
{
   if(event.GetDirection())  // going forwards in the Wizard
   {
       if(gv->curbootloadermethod==wxT("ipodpatcher"))
       {
            if(IpodLocationText->GetLabel() == wxT("no Ipod found") ||
                    IpodLocationText->GetLabel() ==wxT("More than 1 Ipod found") ||
                    IpodLocationText->GetLabel() ==wxT(""))
            {
                WARN_DIALOG(wxT("No valid ipod found!"), wxT("Select Location"));
                event.Veto();       //stop pagechanging
            }
       }
   }
}

void wxIpodLocationPage::OnIpodLocationBtn(wxCommandEvent& event)
{
    wxLogVerbose(wxT("=== begin wxIpodLocationPage::OnIpodLocationBtn"));
    struct ipod_t ipod;
    int n = ipod_scan(&ipod);
    gv->curbootloader=wxT("");

    if(n == 0)
      IpodLocationText->SetLabel(wxT("no Ipod found"));
    else if( n==1)
    {
      gv->curbootloader=wxT("bootloader-");
      gv->curbootloader.Append(ipod.targetname);
      IpodLocationText->SetLabel(ipod.modelstr);
    }
    else
      IpodLocationText->SetLabel(wxT("More than 1 Ipod found"));

    if(ipod.macpod)
      IpodLocationExtraText->SetLabel(wxT("This Ipod is a Mac formated Ipod\n"
                                      "Rockbox will not work on this.\n"
                                      "You have to convert it first to Fat32"));
    wxLogVerbose(wxT("=== end wxIpodLocationPage::OnIpodLocationBtn"));

}

BEGIN_EVENT_TABLE(wxBootLocationPage,wxWizardPageSimple)
	EVT_BUTTON   (ID_BOOTLOCATION_BTN, wxBootLocationPage::OnBootLocationBtn)
	EVT_WIZARD_PAGE_CHANGING(wxID_ANY, wxBootLocationPage::OnWizardPageChanging)
	EVT_WIZARD_PAGE_CHANGED (wxID_ANY, wxBootLocationPage::OnPageShown)
END_EVENT_TABLE();

wxBootLocationPage::wxBootLocationPage(wxWizard* parent) : wxWizardPageSimple(parent)
    {

    wxBoxSizer* WxBoxSizer1 = new wxBoxSizer(wxVERTICAL);

    // Header text
    BootLocationLabel = new wxStaticText(this, wxID_ANY,
        wxT("Rockbox utility needs to know the folder where your audio\n"
            "device is located on your computer.  Currently Rockbox utility\n"
            "is configured to use the following location:"));
    WxBoxSizer1->Add(BootLocationLabel,0,wxGROW | wxALL, 5);

    // device location
   	wxBoxSizer* WxBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
   	WxBoxSizer1->Add(WxBoxSizer3,0,
        wxGROW | wxALIGN_CENTER_VERTICAL | wxALL, 5);

    if (gv->curdestdir == wxT("")) gv->curdestdir = wxT("<none>");
  	BootLocationText = new wxStaticText(this, wxID_ANY, gv->curdestdir,
        wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
    WxBoxSizer3->Add(BootLocationText,1,
        wxGROW | wxALIGN_CENTER_VERTICAL | wxALL, 5);

   	BootLocationBtn = new wxButton(this, ID_BOOTLOCATION_BTN, wxT("Change"),
        wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator,
        wxT("BootLocationBtn"));
   	BootLocationBtn->SetToolTip(wxT("Select the location of your audio device"));
   	WxBoxSizer3->Add(BootLocationBtn,0,wxALIGN_CENTER_VERTICAL | wxALL, 5);


    BootLocationInfo = new wxStaticText(this, wxID_ANY, wxT(""),
        wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
    WxBoxSizer1->Add(BootLocationInfo,0,
        wxGROW | wxALIGN_CENTER_VERTICAL | wxALL, 5);

   	SetSizer(WxBoxSizer1);
    WxBoxSizer1->Fit(this);
}

wxWizardPage* wxBootLocationPage::GetPrev() const
{
    if(gv->curbootloadermethod != wxT("fwpatcher"))
    {
        if(wxWizardPageSimple::GetPrev()->GetPrev() != NULL)
            return wxWizardPageSimple::GetPrev()->GetPrev();

    }

    return wxWizardPageSimple::GetPrev();
}

wxWizardPage* wxBootLocationPage::GetNext() const
{
    if(gv->curbootloadermethod == wxT("ipodpatcher"))
    {
        return wxWizardPageSimple::GetNext();  // if ipod then this is not the last page
    }
    else return NULL;  // else this is the last page
}

void wxBootLocationPage::OnPageShown(wxWizardEvent& event)
{
   if(gv->curplat == wxT("h10") || gv->curplat == wxT("h10_5gb"))
   {
      BootLocationInfo->SetLabel(wxT("Your Device needs to be in UMS Mode. \n\n"
                                     "If it is an MTP device, you can do this by \n"
                                     "reseting you Device via the Pinhole,or disconnecting the Battery \n"
                                     "then connecting it via the Data cable with the PC. \n"
                                     "Then press and hold Next,push the Power button, and \n"
                                     "continue to hold the Next button until the \n"
                                     "USB-Connected Screen appears." ));
   }
   else
   {
      BootLocationInfo->SetLabel("");
   }

}

void wxBootLocationPage::OnWizardPageChanging(wxWizardEvent& event)
{
   if(event.GetDirection())  // going forwards in the Wizard
   {
      if(!wxDirExists(BootLocationText->GetLabel()))
      {
          WARN_DIALOG(wxT("You have not selected a valid location for your audio "
                    "device"), wxT("Select Location"));
           event.Veto();
      }

   }
}

bool wxBootLocationPage::TransferDataFromWindow()
{
    gv->curdestdir = BootLocationText->GetLabel();
    return true;

}


void wxBootLocationPage::OnBootLocationBtn(wxCommandEvent& event)
{
   const wxString& temp = wxDirSelector(
        wxT("Please select the location of your audio device"), gv->curdestdir);
   wxLogVerbose(wxT("=== begin wxBootLocationPage::OnBootLocationBtn(event)"));
   if (!temp.empty())
   {
           BootLocationText->SetLabel(temp);
   }
   wxLogVerbose(wxT("=== end wxBootLocationPage::OnBootLocationBtn"));

}


BEGIN_EVENT_TABLE(wxFirmwareLocationPage,wxWizardPageSimple)
	EVT_BUTTON   (ID_FIRMWARELOCATION_BTN, wxFirmwareLocationPage::OnFirmwareFilenameBtn)
	EVT_WIZARD_PAGE_CHANGING(wxID_ANY, wxFirmwareLocationPage::OnWizardPageChanging)
END_EVENT_TABLE();

wxFirmwareLocationPage::wxFirmwareLocationPage(wxWizard* parent) : wxWizardPageSimple(parent)
{

    wxBoxSizer* WxBoxSizer1 = new wxBoxSizer(wxVERTICAL);

    // Header text
    FirmwareLocationText = new wxStaticText(this, wxID_ANY,
        wxT("For this step Rockbox Utility needs an original Firmware.\n"
             "You can download this from the Manufacturers Website."));
    WxBoxSizer1->Add(FirmwareLocationText,0,wxGROW | wxALL, 5);

    // Filename text
    wxBoxSizer* WxBoxSizer4 = new wxBoxSizer(wxHORIZONTAL);
   	WxBoxSizer1->Add(WxBoxSizer4,0,
        wxGROW | wxALIGN_CENTER_VERTICAL | wxALL, 5);

    FirmwareLocationFilename = new wxStaticText(this, wxID_ANY, gv->curfirmware,
        wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
    WxBoxSizer4->Add(FirmwareLocationFilename,1,
        wxGROW | wxALIGN_CENTER_VERTICAL | wxALL, 5);

    // Button
    FirmwareLocationBtn = new wxButton(this, ID_FIRMWARELOCATION_BTN, wxT("Explore"),
        wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator,
        wxT("FirmwareLocationBtn"));
    FirmwareLocationBtn->SetToolTip(wxT("Select the location of the downloaded Firmware"));
    WxBoxSizer4->Add(FirmwareLocationBtn,0,wxALIGN_CENTER_VERTICAL | wxALL, 5);


   	SetSizer(WxBoxSizer1);
    WxBoxSizer1->Fit(this);
}

void wxFirmwareLocationPage::OnWizardPageChanging(wxWizardEvent& event)
{
   if(event.GetDirection())  // going forwards in the Wizard
   {
        if( !wxFileExists(gv->curfirmware))
        {
            WARN_DIALOG(wxT("You have not selected a valid location for the firmware "
                      "file"), wxT("Select File"));
            event.Veto();
        }
   }

}

void wxFirmwareLocationPage::OnFirmwareFilenameBtn(wxCommandEvent& event)
{
     wxString temp = wxFileSelector(
        wxT("Please select the location of the original Firmware"), gv->curdestdir,"","","*.hex");
       wxLogVerbose(wxT("=== begin wxFirmwareLocationPage::OnFirmwareFilenameBtn(event)"));
       if (!temp.empty())
       {
           gv->curfirmware=temp;
           if(temp.Length() > 30)
           {
                temp.Remove(0, temp.Length()-30);
                temp.Prepend("...");
           }
           FirmwareLocationFilename->SetLabel(temp);
       }
       wxLogVerbose(wxT("=== end wxFirmwareLocationPage::OnFirmwareFilenameBtn"));
}

BEGIN_EVENT_TABLE(wxLocationPage,wxWizardPageSimple)
	EVT_BUTTON   (ID_LOCATION_BTN, wxLocationPage::OnLocationBtn)
	EVT_WIZARD_PAGE_CHANGING(wxID_ANY, wxLocationPage::OnWizardPageChanging)
	EVT_WIZARD_PAGE_CHANGED(wxID_ANY, wxLocationPage::OnPageShown)
END_EVENT_TABLE();

wxLocationPage::wxLocationPage(wxWizard* parent) : wxWizardPageSimple(parent)
    {
    wxBoxSizer* WxBoxSizer1 = new wxBoxSizer(wxVERTICAL);

    wxStaticText* WxStaticText1 = new wxStaticText(this, wxID_ANY,
        wxT("Rockbox utility needs to know the folder where your audio\n"
            "device is located on your computer.  Currently Rockbox utility\n"
            "is configured to use the following location:"));
    WxBoxSizer1->Add(WxStaticText1,0,wxGROW | wxALL, 5);

   	wxBoxSizer* WxBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
   	WxBoxSizer1->Add(WxBoxSizer3,0,
        wxGROW | wxALIGN_CENTER_VERTICAL | wxALL, 5);

    if (gv->curdestdir == wxT("")) gv->curdestdir = wxT("<none>");
  	LocationText = new wxStaticText(this, wxID_ANY, gv->curdestdir,
        wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
    WxBoxSizer3->Add(LocationText,1,
        wxGROW | wxALIGN_CENTER_VERTICAL | wxALL, 5);

   	wxButton* LocationBtn = new wxButton(this, ID_LOCATION_BTN, wxT("Change"),
        wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator,
        wxT("LocationBtn"));
   	LocationBtn->SetToolTip(wxT("Select the location of your audio device"));
   	WxBoxSizer3->Add(LocationBtn,0,wxALIGN_CENTER_VERTICAL | wxALL, 5);

   	LocationInfo = new wxStaticText(this, wxID_ANY, wxT(""),
        wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
    WxBoxSizer1->Add(LocationInfo,0,wxGROW | wxALL, 5);

   	SetSizer(WxBoxSizer1);
    WxBoxSizer1->Fit(this);
}


void wxLocationPage::OnPageShown(wxWizardEvent& event)
{
   if(gv->curplat == wxT("h10") || gv->curplat == wxT("h10_5gb"))
   {
      LocationInfo->SetLabel(wxT("Your Device needs to be in UMS Mode. \n\n"
                                     "If it is an MTP device, you can do this by \n"
                                     "reseting you Device via the Pinhole,or disconnecting the Battery \n"
                                     "then connecting it via the Data cable with the PC. \n"
                                     "Then press and hold Next,push the Power button, and \n"
                                     "continue to hold the Next button until the \n"
                                     "USB-Connected Screen appears." ));
   }
   else
   {
      LocationInfo->SetLabel("");
   }


}

void wxLocationPage::OnWizardPageChanging(wxWizardEvent& event)
{
   if(event.GetDirection())  // going forwards in the Wizard
   {
        if(!wxDirExists(LocationText->GetLabel()))
        {
              WARN_DIALOG(wxT("You have not selected a valid location for your audio "
            "device"), wxT("Select Location"));
            event.Veto();
        }
   }
}

bool wxLocationPage::TransferDataFromWindow()
{
    gv->curdestdir = LocationText->GetLabel();
    return true;
}

void wxLocationPage::OnLocationBtn(wxCommandEvent& event)
{
    const wxString& temp = wxDirSelector(
        wxT("Please select the location of your audio device"), gv->curdestdir);
    wxLogVerbose(wxT("=== begin wxLocationPage::OnLocationBtn(event)"));
    if (!temp.empty())
    {
        LocationText->SetLabel(temp);
    }
    wxLogVerbose(wxT("=== end wxLocationPage::OnLocationBtn"));
}

BEGIN_EVENT_TABLE(wxBuildPage,wxWizardPageSimple)
	EVT_RADIOBOX                     (ID_BUILD_BOX, wxBuildPage::OnBuildBox)
	EVT_WIZARD_PAGE_CHANGED          (wxID_ANY, wxBuildPage::OnPageShown)
END_EVENT_TABLE();


wxBuildPage::wxBuildPage(wxWizard *parent) : wxWizardPageSimple(parent)
{
    wxString buf;

    wxBoxSizer* WxBoxSizer1 = new wxBoxSizer(wxVERTICAL);

    wxStaticText* WxStaticText1 = new wxStaticText(this, wxID_ANY,
    wxT("Please select the Rockbox version you would like "
        "to install on your audio\ndevice:"));
    WxBoxSizer1->Add(WxStaticText1,0,wxGROW | wxALL,5);

    wxArrayString* array = new wxArrayString();
    buf.Printf(wxT("Rockbox stable version (%s)") , gv->last_release.c_str());
    array->Add(buf);
    array->Add(wxT("Archived Build"));
    array->Add(wxT("Current Build "));

    BuildRadioBox = new wxRadioBox(this, ID_BUILD_BOX, wxT("Version"),
        wxDefaultPosition, wxDefaultSize, *array, 0, wxRA_SPECIFY_ROWS);
    WxBoxSizer1->Add(BuildRadioBox, 0, wxGROW | wxALL, 5);
    delete array;

    wxStaticBox* WxStaticBox1 = new wxStaticBox(this, wxID_ANY, wxT("Details:"));
    wxStaticBoxSizer* WxStaticBoxSizer2 = new wxStaticBoxSizer(WxStaticBox1,
        wxVERTICAL);
    DetailText = new wxStaticText(this, wxID_ANY, wxT(""));
    WxBoxSizer1->Add(WxStaticBoxSizer2, 1, wxGROW | wxALL, 5);
    WxStaticBoxSizer2->Add(DetailText, 1, wxGROW | wxALL, 5);

    wxStaticText* WxStaticText2 = new wxStaticText(this, wxID_ANY,
        wxT("Rockbox Utility stores copies of Rockbox it has downloaded on the\n"
        "local hard disk to save network traffic.  If your local copy is\n"
        "no longer working, tick this box to download a fresh copy.") );
    WxBoxSizer1->Add(WxStaticText2, 0 , wxALL, 5);

    NoCacheCheckBox = new wxCheckBox(this, wxID_ANY,
        wxT("Don't use locally cached copies of Rockbox") );
    WxBoxSizer1->Add(NoCacheCheckBox, 0, wxALL, 5);

    SetSizer(WxBoxSizer1);
    WxBoxSizer1->Fit(this);
    WxBoxSizer1->SetSizeHints(this);
}

bool wxBuildPage::TransferDataFromWindow()
{
    gv->curbuild = BuildRadioBox->GetSelection();
    gv->nocache = (gv->curbuild == BUILD_BLEEDING) ? true :
                  NoCacheCheckBox->IsChecked();
    return true;
}

void wxBuildPage::OnBuildBox(wxCommandEvent& event)
{
    wxString str;

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
}

void wxBuildPage::OnPageShown(wxWizardEvent& event)
{
    wxCommandEvent updateradiobox(wxEVT_COMMAND_RADIOBOX_SELECTED,
        ID_BUILD_BOX);

    if (gv->plat_released[gv->curplatnum] )
    {
        BuildRadioBox->Enable(BUILD_RELEASE, true);
        BuildRadioBox->SetSelection(BUILD_RELEASE);
    } else {
        BuildRadioBox->Enable(BUILD_RELEASE, false);
        BuildRadioBox->SetSelection(BUILD_DAILY);

    }

    wxPostEvent(this, updateradiobox);
}

wxFullUninstallPage::wxFullUninstallPage(wxWizard* parent) :
    wxWizardPageSimple(parent)
{
    wxString buf;

	wxBoxSizer* WxBoxSizer1 = new wxBoxSizer(wxVERTICAL);

    wxStaticText* WxStaticText1 = new wxStaticText(this, wxID_ANY,
        wxT("Rockbox Utility normally uninstalls Rockbox using an uninstall\n"
          "file created during installation.  This means that when Rockbox is\n"
          "uninstalled all your configuration files are preserved.  However,\n"
          "you can also perform a full uninstall, which will completely\n"
          "remove all traces of Rockbox from your system, and can be used\n"
          "even if Rockbox was previously installed manually.\n\n"
          "Archos users will need to reinstall any firmware upgrades obtained\n"
          "from Archos after a full uninstall."));
    WxBoxSizer1->Add(WxStaticText1,0,wxGROW | wxALL,5);

    FullCheckBox = new wxCheckBox(this, wxID_ANY,
        wxT("Perform a full uninstall"));
    WxBoxSizer1->Add(FullCheckBox, 0, wxALL, 5);

    SetSizer(WxBoxSizer1);
    WxBoxSizer1->Fit(this);
    WxBoxSizer1->SetSizeHints(this);
}

bool wxFullUninstallPage::TransferDataFromWindow()
{
    gv->curisfull = FullCheckBox->IsChecked();
    return true;
}
