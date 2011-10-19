/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2009 by Maurus Cuelenaere
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
#include "bootloaderinstallchinachip.h"

#include "../chinachippatcher/chinachip.h"

BootloaderInstallChinaChip::BootloaderInstallChinaChip(QObject *parent)
        : BootloaderInstallBase(parent)
{
    (void)parent;
}

QString BootloaderInstallChinaChip::ofHint()
{
    return tr("Bootloader installation requires you to provide "
               "a firmware file of the original firmware (HXF file). "
               "You need to download this file yourself due to legal "
               "reasons. Please refer to the "
               "<a href='http://www.rockbox.org/manual.shtml'>manual</a> and the "
               "<a href='http://www.rockbox.org/wiki/OndaVX747"
               "#Download_and_extract_a_recent_ve'>OndaVX747</a> wiki page on "
               "how to obtain this file.<br/>"
               "Press Ok to continue and browse your computer for the firmware "
               "file.");
}

void BootloaderInstallChinaChip::logString(char* format, va_list args, int type)
{
    QString translation = QCoreApplication::translate("", format, NULL, QCoreApplication::UnicodeUTF8);

    emit logItem(QString().vsprintf(translation.toLocal8Bit(), args), type);
    QCoreApplication::processEvents();
}

static void info(void* userdata, char* format, ...)
{
    BootloaderInstallChinaChip* pThis = (BootloaderInstallChinaChip*) userdata;
    va_list args;

    va_start(args, format);
    pThis->logString(format, args, LOGINFO);
    va_end(args);
}

static void err(void* userdata, char* format, ...)
{
    BootloaderInstallChinaChip* pThis = (BootloaderInstallChinaChip*) userdata;
    va_list args;

    va_start(args, format);
    pThis->logString(format, args, LOGERROR);
    va_end(args);
}

bool BootloaderInstallChinaChip::install()
{
    if(m_offile.isEmpty())
        return false;

    emit logItem(tr("Downloading bootloader file"), LOGINFO);

    connect(this, SIGNAL(downloadDone()), this, SLOT(installStage2()));
    downloadBlStart(m_blurl);

    return true;
}

void BootloaderInstallChinaChip::installStage2()
{
    m_tempfile.open();
    QString blfile = m_tempfile.fileName();
    m_tempfile.close();

    QString backupfile = QFileInfo(m_blfile).absoluteDir().absoluteFilePath("ccpmp.bin");

    int ret = chinachip_patch(m_offile.toLocal8Bit(), blfile.toLocal8Bit(), m_blfile.toLocal8Bit(),
                              backupfile.toLocal8Bit(), &info, &err, (void*)this);
    qDebug() << "chinachip_patch" << ret;

    emit done(ret);
}

bool BootloaderInstallChinaChip::uninstall()
{
    /* TODO: only way is to restore the OF */
    return false;
}

BootloaderInstallBase::BootloaderType BootloaderInstallChinaChip::installed()
{
    /* TODO: find a way to figure this out */
    return BootloaderUnknown;
}

BootloaderInstallBase::Capabilities BootloaderInstallChinaChip::capabilities()
{
    return (Install | IsFile | NeedsOf);
}
