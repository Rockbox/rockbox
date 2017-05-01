/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2017 by Lorenzo Miori
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
#include "bootloaderinstallypr.h"
#include "utils.h"
#include "Logger.h"
#include <QByteArray>
#include <QtEndian>

/* target specific includes */

#include "../../utils/ypr0tools/bsdiff/bspatch.h"
#include "../../utils/ypr0tools/common.h"
#include "../../utils/ypr0tools/samsung_ypr0.h"

BootloaderInstallYpr::BootloaderInstallYpr(QObject *parent)
        : BootloaderInstallBase(parent)
{
}

QString BootloaderInstallYpr::ofHint()
{
    return tr("Bootloader installation requires you to provide "
               "a firmware file of the original firmware (hex file). "
               "You need to download this file yourself due to legal "
               "reasons. Please refer to the "
               "<a href='http://www.rockbox.org/manual.shtml'>manual</a> and the "
               "<a href='http://www.rockbox.org/wiki/SamsungYPR0'>SamsungYPR0</a> wiki page on "
               "how to obtain this file.<br/>"
               "Press Ok to continue and browse your computer for the firmware "
               "file.");
}

#define YPR_PROGRESS_MAX    (5)
bool BootloaderInstallYpr::install(void)
{
    FILE * cramfs_output = NULL;
    FILE *cramfs_input = NULL;
    uint32_t newsize, oldsize;
    struct bspatch_stream stream;

    uint8_t *cramfs_in_data = NULL;
    uint8_t *cramfs_new_data = NULL;

    int progress = 0;

    if(m_offile.isEmpty())
        return false;

    QCoreApplication::processEvents();

    emit logProgress(progress++, YPR_PROGRESS_MAX);

    /* get a temporary directory to perform the firmware patching */
    QTemporaryDir tempdir;

    if (tempdir.isValid())
    {
        emit logItem("Tempdir: " + tempdir.path(), LOGINFO);

        /* decrypt the firmware file first */
        emit logItem(tr("Start decrypting the firmware file"), LOGINFO);
        int decrypt_res = ypr_decrypt(m_offile.toLatin1().data(), tempdir.path().toLatin1().data());
        if (decrypt_res != 0)
        {
            emit logItem("Firmware unpacking failed: " + decrypt_res, LOGERROR);
        }

        emit logProgress(progress++, YPR_PROGRESS_MAX);

        /* prepare paths */
        QString cramfs_path = QDir(tempdir.path().toLatin1().data()).filePath("cramfs-fsl.rom");

        /* uncompress the patch stream */

        /* qt requires a 4 bytes big-endian header indicating the uncompressed size,
         * this is already added in the patch data stream
         */
        QByteArray patch = qUncompress(samsung_ypr0, LEN_samsung_ypr0);

        /* patch the firmware */
        if (memcmp(patch.data(), "ENDSLEY/BSDIFF43", 16) != 0)
        {
            emit logItem("The patch data stream is invalid", LOGINFO);
            emit done(false);
            return false;
        }
        else
        {
            emit logItem("Patch has been uncompressed, now applying", LOGINFO);
            emit logProgress(progress++, YPR_PROGRESS_MAX);

            /* Read the original cramfs-fsl.rom image */
            cramfs_input = fopen(cramfs_path.toStdString().c_str(), "rb");

            if (cramfs_input != NULL)
            {
                /* read the entire file in memory, usually ~15MB */
                fseek(cramfs_input, 0, SEEK_END);
                oldsize = ftell(cramfs_input);
                fseek(cramfs_input, 0, SEEK_SET);
                cramfs_in_data = (uint8_t*)malloc(oldsize + 1);
                if (cramfs_in_data != NULL)
                {
                    /* allocation succeded, go on */
                    size_t fread_check = fread(cramfs_in_data, sizeof(uint8_t), oldsize, cramfs_input);
                    if (fread_check != oldsize)
                    {
                        /* error reading the file */
                        emit logItem("Error reading original cramfs!", LOGERROR);
                    }
                    /* close the file */
                    fclose(cramfs_input);
                    emit logItem("Original firmware read OK", LOGINFO);
                    emit logProgress(progress++, YPR_PROGRESS_MAX);
                }
                else
                {
                    /* allocation failed */
                    emit logItem("Memory allocation failed!", LOGERROR);
                }
            }

            if (cramfs_in_data != NULL)
            {
                /* original cramfs data is valid */

                /* Read lengths from the bsdiff header */
                newsize = offtin((uint8_t*)(patch.data() + 16));

                /* Allocate the space for the patched data */
                cramfs_new_data = (uint8_t*)malloc(newsize + 1);

                if (cramfs_new_data != NULL)
                {
                    /* Allocation succeded, go on */

                    /* simulate a file descriptor, but reading from a buffer */
                    stream.read = bspatch_buffer_read;
                    /* the actual patch data is located after the bsdiff header of 24 bytes,
                     * therefore add 24 bytes to the buffer pointer.
                     */
                    bsdiff_stream_data_t bsdiff_xfer = {
                        (uint8_t*)patch.data() + 24U,
                        (uint32_t)patch.size(),
                        0U };
                    stream.opaque = &bsdiff_xfer;

                    if (bspatch((uint8_t*)cramfs_in_data, oldsize, cramfs_new_data, newsize, &stream))
                    {
                        /* Patching has failed */
                        emit logItem("Patch operation has failed!", LOGERROR);
                    }
                    else
                    {
                         emit logProgress(progress++, YPR_PROGRESS_MAX);

                        /* Patching succeded: overwrite cramfs file */
                        emit logItem("Saving patched cramfs to disk", LOGINFO);
                        cramfs_output = fopen(cramfs_path.toStdString().c_str(), "wb");

                        if (cramfs_output != NULL)
                        {
                            /* file opened ok, write the buffer */
                            (void)fwrite(cramfs_new_data, sizeof(uint8_t), newsize, cramfs_output);
                            /* close the file (also if error, we do not care) */
                            fclose(cramfs_output);

                            emit logProgress(progress++, YPR_PROGRESS_MAX);

                            /* encrypt the firmware file again */
                            QTemporaryFile targethex;
                            targethex.open();
                            QString targethexName = targethex.fileName();

                            int encrypt_res = ypr_encrypt((char*)targethexName.toLatin1().data(), tempdir.path().toLatin1().data());

                            if (encrypt_res != 0)
                            {
                                emit logItem("Firmware packing failed: " + encrypt_res, LOGERROR);
                            }

                            emit logProgress(progress++, YPR_PROGRESS_MAX);

                            /* Check firmware integrity */
                            QByteArray filedata;
                            filedata = targethex.readAll();
                            targethex.close();
                            QString hash = QCryptographicHash::hash(filedata,
                                    QCryptographicHash::Md5).toHex();
                            LOG_INFO() << "created hexfile hash:" << hash;

                            emit logItem(tr("Checking modified firmware file"), LOGINFO);
                            if(hash != QString(SAMSUNG_YPR0_INFO.md5sum)) {
                                emit logItem(tr("Error: modified file checksum wrong"), LOGERROR);
                                targethex.remove();
                                emit done(true);
                            }
                            else
                            {
                                /* copy the file to the player, if not already present */
                                if(!Utils::resolvePathCase(m_blfile).isEmpty()) {
                                    emit logItem(tr("A firmware file is already present on player"), LOGERROR);
                                    emit done(true);
                                }
                                else
                                {
                                    /* check copy progress */
                                    if(targethex.copy(m_blfile)) {
                                        emit logItem(tr("Success: modified firmware file created"), LOGINFO);
                                    }
                                    else {
                                        emit logItem(tr("Copying modified firmware file failed"), LOGERROR);
                                        emit done(true);
                                    }
                                }

                            }

                            logInstall(LogAdd);
                        }
                    }
                }
                else
                {
                    /* allocation failed */
                    emit logItem("Memory allocation failed!", LOGERROR);
                }
            }
        }

        /* Cleanup */

        if (cramfs_in_data != NULL)
            free(cramfs_in_data);

        if (cramfs_new_data != NULL)
            free(cramfs_new_data);

        emit done(true);
        return true;
    }
    else
    {
        /* unable to create a temporary directory */
        emit logItem(tr("Unable to get a temporary directory"), LOGERROR);
        emit done(false);
        return false;
    }

}


