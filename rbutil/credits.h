#ifndef CREDITS_H_INCLUDED
#define CREDITS_H_INCLUDED

#define RBUTIL_FULLNAME "The Rockbox Utility"
#define RBUTIL_VERSION "Version 0.2.0.0"

static char* rbutil_developers[] = {
    "Christi Alice Scarborough",
    ""
};

//static char* rbutil_translators[] = (
//    ""
//);

#define RBUTIL_WEBSITE "http://www.rockbox.org/"
#define RBUTIL_COPYRIGHT "(C) 2005-6 The Rockbox Team - " \
        "released under the GNU Public License v2"
#define RBUTIL_DESCRIPTION "Utility for performing housekeepng tasks for" \
        "the Rockbox audio jukebox firmware."


class AboutDlg: public wxDialog
{
	public:
		AboutDlg(rbutilFrm *parent);
		~AboutDlg();
};

#include <wx/hyperlink.h>

#endif // CREDITS_H_INCLUDED
