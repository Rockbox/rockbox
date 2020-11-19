/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <QFileDialog>
#include "progressloggergui.h"

#include "sysinfo.h"
#include "systrace.h"

ProgressLoggerGui::ProgressLoggerGui(QWidget* parent): ProgressloggerInterface(parent)
{
    downloadProgress = new QDialog(parent);
    downloadProgress->setModal(true);
    dp.setupUi(downloadProgress);
    dp.listProgress->setAlternatingRowColors(true);
    dp.saveLog->hide();
    connect(dp.saveLog,SIGNAL(clicked()),this,SLOT(saveErrorLog()));
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
            item->setIcon(QIcon(":/icons/go-next.svg"));
            break;
        case LOGINFO:
            item->setIcon(QIcon(":/icons/dialog-information.svg"));
            break;
        case LOGWARNING:
            item->setIcon(QIcon(":/icons/dialog-warning.svg"));
            break;
        case LOGERROR:
            item->setIcon(QIcon(":/icons/dialog-error.svg"));
            dp.saveLog->show();
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
    dp.buttonAbort->setIcon(QIcon(QString::fromUtf8(":/icons/process-stop.svg")));

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
    dp.buttonAbort->setIcon(QIcon(QString::fromUtf8(":/icons/go-next.svg")));

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

void ProgressLoggerGui::saveErrorLog()
{
    QString filename = QFileDialog::getSaveFileName(downloadProgress,
            tr("Save system trace log"), QDir::homePath(), "*.log");
    if(filename.isEmpty())
        return;

    QFile file(filename);
    if(!file.open(QIODevice::WriteOnly))
        return;

    //Logger texts
    QString loggerTexts = "\n*********************************************\n"
                          "***************  Logger   *******************\n"
                          "*********************************************\n";

    file.write(loggerTexts.toUtf8(), loggerTexts.size());


    int i=0;
    loggerTexts = "";
    while(dp.listProgress->item(i) != nullptr)
    {
        loggerTexts.append(dp.listProgress->item(i)->text());
        loggerTexts.append("\n");
        i++;
    }
    file.write(loggerTexts.toUtf8(), loggerTexts.size());

    //systeminfo
    QString info = "\n*********************************************\n"
                   "************  SYSTEMINFO  *******************\n"
                   "*********************************************\n";

    file.write(info.toUtf8(), info.size());
    info = Sysinfo::getInfo(Sysinfo::InfoText);
    file.write(info.toUtf8(), info.size());

    // trace
    QString trace = "\n*********************************************\n"
                    "***********  TRACE **************************\n"
                    "*********************************************\n";
    file.write(trace.toUtf8(), trace.size());
    trace = SysTrace::getTrace();
    file.write(trace.toUtf8(), trace.size());

    file.close();
}

