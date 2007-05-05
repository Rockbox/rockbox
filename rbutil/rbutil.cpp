/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * Module: rbutil
 * File: rbutil.cpp
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

#include "rbutil.h"
#include "installlog.h"


// This class allows us to return directories as well as files to
// wxDir::Traverse
class wxDirTraverserIncludeDirs : public wxDirTraverser
{
public:
    wxDirTraverserIncludeDirs(wxArrayString& files) : m_files(files) { }

    virtual wxDirTraverseResult OnFile(const wxString& filename)
    {
        m_files.Add(filename);
        return wxDIR_CONTINUE;
    }

    virtual wxDirTraverseResult OnDir(const wxString& dirname)
    {
        m_files.Add(dirname);
        return wxDIR_CONTINUE;
    }

private:
    wxArrayString& m_files;
};

wxDEFINE_SCOPED_PTR_TYPE(wxZipEntry);

const wxChar* _rootmatch[] = {
    wxT("rockbox.*"),
    wxT("ajbrec.ajz"),
    wxT("archos.mod"),
    wxT(".scrobbler.*"),
    wxT("battery_bench.txt"),
    wxT("battery.dummy"),
};
const wxArrayString* rootmatch = new wxArrayString(
    (size_t) (sizeof(_rootmatch) / sizeof(wxChar*)), _rootmatch);

bool InstallTheme(wxString Themesrc)
{
    wxString dest,src,err;

    int pos = Themesrc.Find('/',true);
    wxString themename = Themesrc.SubString(pos+1,Themesrc.Length());

    src.Printf(wxT("%s/%s"), gv->themes_url.c_str(),Themesrc.c_str());
    dest.Printf(wxT("%s" PATH_SEP "download" PATH_SEP "%s"),
                  gv->stdpaths->GetUserDataDir().c_str(),themename.c_str());
    if( DownloadURL(src, dest) )
    {
        wxRemoveFile(dest);
        err.Printf(wxT("Unable to download %s"), src.c_str() );
        ERR_DIALOG(err, wxT("Install Theme"));
        return false;
    }

    if(!checkZip(dest))
    {
        err.Printf(wxT("The Zip %s does not contain the correct dir structure"), dest.c_str());
        ERR_DIALOG(err, wxT("Install Theme"));
        return false;
    }

    if(UnzipFile(dest,gv->curdestdir, true))
    {
        err.Printf(wxT("Unable to unzip %s to %s"), dest.c_str(), gv->curdestdir.c_str());
        ERR_DIALOG(err, wxT("Install Theme"));
        return false;
    }

    return true;
}

bool checkZip(wxString zipname)
{
    wxZipEntryPtr       entry;

    wxFFileInputStream* in_file = new wxFFileInputStream(zipname);
    wxZipInputStream* in_zip = new wxZipInputStream(*in_file);

    entry.reset(in_zip->GetNextEntry());

    wxString name = entry->GetName();
    if(entry->IsDir())
    {
        if(name.Contains(wxT(".rockbox")))
        {
            return true;
        }
    }
    return false;
}

