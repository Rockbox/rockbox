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
#include "bootloaderinstall6g.h"
#include "Logger.h"
#include "utils.h"
#include "system.h"
#include "rbsettings.h"
#include "systeminfo.h"

#include "../mk6gboot/mk6gboot.h"


BootloaderInstall6g::BootloaderInstall6g(QObject *parent)
        : BootloaderInstallBase(parent)
{
}


bool BootloaderInstall6g::install(void)
{
    LOG_INFO() << "installing bootloader";
#if 1
    // use local bootloader, must be located in the same folder as rbutil.exe
    QString absPath = QDir::current().absoluteFilePath(SystemInfo::value(
                SystemInfo::CurBootloaderName).toString().section('/', -1));
    //setBlUrl(QUrl("file://" + absPath)); // XXX: malformed QUrl on Win Qt5
    setBlUrl(QUrl::fromLocalFile(absPath));
#endif
    doInstall = true;
    return installStage1();
}


bool BootloaderInstall6g::uninstall(void)
{
    LOG_INFO() << "uninstalling bootloader";
    doInstall = false;
    return installStage1();
}


bool BootloaderInstall6g::installStage1(void)
{
    LOG_INFO() << "installStage1";

    mntpoint = RbSettings::value(RbSettings::Mountpoint).toString();

    if (!Utils::mountpoints(Utils::MountpointsSupported).contains(mntpoint)) {
        LOG_ERROR() << "iPod not mounted:" << mntpoint;
        emit logItem(tr("Could not find mounted iPod."), LOGERROR);
        emit done(true);
        return false;
    }

    if (doInstall) {
        // download firmware from server
        emit logItem(tr("Downloading bootloader file..."), LOGINFO);
        connect(this, SIGNAL(downloadDone()), this, SLOT(installStageMkdfu()));
        downloadBlStart(m_blurl);
    }
    else {
        installStageMkdfu();
    }

    return true;
}


void BootloaderInstall6g::installStageMkdfu(void)
{
    int dfu_type;
    QString dfu_arg;
    char errstr[200];

    LOG_INFO() << "installStageMkdfu";

    setProgress(0);
    aborted = false;
    connect(this, SIGNAL(installAborted()), this, SLOT(abortInstall()));
    connect(this, SIGNAL(done(bool)), this, SLOT(installDone(bool)));

    if (doInstall) {
        dfu_type = DFU_INST;
        m_tempfile.open();
        dfu_arg = m_tempfile.fileName();
        m_tempfile.close();
    }
    else {
        dfu_type = DFU_UNINST;
        dfu_arg = RbSettings::value(RbSettings::Platform).toString();
    }

    // build DFU image
    dfu_buf = mkdfu(dfu_type, dfu_arg.toLocal8Bit().data(),
                                &dfu_size, errstr, sizeof(errstr));
    if (!dfu_buf) {
        LOG_ERROR() << "mkdfu() failed:" << errstr;
        emit logItem(errstr, LOGERROR);
        emit logItem(tr("Could not make DFU imagen."), LOGERROR);
        emit done(true);
        return;
    }

    LOG_INFO() << "preparing installStageWaitForEject";
    emit logItem(tr("Ejecting iPod..."), LOGINFO);
    setProgress(10);
    scanTimer = QTime();
    installStageWaitForEject();
}


void BootloaderInstall6g::installStageWaitForEject(void)
{
    if (!updateProgress())
        return; /* aborted */

    if (scanTimer.isNull() || (scanTimer.elapsed() > 3000)) {
        scanSuccess = Utils::ejectDevice(mntpoint);
        if (!scanSuccess) {
            scanSuccess = !Utils::mountpoints(
                        Utils::MountpointsSupported).contains(mntpoint);
        }
        scanTimer.start();
    }
    if (!scanSuccess) {
        if (!actionShown) {
            emit logItem(tr("Action required:\n"
                            "  Please make sure no programs are accessing\n"
                            "  files on the device. If ejecting still fails\n"
                            "  please use your computers eject funtionality."),
                            LOGWARNING);
            actionShown = true;
        }
        QTimer::singleShot(250, this, SLOT(installStageWaitForEject()));
        return;
    }
    emit logItem(tr("Device successfully ejected."), LOGINFO);

    LOG_INFO() << "preparing installStageWaitForProcs";
    setProgress(40, 18);
    scanTimer = QTime();
    installStageWaitForProcs();
}


