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

#include "bootloaderinstallbase.h"
#include "utils.h"

BootloaderInstallBase::BootloaderType BootloaderInstallBase::installed(void)
{
    return BootloaderUnknown;
}


BootloaderInstallBase::Capabilities BootloaderInstallBase::capabilities(void)
{
    return 0;
}


void BootloaderInstallBase::downloadBlStart(QUrl source)
{
    m_http.setFile(&m_tempfile);
    m_http.setCache(true);
    connect(&m_http, SIGNAL(done(bool)), this, SLOT(downloadBlFinish(bool)));
    // connect the http read signal to our logProgess *signal*
    // to immediately emit it without any helper function.
    connect(&m_http, SIGNAL(dataReadProgress(int, int)),
            this, SIGNAL(logProgress(int, int)));
    m_http.getFile(source);
}


void BootloaderInstallBase::downloadReqFinished(int id, bool error)
{
    qDebug() << __FILE__ << "::" << __func__ << id << error;
    qDebug() << "error:" << m_http.errorString();

    downloadBlFinish(error);
}


void BootloaderInstallBase::downloadBlFinish(bool error)
{
    qDebug() << __FILE__ << "::" << __func__ << ": error =" << error;

    // update progress bar
    emit logProgress(100, 100);

    if(m_http.httpResponse() != 200) {
        emit logItem(tr("Download error: received HTTP error %1.")
                .arg(m_http.errorString()), LOGERROR);
        emit done(true);
        return;
    }
    if(error) {
        emit logItem(tr("Download error: %1")
                .arg(m_http.error()), LOGERROR);
        emit done(true);
        return;
    }
    else if(m_http.isCached())
        emit logItem(tr("Download finished (cache used)."), LOGOK);
    else
        emit logItem(tr("Download finished."), LOGOK);

    QCoreApplication::processEvents();
    m_blversion = m_http.timestamp();
    emit downloadDone();
}

void BootloaderInstallBase::installBlfile(void)
{
    qDebug() << __FILE__ << __func__;
}


//! @brief backup OF file.
//! @param to folder to write backup file to. Folder will get created.
//! @return true on success, false on error.

bool BootloaderInstallBase::backup(QString to)
{
    qDebug() << __func__;
    QDir targetDir(".");
    emit logItem(tr("Creating backup of original firmware file."), LOGINFO);
    if(!targetDir.mkpath(to)) {
        emit logItem(tr("Creating backup folder failed"), LOGERROR);
        return false;
    }
    QString tofile = to + "/" + QFileInfo(m_blfile).fileName();
    qDebug() << "trying to backup" << m_blfile << "to" << tofile;
    if(!QFile::copy(resolvePathCase(m_blfile), tofile)) {
        emit logItem(tr("Creating backup copy failed."), LOGERROR);
        return false;
    }
    emit logItem(tr("Backup created."), LOGOK);
    return true;
}


//! @brief log installation to logfile.
//! @param mode action to perform. 0: add to log, 1: remove from log.
//! @return 0 on success
int BootloaderInstallBase::logInstall(LogMode mode)
{
    int result = 0;
    QString section = m_blurl.path().section('/', -1);
    QSettings s(m_logfile, QSettings::IniFormat, this);
    emit logItem(tr("Creating installation log"), LOGINFO);

    if(mode == LogAdd) {
        s.setValue("Bootloader/" + section, m_blversion.toString(Qt::ISODate));
        qDebug() << m_blversion.toString(Qt::ISODate);
    }
    else {
        s.remove("Bootloader/" + section);
    }
    s.sync();

    emit logItem(tr("Installation log created"), LOGOK);

    return result;
}


//! @brief Return post install hints string.
//! @param model model string
//! @return hints.
QString BootloaderInstallBase::postinstallHints(QString model)
{
    bool hint = false;
    QString msg = tr("Bootloader installation is almost complete. "
            "Installation <b>requires</b> you to perform the "
            "following steps manually:");

    msg += "<ol>";
    msg += tr("<li>Safely remove your player.</li>");
    if(model == "h100" || model == "h120" || model == "h300") {
        hint = true;
        msg += tr("<li>Reboot your player into the original firmware.</li>"
                "<li>Perform a firmware upgrade using the update functionality "
                "of the original firmware. Please refer to your player's manual "
                "on details.</li>"
                "<li>After the firmware has been updated reboot your player.</li>");
    }
    if(model == "iaudiox5" || model == "iaudiom5"
            || model == "iaudiox5v" || model == "iaudiom3") {
        hint = true;
        msg += tr("<li>Turn the player off</li>"
                "<li>Insert the charger</li>");
    }
    if(model == "gigabeatf") {
        hint = true;
        msg += tr("<li>Unplug USB and power adaptors</li>"
                "<li>Hold <i>Power</i> to turn the player off</li>"
                "<li>Toggle the battery switch on the player</li>"
                "<li>Hold <i>Power</i> to boot into Rockbox</li>");
    }

    msg += "</ol>";
    msg += tr("<p><b>Note:</b> You can safely install other parts first, but "
            "the above steps are <b>required</b> to finish the installation!</p>");

    if(hint)
        return msg;
    else
        return QString("");
}

