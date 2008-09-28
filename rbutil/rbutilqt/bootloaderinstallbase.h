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

#ifndef BOOTLOADERINSTALLBASE_H
#define BOOTLOADERINSTALLBASE_H

#include <QtCore>
#include "progressloggerinterface.h"
#include "httpget.h"


class BootloaderInstallBase : public QObject
{
    Q_OBJECT

    public:
        enum Capability
            { Install = 0x01, Uninstall = 0x02, Backup = 0x04,
              IsFile = 0x08, IsRaw = 0x10, NeedsFlashing = 0x20,
              CanCheckInstalled = 0x40, CanCheckVersion = 0x80 };
        Q_DECLARE_FLAGS(Capabilities, Capability)

        enum BootloaderType
            { BootloaderNone, BootloaderRockbox, BootloaderOther, BootloaderUnknown };

        BootloaderInstallBase(QObject *parent = 0) : QObject(parent)
            { }

        virtual bool install(void)
            { return false; }
        virtual bool uninstall(void)
            { return false; }
        virtual BootloaderType installed(void);
        virtual Capabilities capabilities(void);
        bool backup(QString to);

        void setBlFile(QString f)
            { m_blfile = f; }
        void setBlUrl(QUrl u)
            { m_blurl = u; }
        void setLogfile(QString f)
            { m_logfile = f; }

        static QString postinstallHints(QString model);

    protected slots:
        void downloadReqFinished(int id, bool error);
        void downloadBlFinish(bool error);
        void installBlfile(void);
    protected:
        enum LogMode
            { LogAdd, LogRemove };

        void downloadBlStart(QUrl source);
        int logInstall(LogMode mode);

        HttpGet m_http;        //! http download object
        QString m_blfile;      //! bootloader filename on player
        QString m_logfile;     //! file for installation log
        QUrl m_blurl;          //! bootloader download URL
        QTemporaryFile m_tempfile; //! temporary file for download
        QDateTime m_blversion; //! download timestamp used for version information

    signals:
        void downloadDone(void); //! internal signal sent when download finished.
        void done(bool);
        void logItem(QString, int); //! set logger item
        void logProgress(int, int); //! set progress bar.
};

Q_DECLARE_OPERATORS_FOR_FLAGS(BootloaderInstallBase::Capabilities)

#endif

