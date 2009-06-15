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
    setRunning();
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
    // set maximum first to avoid setting a value outside of the max range.
    // If the current value is outside of the valid range QProgressBar
    // calls reset() internally.
    setProgressMax(max);
    setProgressValue(value);
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


/** Set logger into "running" state -- the reporting process is still running.
 *  Display "Abort" and emit the aborted() signal on button press.
 */
void ProgressLoggerGui::setRunning()
{
    dp.buttonAbort->setText(tr("&Abort"));
    dp.buttonAbort->setIcon(QIcon(QString::fromUtf8(":/icons/process-stop.png")));

    // make sure to not close the window on button press.
    disconnect(dp.buttonAbort, SIGNAL(clicked()), downloadProgress, SLOT(close()));
    // emit aborted() once button is pressed but not closed().
    disconnect(dp.buttonAbort, SIGNAL(clicked()), this, SIGNAL(closed()));
    connect(dp.buttonAbort, SIGNAL(clicked()), this, SIGNAL(aborted()));

}


/** Set logger into "finished" state -- the reporting process is finished.
 *  Display "Ok". Don't emit aborted() as there is nothing running left.
 *  Close logger on button press and emit closed().
 */
void ProgressLoggerGui::setFinished()
{
    dp.buttonAbort->setText(tr("&Ok"));
    dp.buttonAbort->setIcon(QIcon(QString::fromUtf8(":/icons/go-next.png")));

    // close the window on button press.
    connect(dp.buttonAbort, SIGNAL(clicked()), downloadProgress, SLOT(close()));
    // emit closed() once button is pressed but not aborted().
    disconnect(dp.buttonAbort, SIGNAL(clicked()), this, SIGNAL(aborted()));
    connect(dp.buttonAbort, SIGNAL(clicked()), this, SIGNAL(closed()));
}


void ProgressLoggerGui::close()
{
    downloadProgress->close();
}

void ProgressLoggerGui::show()
{
    downloadProgress->show();
}


