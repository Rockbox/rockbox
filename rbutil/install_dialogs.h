#ifndef INSTALL_DIALOGS_H_INCLUDED
#define INSTALL_DIALOGS_H_INCLUDED

#include "rbutil.h"

#include "rbutilCtrls.h"
class bootloaderInstallDlg : public wxDialog
{
    DECLARE_CLASS( bootloaderInstallDlg )
    DECLARE_EVENT_TABLE()
public:
enum {
			ID_DEVICEPOS = 1002,
			ID_FIRMWARE = 1003,
     }; //End of Enum
public:
	bootloaderInstallDlg( );
    bootloaderInstallDlg( wxWindow* parent,
            wxWindowID id = wxID_ANY,
            const wxString& caption = wxT("Bootloader Installation"),
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
    // Member initialization
    void Init();
    //Creation
    bool Create( wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxString& caption = wxT("Bootloader Installation"),
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxDEFAULT_DIALOG_STYLE |wxRESIZE_BORDER );
    // Creates the controls and sizers
    void CreateControls();

    bool TransferDataFromWindow();
    bool TransferDataToWindow();

private:

    DevicePositionCtrl* m_devicepos;
    FirmwarePositionCtrl* m_firmwarepos;


};


class fontInstallDlg : public wxDialog
{
    DECLARE_CLASS( fontInstallDlg )
    DECLARE_EVENT_TABLE()
public:
enum {
			ID_DEVICEPOS = 1002,
     }; //End of Enum
public:
	fontInstallDlg( );
    fontInstallDlg( wxWindow* parent,
            wxWindowID id = wxID_ANY,
            const wxString& caption = wxT("Font Installation"),
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER  );
    /// Member initialization
    void Init();
    /// Creation
    bool Create( wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxString& caption = wxT("Font Installation"),
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER  );
    /// Creates the controls and sizers
    void CreateControls();

    bool TransferDataFromWindow();
    bool TransferDataToWindow();

private:
     DevicePositionCtrl* m_devicepos;
};


class rockboxDeInstallDlg : public wxDialog
{
    DECLARE_CLASS( rockboxDeInstallDlg )
    DECLARE_EVENT_TABLE()
public:
enum {
			ID_DEVICEPOS = 1002,
			ID_FULL_CHCKBX = 1003,
     }; //End of Enum
public:
	rockboxDeInstallDlg( );
    rockboxDeInstallDlg( wxWindow* parent,
            wxWindowID id = wxID_ANY,
            const wxString& caption = wxT("Rockbox Deinstallation"),
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style =wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER  );
    /// Member initialization
    void Init();
    /// Creation
    bool Create( wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxString& caption = wxT("Rockbox Deinstallation"),
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style =wxDEFAULT_DIALOG_STYLE |wxRESIZE_BORDER );
    /// Creates the controls and sizers
    void CreateControls();

    bool TransferDataFromWindow();
    bool TransferDataToWindow();

private:
    DevicePositionCtrl* m_devicepos;
};

class themesInstallDlg : public wxDialog
{
    DECLARE_CLASS( themesInstallDlg )
    DECLARE_EVENT_TABLE()
public:
enum {
			ID_DEVICE = 1001,
			ID_DEVICEPOS = 1002,
			ID_THEME = 1006,
     }; //End of Enum
public:
	themesInstallDlg( );
    themesInstallDlg( wxWindow* parent,
            wxWindowID id = wxID_ANY,
            const wxString& caption = wxT("Themes Installation"),
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxDEFAULT_DIALOG_STYLE |wxRESIZE_BORDER );
    // Creation
    bool Create( wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxString& caption = wxT("Themes Installation"),
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxDEFAULT_DIALOG_STYLE |wxRESIZE_BORDER );
    // Creates the controls and sizers
    void CreateControls();

    bool TransferDataFromWindow();
    bool TransferDataToWindow();

private:
    wxString currentPreview;
    DevicePositionCtrl* m_devicepos;
    ThemeCtrl*  m_theme;

};

class rockboxInstallDlg : public wxDialog
{
    DECLARE_CLASS( rockboxInstallDlg )
    DECLARE_EVENT_TABLE()
public:
enum {
			ID_DEVICEPOS = 1002,
			ID_BUILD_BOX = 1006,
			ID_DEVICE_POS_CTRL = 1007,
			ID_DETAIL_TXT = 1008,
			ID_NOCACHE_CHCKBX =1009,

     }; //End of Enum
public:
	rockboxInstallDlg( );
    rockboxInstallDlg( wxWindow* parent,
            wxWindowID id = wxID_ANY,
            const wxString& caption = wxT("Rockbox Installation"),
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxDEFAULT_DIALOG_STYLE |wxRESIZE_BORDER  );
    // Creation
    bool Create( wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxString& caption = wxT("Rockbox Installation"),
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxDEFAULT_DIALOG_STYLE |wxRESIZE_BORDER );
    // Creates the controls and sizers
    void CreateControls();

    void OnBuildBox(wxCommandEvent& event);

    bool TransferDataFromWindow();
    bool TransferDataToWindow();

private:
    DevicePositionCtrl* m_devicepos;
};

#endif // INSTALL_DIALOGS_H_INCLUDED
