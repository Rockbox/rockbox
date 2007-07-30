/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id: progressloggergui.cpp 14027 2007-07-27 17:42:49Z domonoky $
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
 
#include "progressloggergui.h"

ProgressLoggerGui::ProgressLoggerGui(QObject* parent): ProgressloggerInterface(parent)
{
    downloadProgress = new QDialog();
    downloadProgress->setModal(true);
    dp.setupUi(downloadProgress);
    connect(dp.buttonAbort, SIGNAL(clicked()), this, SLOT(abort()));
}


void ProgressLoggerGui::addItem(QString text)
{
    dp.listProgress->addItem(text);
}  

void ProgressLoggerGui::setProgressValue(int  value)
{
    dp.progressBar->setValue(value);
} 

void ProgressLoggerGui::setProgressMax(int max)
{
    dp.progressBar->setMaximum(max);
}

int ProgressLoggerGui::getProgressMax()
{
    return dp.progressBar->maximum();
} 

void ProgressLoggerGui::abort()
{
    dp.buttonAbort->setText(tr("&Ok"));
    disconnect(dp.buttonAbort, SIGNAL(clicked()), this, SLOT(abort()));
    connect(dp.buttonAbort, SIGNAL(clicked()), downloadProgress, SLOT(close()));
    connect(dp.buttonAbort, SIGNAL(clicked()), this, SIGNAL(closed()));
    emit aborted();
}

void ProgressLoggerGui::close()
{
    downloadProgress->close();
}

void ProgressLoggerGui::show()
{
    downloadProgress->show();
}


