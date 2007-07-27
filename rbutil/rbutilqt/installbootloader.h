/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id: installbootloader.h 13990 2007-07-25 22:26:10Z Dominik Wenger $
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
 
#ifndef INSTALLBOOTLOADER_H
#define INSTALLBOOTLOADER_H

#include <QtGui>

#include "ui_installprogressfrm.h"
#include "httpget.h"

extern "C" {
    // Ipodpatcher
    #include "../ipodpatcher/ipodpatcher.h"
};

bool initIpodpatcher();

class BootloaderInstaller : public QObject
{
    Q_OBJECT
    
public:
    BootloaderInstaller(QObject* parent);
    ~BootloaderInstaller() {}

    void install(Ui::InstallProgressFrm* dp);
    void uninstall(Ui::InstallProgressFrm* dp);
    
    void setMountPoint(QString mountpoint) {m_mountpoint = mountpoint;}
    void setProxy(QUrl proxy) {m_proxy= proxy;}
    void setDevice(QString device) {m_device= device;}
    void setBootloaderMethod(QString method) {m_bootloadermethod= method;}
    void setBootloaderName(QString name){m_bootloadername= name;}
    void setBootloaderBaseUrl(QString baseUrl){m_bootloaderUrlBase = baseUrl;}
   
signals:
    void done(bool error);  //installation finished.
    
    // internal signals. Dont use this from out side.
    void prepare();
    void finish();    
   
private slots:
    void updateDataReadProgress(int, int);
    void downloadDone(bool);
    void downloadRequestFinished(int, bool);
    
    // gigabeat specific routines
    void gigabeatPrepare();
    void gigabeatFinish();
    
    //iaudio specific routines
    void iaudioPrepare();
    void iaudioFinish();
    
    //h10 specific routines
    void h10Prepare();
    void h10Finish();
    
    //ipod specific routines
    void ipodPrepare();
    void ipodFinish();
    
private:
    QString m_mountpoint, m_device,m_bootloadermethod,m_bootloadername;
    QString m_bootloaderUrlBase,m_tempfilename;
    QUrl m_proxy;
    bool m_install;
    
    
    HttpGet *getter;
    QTemporaryFile downloadFile;
    
    Ui::InstallProgressFrm* m_dp;

};
#endif
