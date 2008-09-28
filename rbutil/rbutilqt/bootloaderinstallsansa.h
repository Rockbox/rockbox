/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2008 by Dominik Riebeling
 *   $Id:$
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef BOOTLOADERINSTALLSANSA_H
#define BOOTLOADERINSTALLSANSA_H

#include <QtCore>
#include "bootloaderinstallbase.h"


// bootloader installation class for devices handled by sansapatcher.
class BootloaderInstallSansa : public BootloaderInstallBase
{
    Q_OBJECT

    public:
        BootloaderInstallSansa(QObject *parent = 0);
        ~BootloaderInstallSansa();
        bool install(void);
        bool uninstall(void);
        BootloaderInstallBase::BootloaderType installed(void);
        Capabilities capabilities(void);

    private:

    private slots:
        void installStage2(void);
};


#endif

