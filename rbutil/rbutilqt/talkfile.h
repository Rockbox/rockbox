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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/


#ifndef TALKFILE_H
#define TALKFILE_H

#include <QtCore>
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
    void setRecursive(bool ov) {m_recursive = ov;}
    void setStripExtensions(bool ov) {m_stripExtensions = ov;}
    void setTalkFolders(bool ov) {m_talkFolders = ov;} 
    void setTalkFiles(bool ov) {m_talkFiles = ov;}
    
private slots:
    void abort();

private:
    bool cleanup(QStringList list);
    QString stripExtension(QString filename);
    void doAbort(QStringList cleanupList);
    void resetProgress(int max);
    bool createDirAndFileMaps(QDir startDir,QMultiMap<QString,QString> *dirMap,QMultiMap<QString,QString> *fileMap);
    bool voiceList(QStringList toSpeak,QString* errString);
    bool encodeList(QStringList toEncode,QString* errString);
    bool copyTalkDirFiles(QMultiMap<QString,QString> dirMap,QString* errString);
    bool copyTalkFileFiles(QMultiMap<QString,QString> fileMap,QString* errString);
    
    TTSBase* m_tts;
    EncBase* m_enc;
    RbSettings* settings;
   
    QDir   m_dir;
    QString m_mountpoint;
    int m_progress;
 
    bool m_overwriteTalk;
    bool m_recursive;
    bool m_stripExtensions;
    bool m_talkFolders;
    bool m_talkFiles;

    ProgressloggerInterface* m_logger;

    bool m_abort;
};


#endif

