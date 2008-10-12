/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2008 by Dominik Riebeling
 *   $Id$
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef BOOTLOADERINSTALLIPOD_H
#define BOOTLOADERINSTALLIPOD_H

#include <QtCore>
#include "bootloaderinstallbase.h"
#include "../ipodpatcher/ipodpatcher.h"

// installer class derivate for Ipod installation
// based on ipodpatcher.
class BootloaderInstallIpod : public BootloaderInstallBase
{
    Q_OBJECT

    public:
        BootloaderInstallIpod(QObject *parent = 0);
        ~BootloaderInstallIpod();
        bool install(void);
        bool uninstall(void);
        BootloaderInstallBase::BootloaderType installed(void);
        Capabilities capabilities(void);

    private slots:
        void installStage2(void);

    private:
        bool ipodInitialize(struct ipod_t *);
};


#endif

