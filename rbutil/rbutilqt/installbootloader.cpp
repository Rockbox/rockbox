/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id: installbootloader.cpp 13990 2007-07-25 22:26:10Z Dominik Wenger $
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
 
#include "installbootloader.h"

BootloaderInstaller::BootloaderInstaller(QObject* parent): QObject(parent) 
{

}

void BootloaderInstaller::install(Ui::InstallProgressFrm* dp)
{
    m_dp = dp;
    m_install = true;
    m_dp->listProgress->addItem(tr("Starting bootloader installation"));
    
    if(m_bootloadermethod == "gigabeatf")
    {
        // connect internal signal
        connect(this,SIGNAL(prepare()),this,SLOT(gigabeatPrepare())); 
        connect(this,SIGNAL(finish()),this,SLOT(gigabeatFinish())); 
    }
    else if(m_bootloadermethod == "iaudio")
    {
        // connect internal signal
        connect(this,SIGNAL(prepare()),this,SLOT(iaudioPrepare()));  
        connect(this,SIGNAL(finish()),this,SLOT(iaudioFinish()));       
    }
    else if(m_bootloadermethod == "h10")
    {
        // connect internal signal
        connect(this,SIGNAL(prepare()),this,SLOT(h10Prepare()));  
        connect(this,SIGNAL(finish()),this,SLOT(h10Finish()));       
    }
    else if(m_bootloadermethod == "ipodpatcher")
    {
        // connect internal signal
        connect(this,SIGNAL(prepare()),this,SLOT(ipodPrepare()));  
        connect(this,SIGNAL(finish()),this,SLOT(ipodFinish()));       
    }       
    else
    {
        m_dp->listProgress->addItem(tr("unsupported install Method"));
        emit done(true);
        return;
    }
    
    emit prepare();
}

void BootloaderInstaller::uninstall(Ui::InstallProgressFrm* dp)
{
    m_dp = dp;
    m_install = false;
    m_dp->listProgress->addItem(tr("Starting bootloader uninstallation"));
    
    if(m_bootloadermethod == "gigabeatf")
    {
       // connect internal signal
       connect(this,SIGNAL(prepare()),this,SLOT(gigabeatPrepare())); 
    }
    else if(m_bootloadermethod == "iaudio")
    {
        m_dp->listProgress->addItem(tr("No uninstallation possible"));
        emit done(true);
        return;      
    }
    else if(m_bootloadermethod == "iaudio")
    {
        // connect internal signal
       connect(this,SIGNAL(prepare()),this,SLOT(h10Prepare())); 
    }
    else if(m_bootloadermethod == "ipodpatcher")
    {
        // connect internal signal
        connect(this,SIGNAL(prepare()),this,SLOT(ipodPrepare())); 
    }
    else
    {
        m_dp->listProgress->addItem(tr("unsupported install Method"));
        emit done(true);
        return;
    }
    
    emit prepare();
}

void BootloaderInstaller::downloadRequestFinished(int id, bool error)
{
    qDebug() << "BootloaderInstall::downloadRequestFinished" << id << error;
    qDebug() << "error:" << getter->errorString();

    downloadDone(error);
}

void BootloaderInstaller::downloadDone(bool error)
{
    qDebug() << "Install::downloadDone, error:" << error;
    
     // update progress bar
     
    int max = m_dp->progressBar->maximum();
    if(max == 0) {
        max = 100;
        m_dp->progressBar->setMaximum(max);
    }
    m_dp->progressBar->setValue(max);
    if(getter->httpResponse() != 200) {
        m_dp->listProgress->addItem(tr("Download error: received HTTP error %1.").arg(getter->httpResponse()));
        m_dp->buttonAbort->setText(tr("&Ok"));
        emit done(true);
        return;
    }
    if(error) {
        m_dp->listProgress->addItem(tr("Download error: %1").arg(getter->errorString()));
        m_dp->buttonAbort->setText(tr("&Ok"));
        emit done(true);
        return;
    }
    else m_dp->listProgress->addItem(tr("Download finished."));
    
    emit finish();
   
}

void BootloaderInstaller::updateDataReadProgress(int read, int total)
{
    m_dp->progressBar->setMaximum(total);
    m_dp->progressBar->setValue(read);
    qDebug() << "progress:" << read << "/" << total;
}


/**************************************************
*** gigabeat secific code 
***************************************************/

