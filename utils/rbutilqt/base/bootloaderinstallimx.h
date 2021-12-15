/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2011 by Jean-Louis Biasini
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef BOOTLOADERINSTALLIMX_H
#define BOOTLOADERINSTALLIMX_H

#include <QtCore>
#include "bootloaderinstallbase.h"

class BootloaderThreadImx;

//! bootloader installation class for devices handled by mkimxboot.
class BootloaderInstallImx : public BootloaderInstallBase
{
    Q_OBJECT
    public:
        BootloaderInstallImx(QObject *parent);
        bool install(void);
        bool uninstall(void);
        BootloaderInstallBase::BootloaderType installed(void);
        Capabilities capabilities(void);
        QString ofHint();

    private slots:
        void installStage2(void);
        void installStage3(void);

    private:
        BootloaderThreadImx *m_thread;
        QTemporaryFile m_patchedFile;
};

#endif
