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

#include <QtGui>
#include "progressloggerinterface.h"

#include "encoders.h"
#include "tts.h"

class TalkFileCreator :public QObject
{
    Q_OBJECT

public:
    TalkFileCreator(QObject* parent=0);

    bool createTalkFiles(ProgressloggerInterface* logger);

    void setSettings(RbSettings* sett) { settings = sett;}
    
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
    EncBase* m_enc;
    RbSettings* settings;
   
    QDir   m_dir;
    QString m_mountpoint;
 
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


#endif

