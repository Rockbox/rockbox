/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * Module: rbutil
 * File: tts.cpp
 *
 * Copyright (C) 2007 Dominik wenger
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "talkfile.h"

TalkFileCreator::TalkFileCreator()
{
    m_supportedTTS.Add(wxT("espeak"));
    m_supportedTTSOpts.Add(wxT(""));

    m_supportedEnc.Add(wxT("lame"));
    m_supportedEncOpts.Add(wxT("--vbr-new -t --nores -S"));

}

bool TalkFileCreator::initEncoder()
{
    if(::wxFileExists(m_EncExec))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool TalkFileCreator::initTTS()
{
    if(::wxFileExists(m_TTSexec))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool TalkFileCreator::createTalkFiles()
{
    if(!initTTS())
    {
        MESG_DIALOG(wxT("Init of TTS engine failed") );
        return false;
    }
    if(!initEncoder())
    {
        MESG_DIALOG(wxT("Init of encoder failed") );
        return false;
    }

    // enumerate the dirs
    wxDir talkdir(m_dir);
    TalkTraverser traverser(this);
    if(talkdir.Traverse(traverser) == (size_t)-1)
        return false;
    else
        return true;


}

bool TalkFileCreator::voice(wxString text,wxString wavfile)
{
    if(m_curTTS == wxT("espeak"))
    {
        wxArrayString out;
        wxArrayString err;
        wxExecute(m_TTSexec+wxT(" ")+m_TTSOpts+wxT(" -w \"")+wavfile+wxT("\" \"")+text+wxT("\""),out,err);
        return true;
    }
    else
    {
        MESG_DIALOG(wxT("Unsupported TTS engine") );
        return false;
    }
}

bool TalkFileCreator::encode(wxString input,wxString output)
{
    if(m_curEnc == wxT("lame"))
    {
        wxArrayString out;
        wxArrayString err;
        wxExecute(m_EncExec+wxT(" ")+m_EncOpts+wxT(" \"")+input+wxT("\" \"")+output+wxT("\""),out,err);
        return true;
    }
    else
    {
        MESG_DIALOG(wxT("Unsupported encoder") );
        return false;
    }

}

wxString TalkFileCreator::getTTsOpts(wxString ttsname)
{
    int index = m_supportedTTS.Index(ttsname);

    return m_supportedTTSOpts[index];
}

wxString TalkFileCreator::getEncOpts(wxString encname)
{
    int index = m_supportedEnc.Index(encname);

    return m_supportedEncOpts[index];
}

wxDirTraverseResult TalkTraverser::OnFile(const wxString& file)
{
    if(file.EndsWith(wxT(".talk")) || file.EndsWith(wxT(".talk.wav")))
    {
        return wxDIR_CONTINUE;
    }

    wxFileName fname(file);
    wxString toSpeak;
    if(m_talkcreator->m_stripExtensions)
    {
        toSpeak = fname.GetName();
    }
    else
    {
        toSpeak = fname.GetName()+fname.GetExt();
    }
    wxString filename = file+ wxT(".talk");
    wxString wavname = filename + wxT(".wav");

    if(!wxFileExists(filename) || m_talkcreator->m_overwriteTalk)
    {
        if(!wxFileExists(wavname) || m_talkcreator->m_overwriteWav)
        {
            if(!m_talkcreator->voice(toSpeak,wavname))
            {
                return wxDIR_STOP;
            }
        }
        if(!m_talkcreator->encode(wavname,filename))
        {
            return wxDIR_STOP;
        }
    }

    if(m_talkcreator->m_removeWav)
    {
        wxRemoveFile(wavname);
    }

    return wxDIR_CONTINUE;
}

wxDirTraverseResult TalkTraverser::OnDir(const wxString& dirname)
{
    wxFileName fname(dirname,wxEmptyString);
    wxArrayString dirs=fname.GetDirs();
    wxString toSpeak = dirs[dirs.GetCount()-1];

    wxString filename = dirname + wxT(PATH_SEP "_dirname.talk");
    wxString wavname = filename + wxT(".wav");

    if(!wxFileExists(filename) || m_talkcreator->m_overwriteTalk)
    {
        if(!wxFileExists(wavname) || m_talkcreator->m_overwriteWav)
        {
            if(!m_talkcreator->voice(toSpeak,wavname))
            {
                return wxDIR_STOP;
            }
        }
        if(!m_talkcreator->encode(wavname,filename))
        {
            return wxDIR_STOP;
        }
    }

    if(m_talkcreator->m_removeWav)
    {
        wxRemoveFile(wavname);
    }

    if(!m_talkcreator->m_recursive)
        return wxDIR_IGNORE;
    else
        return wxDIR_CONTINUE;
}
