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

#ifndef BOOTLOADERINSTALLHEX_H
#define BOOTLOADERINSTALLHEX_H

#include <QtCore>
#include "bootloaderinstallbase.h"


// bootloader installation derivate based on fwpatcher
// This will patch a given hex file using (de)scramble / mkboot
// and put it on the player.
class BootloaderInstallHex : public BootloaderInstallBase
{
    Q_OBJECT

    public:
        BootloaderInstallHex(QObject *parent = 0);
        bool install(void);
        bool uninstall(void);
        BootloaderInstallBase::BootloaderType installed(void);
        Capabilities capabilities(void);

        void setHexfile(QString h)
            { m_hex = h; }

    private:
        QString m_hex;
        int m_hashindex;
        int m_model;
        QTemporaryFile m_descrambled;
        QString scrambleError(int);

    private slots:
        void installStage2(void);
};


#endif

