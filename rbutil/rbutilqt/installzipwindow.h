/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id: installzipwindow.h 14027 2007-07-27 17:42:49Z domonoky $
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef INSTALLZIPWINDOW_H
#define INSTALLZIPWINDOW_H

#include <QtGui>

#include <QSettings>

#include "ui_installzipfrm.h"
#include "installzip.h"
#include "progressloggergui.h"

class InstallZipWindow : public QDialog
{
    Q_OBJECT
    public:
        InstallZipWindow(QWidget *parent = 0);
        void setProxy(QUrl);
        void setMountPoint(QString);
        void setUrl(QString);
        void setLogSection(QString name){logsection = name; }
        void setUserSettings(QSettings*);
        void setDeviceSettings(QSettings*);

    public slots:
        void accept(void);

    private:
        Ui::InstallZipFrm ui;
        QUrl proxy;
        QSettings *devices;
        QSettings *userSettings;
        ProgressLoggerGui* logger;
        QString file;
        QString fileName;
        QString mountPoint;
        QString url;
        QString logsection;
        ZipInstaller* installer;

    private slots:
        void browseFolder(void);
        void done(bool);
        
};


#endif
