/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2008 by Dominik Wenger
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
#include "bootloaderinstallams.h"
#include "Logger.h"

#include "../mkamsboot/mkamsboot.h"

BootloaderInstallAms::BootloaderInstallAms(QObject *parent)
        : BootloaderInstallBase(parent)
{
}

QString BootloaderInstallAms::ofHint()
{
    return tr("Bootloader installation requires you to provide "
              "a copy of the original Sandisk firmware (bin file). "
              "This firmware file will be patched and then installed to your "
              "player along with the rockbox bootloader. "
              "You need to download this file yourself due to legal "
              "reasons. Please browse the "
              "<a href='http://forums.sandisk.com/sansa/'>Sansa Forums</a> "
              "or refer to the "
              "<a href='http://www.rockbox.org/manual.shtml'>manual</a> and "
              "the <a href='http://www.rockbox.org/wiki/SansaAMS'>SansaAMS</a> "
              "wiki page on how to obtain this file.<br/>"
              "<b>Note:</b> This file is not present on your player and will "
              "disappear automatically after installing it.<br/><br/>"
              "Press Ok to continue and browse your computer for the firmware "
              "file.");
}

bool BootloaderInstallAms::install(void)
{
    if(m_offile.isEmpty())
        return false;

    LOG_INFO() << "installing bootloader";

    // download firmware from server
    emit logItem(tr("Downloading bootloader file"), LOGINFO);

    connect(this, &BootloaderInstallBase::downloadDone, this, &BootloaderInstallAms::installStage2);
    downloadBlStart(m_blurl);

    return true;
}

void BootloaderInstallAms::installStage2(void)
{
    LOG_INFO() << "installStage2";

    unsigned char* buf;
    unsigned char* of_packed;
    int of_packedsize;
    unsigned char* rb_packed;
    int rb_packedsize;
    off_t len;
    struct md5sums sum;
    char md5sum[33]; /* 32 hex digits, plus terminating zero */
    int n;
    int model;
    int firmware_size;
    int bootloader_size;
    int patchable;
    int totalsize;
    char errstr[200];

    sum.md5 = md5sum;

    m_tempfile.open();
    QString bootfile = m_tempfile.fileName();
    m_tempfile.close();

    /* Load bootloader file */
    rb_packed = load_rockbox_file(bootfile.toLocal8Bit().data(), &model,
                                  &bootloader_size,&rb_packedsize,
                                    errstr,sizeof(errstr));
    if (rb_packed == nullptr)
    {
        LOG_ERROR() << "could not load bootloader: " << bootfile;
        emit logItem(errstr, LOGERROR);
        emit logItem(tr("Could not load %1").arg(bootfile), LOGERROR);
        emit done(true);
        return;
    }

    /* Load original firmware file */
    buf = load_of_file(m_offile.toLocal8Bit().data(), model, &len, &sum,
                        &firmware_size, &of_packed ,&of_packedsize,
                        errstr, sizeof(errstr));
    if (buf == nullptr)
    {
        LOG_ERROR() << "could not load OF: " << m_offile;
        emit logItem(errstr, LOGERROR);
        emit logItem(tr("Could not load %1").arg(m_offile), LOGERROR);
        free(rb_packed);
        emit done(true);
        return;
    }

    /* check total size */
    patchable = check_sizes(sum.model, rb_packedsize, bootloader_size,
            of_packedsize, firmware_size, &totalsize, errstr, sizeof(errstr));

    if (!patchable)
    {
        LOG_ERROR() << "No room to insert bootloader";
        emit logItem(errstr, LOGERROR);
        emit logItem(tr("No room to insert bootloader, try another firmware version"),
                     LOGERROR);
        free(buf);
        free(of_packed);
        free(rb_packed);
        emit done(true);
        return;
    }

    /* patch the firmware */
    emit logItem(tr("Patching Firmware..."), LOGINFO);

    patch_firmware(sum.model,firmware_revision(sum.model),firmware_size,buf,
                   len,of_packed,of_packedsize,rb_packed,rb_packedsize);

    /* write out file */
    QFile out(m_blfile);

    if(!out.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        LOG_ERROR() << "Could not open" <<  m_blfile << "for writing";
        emit logItem(tr("Could not open %1 for writing").arg(m_blfile),LOGERROR);
        free(buf);
        free(of_packed);
        free(rb_packed);
        emit done(true);
        return;
    }

    n = out.write((char*)buf, len);

    if (n != len)
    {
        LOG_ERROR() << "Could not write firmware file";
        emit logItem(tr("Could not write firmware file"),LOGERROR);
        free(buf);
        free(of_packed);
        free(rb_packed);
        emit done(true);
        return;
    }

    out.close();

    free(buf);
    free(of_packed);
    free(rb_packed);

    //end of install
    LOG_INFO() << "install successfull";
    emit logItem(tr("Success: modified firmware file created"), LOGINFO);
    logInstall(LogAdd);
    emit done(false);
    return;
}

bool BootloaderInstallAms::uninstall(void)
{
    emit logItem(tr("To uninstall, perform a normal upgrade with an unmodified "
                    "original firmware"), LOGINFO);
    logInstall(LogRemove);
    emit done(true);
    return false;
}

BootloaderInstallBase::BootloaderType BootloaderInstallAms::installed(void)
{
    return BootloaderUnknown;
}

BootloaderInstallBase::Capabilities BootloaderInstallAms::capabilities(void)
{
    return (Install | NeedsOf);
}

