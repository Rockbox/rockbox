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

#include <QtCore>
#include "progressloggerinterface.h"
#include "bootloaderinstallbase.h"

//! install a bootloader by putting a single file on the player.
//  This installation method is used by Iaudio (firmware is flashed
//  automatically) and Gigabeat (Firmware is a file, OF needs to get
//  renamed).
class BootloaderInstallFile : public BootloaderInstallBase
{
    Q_OBJECT

    public:
        BootloaderInstallFile(QObject *parent = 0);
        bool install(void);
        bool uninstall(void);
        BootloaderInstallBase::BootloaderType installed(void);
        Capabilities capabilities(void);

    private slots:
        void installStage2(void);

    private:
};

