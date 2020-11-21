/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2011 by Jean-Louis Biasini
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <QtCore>
#include <QtDebug>
#include "bootloaderinstallbase.h"
#include "bootloaderinstallimx.h"
#include "../mkimxboot/mkimxboot.h"
#include "Logger.h"

// class for running mkimxboot() in a separate thread to keep the UI responsive.
class BootloaderThreadImx : public QThread
{
    public:
        void run(void);
        void setInputFile(QString f)
        { m_inputfile = f; }
        void setOutputFile(QString f)
        { m_outputfile = f; }
        void setBootloaderFile(QString f)
        { m_bootfile = f; }
        enum imx_error_t error(void)
        { return m_error; }
    private:
        QString m_inputfile;
        QString m_bootfile;
        QString m_outputfile;
        enum imx_error_t m_error;
};


void BootloaderThreadImx::run(void)
{
    LOG_INFO() << "Thread started.";
    struct imx_option_t opt;
    memset(&opt, 0, sizeof(opt));
    opt.debug = false;
    opt.output = IMX_DUALBOOT;
    opt.fw_variant = VARIANT_DEFAULT;

    m_error = mkimxboot(m_inputfile.toLocal8Bit().constData(),
            m_bootfile.toLocal8Bit().constData(),
            m_outputfile.toLocal8Bit().constData(), opt);
    LOG_INFO() << "Thread finished, result:" << m_error;
}


BootloaderInstallImx::BootloaderInstallImx(QObject *parent)
        : BootloaderInstallBase(parent)
{
    m_thread = nullptr;
}


QString BootloaderInstallImx::ofHint()
{
    return tr("Bootloader installation requires you to provide "
              "a copy of the original Sandisk firmware (firmware.sb file). "
              "This file will be patched with the Rockbox bootloader and "
              "installed to your player. You need to download this file "
              "yourself due to legal reasons. Please browse the "
              "<a href='http://forums.sandisk.com/sansa/'>Sansa Forums</a> "
              "or refer to the "
              "<a href= 'http://www.rockbox.org/wiki/SansaFuzePlus'>SansaFuzePlus</a> "
              "wiki page on how to obtain this file.<br/>"
              "Press Ok to continue and browse your computer for the firmware "
              "file.");
}


/** Start bootloader installation.
 */
bool BootloaderInstallImx::install(void)
{
    if(!QFileInfo(m_offile).isReadable())
    {
        LOG_ERROR() << "could not read original firmware file"
                 << m_offile;
        emit logItem(tr("Could not read original firmware file"), LOGERROR);
        return false;
    }

    LOG_INFO() << "downloading bootloader";
    // download bootloader from server
    emit logItem(tr("Downloading bootloader file"), LOGINFO);
    connect(this, &BootloaderInstallBase::downloadDone, this, &BootloaderInstallImx::installStage2);
    downloadBlStart(m_blurl);
    return true;
}


void BootloaderInstallImx::installStage2(void)
{
    LOG_INFO() << "patching file...";
    emit logItem(tr("Patching file..."), LOGINFO);
    m_tempfile.open();

    // we have not detailed progress on the patching so just show a busy
    // indicator instead.
    emit logProgress(0, 0);
    m_patchedFile.open();
    m_thread = new BootloaderThreadImx();
    m_thread->setInputFile(m_offile);
    m_thread->setBootloaderFile(m_tempfile.fileName());
    m_thread->setOutputFile(m_patchedFile.fileName());
    m_tempfile.close();
    m_patchedFile.close();
    connect(m_thread, &QThread::finished, this, &BootloaderInstallImx::installStage3);
    connect(m_thread, SIGNAL(terminated()), this, SLOT(installStage3()));
    m_thread->start();
}


void BootloaderInstallImx::installStage3(void)
{
    enum imx_error_t err = m_thread->error();
    emit logProgress(1, 1);
    // if the patch failed 
    if (err != IMX_SUCCESS)
    {
        LOG_ERROR() << "Could not patch the original firmware file";
        emit logItem(tr("Patching the original firmware failed"), LOGERROR);
        emit done(true);
        return;
    }

    LOG_INFO() << "Original Firmware succesfully patched";
    emit logItem(tr("Succesfully patched firmware file"), LOGINFO);

    // if a bootloader is already present delete it.
    QString fwfile(m_blfile);
    if(QFileInfo(fwfile).isFile())
    {
        LOG_INFO() << "deleting old target file";
        QFile::remove(fwfile);
    }

    // place (new) bootloader. Copy, since the temporary file will be removed
    // automatically.
    LOG_INFO() << "moving patched bootloader to" << fwfile;
    if(m_patchedFile.copy(fwfile))
    {
        emit logItem(tr("Bootloader successful installed"), LOGOK);
        logInstall(LogAdd);
        emit done(false);
    }
    else
    {
        emit logItem(tr("Patched bootloader could not be installed"), LOGERROR);
        emit done(true);
    }
    // clean up thread object.
    delete m_thread;
    return;
}


bool BootloaderInstallImx::uninstall(void)
{
    emit logItem(tr("To uninstall, perform a normal upgrade with an unmodified "
                    "original firmware."), LOGINFO);
    logInstall(LogRemove);
    emit done(true);
    return false;
}


BootloaderInstallBase::BootloaderType BootloaderInstallImx::installed(void)
{
    return BootloaderUnknown;
}


BootloaderInstallBase::Capabilities BootloaderInstallImx::capabilities(void)
{
    return (Install | NeedsOf);
}

