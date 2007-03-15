/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * Module: rbutil
 * File: installlog.cpp
 *
 * Copyright (C) 2006 Christi Alice Scarborough
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "installlog.h"
#include "rbutil.h"

InstallLog::InstallLog(wxString logname, bool CreateLog)
{
    wxString buf;
    dirtyflag = true;

    if (! CreateLog && ! wxFileExists(logname) ) return;

    logfile = new wxFileConfig(wxEmptyString, wxEmptyString, logname);


    if (!logfile)
    {
        buf.Printf(_("Failed to create install log file: %s"), logname.c_str());
        wxLogWarning(buf);
        return;
    }

    logfile->SetPath(wxT("/InstallLog"));
    if (logfile->Exists(wxT("Version")) &&
        logfile->Read(wxT("Version"), 0l) != LOGFILE_VERSION )
    {
        buf.Printf(_("Logfile version mismatch: %s"), logname.c_str() );
        wxLogWarning(buf);
        delete logfile;
        return;
    }

    logfile->Write(wxT("Version"), LOGFILE_VERSION);
    dirtyflag = false;
}

InstallLog::~InstallLog()
{
    if (dirtyflag) return;

    delete logfile;
}

unsigned int InstallLog::WriteFile(wxString filepath, bool isDir)
{
    wxString key, buf;
    long installcount = 0;

    if (dirtyflag) return true;

    filepath.Replace(PATH_SEP, wxT("/") );

    if (filepath.GetChar(0) == '/')
        filepath = filepath.Right(filepath.Len() - 1);

    logfile->SetPath(wxT("/FilePaths"));
    installcount = logfile->Read(filepath, 0l);

    if (isDir)
    {
        filepath.Append(wxT("/" DIRECTORY_KLUDGE) ); // Needed for empty dirs
    }

    logfile->Write(filepath, ++installcount);

    return false;
}

unsigned int InstallLog::WriteFile(wxArrayString filepaths)
{
    unsigned long i;
    unsigned int finalrc = false;
    wxString thisone;

    if (dirtyflag) return true;

    for (i = 0; i < filepaths.GetCount(); i++);
    {
        if ( WriteFile(filepaths[i]) )
        {
            finalrc++;
        }
    }

    return finalrc;
}

wxArrayString* InstallLog::GetInstalledFiles()
{
    wxString curdir = wxT("");

    if (dirtyflag) return NULL;
    workingAS.Clear();

    EnumerateCurDir(wxT(""));

    wxArrayString* out = new wxArrayString(workingAS);
    return out;
}

void InstallLog::EnumerateCurDir(wxString curdir)
{
    bool contflag;
    wxString curname, buf, buf2, pathcache;
    long dummy;

    buf.Printf(wxT("/FilePaths/%s"), curdir.c_str());
    pathcache = logfile->GetPath();
    logfile->SetPath(buf);

    contflag = logfile->GetFirstGroup(curname, dummy);
    while (contflag)
    {
        buf.Printf(wxT("%s/%s"), curdir.c_str(), curname.c_str() );
        buf2 = buf; buf2.Replace(wxT("/"), PATH_SEP);
        workingAS.Add(buf2);
        EnumerateCurDir(buf);
        contflag = logfile->GetNextGroup(curname, dummy);
    }

    contflag = logfile->GetFirstEntry(curname, dummy);
    while (contflag)
    {
        if (curname != wxT(DIRECTORY_KLUDGE) )
        {
            buf.Printf(wxT("%s/%s"), curdir.c_str(), curname.c_str() );
            buf2 = buf; buf2.Replace(wxT("/"), PATH_SEP);
            workingAS.Add(buf2);
        }
        contflag = logfile->GetNextEntry(curname, dummy);
    }

    logfile->SetPath(pathcache);
}