void BootloaderInstaller::gigabeatPrepare()
{
    if(m_install)         // Installation
    {
        QString url = m_bootloaderUrlBase + "/gigabeat/" + m_bootloadername;
      
        m_dp->listProgress->addItem(tr("Downloading file %1.%2")
            .arg(QFileInfo(url).baseName(), QFileInfo(url).completeSuffix()));

        // temporary file needs to be opened to get the filename
        downloadFile.open();
        m_tempfilename = downloadFile.fileName();
        downloadFile.close();
        // get the real file.
        getter = new HttpGet(this);
        getter->setProxy(m_proxy);
        getter->setFile(&downloadFile);
        getter->getFile(QUrl(url));
        // connect signals from HttpGet
        connect(getter, SIGNAL(done(bool)), this, SLOT(downloadDone(bool)));
        //connect(getter, SIGNAL(requestFinished(int, bool)), this, SLOT(downloadRequestFinished(int, bool)));
        connect(getter, SIGNAL(dataReadProgress(int, int)), this, SLOT(updateDataReadProgress(int, int)));
         
   }
   else                 //UnInstallation
   {
        QString firmware = m_mountpoint + "/GBSYSTEM/FWIMG/FWIMG01.DAT";
        QString firmwareOrig = firmware.append(".ORIG");
        
        QFileInfo firmwareOrigFI(firmwareOrig);
        
        // check if original firmware exists
        if(!firmwareOrigFI.exists())
        {
             m_dp->listProgress->addItem(tr("Could not find the Original Firmware at: %1")
                    .arg(firmwareOrig));
            emit done(true);
            return;        
        }
        
        QFile firmwareFile(firmware);
        QFile firmwareOrigFile(firmwareOrig);
        
        //remove modified firmware
        if(!firmwareFile.remove())
        {
             m_dp->listProgress->addItem(tr("Could not remove the Firmware at: %1")
                    .arg(firmware));
            emit done(true);
            return;        
        }
        
        //copy  original firmware
        if(!firmwareOrigFile.copy(firmware))   
        {
             m_dp->listProgress->addItem(tr("Could not copy the Firmware from: %1 to %2")
                    .arg(firmwareOrig,firmware));
            emit done(true);
            return;        
        }
        
        emit done(false);  //success
   }
   
}

void BootloaderInstaller::gigabeatFinish()  
{
    // this step is only need for installation, so no code for uninstall here

    m_dp->listProgress->addItem(tr("Finishing bootloader install"));

    QString firmware = m_mountpoint + "/GBSYSTEM/FWIMG/" + m_bootloadername;
    
    QFileInfo firmwareFI(firmware);
    
    // check if firmware exists
    if(!firmwareFI.exists())
    {
        m_dp->listProgress->addItem(tr("Could not find the Firmware at: %1")
            .arg(firmware));
        emit done(true);
        return;        
    }
    
    QString firmwareOrig = firmware;
    firmwareOrig.append(".ORIG");
    QFileInfo firmwareOrigFI(firmwareOrig);
    
    // rename the firmware, if there is no original firmware there
    if(!firmwareOrigFI.exists())
    {
        QFile firmwareFile(firmware);
        if(!firmwareFile.rename(firmwareOrig))
        {
            m_dp->listProgress->addItem(tr("Could not rename: %1 to %2")
                                .arg(firmware,firmwareOrig));
            emit done(true);
            return;
        }
    }
    else // or remove the normal firmware, if the original is there
    {
        QFile firmwareFile(firmware);
        firmwareFile.remove();
    }
    
    //copy the firmware
    if(!downloadFile.copy(firmware))
    {
        m_dp->listProgress->addItem(tr("Could not copy: %1 to %2")
                                .arg(m_tempfilename,firmware));
        emit done(true);
        return;
    }
    
    downloadFile.remove();

    m_dp->listProgress->addItem(tr("Bootloader install finished successfully."));
    m_dp->buttonAbort->setText(tr("&Ok"));
    
    emit done(false);  // success

}

