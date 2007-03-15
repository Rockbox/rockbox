/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * Module: rbutil
 * File: wizard_pages.h
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


#ifndef __wizard_pages_HPP_
#define __wizard_pages_HPP_

#include "rbutil.h"


////// Dialog for Preview of Theme //////////////
class wxThemeImageDialog : public wxDialog
{
private:
   DECLARE_EVENT_TABLE()

public:
    wxThemeImageDialog(wxWindow* parent,wxWindowID id,wxString title,wxBitmap bmp);
    void OnPaint(wxPaintEvent& WXUNUSED(event));
    void SetImage(wxBitmap bmp);
    void OnClose(wxCloseEvent& event);

private:
    wxBitmap m_bitmap;
};



class wxThemesPage : public wxWizardPageSimple
{
private:
    DECLARE_EVENT_TABLE()

public:
    enum {
         ID_PREVIEW_BTN      = 1000,
         ID_LISTBOX          = 1001,
         ID_INSTALLCHECKBOX  = 1002,
    };

public:
    wxThemesPage(wxWizard *parent);
    virtual bool TransferDataFromWindow(void);
    void OnPageShown(wxWizardEvent& event);
    void OnPreviewBtn(wxCommandEvent& event);
    void OnListBox(wxCommandEvent& event);
    void OnCheckBox(wxCommandEvent& event);
    void OnWizardPageChanging(wxWizardEvent& event);

public:
   wxListBox* ThemesListBox;
   wxButton* m_previewBtn;
   wxStaticText* m_desc;
   wxStaticText* m_size;
   wxCheckBox* m_InstallCheckBox;
   wxThemeImageDialog* myImageDialog;

   wxArrayString m_Themes;
   wxArrayString m_Themes_path;
   wxArrayString m_Themes_image;
   wxArrayString m_Themes_desc;
   wxArrayString m_Themes_size;
   wxArrayInt    m_installTheme;

};

class wxBootPlatformPage : public wxWizardPageSimple
{
public:
    wxBootPlatformPage(wxWizard *parent);
    virtual bool TransferDataFromWindow(void);
    virtual wxWizardPage *GetNext() const;
    void SetNext(wxWizardPage * next) {wxWizardPageSimple::SetNext(next); my_next = next;}

public:
    wxListBox* BootPlatformListBox;
    wxWizardPage *my_next;
};

class wxIpodLocationPage : public wxWizardPageSimple
{
private:
    DECLARE_EVENT_TABLE()

public:
    enum {
         ID_IPODLOCATION_BTN      = 1000,
    };
public:
    wxIpodLocationPage(wxWizard* parent);
   	void OnIpodLocationBtn(wxCommandEvent& event);
   	void OnWizardPageChanging(wxWizardEvent& event);
   	virtual wxWizardPage *GetPrev() const;
    void SetPrev(wxWizardPage * prev) {wxWizardPageSimple::SetPrev(prev); my_prev = prev;}

private:
    wxStaticText* IpodLocationText;
    wxStaticText* IpodLocationLabel;
    wxStaticText* IpodLocationExtraText;
    wxButton* IpodLocationBtn;
    wxWizardPage *my_prev;
};



class wxBootLocationPage : public wxWizardPageSimple
{
private:
    DECLARE_EVENT_TABLE()

public:
    enum {
         ID_BOOTLOCATION_BTN      = 1000,
    };

public:
    wxBootLocationPage(wxWizard* parent);
    virtual bool TransferDataFromWindow(void);
   	void OnBootLocationBtn(wxCommandEvent& event);
   	void OnPageShown(wxWizardEvent& event);
   	void OnWizardPageChanging(wxWizardEvent& event);
   	virtual wxWizardPage *GetPrev() const;
   	virtual wxWizardPage *GetNext() const;
    void SetPrev(wxWizardPage * prev) {wxWizardPageSimple::SetPrev(prev); my_prev = prev;}

private:
    wxStaticText* BootLocationText;
    wxStaticText* BootLocationLabel;
    wxStaticText* BootLocationInfo;
    wxButton* BootLocationBtn;
    wxWizardPage *my_prev;

};

class wxFirmwareLocationPage : public wxWizardPageSimple
{
  private:
    DECLARE_EVENT_TABLE()

public:
    enum {
         ID_FIRMWARELOCATION_BTN      = 1000,
    };

public:
    wxFirmwareLocationPage(wxWizard* parent);
   	void OnFirmwareFilenameBtn(wxCommandEvent& event);
   	void OnWizardPageChanging(wxWizardEvent& event);

private:
    wxStaticText* FirmwareLocationText;
    wxStaticText* FirmwareLocationFilename;
    wxButton* FirmwareLocationBtn;

};


class wxPlatformPage : public wxWizardPageSimple
{
public:
    wxPlatformPage(wxWizard *parent);
    virtual bool TransferDataFromWindow(void);

public:
    wxListBox* PlatformListBox;
};

class wxLocationPage : public wxWizardPageSimple
{
private:
    DECLARE_EVENT_TABLE()

public:
    enum {
         ID_LOCATION_BTN      = 1000,
    };

public:
    wxLocationPage(wxWizard* parent);
    virtual bool TransferDataFromWindow(void);
    void OnWizardPageChanging(wxWizardEvent& event);
   	void OnLocationBtn(wxCommandEvent& event);
   	void OnPageShown(wxWizardEvent& event);

private:
    wxStaticText* LocationText;
    wxStaticText* BootLocationInfo;
     wxStaticText* LocationInfo;
};

class wxBuildPage : public wxWizardPageSimple
{
private:
    DECLARE_EVENT_TABLE()

public:
   enum {
        ID_BUILD_BOX       = 1000,
   };

public:
    wxBuildPage(wxWizard *parent);
    virtual bool TransferDataFromWindow(void);
    void OnBuildBox(wxCommandEvent& event);
    void OnPageShown(wxWizardEvent& event);

private:
    wxRadioBox* BuildRadioBox;
    wxStaticText* DetailText;
    wxCheckBox* NoCacheCheckBox;
};

class wxFullUninstallPage : public wxWizardPageSimple
{
public:
    wxFullUninstallPage(wxWizard *parent);
    virtual bool TransferDataFromWindow(void);

private:
    wxCheckBox* FullCheckBox;
};


#endif
