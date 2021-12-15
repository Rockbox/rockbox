/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2008 by Dominik Riebeling
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef BOOTLOADERINSTALLS5L_H
#define BOOTLOADERINSTALLS5L_H

#include <QtCore>
#include "bootloaderinstallbase.h"


//! bootloader installation derivate based on mks5lboot
class BootloaderInstallS5l : public BootloaderInstallBase
{
    Q_OBJECT

    public:
        BootloaderInstallS5l(QObject *parent);
        bool install(void);
        bool uninstall(void);
        BootloaderInstallBase::BootloaderType installed(void);
        Capabilities capabilities(void);

    private slots:
        bool installStage1(void);
        void installStageMkdfu(void);
        void installStageWaitForEject(void);
        void installStageWaitForSpindown(void);
        void installStageWaitForProcs(void);
        void installStageWaitForDfu(void);
        void installStageSendDfu(void);
        void installStageWaitForRemount(void);
        void abortInstall(void);
        void installDone(bool);

    private:
        bool doInstall;
        QString mntpoint;
        unsigned char* dfu_buf;
        int dfu_size;
        QList<int> suspendedPids;
        bool aborted;
        bool abortDetected(void);
        QElapsedTimer scanTimer;
        bool scanSuccess;
        // progress
        QElapsedTimer progressTimer;
        int progressTimeout;
        int progCurrent;
        int progOrigin;
        int progTarget;
        bool actionShown;
        void setProgress(int, int=0);
        bool updateProgress(void);
};

#endif