/**************************************************
*** iaudio secific code 
***************************************************/
void BootloaderInstaller::iaudioPrepare()
{

    QString url = m_bootloaderUrlBase + "/iaudio/" + m_bootloadername;
      
    m_dp->listProgress->addItem(tr("Downloading file %1.%2")
          .arg(QFileInfo(url).baseName(), QFileInfo(url).completeSuffix()));

    // temporary file needs to be opened to get the filename
    downloadFile.open();
    m_tempfilename = downloadFile.fileName();
    downloadFile.close();
    // get the real file.
    getter = new HttpGet(this);
    getter->setProxy(m_proxy);
    getter->setFile(&downloadFile);
    getter->getFile(QUrl(url));
    // connect signals from HttpGet
    connect(getter, SIGNAL(done(bool)), this, SLOT(downloadDone(bool)));
    //connect(getter, SIGNAL(requestFinished(int, bool)), this, SLOT(downloadRequestFinished(int, bool)));
    connect(getter, SIGNAL(dataReadProgress(int, int)), this, SLOT(updateDataReadProgress(int, int)));   

}  

void BootloaderInstaller::iaudioFinish()
{
    QString firmware = m_mountpoint + "/FIRMWARE/" + m_bootloadername;
    
    //copy the firmware
    if(!downloadFile.copy(firmware))
    {
        m_dp->listProgress->addItem(tr("Could not copy: %1 to %2")
                                .arg(m_tempfilename,firmware));
        emit done(true);
        return;
    }
    
    downloadFile.remove();

    m_dp->listProgress->addItem(tr("Bootloader install finished successfully."));
    m_dp->buttonAbort->setText(tr("&Ok"));
    
    emit done(false);  // success
   
}  


/**************************************************
*** h10 secific code 
***************************************************/
void BootloaderInstaller::h10Prepare()
{
    if(m_install)         // Installation
    {
        QString url = m_bootloaderUrlBase + "/iriver/" + m_bootloadername;
      
        m_dp->listProgress->addItem(tr("Downloading file %1.%2")
          .arg(QFileInfo(url).baseName(), QFileInfo(url).completeSuffix()));

        // temporary file needs to be opened to get the filename
        downloadFile.open();
        m_tempfilename = downloadFile.fileName();
        downloadFile.close();
        // get the real file.
        getter = new HttpGet(this);
        getter->setProxy(m_proxy);
        getter->setFile(&downloadFile);
        getter->getFile(QUrl(url));
        // connect signals from HttpGet
        connect(getter, SIGNAL(done(bool)), this, SLOT(downloadDone(bool)));
        //connect(getter, SIGNAL(requestFinished(int, bool)), this, SLOT(downloadRequestFinished(int, bool)));
        connect(getter, SIGNAL(dataReadProgress(int, int)), this, SLOT(updateDataReadProgress(int, int)));   
    }
    else             // Uninstallation
    {
    
        QString firmwarename = m_bootloadername.section('/', -1);

        QString firmware = m_mountpoint + "/SYSTEM/" + firmwarename;
        QString firmwareOrig = m_mountpoint + "/SYSTEM/Original.mi4";
    
        QFileInfo firmwareFI(firmware);
        if(!firmwareFI.exists())  //Firmware dosent exists on player
        {
            firmware = m_mountpoint + "/SYSTEM/H10EMP.mi4";   //attempt other firmwarename
            firmwareFI.setFile(firmware);
            if(!firmwareFI.exists())  //Firmware dosent exists on player
            {
                m_dp->listProgress->addItem(tr("Firmware doesn not exist: %1")
                                .arg(firmware));
                emit done(true);
                return;
            }
        }
       
        QFileInfo firmwareOrigFI(firmwareOrig);
        if(!firmwareOrigFI.exists())  //Original Firmware dosent exists on player
        {
            m_dp->listProgress->addItem(tr("Original Firmware doesn not exist: %1")
                                .arg(firmwareOrig));
            emit done(true);
            return;
        }
       
        QFile firmwareFile(firmware);
        QFile firmwareOrigFile(firmwareOrig);
        
        //remove modified firmware
        if(!firmwareFile.remove())
        {
             m_dp->listProgress->addItem(tr("Could not remove the Firmware at: %1")
                    .arg(firmware));
            emit done(true);
            return;        
        }
        
        //copy  original firmware
        if(!firmwareOrigFile.copy(firmware))   
        {
             m_dp->listProgress->addItem(tr("Could not copy the Firmware from: %1 to %2")
                    .arg(firmwareOrig,firmware));
            emit done(true);
            return;        
        }
        
        emit done(false);  //success
    
    }
}

