/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2012 by Dominik Riebeling
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef SELECTIVEINSTALLWIDGET_H
#define SELECTIVEINSTALLWIDGET_H

#include <QWidget>
#include "ui_selectiveinstallwidgetfrm.h"
#include "progressloggergui.h"
#include "zipinstaller.h"
#include "themesinstallwindow.h"
#include "playerbuildinfo.h"

class SelectiveInstallWidget : public QWidget
{
    Q_OBJECT
    public:
        SelectiveInstallWidget(QWidget* parent = nullptr);
        void installBootloaderHints(void);

    public slots:
        void updateVersion(void);
        void saveSettings(void);
        void startInstall(void);

    private slots:
        void continueInstall(bool);
        void selectedVersionChanged(int);
        void updateVoiceLangs();

    private:
        void installBootloader(void);
        void installRockbox(void);
        void installFonts(void);
        void installVoicefile(void);
        void installManual(void);
        void installThemes(void);
        void installPluginData(void);
        void installBootloaderPost(void);

    signals:
        void installSkipped(bool);

    private:
        void changeEvent(QEvent *e);

        Ui::SelectiveInstallWidget ui;
        QString m_target;
        QString m_blmethod;
        QString m_mountpoint;
        ProgressLoggerGui *m_logger;
        int m_installStage;
        ZipInstaller *m_zipinstaller;
        ThemesInstallWindow *m_themesinstaller;
        PlayerBuildInfo::BuildType m_buildtype;
};

#endif

