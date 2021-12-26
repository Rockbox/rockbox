/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
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
#include "progressloglevels.h"

#include "talkgenerator.h"

class TalkFileCreator :public QObject
{
    Q_OBJECT

public:
    TalkFileCreator(QObject* parent);

    bool createTalkFiles();

    void setDir(QString dir) {m_dir = dir;}
    void setMountPoint(QString mountpoint) {m_mountpoint = mountpoint;}

    void setGenerateOnlyNew(bool ov) {m_generateOnlyNew = ov;}
    void setRecursive(bool ov) {m_recursive = ov;}
    void setStripExtensions(bool ov) {m_stripExtensions = ov;}
    void setTalkFolders(bool ov) {m_talkFolders = ov;}
    void setTalkFiles(bool ov) {m_talkFiles = ov;}
    void setIgnoreFiles(QStringList wildcards) {m_ignoreFiles = wildcards;}
public slots:
    void abort();

signals:
    void done(bool);
    void aborted();
    void logItem(QString, int); //! set logger item
    void logProgress(int, int); //! set progress bar.

private:
    bool cleanup();
    QString stripExtension(QString filename);
    void doAbort();
    void resetProgress(int max);
    bool copyTalkFiles(QString* errString);

    bool createTalkList(QDir startDir);

    QString m_dir;
    QString m_mountpoint;

    bool m_generateOnlyNew;
    bool m_recursive;
    bool m_stripExtensions;
    bool m_talkFolders;
    bool m_talkFiles;
    QStringList m_ignoreFiles;

    bool m_abort;

    QList<TalkGenerator::TalkEntry> m_talkList;
};


#endif

