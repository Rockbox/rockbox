//---------------------------------------------------------------------------
//
// Name:        rbutil.cpp
// Author:      Christi Scarborough
// Created:     06-12-05 04:08
//
//---------------------------------------------------------------------------

#include "rbutil.h"

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

int DownloadURL(wxString src, wxString dest)
{
    int input, errnum = 0, success = false;
    wxString buf, errstr;
    wxLogVerbose(_("=== begin DownloadURL(%s,%s)"), src.c_str(),
        dest.c_str());

    buf.Printf(_("Fetching %s"), src.c_str());
    wxProgressDialog* progress = new wxProgressDialog(_("Downloading"),
                buf, 100, NULL, wxPD_APP_MODAL |
                wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_ELAPSED_TIME |
                wxPD_REMAINING_TIME | wxPD_CAN_ABORT);
    progress->Update(0);

    input = true;
    wxURL* in_http = new wxURL(src);
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
                                errstr = _("Download aborted by user");
                                errnum = 1000;
                                break;
                            }

                        } else
                        {
                            errnum = os->GetLastError();
                            errstr.Printf(_("Can't write to output stream (%s)"),
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
                        errstr.Printf(_("Can't read from input stream (%s)"),
                                      stream_err_str(errnum).c_str() );
                    }
                }

                os->Close();
                if (! errnum)
                {
                    errnum = os->GetLastError();
                    errstr.Printf(_("Can't close output file (%s)"),
                            stream_err_str(errnum).c_str() );

                    input = false;
                }

                if (! errnum) success = true;

                } else
                {
                    errnum = is->GetLastError();
                    errstr.Printf(_("Can't get input stream size (%s)"),
                            stream_err_str(errnum).c_str() );

                }
            } else
            {
                errnum = in_http->GetError();
                errstr.Printf(_("Can't get input stream (%d)"), errnum);
            }
            delete is;
        } else
        {
            errnum = os->GetLastError();
            errstr.Printf(_("Can't create output stream (%s)"),
                    stream_err_str(errnum).c_str() );
        }
        delete os;
    } else
    {
        errstr.Printf(_("Can't open URL %s (%d)"), src.c_str(),
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
            buf.Printf(_("%s reading\n%s"),
                errstr.c_str(), src.c_str());
            ERR_DIALOG(buf, _("Download URL"));
        } else
        {
            buf.Printf(_("%s writing to download\n/%s"),
                errstr.c_str(), dest.c_str());
            ERR_DIALOG(buf, _("Download URL"));
        }

    }

    wxLogVerbose(_("=== end DownloadURL"));
    return errnum;
}

