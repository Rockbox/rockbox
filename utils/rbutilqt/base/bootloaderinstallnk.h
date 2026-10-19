/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2026 by Michael Giacomelli
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef BOOTLOADERINSTALLNK_H
#define BOOTLOADERINSTALLNK_H

#include <QtCore>
#include "bootloaderinstallbase.h"

//! Sends an nk.bin to a Gigabeat S running its original firmware, over MTP.
//! Runs in its own thread, since the transfer blocks.
class BootloaderThreadNk : public QThread
{
    Q_OBJECT
    public:
        void run(void);
        void setFile(const QString& f)
            { m_file = f; }
        //! true if the whole file went to the player
        bool sent(void)
            { return m_sent; }
        //! the player's model name, as it reports it
        QString device(void)
            { return m_device; }
        //! what went wrong, or what the player reported after the transfer
        QString error(void)
            { return m_error; }
        static bool supported(void);

    signals:
        void progress(int, int);

    private:
        QString m_file;
        bool m_sent = false;
        QString m_device;
        QString m_error;
};


//! bootloader installation class for the Gigabeat S: a single-boot nk.bin
//! made by mknkboot, sent over MTP like a firmware update.
class BootloaderInstallNk : public BootloaderInstallBase
{
    Q_OBJECT
    public:
        BootloaderInstallNk(QObject *parent);
        bool install(void);
        bool uninstall(void);
        BootloaderInstallBase::BootloaderType installed(void);
        Capabilities capabilities(void);

    private slots:
        void installStage2(void);
        void installStage3(void);

    private:
        BootloaderThreadNk *m_thread;
        QTemporaryFile m_nkfile;
};

#endif
