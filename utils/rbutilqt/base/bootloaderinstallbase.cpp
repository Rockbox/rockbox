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
#include "utils.h"
#include "ziputil.h"
#include "mspackutil.h"
#include "Logger.h"

#if defined(Q_OS_MACX)
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#endif


BootloaderInstallBase::BootloaderType BootloaderInstallBase::installed(void)
{
    return BootloaderUnknown;
}


BootloaderInstallBase::Capabilities BootloaderInstallBase::capabilities(void)
{
    return Capabilities();
}


void BootloaderInstallBase::downloadBlStart(QUrl source)
{
    m_http.setFile(&m_tempfile);
    m_http.setCache(true);
    connect(&m_http, &HttpGet::done, this, &BootloaderInstallBase::downloadBlFinish);
    // connect the http read signal to our logProgess *signal*
    // to immediately emit it without any helper function.
    connect(&m_http, &HttpGet::dataReadProgress,
            this, &BootloaderInstallBase::logProgress);
    m_http.getFile(source);
}


void BootloaderInstallBase::downloadReqFinished(int id, bool error)
{
    LOG_INFO() << "Download Request" << id
               << "finished, error:" << m_http.errorString();

    downloadBlFinish(error);
}


void BootloaderInstallBase::downloadBlFinish(bool error)
{
    LOG_INFO() << "Downloading bootloader finished, error:"
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
                .arg(m_http.errorString()), LOGERROR);
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
    LOG_INFO() << "installBlFile(void)";
}


void BootloaderInstallBase::progressAborted(void)
{
    LOG_INFO() << "progressAborted(void)";
    emit installAborted();
}


//! @brief backup OF file.
//! @param to folder to write backup file to. Folder will get created.
//! @return true on success, false on error.
bool BootloaderInstallBase::backup(QString to)
{
    LOG_INFO() << "Backing up bootloader file";
    QDir targetDir(".");
    emit logItem(tr("Creating backup of original firmware file."), LOGINFO);
    if(!targetDir.mkpath(to)) {
        emit logItem(tr("Creating backup folder failed"), LOGERROR);
        return false;
    }
    QString tofile = to + "/" + QFileInfo(m_blfile).fileName();
    LOG_INFO() << "trying to backup" << m_blfile << "to" << tofile;
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
        LOG_INFO() << "Writing log, version:"
                   << m_blversion.toString(Qt::ISODate);
    }
    else {
        s.remove("Bootloader/" + section);
    }
    s.sync();

    emit logItem(tr("Installation log created"), LOGOK);

    return result;
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
            LOG_INFO() << "Player not remounted yet" << m_remountDevice;
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
    ArchiveUtil *util = nullptr;

    // check if we're actually looking for a zip file. If so we must avoid
    // trying to unzip it.
    bool wantZip = false;
    for (int i = 0; i < blfile.size(); i++)
    {
        if (blfile.at(i).endsWith(".zip"))
            wantZip = true;
    }

    // try ZIP first
    ZipUtil *zu = new ZipUtil(this);
    if(zu->open(of) && !wantZip)
    {
        emit logItem(tr("Zip file format detected"), LOGINFO);
        util = zu;
    }
    else
        delete zu;

    // if ZIP failed, try CAB
    if(util == nullptr)
    {
        MsPackUtil *msu = new MsPackUtil(this);
        if(msu->open(of))
        {
            emit logItem(tr("CAB file format detected"), LOGINFO);
            util = msu;
        }
        else
            delete msu;
    }

    // check if the file set is in zip format
    if(util) {
        QStringList contents = util->files();
        LOG_INFO() << "archive contains:" << contents;
        for(int i = 0; i < blfile.size(); ++i) {
            // strip any path, we don't know the structure in the zip
            QString f = QFileInfo(blfile.at(i)).fileName();
            LOG_INFO() << "searching archive for" << f;
            // contents.indexOf() works case sensitive. Since the filename
            // casing is unknown (and might change) do this manually.
            // FIXME: support files in folders
            for(int j = 0; j < contents.size(); ++j) {
                if(contents.at(j).compare(f, Qt::CaseInsensitive) == 0) {
                    found = true;
                    emit logItem(tr("Extracting firmware %1 from archive")
                            .arg(f), LOGINFO);
                    // store in class temporary file
                    m_tempof.open();
                    m_offile = m_tempof.fileName();
                    m_tempof.close();
                    if(!util->extractArchive(m_offile, contents.at(j))) {
                        emit logItem(tr("Error extracting firmware from archive"), LOGERROR);
                        found = false;
                        break;
                    }
                    break;
                }
            }
        }
        if(!found) {
            emit logItem(tr("Could not find firmware in archive"), LOGERROR);
        }
        delete util;
    }
    else {
        m_offile = of;
        found = true;
    }

    return found;
}