int DownloadURL(wxString src, wxString dest)
{
    int input, errnum = 0, success = false;
    wxString buf, errstr;
    wxLogVerbose(wxT("=== begin DownloadURL(%s,%s)"), src.c_str(),
        dest.c_str());

    buf.Printf(wxT("Fetching %s"), src.c_str());
    wxProgressDialog* progress = new wxProgressDialog(wxT("Downloading"),
                buf, 100, NULL, wxPD_APP_MODAL |
                wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_ELAPSED_TIME |
                wxPD_REMAINING_TIME | wxPD_CAN_ABORT);
    progress->SetSize(500,200);
    progress->Update(0);


    input = true;
    wxURL* in_http = new wxURL(src);

    if(gv->proxy_url != wxT(""))
        in_http->SetProxy(gv->proxy_url);

    if (in_http->GetError() == wxURL_NOERR)
    {

    wxFFileOutputStream* os = new wxFFileOutputStream(dest);
    input = false;
    if (os->IsOk())
    {
        wxInputStream* is = in_http->GetInputStream();
        input = true;
        if (is)
        {
            size_t filesize = is->GetSize();
            input = true;
            if (is->IsOk())
            {
                char buffer[FILE_BUFFER_SIZE + 1];
                size_t current = 0;

                while (! is->Eof())
                {
                    is->Read(buffer, FILE_BUFFER_SIZE);
                    input = true;
                    if (is->LastRead() )
                    {
                        os->Write(buffer, is->LastRead());
                        input = false;
                        if (os->IsOk())
                        {
                            current += os->LastWrite();
                            if (!progress->Update(current * 100 / filesize))
                            {
                                errstr = wxT("Download aborted by user");
                                errnum = 1000;
                                break;
                            }

                        } else
                        {
                            errnum = os->GetLastError();
                            errstr.Printf(wxT("Can't write to output stream (%s)"),
                                        stream_err_str(errnum).c_str() );

                            break;
                        }

                    } else
                    {
                        errnum = is->GetLastError();
                        if (errnum == wxSTREAM_EOF)
                        {
                            errnum = 0;
                            break;
                        }
                        errstr.Printf(wxT("Can't read from input stream (%s)"),
                                      stream_err_str(errnum).c_str() );
                    }
                }

                os->Close();
                if (! errnum)
                {
                    errnum = os->GetLastError();
                    errstr.Printf(wxT("Can't close output file (%s)"),
                            stream_err_str(errnum).c_str() );

                    input = false;
                }

                if (! errnum) success = true;

                } else
                {
                    errnum = is->GetLastError();
                    errstr.Printf(wxT("Can't get input stream size (%s)"),
                            stream_err_str(errnum).c_str() );

                }
            } else
            {
                errnum = in_http->GetError();
                errstr.Printf(wxT("Can't get input stream (%d)"), errnum);
            }
            delete is;
        } else
        {
            errnum = os->GetLastError();
            errstr.Printf(wxT("Can't create output stream (%s)"),
                    stream_err_str(errnum).c_str() );
        }
        delete os;
    } else
    {
        errstr.Printf(wxT("Can't open URL %s (%d)"), src.c_str(),
            in_http->GetError() );
        errnum = 100;
    }

    delete in_http;
    delete progress;

    if (!success)
    {
        if (errnum == 0) errnum = 999;
        if (input)
        {
            buf.Printf(wxT("%s reading\n%s"),
                errstr.c_str(), src.c_str());
            ERR_DIALOG(buf, wxT("Download URL"));
        } else
        {
            buf.Printf(wxT("%s writing to download\n/%s"),
                errstr.c_str(), dest.c_str());
            ERR_DIALOG(buf, wxT("Download URL"));
        }

    }

    wxLogVerbose(wxT("=== end DownloadURL"));
    return errnum;
}