void BootloaderInstall6g::installStageWaitForProcs(void)
{
    if (!updateProgress())
        return; /* aborted */

    if (scanTimer.isNull() || (scanTimer.elapsed() > 1000)) {
        scanSuccess = Utils::findRunningProcess(QStringList("iTunes")).isEmpty();
        scanTimer.start();
    }
    if (!scanSuccess) {
        if (!actionShown) {
            emit logItem(tr("Action required:\n"
                            "  Close iTunes application."), LOGWARNING);
            actionShown = true;
        }
        QTimer::singleShot(250, this, SLOT(installStageWaitForProcs()));
        return;
    }
    if (actionShown) {
        emit logItem(tr("iTunes closed."), LOGINFO);
        if (!updateProgress())
            return; /* aborted */
    }

    QList<int> helperPids = Utils::findRunningProcess(
                    QStringList("iTunesHelper"))["iTunesHelper.exe"];
    suspendedPids = Utils::suspendProcess(helperPids, true);
    if (suspendedPids.size() != helperPids.size()) {
        emit logItem(tr("Could not suspend iTunesHelper. Stop it\n"
                        "using the Task Manager, and try again."), LOGERROR);
        emit done(true);
        return;
    }

    LOG_INFO() << "preparing installStageWaitForSpindown";
    // for Windows: skip waiting if the HDD was ejected a time ago
    if (progressTimer.elapsed() < progressTimeout)
        emit logItem(tr("Waiting for HDD spin-down..."), LOGINFO);
    installStageWaitForSpindown();
}


void BootloaderInstall6g::installStageWaitForSpindown(void)
{
    if (!updateProgress())
        return; /* aborted */

    if (progressTimer.elapsed() < progressTimeout) {
        QTimer::singleShot(250, this, SLOT(installStageWaitForSpindown()));
        return;
    }

    LOG_INFO() << "preparing installStageWaitForDfu";
    emit logItem(tr("Waiting for DFU mode..."), LOGINFO);
    emit logItem(tr("Action required:\n"
                    "  Press and hold SELECT+MENU buttons, after\n"
                    "  about 12 seconds a new action will require\n"
                    "  you to release the buttons, DO IT QUICKLY,\n"
                    "  otherwise the process could fail."), LOGWARNING);
    scanTimer = QTime();
    installStageWaitForDfu();
}


void BootloaderInstall6g::installStageWaitForDfu(void)
{
    if (!updateProgress())
        return; /* aborted */

    if (scanTimer.isNull() || (scanTimer.elapsed() > 2000)) {
        scanSuccess = System::listUsbIds().contains(0x05ac1223);
        scanTimer.start();
    }
    if (!scanSuccess) {
        QTimer::singleShot(250, this, SLOT(installStageWaitForDfu()));
        return;
    }
    emit logItem(tr("DFU mode detected."), LOGINFO);

    emit logItem(tr("Action required:\n"
                    "  Release SELECT+MENU buttons and wait."), LOGWARNING);

    // Once the iPod enters DFU mode, the device will reset again if
    // SELECT+MENU remains pressed for another 8 seconds. To avoid a
    // reset while the NOR is being written, we wait ~10 seconds
    // before sending the DFU imagen.
    LOG_INFO() << "preparing installStageSendDfu";
    setProgress(60, 10);
    installStageSendDfu();
}


void BootloaderInstall6g::installStageSendDfu(void)
{
    if (!updateProgress())
        return; /* aborted */

    if (progressTimer.elapsed() < progressTimeout) {
        QTimer::singleShot(250, this, SLOT(installStageSendDfu()));
        return;
    }

    if (!System::listUsbIds().contains(0x05ac1223)) {
        LOG_ERROR() << "device not in DFU mode";
        emit logItem(tr("Device is not in DFU mode. It seems that\n"
                        "the previous required action failed, please\n"
                        "try again."), LOGERROR);
        emit done(true);
        return;
    }

#if defined(Q_OS_WIN32)
    emit logItem(tr("Transfering DFU imagen..."), LOGINFO);
    if (!updateProgress())
        return; /* aborted */
#endif

    char errstr[200];
    if (!ipoddfu_send(0x1223, dfu_buf, dfu_size, errstr, sizeof(errstr))) {
        LOG_ERROR() << "ipoddfu_send() failed:" << errstr;
#if defined(Q_OS_WIN32)
        if (strstr(errstr, "open USB device: LIBUSB_ERROR_NOT_SUPPORTED"))
        {
            emit logItem(tr("No valid DFU USB driver found. Install\n"
                            "iTunes or a WinUSB driver and try again."),
                            LOGERROR);
        }
        else
#endif
        {
            emit logItem(errstr, LOGERROR);
            emit logItem(tr("Could not transfer DFU imagen."), LOGERROR);
        }
        emit done(true);
        return;
    }
    emit logItem(tr("DFU transfer completed."), LOGINFO);

    LOG_INFO() << "preparing installStageWaitForRemount";
    emit logItem(tr("Restarting iPod, waiting for remount..."), LOGINFO);
    setProgress(99, 45);
    scanTimer = QTime();
    installStageWaitForRemount();
}


