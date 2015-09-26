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

#include "../mk6gboot/mk6gboot.h"

#define SLEEP_MS        250
#define TICKS_PER_HZ    4

BootloaderInstall6g::BootloaderInstall6g(QObject *parent)
        : BootloaderInstallBase(parent)
{
}

bool BootloaderInstall6g::install(void)
{
    LOG_INFO() << "installing bootloader";
    return installStage1(true);
}

bool BootloaderInstall6g::uninstall(void)
{
    LOG_INFO() << "uninstalling bootloader";
    return installStage1(false);
}

#include "rbsettings.h"
bool BootloaderInstall6g::installStage1(bool install_uninstall)
{
    LOG_INFO() << "installStage1";

    doInstall = install_uninstall;

    mntpoint = RbSettings::value(RbSettings::Mountpoint).toString();

    if (!Utils::mountpoints(Utils::MountpointsSupported).contains(mntpoint)) {
        LOG_ERROR() << "iPod not mounted:" << mntpoint;
        emit logItem(tr("Could not find mounted iPod."), LOGERROR);
        emit done(true);
        return false;
    }

#ifdef DEBUG___USE_LOCAL_FILES
    installStageMkdfu();
#else
    // download firmware from server (uninstaller also needs
    // it to identify the target)
    emit logItem(tr("Downloading bootloader file..."), LOGINFO);

    connect(this, SIGNAL(downloadDone()), this, SLOT(installStageMkdfu()));
    downloadBlStart(m_blurl);
#endif

    return true;
}

void BootloaderInstall6g::installStageMkdfu(void)
{
    char errstr[200];

    LOG_INFO() << "installStageMkdfu";

    setProgress(0);
    iTunesHelperPid = 0;
    aborted = false;
    connect(this, SIGNAL(installAborted()), this, SLOT(abortInstall()));
    connect(this, SIGNAL(done(bool)), this, SLOT(installDone(bool)));

#ifdef DEBUG___USE_LOCAL_FILES
    QString bootfile = "bootloader-ipod6g.ipod";
    emit logItem(tr("Experimental - gerrit patch set 5\n"
                    "http://gerrit.rockbox.org/r/#/c/1221/5\n"
                    "using local file %1").arg(bootfile), LOGWARNING);
#else
    m_tempfile.open();
    QString bootfile = m_tempfile.fileName();
    m_tempfile.close();
#endif

    // load bootloader file and build DFU image
    dfu_buf = mkdfu(bootfile.toLocal8Bit().data(),
                    !doInstall, &dfu_size, errstr, sizeof(errstr));

    if (dfu_buf == NULL) {
        LOG_ERROR() << "mkdfu() failed:" << errstr;
        emit logItem(errstr, LOGERROR);
        emit logItem(tr("Could not load %1").arg(bootfile), LOGERROR);
        emit done(true);
        return;
    }

    emit logItem(tr("Bootloader file loaded correctly."), LOGINFO);

    LOG_INFO() << "preparing installStageWaitForEject";
    emit logItem(tr("Ejecting iPod..."), LOGINFO);
    setProgress(10);
    scanPeriodTicks = 3*TICKS_PER_HZ;
    installStageWaitForEject();
}

void BootloaderInstall6g::installStageWaitForEject(void)
{
    if (!updateProgress())
        return; /* aborted */

    if ((elapsedTicks++ % scanPeriodTicks) || (
#if !defined(Q_OS_LINUX)
            !Utils::ejectDevice(mntpoint) &&
#endif
            Utils::mountpoints(Utils::MountpointsSupported).contains(mntpoint)))
    {
        if (!actionRequired) {
            QString msg = QObject::tr("Action required:\n");
#if defined(Q_OS_LINUX)
            msg += QObject::tr("  Eject the iPod manually.");
#else
            msg += QObject::tr("  Please make sure no programs are accessing\n"
                               "  files on the device. If ejecting still fails\n"
                               "  please use your computers eject funtionality.");
#endif
            emit logItem(msg, LOGWARNING);
            actionRequired = true;
        }

        QTimer::singleShot(SLEEP_MS, this, SLOT(installStageWaitForEject()));
        return;
    }

    emit logItem(tr("Device sucessfully ejected."), LOGINFO);

    LOG_INFO() << "preparing installStageWaitForProcs";
    setProgress(40, 18*TICKS_PER_HZ);
    installStageWaitForProcs();
}

