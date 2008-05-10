/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2008 by Dominik Wenger
 *   $Id: rbzip.cpp 17129 2008-04-15 21:25:57Z bluebrother $
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "rbzip.h"
#include <QtCore>


Zip::ErrorCode RbZip::createZip(QString zip,QString dir,ProgressloggerInterface *dp)
{
    m_logger = dp;
    Zip::ErrorCode error = Ok;
    m_curEntry = 1;
    int numEntrys=0;
    
    m_logger->addItem(tr("Creating Backup: %1").arg(zip),LOGINFO);
    QCoreApplication::processEvents();
   
    // get number of entrys in dir
    QDirIterator it(dir, QDirIterator::Subdirectories);
    while (it.hasNext()) 
    {
        it.next();
        numEntrys++;
        QCoreApplication::processEvents();
    }
    m_logger->setProgressMax(numEntrys);
    
    //! create zip
    error = Zip::createArchive(zip);
    if(error != Ok)
        return error;
    
    //! add the content
    error = Zip::addDirectory(dir);
    if(error != Ok)
        return error;
    
    //! close zip
    error = Zip::closeArchive();
   
   return error;
}

void RbZip::progress()
{
    m_curEntry++;
    m_logger->setProgressValue(m_curEntry);
    QCoreApplication::processEvents(); // update UI
}


