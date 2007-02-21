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

private:
    wxStaticText* LocationText;
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
