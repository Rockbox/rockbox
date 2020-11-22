/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Riebeling
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


#ifndef QRBUTIL_H
#define QRBUTIL_H

#include <QSettings>
#include <QTemporaryFile>
#include <QList>
#include <QTranslator>

#include "ui_rbutilqtfrm.h"
#include "httpget.h"
#include "zipinstaller.h"
#include "progressloggergui.h"
#include "bootloaderinstallbase.h"
#include "infowidget.h"
#include "selectiveinstallwidget.h"
#include "backupdialog.h"

class RbUtilQt : public QMainWindow
{
    Q_OBJECT

    public:
        RbUtilQt(QWidget *parent = nullptr);
        static QList<QTranslator*> translators;
        static bool chkConfig(QWidget *parent = nullptr);

    private:
        InfoWidget *info;
        SelectiveInstallWidget* selectiveinstallwidget;
        BackupDialog *backupdialog;
        Ui::RbUtilQtFrm ui;

        void changeEvent(QEvent *e);
        void initDeviceNames(void);
        QString deviceName(QString);
        QString platform;
        HttpGet *daily;
        HttpGet *bleeding;
        HttpGet *update;
        QString absolutePath;
        QTemporaryFile buildInfo;
        QTemporaryFile bleedingInfo;
        ProgressLoggerGui *logger;
        ZipInstaller *installer;
        QUrl proxy(void);

        volatile bool m_installed;
        volatile bool m_error;
        QString m_networkerror;
        bool m_gotInfo;
        bool m_auto;

    private slots:
        void shutdown(void);
        void about(void);
        void help(void);
        void sysinfo(void);
        void changelog(void);
        void trace(void);
        void eject(void);
        void configDialog(void);
        void updateDevice(void);
        void updateSettings(void);

        void installdone(bool error);

        void createTalkFiles(void);
        void createVoiceFile(void);
        void downloadDone(bool);
        void downloadInfo(void);
        void backup(void);

        void uninstall(void);
        void uninstallBootloader(void);
        void installPortable(void);
        void updateTabs(int);

        void checkUpdate(void);
        void downloadUpdateDone(bool errror);
};

#endif