int UnzipFile(wxString src, wxString destdir, bool isInstall)
{
    wxZipEntryPtr entry;
    wxString in_str, progress_msg, buf, logfile = wxT("");
    int errnum = 0, curfile = 0, totalfiles = 0;
    wxLogVerbose(_("===begin UnzipFile(%s,%s,%i)"),
               src.c_str(), destdir.c_str(), isInstall);

    wxFFileInputStream* in_file = new wxFFileInputStream(src);
    wxZipInputStream* in_zip = new wxZipInputStream(*in_file);
    if (in_file->Ok() )
    {
        if (! in_zip->IsOk() )
        {
            errnum = in_zip->GetLastError();
            buf.Printf(_("Can't open ZIP stream %s for reading (%s)"),
                src.c_str(), stream_err_str(errnum).c_str() );
            ERR_DIALOG(buf, _("Unzip File") );
            delete in_zip;
            delete in_file;
            return true;
        }

        totalfiles = in_zip->GetTotalEntries();
        if (! in_zip->IsOk() )
        {
            errnum = in_zip->GetLastError();
            buf.Printf(_("Error Getting total ZIP entries for %s (%s)"),
                src.c_str(), stream_err_str(errnum).c_str() );
            ERR_DIALOG(buf, _("Unzip File") );
            delete in_zip;
            delete in_file;
            return true;
        }
    } else
    {
        errnum = in_file->GetLastError();
        buf.Printf(_("Can't open %s (%s)"), src.c_str(),
                   stream_err_str(errnum).c_str() );
        ERR_DIALOG(buf, _("Unzip File") );
        delete in_zip;
        delete in_file;
        return true;
    }

    wxProgressDialog* progress = new wxProgressDialog(_("Unpacking archive"),
                  _("Preparing to unpack the downloaded files to your audio"
                  "device"), totalfiles, NULL, wxPD_APP_MODAL |
                  wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_ELAPSED_TIME |
                  wxPD_REMAINING_TIME | wxPD_CAN_ABORT);
    progress->Update(0);

    while (! errnum &&
           (entry.reset(in_zip->GetNextEntry()), entry.get() != NULL) )
    {

        curfile++;
        wxString name = entry->GetName();
        progress_msg.Printf(_("Unpacking %s"), name.c_str());
        if (! progress->Update(curfile, progress_msg) ) {
            buf.Printf(_("Unpacking cancelled by user"));
            MESG_DIALOG(buf);
            errnum = 1000;
            break;
        }

        in_str.Printf(wxT("%s" PATH_SEP "%s"), destdir.c_str(), name.c_str());
        buf = logfile;
        // We leave space for adding a future CRC check, if we ever
        // feel particularly enthunsiastic.
        logfile.Printf(wxT("0 %s\n%s"), name.c_str(), buf.c_str());

        if (entry->IsDir() ) {
            wxDir* dirname = new wxDir(in_str);
            if (! dirname->Exists(in_str) ) {
                if (wxMkDir(in_str) ) {
                    buf.Printf(_("Unable to create directory %s"),
                               in_str.c_str() );
                    errnum = 100;
                    delete dirname;
                    break;
                }
            }
            delete dirname;
            continue;
        }

        wxFFileOutputStream* out = new wxFFileOutputStream(in_str);
        if (! out->IsOk() )
        {
            buf.Printf(_("Can't open file %s for writing"), in_str.c_str() );
            delete out;
            return 100;
        }

        in_zip->Read(*out);
        if (! out->IsOk()) {
            buf.Printf(_("Can't write to %s (%d)"), in_str.c_str(),
                        errnum = out->GetLastError() );
        }

        if (!in_zip->IsOk() && ! in_file->GetLastError() == wxSTREAM_EOF)
        {
            buf.Printf(_("Can't read from %s (%d)"), src.c_str(),
            errnum = in_file->GetLastError() );
        }

        if (! out->Close() && errnum == 0)
        {
            buf.Printf(_("Unable to close %s (%d)"), in_str.c_str(),
                errnum = out->GetLastError() );

        }

        delete out;

    }

    delete in_zip; delete in_file; delete progress;

    if (errnum)
    {
        ERR_DIALOG(buf, _("Unzip File"));
    } else if (isInstall)
    {
        // If this fails, we have no log.  No biggie.
        buf = logfile;
        logfile.Printf(wxT("%s " PATH_SEP "\n%s"), gv->curplat.c_str(),
            buf.c_str());

        buf.Printf(wxT("%s" PATH_SEP UNINSTALL_FILE),
                destdir.c_str());
        wxFFileOutputStream* out = new wxFFileOutputStream(buf);
        out->Write(logfile, logfile.Len());
        out->Close();
        delete out;
    }

    wxLogVerbose(_("=== end UnzipFile"));
    return(errnum);
}