int UnzipFile(wxString src, wxString destdir, bool isInstall)
{
    wxZipEntryPtr       entry;
    wxString            in_str, progress_msg, buf;
    int                 errnum = 0, curfile = 0, totalfiles = 0;
    InstallLog*         log = NULL;

    wxLogVerbose(wxT("===begin UnzipFile(%s,%s,%i)"),
               src.c_str(), destdir.c_str(), isInstall);

    wxFFileInputStream* in_file = new wxFFileInputStream(src);
    wxZipInputStream* in_zip = new wxZipInputStream(*in_file);
    if (in_file->Ok() )
    {
        if (! in_zip->IsOk() )
        {
            errnum = in_zip->GetLastError();
            buf.Printf(wxT("Can't open ZIP stream %s for reading (%s)"),
                src.c_str(), stream_err_str(errnum).c_str() );
            ERR_DIALOG(buf, wxT("Unzip File") );
            delete in_zip;
            delete in_file;
            return true;
        }

        totalfiles = in_zip->GetTotalEntries();
        if (! in_zip->IsOk() )
        {
            errnum = in_zip->GetLastError();
            buf.Printf(wxT("Error Getting total ZIP entries for %s (%s)"),
                src.c_str(), stream_err_str(errnum).c_str() );
            ERR_DIALOG(buf, wxT("Unzip File") );
            delete in_zip;
            delete in_file;
            return true;
        }
    } else
    {
        errnum = in_file->GetLastError();
        buf.Printf(wxT("Can't open %s (%s)"), src.c_str(),
                   stream_err_str(errnum).c_str() );
        ERR_DIALOG(buf, wxT("Unzip File") );
        delete in_zip;
        delete in_file;
        return true;
    }

    wxProgressDialog* progress = new wxProgressDialog(wxT("Unpacking archive"),
                  wxT("Preparing to unpack the downloaded files to your audio"
                  "device"), totalfiles, NULL, wxPD_APP_MODAL |
                  wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_ELAPSED_TIME |
                  wxPD_REMAINING_TIME | wxPD_CAN_ABORT);
    progress->Update(0);

    // We're not overly worried if the logging fails
    if (isInstall)
    {
        buf.Printf(wxT("%s" PATH_SEP UNINSTALL_FILE), destdir.c_str());
        log = new InstallLog(buf);
    }

    while (! errnum &&
           (entry.reset(in_zip->GetNextEntry()), entry.get() != NULL) )
    {

        curfile++;
        wxString name = entry->GetName();
        progress_msg.Printf(wxT("Unpacking %s"), name.c_str());
        if (! progress->Update(curfile, progress_msg) ) {
            buf.Printf(wxT("Unpacking cancelled by user"));
            MESG_DIALOG(buf);
            errnum = 1000;
            break;
        }

        in_str.Printf(wxT("%s" PATH_SEP "%s"), destdir.c_str(), name.c_str());

        if (entry->IsDir() ) {
            if (!wxDirExists(in_str) ) {
                if (! wxMkdir(in_str, 0777) ) {
                    buf.Printf(wxT("Unable to create directory %s"),
                               in_str.c_str() );
                    errnum = 100;
                    break;
                }
            }
            log->WriteFile(name, true); // Directory
            continue;
        }

        wxFFileOutputStream* out = new wxFFileOutputStream(in_str);
        if (! out->IsOk() )
        {
            buf.Printf(wxT("Can't open file %s for writing"), in_str.c_str() );
            delete out;
            return 100;
        } else if (isInstall)
        {
            log->WriteFile(name);
        }

        in_zip->Read(*out);
        if (! out->IsOk()) {
            buf.Printf(wxT("Can't write to %s (%d)"), in_str.c_str(),
                        errnum = out->GetLastError() );
        }

        if (!in_zip->IsOk() && ! in_file->GetLastError() == wxSTREAM_EOF)
        {
            buf.Printf(wxT("Can't read from %s (%d)"), src.c_str(),
            errnum = in_file->GetLastError() );
        }

        if (! out->Close() && errnum == 0)
        {
            buf.Printf(wxT("Unable to close %s (%d)"), in_str.c_str(),
                errnum = out->GetLastError() );

        }

        delete out;

    }

    delete in_zip; delete in_file; delete progress;

    if (errnum)
    {
        ERR_DIALOG(buf, wxT("Unzip File"));
    }

    if (log) delete log;
    wxLogVerbose(wxT("=== end UnzipFile"));
    return(errnum);
}

