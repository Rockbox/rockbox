/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Riebeling
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


#ifndef QRBUTIL_H
#define QRBUTIL_H

#include <QSettings>
#include <QTemporaryFile>

#include "ui_rbutilqtfrm.h"
#include "httpget.h"
#include "installzip.h"
#include "progressloggergui.h"
#include "bootloaderinstallbase.h"

#include "rbsettings.h"

class RbUtilQt : public QMainWindow
{
    Q_OBJECT

    public:
        RbUtilQt(QWidget *parent = 0);

    private:
        Ui::RbUtilQtFrm ui;
        RbSettings* settings;

        void initDeviceNames(void);
        QString deviceName(QString);
        QString platform;
        HttpGet *daily;
        HttpGet *bleeding;
        QString absolutePath;
        QTemporaryFile buildInfo;
        QTemporaryFile bleedingInfo;
        void updateManual(void);
        ProgressLoggerGui *logger;
        ZipInstaller *installer;
        QUrl proxy(void);
        QMap<QString, QString> versmap;
        bool chkConfig(bool);

        volatile bool m_installed;
        volatile bool m_error;
        QString m_networkerror;
        bool m_gotInfo;
        bool m_auto;

    private slots:
        void about(void);
        void help(void);
        void sysinfo(void);
        void configDialog(void);
        void updateDevice(void);
        void updateSettings(void);

        void completeInstall(void);
        void smallInstall(void);
        bool smallInstallInner(void);
        void installdone(bool error);

        void installBtn(void);
        bool installAuto(void);
        void install(void);

        void installBootloaderBtn(void);
        bool installBootloaderAuto(void);
        void installBootloader(void);
        void installBootloaderPost(bool error);

        void installFontsBtn(void);
        bool installFontsAuto(void);
        void installFonts(void);

        bool hasDoom(void);
        void installDoomBtn(void);
        bool installDoomAuto(void);
        void installDoom(void);

        void createTalkFiles(void);
        void createVoiceFile(void);
        void downloadDone(bool);
        void downloadDone(int, bool);
        void downloadBleedingDone(bool);
        void downloadInfo(void);

        void installVoice(void);
        void installThemes(void);
        void uninstall(void);
        void uninstallBootloader(void);
        void downloadManual(void);
        void installPortable(void);
        void updateInfo(void);
        void updateTabs(int);
};

#endif

