#ifndef RBUTILCTRLS_H_INCLUDED
#define RBUTILCTRLS_H_INCLUDED

#include "rbutil.h"


class ImageCtrl: public wxControl
{
DECLARE_DYNAMIC_CLASS(ImageCtrl)
DECLARE_EVENT_TABLE()

public:
    // Constructors
    ImageCtrl() { }
    ImageCtrl(wxWindow* parent, wxWindowID id,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxNO_BORDER,
        const wxValidator& validator = wxDefaultValidator)
        {
            Create(parent, id, pos, size, style, validator);
        }
    // Creation
    bool Create(wxWindow* parent, wxWindowID id,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxNO_BORDER,
        const wxValidator& validator = wxDefaultValidator);

    // Event handlers
    void OnPaint(wxPaintEvent& event);

    wxSize DoGetBestSize() const ;

    void SetBitmap(wxBitmap bmp);

protected:
    wxBitmap m_bitmap;

};

class ThemeCtrl: public wxControl
{
DECLARE_DYNAMIC_CLASS(ThemeCtrl)
DECLARE_EVENT_TABLE()
public:
enum {
			ID_DESC = 10001,
			ID_FILESIZE= 10002,
			ID_INSTALLCHECKBOX= 10003,
			ID_PREVIEW_BITMAP = 10004,
			ID_THEME_LST = 10005,
     }; //End of Enum

public:
    // Constructors
    ThemeCtrl() { Init(); }
    ThemeCtrl(wxWindow* parent, wxWindowID id,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxNO_BORDER,
        const wxValidator& validator = wxDefaultValidator)
        {
            Init();
            Create(parent, id, pos, size, style, validator);
        }
    // Creation
    bool Create(wxWindow* parent, wxWindowID id,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxNO_BORDER,
        const wxValidator& validator = wxDefaultValidator);

     // Creates the controls and sizers
   void CreateControls();
   // Common initialization
   void Init();
   // Event handlers
   void OnThemesLst(wxCommandEvent& event);
   void OnCheckBox(wxCommandEvent& event);

   void setDevice(wxString device);
   wxArrayString getThemesToInstall();

protected:
    wxString m_currentimage;
    wxString m_currentResolution;

    wxStaticText* m_desc;
    wxListBox* m_themeList;
    wxStaticText* m_size;
    wxTextCtrl* m_themedesc;
    ImageCtrl* m_PreviewBitmap;

    wxArrayString m_Themes;
    wxArrayString m_Themes_path;
    wxArrayString m_Themes_size;
    wxArrayString m_Themes_image;
    wxArrayString m_Themes_desc;

};

class OkCancelCtrl: public wxControl
{
DECLARE_DYNAMIC_CLASS(OkCancelCtrl)
DECLARE_EVENT_TABLE()

public:
    // Constructors
    OkCancelCtrl() { Init(); }
    OkCancelCtrl(wxWindow* parent, wxWindowID id,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxNO_BORDER,
        const wxValidator& validator = wxDefaultValidator)
        {
            Init();
            Create(parent, id, pos, size, style, validator);
            }
    // Creation
    bool Create(wxWindow* parent, wxWindowID id,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxNO_BORDER,
        const wxValidator& validator = wxDefaultValidator);

     // Creates the controls and sizers
   void CreateControls();
   // Common initialization
   void Init() { }

protected:
    wxButton* m_OkBtn;
    wxButton* m_CancelBtn;

};

class DeviceSelectorCtrl: public wxControl
{
DECLARE_DYNAMIC_CLASS(DeviceSelectorCtrl)
DECLARE_EVENT_TABLE()
public:
enum {
			ID_DEVICE_CBX = 10001,
			ID_AUTODETECT_BTN= 10002,
     }; //End of Enum

public:
    // Constructors
    DeviceSelectorCtrl() { }
    DeviceSelectorCtrl(wxWindow* parent, wxWindowID id,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxNO_BORDER,
        const wxValidator& validator = wxDefaultValidator)
        {
            Create(parent, id, pos, size, style, validator);
        }
    // Creation
    bool Create(wxWindow* parent, wxWindowID id,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxNO_BORDER,
        const wxValidator& validator = wxDefaultValidator);

     // Creates the controls and sizers
   void CreateControls();
   // Event handlers
   void OnAutoDetect(wxCommandEvent& event);
   void OnComboBox(wxCommandEvent& event);
   // Accessors
   wxString getDevice();
   void setDefault();

protected:
    wxString m_currentDevice;
    wxComboBox* m_deviceCbx;
    wxStaticText* m_desc;
    wxButton* m_autodetectBtn;

};