int Uninstall(const wxString dir, bool isFullUninstall) {
    wxString buf, uninst;
    unsigned int i;
    bool errflag = false;
    InstallLog *log = NULL;
    wxArrayString* FilesToRemove = NULL;

    wxLogVerbose(wxT("=== begin Uninstall(%s,%i)"), dir.c_str(), isFullUninstall);

    wxProgressDialog* progress = new wxProgressDialog(wxT("Uninstalling"),
                wxT("Reading uninstall data from jukebox"), 100, NULL,
                wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_SMOOTH |
                wxPD_ELAPSED_TIME | wxPD_REMAINING_TIME | wxPD_CAN_ABORT);
    progress->Update(0);

    if (! isFullUninstall)
    {

        buf.Printf(wxT("%s" PATH_SEP UNINSTALL_FILE), dir.c_str());
        log = new InstallLog(buf, false); // Don't create the log
        FilesToRemove = log->GetInstalledFiles();
        if (log) delete log;

        if (FilesToRemove == NULL || FilesToRemove->GetCount() < 1) {
            wxLogNull lognull;
            if ( wxMessageDialog(NULL,
                wxT("Rockbox Utility can't find any uninstall data on this "
                "jukebox.\n"
                "Would you like to attempt a full uninstall?\n"
                "(WARNING: A full uninstall removes all files in your Rockbox "
                "folder)"),
                wxT("Standard uninstall not possible"),
                wxICON_EXCLAMATION | wxYES_NO | wxNO_DEFAULT).ShowModal()
                == wxID_YES)
            {
                isFullUninstall = true;
            }
            else {
                MESG_DIALOG(wxT("Uninstall cancelled by user"));
                delete progress;
                return 1000;
            }
        }
    }

    if (isFullUninstall )
    {
        buf.Printf(wxT("%s" PATH_SEP ".rockbox"), dir.c_str());
        if (rm_rf(buf) )
        {
            WARN_DIALOG(wxT("Unable to completely remove Rockbox directory"),
            wxT("Full uninstall") );
            errflag = true;
        }

        wxDir* root = new wxDir(dir);
        wxArrayString* special = new wxArrayString();
        // Search for files for deletion in the jukebox root
        for (i = 0; i < rootmatch->GetCount(); i++)
        {
            const wxString match = (*rootmatch)[i];
            root->GetAllFiles(dir, special, match, wxDIR_FILES);
        }
        delete root;

        // Sort in reverse order so we get directories last
        special->Sort(true);

        for (i = 0; i < special->GetCount(); i++)
        {

            if (wxDirExists((*special)[i]) )
            {
                // We don't check the return code since we don't want non
                // empty dirs disappearing.
                wxRmdir((*special)[i]);

            } else if (wxFileExists((*special)[i]) )
            {
                if (! wxRemoveFile((*special)[i]) )
                {
                    buf.Printf(wxT("Can't delete %s"), (*special)[i].c_str());
                    WARN_DIALOG(buf.c_str(), wxT("Full uninstall"));
                    errflag = true;
                }
            }
            // Otherwise there isn't anything there, so we don't have to worry.
        }
        delete special;
    } else
    {
        wxString instplat, this_path_sep;
        unsigned int totalfiles, rc;
        totalfiles = FilesToRemove->GetCount();
        FilesToRemove->Sort(true); // Reverse alphabetical ie dirs after files

        for (i = 0; i < totalfiles; i++)
        {
            // If we're running on the device, let's not delete our own
            // installation, eh?
            if (gv->portable &&
                FilesToRemove->Item(i).StartsWith(PATH_SEP
                    wxT("RockboxUtility")) )
            {
                continue;
            }

            wxString* buf2 = new wxString;
            buf.Printf(wxT("%s%s"), dir.c_str() , FilesToRemove->Item(i).c_str() );
            buf2->Format(wxT("Deleting %s"), buf.c_str());

            if (! progress->Update((i + 1) * 100 / totalfiles, *buf2) )
            {
                WARN_DIALOG(wxT("Cancelled by user"), wxT("Normal Uninstall"));
                delete progress;
                return true;
            }

            if (wxDirExists(buf) )
            {
                // If we're about to attempt to remove .rockbox. delete
                // install data first
                buf2->Printf(wxT("%s" PATH_SEP ".rockbox"), dir.c_str() );
                if ( buf.IsSameAs(buf2->c_str()) )
                {
                    buf2->Printf(wxT("%s" PATH_SEP UNINSTALL_FILE), dir.c_str());
                    wxRemoveFile(*buf2);
                }

                if ( (rc = ! wxRmdir(buf)) )
                {
                    buf = buf.Format(wxT("Can't remove directory %s"),
                        buf.c_str());
                    errflag = true;
                    WARN_DIALOG(buf.c_str(), wxT("Standard uninstall"));
                }
            } else if (wxFileExists(buf) )
            {
                if ( (rc = ! wxRemoveFile(buf)) )
                {
                    buf = buf.Format(wxT("Can't delete file %s"),
                        buf.c_str());
                    errflag = true;
                    WARN_DIALOG(buf.c_str(), wxT("Standard uninstall"));
                }
            } else
            {
                errflag = true;
                buf = buf.Format(wxT("Can't find file or directory %s"),
                    buf.c_str() );
                WARN_DIALOG(buf.c_str(), wxT("Standard uninstall") );
            }

            uninst = uninst.AfterFirst('\n');
        }
        if (errflag)
        {
            ERR_DIALOG(wxT("Unable to remove some files"),
                wxT("Standard uninstall"))    ;
        }

        if (FilesToRemove != NULL) delete FilesToRemove;
    }

    delete progress;
    wxLogVerbose(wxT("=== end Uninstall"));
    return errflag;
}


