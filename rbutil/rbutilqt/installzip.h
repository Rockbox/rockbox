/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id: installzip.h 13990 2007-07-25 22:26:10Z Dominik Wenger $
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
 
 
#ifndef INSTALLRB_H
#define INSTALLRB_H
 


#include <QtGui>
#include <QtNetwork>

#include "ui_installprogressfrm.h"
#include "httpget.h"
 
class ZipInstaller : public QObject
{ 
    Q_OBJECT
public:
    ZipInstaller(QObject* parent) ;
    ~ZipInstaller(){}
    void install(Ui::InstallProgressFrm* dp);
    void setMountPoint(QString mountpoint) {m_mountpoint = mountpoint;}
    void setFilename(QString filename){m_file = filename;}
    void setUrl(QString url){m_url = url;}
    void setProxy(QUrl proxy) {m_proxy= proxy;}
    void setLogSection(QString name) {m_logsection = name;}
    
signals:
    void done(bool error);
    
private slots:
    void updateDataReadProgress(int, int);
    void downloadDone(bool);
    void downloadRequestFinished(int, bool);

private:    
    QString m_url,m_file,m_mountpoint,m_logsection;
    QUrl m_proxy;
    
    HttpGet *getter;
    QTemporaryFile downloadFile;
    
    Ui::InstallProgressFrm* m_dp;
};
 
 

#endif