void BootloaderInstaller::h10Finish()
{
    QString firmwarename = m_bootloadername.section('/', -1);

    QString firmware = m_mountpoint + "/SYSTEM/" + firmwarename;
    QString firmwareOrig = m_mountpoint + "/SYSTEM/Original.mi4";
    
    QFileInfo firmwareFI(firmware);
    
    if(!firmwareFI.exists())  //Firmware dosent exists on player
    {
        firmware = m_mountpoint + "/SYSTEM/H10EMP.mi4";   //attempt other firmwarename
        firmwareFI.setFile(firmware);
        if(!firmwareFI.exists())  //Firmware dosent exists on player
        {
            m_dp->listProgress->addItem(tr("Firmware does not exist: %1")
                                .arg(firmware));
            emit done(true);
            return;
        }
    }
    
    QFileInfo firmwareOrigFI(firmwareOrig);
    
    if(!firmwareOrigFI.exists())  //there is already a original firmware
    {
        QFile firmwareFile(firmware);
        if(!firmwareFile.rename(firmwareOrig)) //rename Firmware to Original
        {
            m_dp->listProgress->addItem(tr("Could not rename: %1 to %2")
                                .arg(firmware,firmwareOrig));
            emit done(true);
            return;
        }
    }
    else
    {
        QFile firmwareFile(firmware);
        firmwareFile.remove();
    }
        //copy the firmware
    if(!downloadFile.copy(firmware))
    {
        m_dp->listProgress->addItem(tr("Could not copy: %1 to %2")
                                .arg(m_tempfilename,firmware));
        emit done(true);
        return;
    }
    
    downloadFile.remove();

    m_dp->listProgress->addItem(tr("Bootloader install finished successfully."));
    m_dp->buttonAbort->setText(tr("&Ok"));
    
    emit done(false);  // success

}

/**************************************************
*** ipod secific code 
***************************************************/
int verbose =0;
// reserves memory for ipodpatcher
bool initIpodpatcher()
{
    if (ipod_alloc_buffer(&sectorbuf,BUFFER_SIZE) < 0) return true;
      else return false;
}

void BootloaderInstaller::ipodPrepare()
{
    m_dp->listProgress->addItem(tr("Searching for ipods"));
    struct ipod_t ipod;

    int n = ipod_scan(&ipod);
    if (n == 0)
    {
        m_dp->listProgress->addItem(tr("No Ipods found"));
        emit done(true);
        return;
    }
    if (n > 1)
    {
        m_dp->listProgress->addItem(tr("Too many Ipods found"));
        emit done(true);
    }
    
    if(m_install)         // Installation
    {
    
        QString url = m_bootloaderUrlBase + "/ipod/bootloader-" + m_bootloadername + ".ipod";
      
        m_dp->listProgress->addItem(tr("Downloading file %1.%2")
          .arg(QFileInfo(url).baseName(), QFileInfo(url).completeSuffix()));

        // temporary file needs to be opened to get the filename
        downloadFile.open();
        m_tempfilename = downloadFile.fileName();
        downloadFile.close();
        // get the real file.
        getter = new HttpGet(this);
        getter->setProxy(m_proxy);
        getter->setFile(&downloadFile);
        getter->getFile(QUrl(url));
        // connect signals from HttpGet
        connect(getter, SIGNAL(done(bool)), this, SLOT(downloadDone(bool)));
        //connect(getter, SIGNAL(requestFinished(int, bool)), this, SLOT(downloadRequestFinished(int, bool)));
        connect(getter, SIGNAL(dataReadProgress(int, int)), this, SLOT(updateDataReadProgress(int, int)));   
    
    }
    else                 // Uninstallation
    {
        if (ipod_open(&ipod, 0) < 0)
        {
            m_dp->listProgress->addItem(tr("could not open ipod"));
            emit done(true);
            return;
        }

        if (read_partinfo(&ipod,0) < 0)
        {
            m_dp->listProgress->addItem(tr("could not read partitiontable"));
            emit done(true);
            return;
        }

        if (ipod.pinfo[0].start==0)
        {
            m_dp->listProgress->addItem(tr("No partition 0 on disk"));
        
            int i;
            double sectors_per_MB = (1024.0*1024.0)/ipod.sector_size;
            m_dp->listProgress->addItem(tr("[INFO] Part    Start Sector    End Sector   Size (MB)   Type\n"));
            for ( i = 0; i < 4; i++ ) 
            {
                if (ipod.pinfo[i].start != 0) 
                {
                    m_dp->listProgress->addItem(tr("[INFO]    %1      %2    %3  %4   %5 (%6)").arg(
                        i).arg(
                        ipod.pinfo[i].start).arg(
                        ipod.pinfo[i].start+ipod.pinfo[i].size-1).arg(
                        ipod.pinfo[i].size/sectors_per_MB).arg(
                        get_parttype(ipod.pinfo[i].type)).arg(
                        ipod.pinfo[i].type));
                }
            }
            emit done(true);
            return;
        }

        read_directory(&ipod);

        if (ipod.nimages <= 0)
        {
            m_dp->listProgress->addItem(tr("Failed to read firmware directory"));
            emit done(true);
            return;
        }
        if (getmodel(&ipod,(ipod.ipod_directory[0].vers>>8)) < 0)
        {
            m_dp->listProgress->addItem(tr("Unknown version number in firmware (%1)").arg(
                ipod.ipod_directory[0].vers));
            emit done(true);
            return;
        }

        if (ipod.macpod)
        {
            m_dp->listProgress->addItem(tr("Warning this is a MacPod, Rockbox doesnt work on this. Convert it to WinPod"));
        }
        
        if (ipod_reopen_rw(&ipod) < 0) 
        {
            m_dp->listProgress->addItem(tr("Could not open Ipod in RW mode"));
            emit done(true);
            return;
        }
        
        if (ipod.ipod_directory[0].entryOffset==0) {
            m_dp->listProgress->addItem(tr("No bootloader detected."));
            emit done(true);
            return;
        }
       
        if (delete_bootloader(&ipod)==0) 
        {
            m_dp->listProgress->addItem(tr("Successfully removed Bootloader"));
            emit done(true);
            ipod_close(&ipod);
            return;      
        }
        else 
        {
            m_dp->listProgress->addItem(tr("--delete-bootloader failed."));
            emit done(true);
            return;          
        }    
    }   
}