wxString stream_err_str(int errnum)
{
    wxString out;

    switch (errnum) {
    case wxSTREAM_NO_ERROR:
        out = wxT("wxSTREAM_NO_ERROR");
        break;
    case wxSTREAM_EOF:
        out = wxT("wxSTREAM_EOF");
        break;
    case wxSTREAM_WRITE_ERROR:
        out = wxT("wxSTREAM_WRITE_ERROR");
        break;
    case wxSTREAM_READ_ERROR:
        out = wxT("wxSTREAM_READ_ERROR");
        break;
    default:
        out = wxT("UNKNOWN");
        break;
    }
    return out;
}

bool InstallRbutil(wxString dest)
{
    wxArrayString   filestocopy;
    wxString        str, buf, dstr, localpath, destdir;
    unsigned int    i;
    wxDir           dir;
    bool            copied_exe = false, made_rbdir = false;
    InstallLog*     log;

    buf.Printf(wxT("%s" PATH_SEP ".rockbox"), dest.c_str() );

    if (! wxDirExists(buf) )
    {
        wxMkdir(buf);
        made_rbdir = true;
    }

    buf.Printf(wxT("%s" PATH_SEP UNINSTALL_FILE), dest.c_str() );
    log = new InstallLog(buf);
    if (made_rbdir) log->WriteFile(wxT(".rockbox"), true);

    destdir.Printf(wxT("%s" PATH_SEP "RockboxUtility"), dest.c_str());
    if (! wxDirExists(destdir) )
    {
        if (! wxMkdir(destdir, 0777) )
        {
            buf.Printf(wxT("%s (%s)"),
                wxT("Unable to create directory for installer"), destdir.c_str());
            WARN_DIALOG(buf , wxT("Portable install") );
            return false;
        }
        log->WriteFile(wxT("RockboxUtility"), true);
    }

    dir.GetAllFiles(gv->ResourceDir, &filestocopy, wxT("*"),
        wxDIR_FILES);
    if (filestocopy.GetCount() < 1)
    {
        WARN_DIALOG(wxT("No files to copy"), wxT("Portable install") );
        return false;
    }

    // Copy the contents of the program directory
    for (i = 0; i < filestocopy.GetCount(); i++)
    {
        if (filestocopy[i].AfterLast(PATH_SEP_CHR) == EXE_NAME)
        {
            copied_exe = true;
        }

        dstr.Printf(wxT("%s" PATH_SEP "%s"), destdir.c_str(),
            filestocopy[i].AfterLast(PATH_SEP_CHR).c_str());
        if (! wxCopyFile(filestocopy[i], dstr) )
        {
            buf.Printf(wxT("%s (%s -> %s)"),
                wxT("Error copying file"), filestocopy[i].c_str(), dstr.c_str());
            WARN_DIALOG(buf, wxT("Portable Install") );
            return false;
        }
        buf = dstr;
        buf.Replace(dest, wxEmptyString, false);
        log->WriteFile(buf);
    }

    if (! copied_exe)
    {
        str.Printf(wxT("%s" PATH_SEP EXE_NAME), gv->AppDir.c_str());
        dstr.Printf(wxT("%s" PATH_SEP EXE_NAME), destdir.c_str());
        if (! wxCopyFile(str, dstr) )
        {
            buf.Printf(wxT("Can't copy program binary %s -> %s"),
                str.c_str(), dstr.c_str() );
            WARN_DIALOG(buf, wxT("Portable Install") );
            return false;
        }
        buf = dstr;
        buf.Replace(dest, wxEmptyString, false);
        log->WriteFile(buf);
    }

    // Copy the local ini file so that it knows that it's a portable copy
    gv->UserConfig->Flush();
    dstr.Printf(wxT("%s" PATH_SEP "RockboxUtility.cfg"), destdir.c_str());
    if (! wxCopyFile(gv->UserConfigFile, dstr) )
    {
        buf.Printf(wxT("%s (%s -> %s)"),
            wxT("Unable to install user config file"), gv->UserConfigFile.c_str(),
            dstr.c_str() );
        WARN_DIALOG(buf, wxT("Portable Install") );
        return false;
    }
    buf = dstr;
    buf.Replace(dest, wxEmptyString, false);
    log->WriteFile(buf);

    delete log;
    return true;
}

