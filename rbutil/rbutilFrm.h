/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * Module: rbutil
 * File: rbutilFrm.h
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
//#include <wx/aboutdlg.h>
#include <wx/richtext/richtextctrl.h>

#include "rbutil.h"
#include "rbutilCtrls.h"

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
            wxMINIMIZE_BOX|wxMAXIMIZE_BOX | wxCLOSE_BOX);
    virtual ~rbutilFrm();
public:
	    DeviceSelectorCtrl* myDeviceSelector;
	    wxHyperlinkCtrl* manuallink;
	    wxHtmlWindow* manual;
        wxString curManualDevice;

        wxMenuBar *WxMenuBar1;
		wxStaticText *WxStaticText3;
		wxBitmapButton *WxBitmapButton2;
		wxStaticText *WxStaticText2;
		wxBitmapButton *WxBitmapButton1;
		wxBitmapButton *WxBitmapButton3;
		wxBitmapButton *WxBitmapButton4;
		wxBitmapButton *WxBitmapButton5;
		wxBitmapButton *WxBitmapButton6;
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
			ID_PORTABLE_INSTALL = 1037,

			ID_WXSTATICTEXT3 = 1032,
			ID_WXSTATICTEXT4 = 1032,
			ID_REMOVE_BTN = 1039,
			ID_BOOTLOADERREMOVE_BTN = 1038,
			ID_WXSTATICTEXT2 = 1029,
			ID_INSTALL_BTN = 1028,
			ID_WXSTATICBITMAP1 = 1053,
            ID_FONT_BTN = 1128,
            ID_THEMES_BTN = 1139,
            ID_DOOM_BTN = 1140,
            ID_BOOTLOADER_BTN = 1129,
			ID_WXPANEL1 = 1064,

            ID_MANUAL = 1065,
            ID_FILE_PROXY = 1066,

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
	void OnThemesBtn(wxCommandEvent& event);
	void OnBootloaderBtn(wxCommandEvent& event);
	void OnPortableInstall(wxCommandEvent& event);
	void OnBootloaderRemoveBtn(wxCommandEvent& event);
    void OnManualUpdate(wxUpdateUIEvent& event);
    void OnFileProxy(wxCommandEvent& event);
    void OnDoomBtn(wxCommandEvent& event);

    int GetDeviceId();

};

#endif