void BootloaderInstaller::ipodFinish()
{
    struct ipod_t ipod;
    ipod_scan(&ipod);
    
    if (ipod_open(&ipod, 0) < 0)
    {
        m_dp->listProgress->addItem(tr("could not open ipod"));
        emit done(true);
        return;
    }

    if (read_partinfo(&ipod,0) < 0)
    {
        m_dp->listProgress->addItem(tr("could not read partitiontable"));
        emit done(true);
        return;
    }

    if (ipod.pinfo[0].start==0)
    {
        m_dp->listProgress->addItem(tr("No partition 0 on disk"));
        
        int i;
        double sectors_per_MB = (1024.0*1024.0)/ipod.sector_size;

        m_dp->listProgress->addItem(tr("[INFO] Part    Start Sector    End Sector   Size (MB)   Type\n"));
        
        for ( i = 0; i < 4; i++ ) 
        {
         if (ipod.pinfo[i].start != 0) 
         {
            m_dp->listProgress->addItem(tr("[INFO]    %1      %2    %3  %4   %5 (%6)").arg(
                    i).arg(
                   ipod.pinfo[i].start).arg(
                   ipod.pinfo[i].start+ipod.pinfo[i].size-1).arg(
                   ipod.pinfo[i].size/sectors_per_MB).arg(
                   get_parttype(ipod.pinfo[i].type)).arg(
                   ipod.pinfo[i].type));
         }
        }
        emit done(true);
        return;
    }

    read_directory(&ipod);

    if (ipod.nimages <= 0)
    {
        m_dp->listProgress->addItem(tr("Failed to read firmware directory"));
        emit done(true);
        return;
    }
    if (getmodel(&ipod,(ipod.ipod_directory[0].vers>>8)) < 0)
    {
        m_dp->listProgress->addItem(tr("Unknown version number in firmware (%1)").arg(
                ipod.ipod_directory[0].vers));
        emit done(true);
        return;
    }

    if (ipod.macpod)
    {
        m_dp->listProgress->addItem(tr("Warning this is a MacPod, Rockbox doesnt work on this. Convert it to WinPod"));
    }

    if (ipod_reopen_rw(&ipod) < 0) 
    {
        m_dp->listProgress->addItem(tr("Could not open Ipod in RW mode"));
        emit done(true);
        return;
    }

    if (add_bootloader(&ipod, m_tempfilename.toLatin1().data(), FILETYPE_DOT_IPOD)==0) 
    {
        m_dp->listProgress->addItem(tr("Successfully added Bootloader"));
        emit done(true);
        ipod_close(&ipod);
        return;      
    }
    else 
    {
        m_dp->listProgress->addItem(tr("failed to add Bootloader"));
        ipod_close(&ipod);
        emit done(true);
        return;      
    }
        
}


