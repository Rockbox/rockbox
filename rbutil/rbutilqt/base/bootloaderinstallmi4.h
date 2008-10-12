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


// mi4 bootloader file based installation.
// Puts the bootloader file to the correct location and
// renames the OF to OF.mi4.
class BootloaderInstallMi4 : public BootloaderInstallBase
{
    Q_OBJECT

    public:
        BootloaderInstallMi4(QObject *parent = 0);
        bool install(void);
        bool uninstall(void);
        BootloaderInstallBase::BootloaderType installed(void);
        Capabilities capabilities(void);

    private slots:
        void installStage2(void);

    private:
};

