/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2008 by Dominik Wenger
 *   $Id: bootloaderinstallams.h 22317 2009-08-15 13:04:21Z Domonoky $
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef BOOTLOADERINSTALLMPIO_H
#define BOOTLOADERINSTALLMPIO_H

#include <QtCore>
#include "bootloaderinstallbase.h"

//! bootloader installation derivate based on mkmpioboot
class BootloaderInstallMpio : public BootloaderInstallBase
{
    Q_OBJECT
    public:
        BootloaderInstallMpio(QObject *parent);
        bool install(void);
        bool uninstall(void);
        BootloaderInstallBase::BootloaderType installed(void);
        Capabilities capabilities(void);
        QString ofHint();

    private:
        
    private slots:
        void installStage2(void);
};

#endif
