/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id: voicefile.h 15932 2007-12-15 13:13:57Z domonoky $
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/


#ifndef VOICEFILE_H
#define VOICEFILE_H

#include <QtGui>
#include "progressloggerinterface.h"

#include "encoders.h"
#include "tts.h"
#include "httpget.h"

extern "C"
{
    #include "wavtrim.h"
    #include "voicefont.h"
}
    
class VoiceFileCreator :public QObject
{
    Q_OBJECT
public:
    VoiceFileCreator(QObject* parent=0);
    
    //start creation
    bool createVoiceFile(ProgressloggerInterface* logger);

    // set infos
    void setUserSettings(QSettings* setting) { userSettings = setting;}
    void setDeviceSettings(QSettings* setting) { deviceSettings = setting;}
    
    void setMountPoint(QString mountpoint) {m_mountpoint =mountpoint; }
    void setTargetId(int id){m_targetid = id;}
    void setLang(QString name){m_lang =name;}
    void setProxy(QUrl proxy){m_proxy = proxy;}
    
private slots:
    void abort();
    void downloadRequestFinished(int id, bool error);
    void downloadDone(bool error);
    void updateDataReadProgress(int read, int total);
    
private:
   
    // ptr to encoder, tts and settings
    TTSBase* m_tts;
    EncBase* m_enc;
    QSettings *userSettings;
    QSettings *deviceSettings;
    
    HttpGet *getter;
   
    QUrl m_proxy;  //proxy
    QString filename;  //the temporary file
   
    QString m_mountpoint;  //mountpoint of the device
    QString m_path;   //path where the wav and mp3 files are stored to
    int m_targetid;  //the target id
    QString m_lang;  // the language which will be spoken
  
    ProgressloggerInterface* m_logger;

    bool m_abort;
};
   
#endif
