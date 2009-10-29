/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2009 by Tomer Shalev
 *   $Id$
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * This file is a modified version of the AMS installer by Dominik Wenger
 *
 ****************************************************************************/
#ifndef BOOTLOADERINSTALLTCC_H
#define BOOTLOADERINSTALLTCC_H

#include <QtCore>
#include "bootloaderinstallbase.h"

//! bootloader installation derivate based on mktccboot
class BootloaderInstallTcc : public BootloaderInstallBase
{
    Q_OBJECT
    public:
        BootloaderInstallTcc(QObject *parent);
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