class DevicePositionCtrl: public wxControl
{
DECLARE_DYNAMIC_CLASS(DevicePositionCtrl)
DECLARE_EVENT_TABLE()
public:
enum {
			ID_BROWSE_BTN = 10003,
     }; //End of Enum

public:
    // Constructors
    DevicePositionCtrl() { Init(); }
    DevicePositionCtrl(wxWindow* parent, wxWindowID id,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxNO_BORDER,
        const wxValidator& validator = wxDefaultValidator)
        {
            Init();
            Create(parent, id, pos, size, style, validator);
            }
    // Creation
    bool Create(wxWindow* parent, wxWindowID id,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxNO_BORDER,
        const wxValidator& validator = wxDefaultValidator);

     // Creates the controls and sizers
   void CreateControls();
   // Common initialization
   void Init() { }
   // Event handlers
   void OnBrowseBtn(wxCommandEvent& event);
   // Accessors
   wxString getDevicePos();

   void setDefault();

protected:
    wxTextCtrl* m_devicePos;
    wxStaticText* m_desc;
    wxButton* m_browseBtn;

};


class FirmwarePositionCtrl: public wxControl
{
DECLARE_DYNAMIC_CLASS(FirmwarePositionCtrl)
DECLARE_EVENT_TABLE()
public:
enum {
			ID_BROWSE_BTN = 10004,
     }; //End of Enum

public:
    // Constructors
    FirmwarePositionCtrl() { Init(); }
    FirmwarePositionCtrl(wxWindow* parent, wxWindowID id,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxNO_BORDER,
        const wxValidator& validator = wxDefaultValidator)
        {
            Init();
            Create(parent, id, pos, size, style, validator);
            }
    // Creation
    bool Create(wxWindow* parent, wxWindowID id,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxNO_BORDER,
        const wxValidator& validator = wxDefaultValidator);

     // Creates the controls and sizers
   void CreateControls();
   // Common initialization
   void Init() { }
   // Event handlers
   void OnBrowseBtn(wxCommandEvent& event);
   // Accessors
   wxString getFirmwarePos();
    void setDefault();

protected:
    wxTextCtrl* m_firmwarePos;
    wxStaticText* m_desc;
    wxButton* m_browseBtn;

};



#endif

#ifndef RBUTILCTRLS_H_INCLUDED
#define RBUTILCTRLS_H_INCLUDED

#include "rbutil.h"


class ImageCtrl: public wxControl
{
DECLARE_DYNAMIC_CLASS(ImageCtrl)
DECLARE_EVENT_TABLE()

public:
    // Constructors
    ImageCtrl() { }
    ImageCtrl(wxWindow* parent, wxWindowID id,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxNO_BORDER,
        const wxValidator& validator = wxDefaultValidator)
        {
            Create(parent, id, pos, size, style, validator);
        }
    // Creation
    bool Create(wxWindow* parent, wxWindowID id,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxNO_BORDER,
        const wxValidator& validator = wxDefaultValidator);

    // Event handlers
    void OnPaint(wxPaintEvent& event);

    wxSize DoGetBestSize() const ;

    void SetBitmap(wxBitmap bmp);

protected:
    wxBitmap m_bitmap;

};

class ThemeCtrl: public wxControl
{
DECLARE_DYNAMIC_CLASS(ThemeCtrl)
DECLARE_EVENT_TABLE()
public:
enum {
			ID_DESC = 10001,
			ID_FILESIZE= 10002,
			ID_INSTALLCHECKBOX= 10003,
			ID_PREVIEW_BITMAP = 10004,
			ID_THEME_LST = 10005,
     }; //End of Enum

public:
    // Constructors
    ThemeCtrl() { Init(); }
    ThemeCtrl(wxWindow* parent, wxWindowID id,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxNO_BORDER,
        const wxValidator& validator = wxDefaultValidator)
        {
            Init();
            Create(parent, id, pos, size, style, validator);
        }
    // Creation
    bool Create(wxWindow* parent, wxWindowID id,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxNO_BORDER,
        const wxValidator& validator = wxDefaultValidator);

     // Creates the controls and sizers
   void CreateControls();
   // Common initialization
   void Init();
   // Event handlers
   void OnThemesLst(wxCommandEvent& event);
   void OnCheckBox(wxCommandEvent& event);

   void setDevice(wxString device);
   wxArrayString getThemesToInstall();

protected:
    wxString m_currentimage;
    wxString m_currentResolution;

    wxStaticText* m_desc;
    wxListBox* m_themeList;
    wxStaticText* m_size;
    wxStaticText* m_themedesc;
    ImageCtrl* m_PreviewBitmap;

