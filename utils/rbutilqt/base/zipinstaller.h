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


#ifndef ZIPINSTALLER_H
#define ZIPINSTALLER_H

#include <QtCore>

#include "progressloglevels.h"
#include "httpget.h"
#include "Logger.h"

/** Install a file or zip.
 *  Downloads file(s) from a given URL, and installs by either extracting or
 *  copying it to the target path set by setMountpoint().
 */
class ZipInstaller : public QObject
{
    Q_OBJECT
public:
    ZipInstaller(QObject* parent);
    void install(void);
    void setMountPoint(QString& mountpoint) {m_mountpoint = mountpoint;}
    void setUrl(QString& url){m_urllist = QStringList(url);}
    void setUrl(QStringList& url) { m_urllist = url; }
    void setLogSection(QString name) {m_loglist = QStringList(name);}
    void setLogSection(QStringList& name) { m_loglist = name; }
    void setLogVersion(QString v = "")
    { m_verlist = QStringList(v); LOG_INFO() << m_verlist;}
    void setLogVersion(QStringList v)
    { m_verlist = v; LOG_INFO() << m_verlist;}
    /** Change between copy and unzip mode. */
    void setUnzip(bool i) { m_unzip = i; }
    /** Set target filename for copy mode.
     *  If not set the filename part of the download URL is used. */
    void setTarget(QString& t) { m_target = t; }
    void setCache(bool c) { m_usecache = c; }

public slots:
    void abort(void);

private slots:
    void downloadDone(QNetworkReply::NetworkError error);
    void installStart(void);
    void installContinue(void);

signals:
    void done(bool error);
    void cont();
    void logItem(QString, int); //! set logger item
    void logProgress(int, int); //! set progress bar.
    void internalAborted(void);

private:
    QString m_url, m_file, m_mountpoint, m_logsection, m_logver;
    QStringList m_urllist, m_loglist, m_verlist;
    bool m_unzip;
    QString m_target;
    int m_runner;
    bool m_usecache;

    HttpGet *m_getter;
    QTemporaryFile *m_downloadFile;
};



#endif

