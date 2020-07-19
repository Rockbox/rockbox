/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2020 Solomon Peachy
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef BOOTLOADERINSTALLBSPATCH_H
#define BOOTLOADERINSTALLBSPATCH_H

#include <QtCore>
#include "bootloaderinstallbase.h"

class BootloaderThreadBSPatch;

//! bootloader installation class for devices handled by mkimxboot.
class BootloaderInstallBSPatch : public BootloaderInstallBase
{
    Q_OBJECT
    public:
        BootloaderInstallBSPatch(QObject *parent);
        bool install(void);
        bool uninstall(void);
        BootloaderInstallBase::BootloaderType installed(void);
        Capabilities capabilities(void);
        QString ofHint();

    private slots:
        void installStage2(void);
        void installStage3(void);

    private:
        BootloaderThreadBSPatch *m_thread;
        QTemporaryFile m_patchedFile;
};

#endif
