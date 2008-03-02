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

#ifndef INSTALLBOOTLOADER_H
#define INSTALLBOOTLOADER_H

#ifndef CONSOLE
#include <QtGui>
#else
#include <QtCore>
#endif

#include "progressloggerinterface.h"
#include "httpget.h"
#include "irivertools/irivertools.h"

extern "C" {
    // Ipodpatcher
    #include "../ipodpatcher/ipodpatcher.h"
    #include "../sansapatcher/sansapatcher.h"
};

bool initIpodpatcher();
bool initSansapatcher();

class BootloaderInstaller : public QObject
{
    Q_OBJECT

public:
    BootloaderInstaller(QObject* parent);
    ~BootloaderInstaller() {}

    void install(ProgressloggerInterface* dp);
    void uninstall(ProgressloggerInterface* dp);

    void setMountPoint(QString mountpoint) {m_mountpoint = mountpoint;}
    void setProxy(QUrl proxy) {m_proxy= proxy;}
    void setDevice(QString device) {m_device= device;}  //!< the current plattform
    void setBootloaderMethod(QString method) {m_bootloadermethod= method;}
    void setBootloaderName(QString name){m_bootloadername= name;}
    void setBootloaderBaseUrl(QString baseUrl){m_bootloaderUrlBase = baseUrl;}
    void setOrigFirmwarePath(QString path) {m_origfirmware = path;}  //!< for iriver original firmware
    void setBootloaderInfoUrl(QString url) {m_bootloaderinfoUrl =url; } //!< the url for the info file
    bool downloadInfo(); //!< should be called before install/uninstall, blocks until downloaded.
    bool uptodate(); //!< returns wether the bootloader is uptodate

signals:
    void done(bool error);  //installation finished.

signals:   // internal signals. Dont use this from out side.
    void prepare();
    void finish();

private slots:
    void createInstallLog();  // adds the bootloader entry to the log
    void removeInstallLog(); // removes the bootloader entry from the log

    void updateDataReadProgress(int, int);
    void downloadDone(bool);
    void downloadRequestFinished(int, bool);
    void infoDownloadDone(bool);
    void infoRequestFinished(int, bool);
    void installEnded(bool);

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

    //sansa specific routines
    void sansaPrepare();
    void sansaFinish();

    //iriver specific routines
    void iriverPrepare();
    void iriverFinish();
    
    //mrobe100 specific routines
    void mrobe100Prepare();
    void mrobe100Finish();

private:

    HttpGet *infodownloader;
    QTemporaryFile bootloaderInfo;
    volatile bool infoDownloaded;
    volatile bool infoError;

    QString m_mountpoint, m_device,m_bootloadermethod,m_bootloadername;
    QString m_bootloaderUrlBase,m_tempfilename,m_origfirmware;
    QUrl m_proxy;
    QString m_bootloaderinfoUrl;
    bool m_install;

    int series,table_entry;  // for fwpatcher

    HttpGet *getter;
    QTemporaryFile downloadFile;

    ProgressloggerInterface* m_dp;

};
#endif