void BootloaderInstall6g::installStageWaitForProcs(void)
{
#if !defined(Q_OS_LINUX)
    if (!updateProgress())
        return; /* aborted */

    if (Utils::findProcessId("iTunes")) {
        if (!actionRequired) {
            emit logItem(tr("Action required:\n"
                            "  Close iTunes application."), LOGWARNING);
            actionRequired = true;
        }
        elapsedTicks++;
        QTimer::singleShot(SLEEP_MS, this, SLOT(installStageWaitForProcs()));
        return;
    }

    if (actionRequired) {
        emit logItem(tr("iTunes successfully closed."), LOGINFO);
        if (!updateProgress())
            return; /* aborted */
    }

    iTunesHelperPid = Utils::findProcessId("iTunesHelper");
    if (iTunesHelperPid) {
        if (!Utils::suspendResumeProcess(iTunesHelperPid, true)) {
                emit logItem(tr("Could not suspend iTunesHelper. Stop it\n"
                                "using the Task Manager, and try again."),
                                LOGERROR);
            iTunesHelperPid = 0;
            emit done(true);
            return;
        }
    }
#endif

    LOG_INFO() << "preparing installStageWaitForSpindown";
    // for Windows: skip waiting if the HDD was ejected a time ago
    if (elapsedTicks < progressTimeoutTicks)
        emit logItem(tr("Waiting for HDD spin-down..."), LOGINFO);
    installStageWaitForSpindown();
}

void BootloaderInstall6g::installStageWaitForSpindown(void)
{
    if (!updateProgress())
        return; /* aborted */

    if (elapsedTicks++ < progressTimeoutTicks) {
        QTimer::singleShot(SLEEP_MS, this, SLOT(installStageWaitForSpindown()));
        return;
    }

    LOG_INFO() << "preparing installStageWaitForDfu";
    emit logItem(tr("Waiting for DFU mode..."), LOGINFO);
    emit logItem(tr("Action required:\n"
                    "  Press and hold SELECT+MENU buttons, after\n"
                    "  about 12 seconds a new action will require\n"
                    "  you to release the buttons, DO IT QUICKLY,\n"
                    "  otherwise the process could fail."), LOGWARNING);
    scanPeriodTicks = 2*TICKS_PER_HZ;
    installStageWaitForDfu();
}

void BootloaderInstall6g::installStageWaitForDfu(void)
{
    if (!updateProgress())
        return; /* aborted */

    if ((elapsedTicks++ % scanPeriodTicks) ||
                    !System::listUsbIds().contains(0x05ac1223)) {
        QTimer::singleShot(SLEEP_MS, this, SLOT(installStageWaitForDfu()));
        return;
    }

    emit logItem(tr("DFU mode detected."), LOGINFO);
    emit logItem(tr("Action required:\n"
                    "  Release SELECT+MENU buttons and wait."), LOGWARNING);

    LOG_INFO() << "preparing installStageSendDfu";
    setProgress(60, 10*TICKS_PER_HZ);
    installStageSendDfu();
}

