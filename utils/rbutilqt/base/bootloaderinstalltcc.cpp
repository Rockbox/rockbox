/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2009 by Tomer Shalev
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * This file is a modified version of the AMS installer by Dominik Wenger
 *
 ****************************************************************************/

#include <QtCore>
#include "bootloaderinstallbase.h"
#include "bootloaderinstalltcc.h"
#include "../mktccboot/mktccboot.h"

BootloaderInstallTcc::BootloaderInstallTcc(QObject *parent)
        : BootloaderInstallBase(parent)
{
}

QString BootloaderInstallTcc::ofHint()
{
    return tr("Bootloader installation requires you to provide "
              "a firmware file of the original firmware (bin file). "
              "You need to download this file yourself due to legal "
              "reasons. Please refer to the "
              "<a href='http://www.rockbox.org/manual.shtml'>manual</a> and the "
              "<a href='http://www.rockbox.org/wiki/CowonD2Info'>CowonD2Info</a> "
              "wiki page on how to obtain the file.<br/>"
              "Press Ok to continue and browse your computer for the firmware "
              "file.");
}

bool BootloaderInstallTcc::install(void)
{
    if(m_offile.isEmpty())
        return false;

    // Download firmware from server
    emit logItem(tr("Downloading bootloader file"), LOGINFO);

    connect(this, &BootloaderInstallBase::downloadDone, this, &BootloaderInstallTcc::installStage2);
    downloadBlStart(m_blurl);

    return true;
}

void BootloaderInstallTcc::installStage2(void)
{
    unsigned char *of_buf, *boot_buf = nullptr, *patched_buf = nullptr;
    int n, of_size, boot_size, patched_size;
    char errstr[200];
    bool ret = false;

    m_tempfile.open();
    QString bootfile = m_tempfile.fileName();
    m_tempfile.close();

    /* Construct path for write out.
     * Combine path of m_blfile with filename from OF */
    QString outfilename = QFileInfo(m_blfile).absolutePath() + "/" +
        QFileInfo(m_offile).fileName();

    /* Write out file */
    QFile out(outfilename);

    /* Load original firmware file */
    of_buf = file_read(m_offile.toLocal8Bit().data(), &of_size);
    if (of_buf == nullptr)
    {
        emit logItem(errstr, LOGERROR);
        emit logItem(tr("Could not load %1").arg(m_offile), LOGERROR);
        goto exit;
    }

    /* A CRC test in order to reject non OF file */
    if (test_firmware_tcc(of_buf, of_size))
    {
        emit logItem(errstr, LOGERROR);
        emit logItem(tr("Unknown OF file used: %1").arg(m_offile), LOGERROR);
        goto exit;
    }

    /* Load bootloader file */
    boot_buf = file_read(bootfile.toLocal8Bit().data(), &boot_size);
    if (boot_buf == nullptr)
    {
        emit logItem(errstr, LOGERROR);
        emit logItem(tr("Could not load %1").arg(bootfile), LOGERROR);
        goto exit;
    }

    /* Patch the firmware */
    emit logItem(tr("Patching Firmware..."), LOGINFO);

    patched_buf = patch_firmware_tcc(of_buf, of_size, boot_buf, boot_size,
            &patched_size);
    if (patched_buf == nullptr)
    {
        emit logItem(errstr, LOGERROR);
        emit logItem(tr("Could not patch firmware"), LOGERROR);
        goto exit;
    }

    if(!out.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        emit logItem(tr("Could not open %1 for writing").arg(m_blfile),
                LOGERROR);
        goto exit;
    }

    n = out.write((char*)patched_buf, patched_size);
    out.close();
    if (n != patched_size)
    {
        emit logItem(tr("Could not write firmware file"), LOGERROR);
        goto exit;
    }

    /* End of install */
    emit logItem(tr("Success: modified firmware file created"), LOGINFO);
    logInstall(LogAdd);

    ret = true;

exit:
    if (of_buf)
        free(of_buf);

    if (boot_buf)
        free(boot_buf);

    if (patched_buf)
        free(patched_buf);

    emit done(ret);
}

bool BootloaderInstallTcc::uninstall(void)
{
    emit logItem(tr("To uninstall, perform a normal upgrade with an unmodified original firmware"), LOGINFO);
    logInstall(LogRemove);
    emit done(true);
    return false;
}

BootloaderInstallBase::BootloaderType BootloaderInstallTcc::installed(void)
{
    return BootloaderUnknown;
}

BootloaderInstallBase::Capabilities BootloaderInstallTcc::capabilities(void)
{
    return (Install | NeedsOf);
}
