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

#ifndef BOOTLOADERINSTALLBASE_H
#define BOOTLOADERINSTALLBASE_H

#include <QtCore>
#include "progressloggerinterface.h"
#include "httpget.h"

//! baseclass for all Bootloader installs
class BootloaderInstallBase : public QObject
{
    Q_OBJECT
    public:
        enum Capability
            { Install = 0x01, Uninstall = 0x02, Backup = 0x04,
              IsFile = 0x08, IsRaw = 0x10, NeedsOf = 0x20,
              CanCheckInstalled = 0x40, CanCheckVersion = 0x80 };
        Q_DECLARE_FLAGS(Capabilities, Capability)

        enum BootloaderType
            { BootloaderNone, BootloaderRockbox, BootloaderOther, BootloaderUnknown };

        BootloaderInstallBase(QObject *parent) : QObject(parent)
            { }

        //! install the bootloader, must be implemented
        virtual bool install(void) = 0;
        //! uninstall the bootloader, must be implemented
        virtual bool uninstall(void) = 0;
        //! returns the installed bootloader
        virtual BootloaderType installed(void)=0;
        //! returns the capabilities of the bootloader class
        virtual Capabilities capabilities(void)=0;
        //! returns a OF Firmware hint or empty if there is none
        virtual QString ofHint() {return QString();}
        
        
        //! backup a already installed bootloader
        bool backup(QString to);

        //! set the different filenames and paths
        void setBlFile(QStringList f);
        void setBlUrl(QUrl u)
            { m_blurl = u; }
        void setLogfile(QString f)
            { m_logfile = f; }
        void setOfFile(QString f)
            {m_offile = f;}
        
        //! returns a port Install Hint or empty if there is none
        //! static and in the base class, so the installer classes dont need to
        //  be modified for new targets
        static QString postinstallHints(QString model);

    protected slots:
        void downloadReqFinished(int id, bool error);
        void downloadBlFinish(bool error);
        void installBlfile(void);

        // NOTE: we need to keep this slot even on targets that don't need it
        // -- using the preprocessor here confused moc.
        void checkRemount(void);
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
        QString m_offile;      //! path to the offile
#if defined(Q_OS_MACX)
        void waitRemount(void);

        int m_remountTries;
        QString m_remountDevice;
#endif

    signals:
        void downloadDone(void); //! internal signal sent when download finished.
        void done(bool);
        void logItem(QString, int); //! set logger item
        void logProgress(int, int); //! set progress bar.

        // NOTE: we need to keep this signal even on targets that don't need it
        // -- using the preprocessor here confused moc.
        void remounted(bool);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(BootloaderInstallBase::Capabilities)

#endif

