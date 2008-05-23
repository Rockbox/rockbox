/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id$
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "progressloggergui.h"

ProgressLoggerGui::ProgressLoggerGui(QWidget* parent): ProgressloggerInterface(parent)
{
    downloadProgress = new QDialog(parent);
    downloadProgress->setModal(true);
    dp.setupUi(downloadProgress);
    dp.listProgress->setAlternatingRowColors(true);
    connect(dp.buttonAbort, SIGNAL(clicked()), this, SLOT(abort()));
}

void ProgressLoggerGui::addItem(const QString &text)
{
    addItem(text, LOGNOICON);
}

void ProgressLoggerGui::addItem(const QString &text, int flag)
{
    QListWidgetItem* item = new QListWidgetItem(text);

    switch(flag)
    {
        case LOGNOICON:
            break;
        case LOGOK:
            item->setIcon(QIcon(":/icons/go-next.png"));
            break;
        case LOGINFO:
            item->setIcon(QIcon(":/icons/dialog-information.png"));
            break;
        case LOGWARNING:
            item->setIcon(QIcon(":/icons/dialog-warning.png"));
            break;
        case LOGERROR:
            item->setIcon(QIcon(":/icons/dialog-error.png"));
            break;
    }

    dp.listProgress->addItem(item);
    dp.listProgress->scrollToItem(item);
}

void ProgressLoggerGui::setProgress(int value, int max)
{
    setProgressValue(value);
    setProgressMax(max);
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

void ProgressLoggerGui::setProgressVisible(bool b)
{
    dp.progressBar->setVisible(b);
}


void ProgressLoggerGui::abort()
{
    dp.buttonAbort->setText(tr("&Ok"));
    dp.buttonAbort->setIcon(QIcon(QString::fromUtf8(":/icons/go-next.png")));
    disconnect(dp.buttonAbort, SIGNAL(clicked()), this, SLOT(abort()));
    connect(dp.buttonAbort, SIGNAL(clicked()), downloadProgress, SLOT(close()));
    connect(dp.buttonAbort, SIGNAL(clicked()), this, SIGNAL(closed()));
    emit aborted();
}

void ProgressLoggerGui::undoAbort()
{
    dp.buttonAbort->setText(tr("&Abort"));
    dp.buttonAbort->setIcon(QIcon(QString::fromUtf8(":/icons/process-stop.png")));
    connect(dp.buttonAbort, SIGNAL(clicked()), this, SLOT(abort()));
}

void ProgressLoggerGui::close()
{
    downloadProgress->close();
}

void ProgressLoggerGui::show()
{
    downloadProgress->show();
}