void BootloaderInstall6g::installStageSendDfu(void)
{
    if (!updateProgress())
        return; /* aborted */

    if (elapsedTicks++ < progressTimeoutTicks) {
        QTimer::singleShot(SLEEP_MS, this, SLOT(installStageSendDfu()));
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
        if (strstr(errstr, "LIBUSB_ERROR_NOT_SUPPORTED")) {
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
    setProgress(99, 45*TICKS_PER_HZ);
    scanPeriodTicks = 5*TICKS_PER_HZ;
    installStageWaitForRemount();
}

void BootloaderInstall6g::installStageWaitForRemount(void)
{
    if (!updateProgress())
        return; /* aborted */

    if ((elapsedTicks++ % scanPeriodTicks) ||
            !Utils::mountpoints(Utils::MountpointsSupported).contains(mntpoint))
    {
        if (!actionRequired && (elapsedTicks > progressTimeoutTicks)) {
            emit logItem(tr("Action required:\n"
                            "  Could not remount the device, try to do it\n"
                            "  manually. If the iPod didn't restart, force\n"
                            "  a reset by pressing SELECT+MENU buttons\n"
                            "  for about 5 seconds. If the problem could\n"
                            "  not be solved then click 'Abort' to cancel."),
                            LOGWARNING);
            actionRequired = true;
        }
        QTimer::singleShot(SLEEP_MS, this, SLOT(installStageWaitForRemount()));
        return;
    }

    emit logItem(tr("Device is remounted."), LOGOK);

// TODO: work in progress
#if 0
    if (installed() != BootloaderRockbox) {
        LOG_ERROR() << "install/uninstall failed";
        emit logItem(tr("Bootloader %1 failed").
                arg(tr(doInstall ? "install":"uninstall")), LOGERROR);
        emit done(true);
        return;
    }
    emit logItem(tr("Bootloader sucessfully %1").
                arg(tr(doInstall ? "installed":"uninstalled", LOGOK);
#endif

    setProgress(100);
    updateProgress();

    logInstall(LogAdd);
    emit done(false);
}

void BootloaderInstall6g::installDone(bool status)
{
    LOG_INFO() << "installDone, status:" << status;

#if !defined(Q_OS_LINUX)
    if (iTunesHelperPid) {
        if (!Utils::suspendResumeProcess(iTunesHelperPid, false))
            emit logItem(tr("Could not resume iTunesHelper."), LOGWARNING);
        iTunesHelperPid = 0;
    }
#endif
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

void BootloaderInstall6g::setProgress(int progress, int progressTimeout)
{
    elapsedTicks = 0;
    progressTimeoutTicks = progressTimeout;
    originProgress = targetProgress;
    targetProgress = progress;
    actionRequired = false;
}

bool BootloaderInstall6g::updateProgress(void)
{
    if (progressTimeoutTicks)
        currentProgress = qMin(targetProgress, originProgress + elapsedTicks *
                    (targetProgress - originProgress) / progressTimeoutTicks);
    else
        currentProgress = targetProgress;
    emit logProgress(currentProgress, 100);
    QCoreApplication::processEvents();
    return !abortDetected();
}

BootloaderInstallBase::BootloaderType BootloaderInstall6g::installed(void)
{
    return BootloaderRockbox;

// TODO: work in progress
#if 0
#define SECT_SZ 4096
    char buf[SECT_SZ] = {0};
    QString device;
    QString disk;

    device = Utils::resolveDevicename(m_blfile);
    if (device.isEmpty()) {
        LOG_INFO() << "installed: BootloaderUnknown";
        return BootloaderUnknown;
    }

#if defined(Q_OS_WIN32)
    disk = "\\\\.\\PhysicalDrive" + device;
#elif defined(Q_OS_LINUX)
    // XXX: access to /dev/sdX usually requires privileges on Linux
    disk = device.remove(QRegExp("[0-9]+$"));
#elif defined(Q_OS_MACX)
    disk = device.remove(QRegExp("s[0-9]+$"));
#else
    #error No disk paths defined for this platform
#endif

    QFile f(disk);
    f.open(QIODevice::ReadOnly);
    f.seek(SECT_SZ*4);
    f.read(buf, SECT_SZ);
    f.close();

    if (!memcmp(buf, "RBBL", 4)) {
        LOG_INFO() << "installed: BootloaderRockbox";
        return BootloaderRockbox;
    }
    else {
        LOG_INFO() << "installed: BootloaderOther";
        return BootloaderOther;
    }
#endif
}

BootloaderInstallBase::Capabilities BootloaderInstall6g::capabilities(void)
{
    return (Install | Uninstall);
}