    wxArrayString m_Themes;
    wxArrayString m_Themes_path;
    wxArrayString m_Themes_size;
    wxArrayString m_Themes_image;
    wxArrayString m_Themes_desc;

    wxCriticalSection m_setDeviceSection;
    wxCriticalSection m_ThemeSelectSection;

};

class OkCancelCtrl: public wxControl
{
DECLARE_DYNAMIC_CLASS(OkCancelCtrl)
DECLARE_EVENT_TABLE()

public:
    // Constructors
    OkCancelCtrl() { Init(); }
    OkCancelCtrl(wxWindow* parent, wxWindowID id,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxNO_BORDER,
        const wxValidator& validator = wxDefaultValidator)
        {
            Init();
            Create(parent, id, pos, size, style, validator);
            }
    // Creation
    bool Create(wxWindow* parent, wxWindowID id,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxNO_BORDER,
        const wxValidator& validator = wxDefaultValidator);

     // Creates the controls and sizers
   void CreateControls();
   // Common initialization
   void Init() { }

protected:
    wxButton* m_OkBtn;
    wxButton* m_CancelBtn;

};

class DeviceSelectorCtrl: public wxControl
{
DECLARE_DYNAMIC_CLASS(DeviceSelectorCtrl)
DECLARE_EVENT_TABLE()
public:
enum {
			ID_DEVICE_CBX = 10001,
			ID_AUTODETECT_BTN= 10002,
     }; //End of Enum

public:
    // Constructors
    DeviceSelectorCtrl() { }
    DeviceSelectorCtrl(wxWindow* parent, wxWindowID id,bool bootmode,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxNO_BORDER,
        const wxValidator& validator = wxDefaultValidator)
        {
            Create(parent, id,bootmode, pos, size, style, validator);
        }
    // Creation
    bool Create(wxWindow* parent, wxWindowID id,bool bootmode,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxNO_BORDER,
        const wxValidator& validator = wxDefaultValidator);

     // Creates the controls and sizers
   void CreateControls(bool bootmode);
   // Event handlers
   void OnAutoDetect(wxCommandEvent& event);
   void OnComboBox(wxCommandEvent& event);
   // Accessors
   wxString getDevice();
   void setDefault();

protected:
    wxString m_currentDevice;
    wxComboBox* m_deviceCbx;
    wxStaticText* m_desc;
    wxButton* m_autodetectBtn;

};


class DevicePositionCtrl: public wxControl
{
DECLARE_DYNAMIC_CLASS(DevicePositionCtrl)
DECLARE_EVENT_TABLE()
public:
enum {
			ID_BROWSE_BTN = 10003,
     }; //End of Enum

public:
    // Constructors
    DevicePositionCtrl() { Init(); }
    DevicePositionCtrl(wxWindow* parent, wxWindowID id,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxNO_BORDER,
        const wxValidator& validator = wxDefaultValidator)
        {
            Init();
            Create(parent, id, pos, size, style, validator);
            }
    // Creation
    bool Create(wxWindow* parent, wxWindowID id,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxNO_BORDER,
        const wxValidator& validator = wxDefaultValidator);

     // Creates the controls and sizers
   void CreateControls();
   // Common initialization
   void Init() { }
   // Event handlers
   void OnBrowseBtn(wxCommandEvent& event);
   // Accessors
   wxString getDevicePos();

   void setDefault();

protected:
    wxTextCtrl* m_devicePos;
    wxStaticText* m_desc;
    wxButton* m_browseBtn;

};


class FirmwarePositionCtrl: public wxControl
{
DECLARE_DYNAMIC_CLASS(FirmwarePositionCtrl)
DECLARE_EVENT_TABLE()
public:
enum {
			ID_BROWSE_BTN = 10004,
     }; //End of Enum

public:
    // Constructors
    FirmwarePositionCtrl() { Init(); }
    FirmwarePositionCtrl(wxWindow* parent, wxWindowID id,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxNO_BORDER,
        const wxValidator& validator = wxDefaultValidator)
        {
            Init();
            Create(parent, id, pos, size, style, validator);
            }
    // Creation
    bool Create(wxWindow* parent, wxWindowID id,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxNO_BORDER,
        const wxValidator& validator = wxDefaultValidator);

     // Creates the controls and sizers
   void CreateControls();
   // Common initialization
   void Init() { }
   // Event handlers
   void OnBrowseBtn(wxCommandEvent& event);
   // Accessors
   wxString getFirmwarePos();
    void setDefault();

protected:
    wxTextCtrl* m_firmwarePos;
    wxStaticText* m_desc;
    wxButton* m_browseBtn;

};



#endif

