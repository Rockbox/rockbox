/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id$
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/


#ifndef TALKFILE_H
#define TALKFILE_H

#include "progressloggerinterface.h"

class TTSBase : public QObject
{
    Q_OBJECT
public:
    TTSBase(){}
    virtual ~TTSBase(){}
    virtual bool voice(QString text,QString wavfile){(void)text; (void)wavfile; return false;}
    virtual bool start(){return false;}
    virtual bool stop(){return false;}
    
    void setTTSexe(QString exe){m_TTSexec=exe;}
    void setTTsOpts(QString opts) {m_TTSOpts=opts;}
    void setTTsLanguage(QString language) {m_TTSLanguage = language;}
    void setTTsTemplate(QString t) { m_TTSTemplate = t; }
    
protected:
    QString m_TTSexec;
    QString m_TTSOpts;
    QString m_TTSTemplate;
    QString m_TTSLanguage;
};


class TalkFileCreator :public QObject
{
    Q_OBJECT

public:
    TalkFileCreator(QObject* parent=0);

    bool createTalkFiles(ProgressloggerInterface* logger);

    void setTTSexe(QString exe){m_TTSexec=exe;}
    void setEncexe(QString exe){m_EncExec=exe;}

    void setTTsType(QString tts) { m_curTTS = tts; }
    void setTTsOpts(QString opts) {m_TTSOpts=opts;}
    void setTTsLanguage(QString language) {m_TTSLanguage = language;}
    void setTTsTemplate(QString t) { m_curTTSTemplate = t; }

    void setEncType(QString enc) { m_curEnc = enc; }
    void setEncOpts(QString opts) {m_EncOpts=opts;}
    void setEncTemplate(QString t) { m_curEncTemplate = t; }

    void setDir(QDir dir){m_dir = dir; }
    void setMountPoint(QString mountpoint) {m_mountpoint =mountpoint; }

    void setOverwriteTalk(bool ov) {m_overwriteTalk = ov;}
    void setOverwriteWav(bool ov) {m_overwriteWav = ov;}
    void setRemoveWav(bool ov) {m_removeWav = ov;}
    void setRecursive(bool ov) {m_recursive = ov;}
    void setStripExtensions(bool ov) {m_stripExtensions = ov;}
    void setTalkFolders(bool ov) {m_talkFolders = ov;} 
    void setTalkFiles(bool ov) {m_talkFiles = ov;}

private slots:
    void abort();

private:
    TTSBase* m_tts;
    bool initEncoder();
    
    bool encode(QString input,QString output);

    QDir   m_dir;
    QString m_mountpoint;
    QString m_curTTS;
    QString m_TTSexec;
    QString m_TTSOpts;
    QString m_TTSLanguage;
    QString m_curTTSTemplate;

    QString m_curEnc;
    QString m_EncExec;
    QString m_EncOpts;
    QString m_curEncTemplate;

    bool m_overwriteTalk;
    bool m_overwriteWav;
    bool m_removeWav;
    bool m_recursive;
    bool m_stripExtensions;
    bool m_talkFolders;
    bool m_talkFiles;

    ProgressloggerInterface* m_logger;

    bool m_abort;
};

class TTSSapi : public TTSBase
{
public:
    TTSSapi() {};
    virtual bool voice(QString text,QString wavfile);
    virtual bool start();
    virtual bool stop();
    
private:
    QProcess* voicescript;
};

class TTSExes : public TTSBase
{
public:
    TTSExes() {};
    virtual bool voice(QString text,QString wavfile);
    virtual bool start();
    virtual bool stop() {return true;}
    
private:
   
};

#endif

