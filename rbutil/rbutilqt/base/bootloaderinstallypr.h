/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2017 by Lorenzo Miori
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef BOOTLOADERINSTALLYPR_H
#define BOOTLOADERINSTALLYPR_H

#include <QtCore>
#include "bootloaderinstallbase.h"


// bootloader installation derivate based on fwpatcher
// This will patch a given ypr file using (de)scramble / mkboot
// and put it on the player.
class BootloaderInstallYpr : public BootloaderInstallBase
{
    Q_OBJECT

    public:
        BootloaderInstallYpr(QObject *parent = 0);
        bool install(void);
        bool uninstall(void);
        BootloaderInstallBase::BootloaderType installed(void);
        Capabilities capabilities(void);
        QString ofHint();

    private:
        int m_hashindex;
        int m_model;
        QTemporaryFile m_descrambled;
        QString scrambleError(int);

    private slots:
        void installStage2(void);
};


#endif

