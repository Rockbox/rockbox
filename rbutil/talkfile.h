/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * Module: rbutil
 * File: tts.h
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


#ifndef _TALKFILE_H
#define _TALKFILE_H

#include "rbutil.h"

class TalkFileCreator
{
     friend class TalkTraverser;
public:
    TalkFileCreator();
    ~TalkFileCreator() {};

    bool createTalkFiles();

    void setTTSexe(wxString exe){m_TTSexec=exe;}
    void setEncexe(wxString exe){m_EncExec=exe;}

    wxArrayString getSupportedTTS(){return m_supportedTTS;}
    bool setTTsType(wxString tts) {m_curTTS = tts; }
    wxString getTTsOpts(wxString ttsname);
    void setTTsOpts(wxString opts) {m_TTSOpts=opts;}

    wxArrayString getSupportedEnc(){return m_supportedEnc;}
    bool setEncType(wxString enc) {m_curEnc =enc; }
    wxString getEncOpts(wxString encname);
    void setEncOpts(wxString opts) {m_EncOpts=opts;}

    void setDir(wxString dir){m_dir = dir; }

    void setOverwriteTalk(bool ov) {m_overwriteTalk = ov;}
    void setOverwriteWav(bool ov) {m_overwriteWav = ov;}
    void setRemoveWav(bool ov) {m_removeWav = ov;}
    void setRecursive(bool ov) {m_recursive = ov;}
    void setStripExtensions(bool ov) {m_stripExtensions = ov;}

private:

    bool initTTS();
    bool stopTTS();
    bool initEncoder();

    bool encode(wxString input,wxString output);
    bool voice(wxString text,wxString wavfile);

    wxString m_dir;

    wxString m_curTTS;
    wxString m_TTSexec;
    wxArrayString m_supportedTTS;
    wxArrayString m_supportedTTSOpts;
    wxString m_TTSOpts;

    wxString m_curEnc;
    wxString m_EncExec;
    wxArrayString m_supportedEnc;
    wxArrayString m_supportedEncOpts;
    wxString m_EncOpts;

    bool m_overwriteTalk;
    bool m_overwriteWav;
    bool m_removeWav;
    bool m_recursive;
    bool m_stripExtensions;
};


class TalkTraverser: public wxDirTraverser
{
    public:
        TalkTraverser(TalkFileCreator* talkcreator) : m_talkcreator(talkcreator) { }

        virtual wxDirTraverseResult OnFile(const wxString& filename);

        virtual wxDirTraverseResult OnDir(const wxString& dirname);

    private:
        TalkFileCreator* m_talkcreator;
};

#endif