void BootloaderInstallYpr::installStage2(void)
{
//    emit logItem(tr("Adding bootloader to firmware file"), LOGINFO);
//    QCoreApplication::processEvents();

//    // local temp file
//    QTemporaryFile tempbin;

//    // finally copy file to player
//    if(!Utils::resolvePathCase(m_blfile).isEmpty()) {
//        emit logItem(tr("A firmware file is already present on player"), LOGERROR);
//        emit done(true);
//        return;
//    }
//    if(targethex.copy(m_blfile)) {
//        emit logItem(tr("Success: modified firmware file created"), LOGINFO);
//    }
//    else {
//        emit logItem(tr("Copying modified firmware file failed"), LOGERROR);
//        emit done(true);
//        return;
//    }

//    logInstall(LogAdd);
    emit done(false);

    return;
}


bool BootloaderInstallYpr::uninstall(void)
{
    emit logItem(tr("Uninstallation not possible, only installation info removed"), LOGINFO);
    logInstall(LogRemove);
    emit done(true);
    return false;
}


BootloaderInstallBase::BootloaderType BootloaderInstallYpr::installed(void)
{
    return BootloaderUnknown;
}


BootloaderInstallBase::Capabilities BootloaderInstallYpr::capabilities(void)
{
    return (Install | NeedsOf);
}

QString BootloaderInstallYpr::scrambleError(int err)
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

