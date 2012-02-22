/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2008 by Dominik Riebeling
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
#include "bootloaderinstallmi4.h"
#include "bootloaderinstallhex.h"
#include "bootloaderinstallipod.h"
#include "bootloaderinstallsansa.h"
#include "bootloaderinstallfile.h"
#include "bootloaderinstallchinachip.h"
#include "bootloaderinstallams.h"
#include "bootloaderinstalltcc.h"
#include "bootloaderinstallmpio.h"
#include "bootloaderinstallimx.h"
#include "utils.h"
#include "ziputil.h"

#if defined(Q_OS_MACX)
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#endif


BootloaderInstallBase* BootloaderInstallBase::createBootloaderInstaller(QObject* parent,QString type)
{
    if(type == "mi4") {
        return new BootloaderInstallMi4(parent);
    }
    else if(type == "hex") {
        return new BootloaderInstallHex(parent);
    }
    else if(type == "sansa") {
        return new BootloaderInstallSansa(parent);
    }
    else if(type == "ipod") {
        return new BootloaderInstallIpod(parent);
    }
    else if(type == "file") {
        return new BootloaderInstallFile(parent);
    }
    else if(type == "chinachip") {
        return new BootloaderInstallChinaChip(parent);
    }
    else if(type == "ams") {
        return new BootloaderInstallAms(parent);
    }
    else if(type == "tcc") {
        return new BootloaderInstallTcc(parent);
    }
    else if(type == "mpio") {
        return new BootloaderInstallMpio(parent);
    }
    else if(type == "imx") {
        return new BootloaderInstallImx(parent);
    }
    else {
        return NULL;
    }

}

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
    qDebug() << "[BootloaderInstallBase] Download Request" << id
             << "finished, error:" << m_http.errorString();

    downloadBlFinish(error);
}


void BootloaderInstallBase::downloadBlFinish(bool error)
{
    qDebug() << "[BootloaderInstallBase] Downloading bootloader finished, error:"
             << error;

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
    qDebug() << "[BootloaderInstallBase] installBlFile(void)";
}


//! @brief backup OF file.
//! @param to folder to write backup file to. Folder will get created.
//! @return true on success, false on error.

bool BootloaderInstallBase::backup(QString to)
{
    qDebug() << "[BootloaderInstallBase] Backing up bootloader file";
    QDir targetDir(".");
    emit logItem(tr("Creating backup of original firmware file."), LOGINFO);
    if(!targetDir.mkpath(to)) {
        emit logItem(tr("Creating backup folder failed"), LOGERROR);
        return false;
    }
    QString tofile = to + "/" + QFileInfo(m_blfile).fileName();
    qDebug() << "[BootloaderInstallBase] trying to backup" << m_blfile << "to" << tofile;
    if(!QFile::copy(Utils::resolvePathCase(m_blfile), tofile)) {
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
        qDebug() << "[BootloaderInstallBase] Writing log, version:"
                 << m_blversion.toString(Qt::ISODate);
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
    if(model != "sansafuzeplus") {
        msg += tr("<li>Safely remove your player.</li>");
    }
    if(model == "iriverh100" || model == "iriverh120" || model == "iriverh300" ||
       model == "ondavx747") {
        hint = true;
        msg += tr("<li>Reboot your player into the original firmware.</li>"
                "<li>Perform a firmware upgrade using the update functionality "
                "of the original firmware. Please refer to your player's manual "
                "on details.<br/><b>Important:</b> updating the firmware is a "
                "critical process that must not be interrupted. <b>Make sure the "
                "player is charged before starting the firmware update "
                "process.</b></li>"
                "<li>After the firmware has been updated reboot your player.</li>");
    }
    if(model == "sansafuzeplus") {
        hint = true;
        msg += tr("<li>Remove any previously inserted microSD card</li>");
        msg += tr("<li>Disconnect your player. The player will reboot and "
                "perform an update of the original firmware. "
                "Please refer to your players manual on details.<br/>"
                "<b>Important:</b> updating the firmware is a "
                "critical process that must not be interrupted. <b>Make sure the "
                "player is charged before disconnecting the player.</b></li>"
                "<li>After the firmware has been updated reboot your player.</li>");
    }
    if(model == "iaudiox5" || model == "iaudiom5"
            || model == "iaudiox5v" || model == "iaudiom3" || model == "mpioh200") {
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


#if defined(Q_OS_MACX)
void BootloaderInstallBase::waitRemount()
{
    m_remountTries = 600;
    emit logItem(tr("Waiting for system to remount player"), LOGINFO);

    QTimer::singleShot(100, this, SLOT(checkRemount()));
}
#endif


void BootloaderInstallBase::checkRemount()
{
#if defined(Q_OS_MACX)
    if(m_remountTries--) {
        int status = 0;
        // check if device has been remounted
        QCoreApplication::processEvents();
        int num;
        struct statfs *mntinf;

        num = getmntinfo(&mntinf, MNT_WAIT);
        while(num--) {
            if(QString(mntinf->f_mntfromname).startsWith(m_remountDevice)
                    && QString(mntinf->f_fstypename).contains("msdos", Qt::CaseInsensitive))
                status = 1;
            mntinf++;
        }
        if(!status) {
            // still not remounted, restart timer.
            QTimer::singleShot(500, this, SLOT(checkRemount()));
            qDebug() << "[BootloaderInstallBase] Player not remounted yet" << m_remountDevice;
        }
        else {
            emit logItem(tr("Player remounted"), LOGINFO);
            emit remounted(true);
        }
    }
    else {
        emit logItem(tr("Timeout on remount"), LOGERROR);
        emit remounted(false);
    }
#endif
}


//! @brief set list of possible bootloader files and pick the existing one.
//! @param sl list of possible bootloader files.
void BootloaderInstallBase::setBlFile(QStringList sl)
{
    // figue which of the possible bootloader filenames is correct.
    for(int a = 0; a < sl.size(); a++) {
        if(!Utils::resolvePathCase(sl.at(a)).isEmpty()) {
            m_blfile = sl.at(a);
        }
    }
    if(m_blfile.isEmpty()) {
        m_blfile = sl.at(0);
    }
}

bool BootloaderInstallBase::setOfFile(QString of, QStringList blfile)
{
    bool found = false;
    ZipUtil z(this);
    // check if the file set is in zip format
    if(z.open(of)) {
        emit logItem(tr("Zip file format detected"), LOGINFO);
        QStringList contents = z.files();
        qDebug() << "[BootloaderInstallBase] archive contains:" << contents;
        for(int i = 0; i < blfile.size(); ++i) {
            // strip any path, we don't know the structure in the zip
            QString f = QFileInfo(blfile.at(i)).fileName();
            qDebug() << "[BootloaderInstallBase] searching archive for" << f;
            int index = contents.indexOf(f); // FIXME: support files in folders
            if(index >= 0) {
                found = true;
                emit logItem(tr("Extracting firmware %1 from archive")
                             .arg(f), LOGINFO);
                // store in class temporary file
                m_tempof.open();
                m_offile = m_tempof.fileName();
                m_tempof.close();
                if(!z.extractArchive(m_offile, contents.at(index))) {
                    emit logItem(tr("Error extracting firmware from archive"), LOGERROR);
                    found = false;
                    break;
                }
            }
        }
        if(!found) {
            emit logItem(tr("Could not find firmware in archive"), LOGERROR);
        }

    }
    else {
        m_offile = of;
        found = true;
    }
    return found;
}
