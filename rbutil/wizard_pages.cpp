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

wxPlatformPage::wxPlatformPage(wxWizard *parent) : wxWizardPageSimple(parent)
{
    wxBoxSizer* WxBoxSizer1 = new wxBoxSizer(wxVERTICAL);

    wxStaticText* WxStaticText1 = new wxStaticText(this, wxID_ANY,
        _("Please select the model of audio device that you would like to"
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
        WARN_DIALOG(_("You must select an audio device type before proceeding"),
            _("Select Platform"));
        return false;
    } else
    {
        gv->curplatnum = PlatformListBox->GetSelection();
        gv->curplat = gv->plat_id[gv->curplatnum];
        return true;
    }
}

BEGIN_EVENT_TABLE(wxLocationPage,wxWizardPageSimple)
	EVT_BUTTON   (ID_LOCATION_BTN, wxLocationPage::OnLocationBtn)
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

    if (gv->curdestdir == wxT("")) gv->curdestdir = _("<none>");
  	LocationText = new wxStaticText(this, wxID_ANY, gv->curdestdir,
        wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
    WxBoxSizer3->Add(LocationText,1,
        wxGROW | wxALIGN_CENTER_VERTICAL | wxALL, 5);

   	wxButton* LocationBtn = new wxButton(this, ID_LOCATION_BTN, wxT("Change"),
        wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator,
        wxT("LocationBtn"));
   	LocationBtn->SetToolTip(wxT("Select the location of your audio device"));
   	WxBoxSizer3->Add(LocationBtn,0,wxALIGN_CENTER_VERTICAL | wxALL, 5);

   	SetSizer(WxBoxSizer1);
    WxBoxSizer1->Fit(this);
}

bool wxLocationPage::TransferDataFromWindow()
{
    if ( wxDirExists(LocationText->GetLabel() ) )
    {
        gv->curdestdir = LocationText->GetLabel();
        return true;
    } else
    {
        WARN_DIALOG(_("You have not selected a valid location for your audio "
            "device"), _("Select Location"));
        return false;
    }
}

void wxLocationPage::OnLocationBtn(wxCommandEvent& event)
{
    const wxString& temp = wxDirSelector(
        _("Please select the location of your audio device"), gv->curdestdir);
    wxLogVerbose("=== begin wxLocationPage::OnLocationBtn(event)");
    if (!temp.empty())
    {
        LocationText->SetLabel(temp);
    }
    wxLogVerbose("=== end wxLocationPage::OnLocationBtn");
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
    buf.Printf(_("Rockbox stable version (%s)") , gv->last_release.c_str());
    array->Add(buf);
    array->Add(_("Daily Build"));
    array->Add(_("Bleeding Edge Build "));

    BuildRadioBox = new wxRadioBox(this, ID_BUILD_BOX, _("Version"),
        wxDefaultPosition, wxDefaultSize, *array, 0, wxRA_SPECIFY_ROWS);
    WxBoxSizer1->Add(BuildRadioBox, 0, wxGROW | wxALL, 5);
    delete array;

    wxStaticBox* WxStaticBox1 = new wxStaticBox(this, wxID_ANY, _("Details:"));
    wxStaticBoxSizer* WxStaticBoxSizer2 = new wxStaticBoxSizer(WxStaticBox1,
        wxVERTICAL);
    DetailText = new wxStaticText(this, wxID_ANY, wxT(""));
    WxBoxSizer1->Add(WxStaticBoxSizer2, 1, wxGROW | wxALL, 5);
    WxStaticBoxSizer2->Add(DetailText, 1, wxGROW | wxALL, 5);

    wxStaticText* WxStaticText2 = new wxStaticText(this, wxID_ANY,
        _("Rockbox Utility stores copies of Rockbox it has downloaded on the\n"
        "local hard disk to save network traffic.  If your local copy is\n"
        "no longer working, tick this box to download a fresh copy.") );
    WxBoxSizer1->Add(WxStaticText2, 0 , wxALL, 5);

    NoCacheCheckBox = new wxCheckBox(this, wxID_ANY,
        _("Don't use locally cached copies of Rockbox") );
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
                "the last change was made.  This should be avoided unless the\n"
                "daily version is causing problems for some reason.\n\n"
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
        _("Rockbox Utility normally uninstalls Rockbox using an uninstall\n"
          "file created during installation.  This means that when Rockbox is\n"
          "uninstalled all your configuration files are preserved.  However,\n"
          "you can also perform a full uninstall, which will completely\n"
          "remove all traces of Rockbox from your system, and can be used\n"
          "even if Rockbox was previously installed manually.\n\n"
          "Archos users will need to reinstall any firmware upgrades obtained\n"
          "from Archos after a full uninstall."));
    WxBoxSizer1->Add(WxStaticText1,0,wxGROW | wxALL,5);

    FullCheckBox = new wxCheckBox(this, wxID_ANY,
        _("Perform a full uninstall"));
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
