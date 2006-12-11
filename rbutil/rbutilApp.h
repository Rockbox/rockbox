//---------------------------------------------------------------------------
//
// Name:        rbutilApp.h
// Author:      Christi Scarborough
// Created:     03/12/2005 00:35:02
//
//---------------------------------------------------------------------------

#include <wx/wxprec.h>
#ifdef __BORLANDC__
        #pragma hdrstop
#endif
#ifndef WX_PRECOMP
        #include <wx/wx.h>
#endif

#include <wx/msgdlg.h>
#include <wx/config.h>
#include <wx/confbase.h>
#include <wx/fileconf.h>
#include <wx/string.h>
#include <wx/wfstream.h>
#include <wx/fs_inet.h>
#include <wx/fs_zip.h>
#include <wx/stdpaths.h>

#include "rbutilFrm.h"
#include "rbutil.h"

class rbutilFrmApp:public wxApp
{
public:
	bool OnInit();
	int OnExit();
	bool ReadGlobalConfig(rbutilFrm* myFrame);
	void ReadUserConfig(void);
	void WriteUserConfig(void);

};

 
