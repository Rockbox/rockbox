#ifndef __wizard_pages_HPP_
#define __wizard_pages_HPP_

#include "rbutil.h"

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
   	void OnLocationBtn(wxCommandEvent& event);

public:
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

public:
    wxRadioBox* BuildRadioBox;
    wxStaticText* DetailText;
    wxCheckBox* NoCacheCheckBox;
};

class wxFullUninstallPage : public wxWizardPageSimple
{
public:
    wxFullUninstallPage(wxWizard *parent);
    virtual bool TransferDataFromWindow(void);

public:
    wxCheckBox* FullCheckBox;
};


#endif
