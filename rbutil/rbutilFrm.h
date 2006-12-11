//---------------------------------------------------------------------------
//
// Name:        rbutilFrm.h
// Author:      Christi Scarborough
// Created:     03/12/2005 00:35:02
//
//---------------------------------------------------------------------------
#ifndef __rbutilFrm_HPP_
#define __rbutilFrm_HPP_

#include <wx/wxprec.h>
#ifdef __BORLANDC__
        #pragma hdrstop
#endif
#ifndef WX_PRECOMP
        #include <wx/wx.h>
#endif

#include <wx/menu.h>
#include <wx/stattext.h>
#include <wx/bmpbuttn.h>
#include <wx/statbmp.h>
#include <wx/panel.h>

#include <wx/frame.h>
#include <wx/valgen.h>

#include "rbutil.h"
#include "wizard_pages.h"

class rbutilFrm : public wxFrame
{
private:
    DECLARE_EVENT_TABLE()
public:
    rbutilFrm( wxWindow *parent, wxWindowID id = 1,
        const wxString &title = wxT("Rockbox Utility"),
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxCAPTION | wxRESIZE_BORDER | wxSYSTEM_MENU |
            wxMINIMIZE_BOX | wxCLOSE_BOX);
    virtual ~rbutilFrm();
public:
		wxMenuBar *WxMenuBar1;
		wxStaticText *WxStaticText3;
		wxBitmapButton *WxBitmapButton2;
		wxStaticText *WxStaticText2;
		wxBitmapButton *WxBitmapButton1;
		wxBitmapButton *WxBitmapButton3;
		wxBitmapButton *WxBitmapButton4;
		wxFlexGridSizer *WxFlexGridSizer1;
		wxStaticBoxSizer *WxStaticBoxSizer3;
		wxStaticBitmap *WxStaticBitmap1;
		wxBoxSizer *WxBoxSizer2;
		wxPanel *WxPanel1;
		wxBoxSizer *WxBoxSizer1;
public:
	enum {
			ID_FILE_MENU = 1033,
			ID_FILE_EXIT = 1034,
			ID_FILE_ABOUT = 1035,
			ID_FILE_WIPECACHE = 1036,

			ID_WXSTATICTEXT3 = 1032,
			ID_REMOVE_BTN = 1031,
			ID_WXSTATICTEXT2 = 1029,
			ID_INSTALL_BTN = 1028,
			ID_WXSTATICBITMAP1 = 1053,
            ID_FONT_BTN = 1128,
            ID_BOOTLOADER_BTN = 1129,
			ID_WXPANEL1 = 1064,

            ID_DUMMY_VALUE_
   }; //End of Enum
public:
    void rbutilFrmClose(wxCloseEvent& event);
    void CreateGUIControls(void);
	void OnFileExit(wxCommandEvent& event);
	void OnFileAbout(wxCommandEvent &event);
	void OnFileWipeCache(wxCommandEvent &event);
	void OnLocationBtn(wxCommandEvent& event);
	void OnInstallBtn(wxCommandEvent& event);
	void OnRemoveBtn(wxCommandEvent& event);
	void OnFontBtn(wxCommandEvent& event);
	void OnBootloaderBtn(wxCommandEvent& event);

};

#endif




