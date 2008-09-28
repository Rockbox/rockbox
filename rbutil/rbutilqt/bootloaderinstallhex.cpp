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

#include <QtCore>
#include "bootloaderinstallbase.h"
#include "bootloaderinstallhex.h"

#include "../../tools/iriver.h"
#include "../../tools/mkboot.h"

struct md5s {
    const char* orig;
    const char* patched;
};

struct md5s md5sums[] = {
#include "irivertools/h100sums.h"
    { 0, 0 },
#include "irivertools/h120sums.h"
    { 0, 0 },
#include "irivertools/h300sums.h"
    { 0, 0 }
};


BootloaderInstallHex::BootloaderInstallHex(QObject *parent)
        : BootloaderInstallBase(parent)
{
}


bool BootloaderInstallHex::install(void)
{
    if(m_hex.isEmpty())
        return false;
    m_hashindex = -1;

    // md5sum hex file
    emit logItem(tr("checking MD5 hash of input file ..."), LOGINFO);
    QByteArray filedata;
    // read hex file into QByteArray
    QFile file(m_hex);
    file.open(QIODevice::ReadOnly);
    filedata = file.readAll();
    file.close();
    QString hash = QCryptographicHash::hash(filedata,
            QCryptographicHash::Md5).toHex();
    qDebug() << "hexfile hash:" << hash;
    if(file.error() != QFile::NoError) {
        emit logItem(tr("Could not verify original firmware file"), LOGERROR);
        emit done(true);
        return false;
    }
    // check hash and figure model from md5sum
    int i = sizeof(md5sums) / sizeof(struct md5s);
    m_model = 4;
    // 3: h300, 2: h120, 1: h100, 0:invalid
    while(i--) {
        if(md5sums[i].orig == 0)
            m_model--;
        if(!qstrcmp(md5sums[i].orig, hash.toAscii()))
            break;
    }
    if(i < 0) {
        emit logItem(tr("Firmware file not recognized."), LOGERROR);
        return false;
    }
    else {
        emit logItem(tr("MD5 hash ok"), LOGOK);
        m_hashindex = i;
    }

    // check model agains download link.
    QString match[] = {"", "h100", "h120", "h300"};
    if(!m_blurl.path().contains(match[m_model])) {
        emit logItem(tr("Firmware file doesn't match selected player."),
        LOGERROR);
        return false;
    }

    emit logItem(tr("Descrambling file"), LOGINFO);
    m_descrambled.open();
    int result;
    result = iriver_decode(m_hex.toAscii().data(),
        m_descrambled.fileName().toAscii().data(), FALSE, STRIP_NONE);
    qDebug() << "iriver_decode" << result;

    if(result < 0) {
        emit logItem(tr("Error in descramble: %1").arg(scrambleError(result)), LOGERROR);
        return false;
    }

    // download firmware from server
    emit logItem(tr("Downloading bootloader file"), LOGINFO);
    connect(this, SIGNAL(downloadDone()), this, SLOT(installStage2()));

    downloadBlStart(m_blurl);
    return true;
}


void BootloaderInstallHex::installStage2(void)
{
    emit logItem(tr("Adding bootloader to firmware file"), LOGINFO);

    // local temp file
    QTemporaryFile tempbin;
    tempbin.open();
    QString tempbinName = tempbin.fileName();
    tempbin.close();
    // get temporary files filenames -- external tools need this.
    m_descrambled.open();
    QString descrambledName = m_descrambled.fileName();
    m_descrambled.close();
    m_tempfile.open();
    QString tempfileName = m_tempfile.fileName();
    m_tempfile.close();

    int origin = 0;
    switch(m_model) {
        case 3:
            origin = 0x3f0000;
            break;
        case 2:
        case 1:
            origin = 0x1f0000;
            break;
        default:
            origin = 0;
            break;
    }

    // iriver decode already done in stage 1
    int result;
    if((result = mkboot(descrambledName.toLocal8Bit().constData(),
                    tempfileName.toLocal8Bit().constData(),
                    tempbinName.toLocal8Bit().constData(), origin)) < 0)
    {
        QString error;
        switch(result) {
            case -1: error = tr("could not open input file"); break;
            case -2: error = tr("reading header failed"); break;
            case -3: error = tr("reading firmware failed"); break;
            case -4: error = tr("can't open bootloader file"); break;
            case -5: error = tr("reading bootloader file failed"); break;
            case -6: error = tr("can't open output file"); break;
            case -7: error = tr("writing output file failed"); break;
        }
        emit logItem(tr("Error in patching: %1").arg(error), LOGERROR);

        emit done(true);
        return;
    }
    QTemporaryFile targethex;
    targethex.open();
    QString targethexName = targethex.fileName();
    if((result = iriver_encode(tempbinName.toLocal8Bit().constData(),
                    targethexName.toLocal8Bit().constData(), FALSE)) < 0)
    {
        emit logItem(tr("Error in scramble: %1").arg(scrambleError(result)), LOGERROR);
        targethex.close();

        emit done(true);
        return;
    }

    // finally check the md5sum of the created file
    QByteArray filedata;
    filedata = targethex.readAll();
    targethex.close();
    QString hash = QCryptographicHash::hash(filedata,
            QCryptographicHash::Md5).toHex();
    qDebug() << "created hexfile hash:" << hash;

    emit logItem(tr("Checking modified firmware file"), LOGINFO);
    if(hash != QString(md5sums[m_hashindex].patched)) {
        emit logItem(tr("Error: modified file checksum wrong"), LOGERROR);
        targethex.remove();
        emit done(true);
        return;
    }
    // finally copy file to player
    targethex.copy(m_blfile);

    emit logItem(tr("Success: modified firmware file created"), LOGINFO);
    logInstall(LogAdd);
    emit done(false);

    return;
}


bool BootloaderInstallHex::uninstall(void)
{
    emit logItem("Uninstallation not possible, only installation info removed", LOGINFO);
    logInstall(LogRemove);
    return false;
}


BootloaderInstallBase::BootloaderType BootloaderInstallHex::installed(void)
{
    return BootloaderUnknown;
}


BootloaderInstallBase::Capabilities BootloaderInstallHex::capabilities(void)
{
    return (Install | NeedsFlashing);
}

QString BootloaderInstallHex::scrambleError(int err)
{
    QString error;
    switch(err) {
        case -1: error = tr("Can't open input file"); break;
        case -2: error = tr("Can't open output file"); break;
        case -3: error = tr("invalid file: header length wrong"); break;
        case -4: error = tr("invalid file: unrecognized header"); break;
        case -5: error = tr("invalid file: \"length\" field wrong"); break;
        case -6: error = tr("invalid file: \"length2\" field wrong"); break;
        case -7: error = tr("invalid file: internal checksum error"); break;
        case -8: error = tr("invalid file: \"length3\" field wrong"); break;
        default: error = tr("unknown"); break;
    }
    return error;
}