void BootloaderInstall6g::installStageWaitForRemount(void)
{
    if (!updateProgress())
        return; /* aborted */

    if (scanTimer.isNull() || (scanTimer.elapsed() > 5000)) {
        scanSuccess = Utils::mountpoints(
                    Utils::MountpointsSupported).contains(mntpoint);
        scanTimer.start();
    }
    if (!scanSuccess) {
        if (!actionShown && (progressTimer.elapsed() > progressTimeout)) {
            emit logItem(tr("Action required:\n"
                            "  Could not remount the device, try to do it\n"
                            "  manually. If the iPod didn't restart, force\n"
                            "  a reset by pressing SELECT+MENU buttons\n"
                            "  for about 5 seconds. If the problem could\n"
                            "  not be solved then click 'Abort' to cancel."),
                            LOGWARNING);
            actionShown = true;
        }
        QTimer::singleShot(250, this, SLOT(installStageWaitForRemount()));
        return;
    }
    emit logItem(tr("Device remounted."), LOGINFO);

    emit logItem(tr("Bootloader successfully %1.").
            arg(tr(doInstall ? "installed" : "uninstalled")), LOGOK);

    logInstall(doInstall ? LogAdd : LogRemove);
    emit logProgress(1, 1);
    emit done(false);
}


void BootloaderInstall6g::installDone(bool status)
{
    LOG_INFO() << "installDone, status:" << status;
    if (Utils::suspendProcess(suspendedPids, false).size() != suspendedPids.size())
        emit logItem(tr("Could not resume iTunesHelper."), LOGWARNING);
}


void BootloaderInstall6g::abortInstall(void)
{
    LOG_INFO() << "abortInstall";
    aborted = true;
    disconnect(this, SIGNAL(installAborted()), this, SLOT(abortInstall()));
}


bool BootloaderInstall6g::abortDetected(void)
{
    if (aborted) {
        LOG_ERROR() << "abortDetected";
        emit logItem(tr("%1 aborted by user.").
                arg(tr(doInstall ? "Install" : "Uninstall")), LOGERROR);
        emit done(true);
        return true;
    }
    return false;
}


void BootloaderInstall6g::setProgress(int progress, int secondsTimeout)
{
    progressTimer.start();
    progressTimeout = secondsTimeout * 1000;
    progOrigin = progTarget;
    progTarget = progress;
    actionShown = false;
}


bool BootloaderInstall6g::updateProgress(void)
{
    if (progressTimeout) {
        progCurrent = qMin(progTarget, progOrigin +
            progressTimer.elapsed()*(progTarget-progOrigin)/progressTimeout);
    }
    else {
        progCurrent = progTarget;
    }
    emit logProgress(progCurrent, 100);
    QCoreApplication::processEvents();
    return !abortDetected();
}


BootloaderInstallBase::BootloaderType BootloaderInstall6g::installed(void)
{
    bool rbblInstalled;

    QString device = Utils::resolveDevicename(m_blfile);
    if (device.isEmpty()) {
        LOG_INFO() << "installed: BootloaderUnknown";
        return BootloaderUnknown;
    }

    // rely on logfile
    QString logfile = RbSettings::value(RbSettings::Mountpoint).toString()
                + "/.rockbox/rbutil.log";
    QSettings s(logfile, QSettings::IniFormat, this);
    QString section = SystemInfo::value(
                SystemInfo::CurBootloaderName).toString().section('/', -1);
    rbblInstalled = s.contains("Bootloader/" + section);

    if (rbblInstalled) {
        LOG_INFO() << "installed: BootloaderRockbox";
        return BootloaderRockbox;
    }
    else {
        LOG_INFO() << "installed: BootloaderOther";
        return BootloaderOther;
    }
}


BootloaderInstallBase::Capabilities BootloaderInstall6g::capabilities(void)
{
    return (Install | Uninstall);
}
