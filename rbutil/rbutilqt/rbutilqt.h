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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
#include "installbootloader.h"

class RbUtilQt : public QMainWindow
{
    Q_OBJECT

    public:
        RbUtilQt(QWidget *parent = 0);

    private:
        Ui::RbUtilQtFrm ui;
        QSettings *devices;
        QSettings *userSettings;
        void initDeviceNames(void);
        QString deviceName(QString);
        QString platform;
        HttpGet *daily;
        QString absolutePath;
        QTemporaryFile buildInfo;
        void updateManual(void);
        ProgressLoggerGui *logger;
        ZipInstaller *installer;
        BootloaderInstaller* blinstaller;
        QUrl proxy(void);

    private slots:
        void about(void);
        void configDialog(void);
        void updateDevice(void);
        void updateSettings(void);
        void install(void);
        void installBl(void);
        void installFonts(void);
        void installDoom(void);
        void createTalkFiles(void);
        void downloadDone(bool);
        void downloadDone(int, bool);
        void downloadInfo(void);
        void installVoice(void);
        void installThemes(void);
        void uninstall(void);
        void uninstallBootloader(void);
        void downloadManual(void);
        void installPortable(void);
};

#endif