int Uninstall(const wxString dir, bool isFullUninstall) {
    wxString buf, uninst;
    bool gooddata = false;
    unsigned int i;
    bool errflag = false;

    wxLogVerbose(_("=== begin Uninstall(%s,%i)"), dir.c_str(), isFullUninstall);

    wxProgressDialog* progress = new wxProgressDialog(_("Uninstalling"),
                _("Reading uninstall data from jukebox"), 100, NULL,
                wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_SMOOTH |
                wxPD_ELAPSED_TIME | wxPD_REMAINING_TIME | wxPD_CAN_ABORT);
    progress->Update(0);

    if (! isFullUninstall)
    {
       buf.Printf(wxT("%s" PATH_SEP UNINSTALL_FILE), dir.c_str());
       if ( wxFileExists(buf) )
       {
           wxFFileInputStream* uninst_data = new wxFFileInputStream(buf);
           if (uninst_data->Ok() )
           {
                wxStringOutputStream* out = new wxStringOutputStream(&uninst);
                uninst_data->Read(*out);
                if (uninst_data->GetLastError() == wxSTREAM_EOF &&
                    ! out->GetLastError() )
                {
                    gooddata = true;
                }
                delete out;
           }
           delete uninst_data;
       }

       if (! gooddata) {
            wxLogNull lognull;
            if ( wxMessageDialog(NULL,
                _("Rockbox Utility can't find any uninstall data on this "
                "jukebox.\n"
                "Would you like to attempt a full uninstall?\n"
                "(WARNING: A full uninstall removes all files in your Rockbox "
                "folder)"),
                _("Standard uninstall not possible"),
                wxICON_EXCLAMATION | wxYES_NO | wxNO_DEFAULT).ShowModal()
                == wxID_YES)
            {
                isFullUninstall = true;
            }
            else {
                MESG_DIALOG(_("Uninstall cancelled by user"));
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
            WARN_DIALOG(_("Unable to completely remove Rockbox directory"),
            _("Full uninstall") );
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
                    buf.Printf(_("Can't delete %s"), (*special)[i].c_str());
                    WARN_DIALOG(buf.c_str(), _("Full uninstall"));
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

        // First line is "<platform><space><path seperator>\n"
        instplat = uninst.BeforeFirst(wxT(' '));
        this_path_sep = uninst.AfterFirst(wxT(' ')).BeforeFirst(wxT('\n'));
        uninst = uninst.AfterFirst(wxT('\n'));
        totalfiles = uninst.Freq(wxT('\n'));

        i = 0;
        while ((buf = uninst.BeforeFirst(wxT('\n'))) != "" )
        {
            // These lines are all "<crc (unused)><space><filename>\n"
            buf = buf.AfterFirst(wxT(' '));
            buf = buf.Format(wxT("%s" PATH_SEP "%s"), dir.c_str(), buf.c_str());
            // So we can install under Linux and still uninstall under Win
            buf.Replace(this_path_sep, PATH_SEP);

            wxString* buf2 = new wxString;
            buf2->Format(_("Deleting %s"), buf.c_str());
            if (! progress->Update(++i * 100 / totalfiles, *buf2) )
            {
                WARN_DIALOG(_("Cancelled by user"), _("Normal Uninstall"));
                delete progress;
                return true;
            }

            if (wxDirExists(buf) )
            {
                // If we're about to attempt to remove .rockbox. delete
                // install data first
                buf2->Printf(wxT("%s" PATH_SEP ".rockbox" PATH_SEP), dir.c_str() );
                if ( buf.IsSameAs(buf2->c_str()) )
                {
                    buf2->Printf(wxT("%s" PATH_SEP UNINSTALL_FILE), dir.c_str());
                    wxRemoveFile(*buf2);
                }

                if ( rc = ! wxRmdir(buf) )
                {
                    buf = buf.Format(_("Can't remove directory %s"),
                        buf.c_str());
                    errflag = true;
                    WARN_DIALOG(buf.c_str(), _("Standard uninstall"));
                }
            } else if (wxFileExists(buf) )
            {
                if ( rc = ! wxRemoveFile(buf) )
                {
                    buf = buf.Format(_("Can't delete file %s"),
                        buf.c_str());
                    errflag = true;
                    WARN_DIALOG(buf.c_str(), _("Standard uninstall"));
                }
            } else
            {
                errflag = true;
                buf = buf.Format(_("Can't find file or directory %s"),
                    buf.c_str() );
                WARN_DIALOG(buf.c_str(), _("Standard uninstall") );
            }

            uninst = uninst.AfterFirst('\n');
        }
        if (errflag)
        {
        ERR_DIALOG(_("Unable to remove some files"),
                            _("Standard uninstall"))    ;
        }
   }

    delete progress;
    wxLogVerbose(_("=== end Uninstall"));
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

bool rm_rf(wxString file)
{
    wxLogVerbose(_("=== begin rm-rf(%s)"), file.c_str() );

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

        wxProgressDialog* progress = new wxProgressDialog(_("Removing files"),
                _("Deleting files"), selected.GetCount(), NULL,
                wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_SMOOTH |
                wxPD_ELAPSED_TIME | wxPD_REMAINING_TIME | wxPD_CAN_ABORT);

        for (i = 0; i < selected.GetCount(); i++)
        {
            wxLogVerbose(selected[i]);
            if (progress != NULL)
            {
                buf.Printf(_("Deleting %s"), selected[i].c_str() );
                if (! progress->Update(i, buf))
                {
                    WARN_DIALOG(_("Cancelled by user"), _("Erase Files"));
                    delete progress;
                    return true;
                }
            }

            if (wxDirExists(selected[i]) )
            {
                if (rc = ! wxRmdir(selected[i]) )
                {
                    buf.Printf(_("Can't remove directory %s"),
                        selected[i].c_str());
                    errflag = true;
                    WARN_DIALOG(buf.c_str(), _("Erase files"));
                }
            } else if (rc = ! wxRemoveFile(selected[i]) )
            {
                buf.Printf(_("Error deleting file %s"), selected[i].c_str() );
                errflag = true;
                WARN_DIALOG(buf.c_str(),_("Erase files"));
            }
        }
        delete progress;
    } else
    {
        buf.Printf(_("Can't find expected file %s"), file.c_str());
        WARN_DIALOG(buf.c_str(), _("Erase files"));
        return true;
    }

    wxLogVerbose(_("=== end rm-rf"));
    return rc ? true : false;
}