bool rm_rf(wxString file)
{
    wxLogVerbose(wxT("=== begin rm-rf(%s)"), file.c_str() );

    wxString buf;
    wxArrayString selected;
    wxDirTraverserIncludeDirs wxdtid(selected);
    unsigned int rc = 0, i;
    bool errflag = false;

    if (wxFileExists(file) )
    {
        rc = ! wxRemoveFile(file);
    } else if (wxDirExists(file) )
    {
        wxDir* dir = new wxDir(file);;
        dir->Traverse(wxdtid);
        delete dir;
        // Sort into reverse alphabetical order for deletion in correct order
        // (directories after files)
        selected.Sort(true);
        selected.Add(file);

        wxProgressDialog* progress = new wxProgressDialog(wxT("Removing files"),
                wxT("Deleting files"), selected.GetCount(), NULL,
                wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_SMOOTH |
                wxPD_ELAPSED_TIME | wxPD_REMAINING_TIME | wxPD_CAN_ABORT);

        for (i = 0; i < selected.GetCount(); i++)
        {
            wxLogVerbose(selected[i]);
            if (progress != NULL)
            {
                buf.Printf(wxT("Deleting %s"), selected[i].c_str() );
                if (! progress->Update(i, buf))
                {
                    WARN_DIALOG(wxT("Cancelled by user"), wxT("Erase Files"));
                    delete progress;
                    return true;
                }
            }

            if (wxDirExists(selected[i]) )
            {
                if ((rc = ! wxRmdir(selected[i])) )
                {
                    buf.Printf(wxT("Can't remove directory %s"),
                        selected[i].c_str());
                    errflag = true;
                    WARN_DIALOG(buf.c_str(), wxT("Erase files"));
                }
            } else if ((rc = ! wxRemoveFile(selected[i])) )
            {
                buf.Printf(wxT("Error deleting file %s"), selected[i].c_str() );
                errflag = true;
                WARN_DIALOG(buf.c_str(),wxT("Erase files"));
            }
        }
        delete progress;
    } else
    {
        buf.Printf(wxT("Can't find expected file %s"), file.c_str());
        WARN_DIALOG(buf.c_str(), wxT("Erase files"));
        return true;
    }

    wxLogVerbose(wxT("=== end rm-rf"));
    return rc ? true : false;
}


