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


#ifndef INSTALLZIP_H
#define INSTALLZIP_H



#include <QtCore>
#include <QtNetwork>

#include "progressloggerinterface.h"
#include "httpget.h"

class ZipInstaller : public QObject
{
    Q_OBJECT
public:
    ZipInstaller(QObject* parent) ;
    ~ZipInstaller(){}
    void install(ProgressloggerInterface* dp);
    void setMountPoint(QString mountpoint) {m_mountpoint = mountpoint;}
    void setUrl(QString url){m_urllist = QStringList(url);}
    void setUrl(QStringList url) { m_urllist = url; }
    void setLogSection(QString name) {m_loglist = QStringList(name);}
    void setLogSection(QStringList name) { m_loglist = name; }
    void setLogVersion(QString v) { m_verlist = QStringList(v); qDebug() << m_verlist;}
    void setLogVersion(QStringList v) { m_verlist = v; qDebug() << m_verlist;}
    void setUnzip(bool i) { m_unzip = i; }
    void setTarget(QString t) { m_target = t; }
    void setCache(QDir c) { m_cache = c; };
    void setCache(bool c) { m_usecache = c; };
    void setCache(QString c) { m_cache = QDir(c);}

signals:
    void done(bool error);
    void cont();

private slots:
    void updateDataReadProgress(int, int);
    void downloadDone(bool);
    void installStart(void);
    void installContinue(void);

private:
    void installSingle(ProgressloggerInterface *dp);
    QString m_url, m_file, m_mountpoint, m_logsection, m_logver;
    QStringList m_urllist, m_loglist, m_verlist;
    bool m_unzip;
    QString m_target;
    int runner;
    QDir m_cache;
    bool m_usecache;

    HttpGet *getter;
    QTemporaryFile *downloadFile;

    ProgressloggerInterface* m_dp;
};



#endif

