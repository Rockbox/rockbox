/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Riebeling
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <QFileDialog>
#include <QScrollBar>
#include "systrace.h"
#include "ui_systracefrm.h"

#include "rbsettings.h"
#include "Logger.h"


SysTrace::SysTrace(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    ui.textTrace->setReadOnly(true);
    ui.textTrace->setLayoutDirection(Qt::LeftToRight);
    refresh();

    connect(ui.buttonClose, &QAbstractButton::clicked,  this, &SysTrace::close);
    connect(ui.buttonSave, &QAbstractButton::clicked,  this, &SysTrace::saveCurrentTrace);
    connect(ui.buttonSavePrevious, &QAbstractButton::clicked,  this, &SysTrace::savePreviousTrace);
    connect(ui.buttonRefresh, &QAbstractButton::clicked,  this, &SysTrace::refresh);
}

void SysTrace::refresh(void)
{
    int pos = ui.textTrace->verticalScrollBar()->value();

    QString debugbuffer;
    QFile tracefile(QDir::tempPath() + "/rbutil-trace.log");
    tracefile.open(QIODevice::ReadOnly);
    QTextStream c(&tracefile);
    QString line;
    QString color;
    while(!c.atEnd()) {
        line = c.readLine();
        if(line.contains("Warning"))
            color = "orange";
        else if(line.contains("Error"))
            color = "red";
        else if(line.contains("Debug"))
            color = "blue";
#if 0
        else if(line.contains("INFO"))
            color = "green";
#endif
        else
            color = "black";
        debugbuffer += QString("<div style='color:%1;'>%2</div>").arg(color, line);
    }
    tracefile.close();
    ui.textTrace->setHtml("<pre>" + debugbuffer + "</pre>");
    ui.textTrace->verticalScrollBar()->setValue(pos);
    QString oldlog = RbSettings::value(RbSettings::CachePath).toString()
                     + "/rbutil-trace.log";
    ui.buttonSavePrevious->setEnabled(QFileInfo(oldlog).isFile());
}


QString SysTrace::getTrace(void)
{
    QString debugbuffer;
    QFile tracefile(QDir::tempPath() + "/rbutil-trace.log");
    tracefile.open(QIODevice::ReadOnly);
    QTextStream c(&tracefile);
    debugbuffer = c.readAll();
    tracefile.close();

    return debugbuffer;
}


void SysTrace::save(QString filename)
{
    if(filename.isEmpty())
        return;
    LOG_INFO() << "saving trace at" <<  QDateTime::currentDateTime().toString(Qt::ISODate);
    QFile::copy(QDir::tempPath() + "/rbutil-trace.log", filename);

}

void SysTrace::saveCurrentTrace(void)
{
    QString fp = QFileDialog::getSaveFileName(this, tr("Save system trace log"),
                        QDir::homePath(), "*.log");
    if(!fp.isEmpty())
        save(fp);
}


void SysTrace::savePreviousTrace(void)
{
    QString fp = QFileDialog::getSaveFileName(this, tr("Save system trace log"),
                          QDir::homePath(), "*.log");
    if(fp.isEmpty())
        return;

    QString oldlog = QDir::tempPath() + "/rbutil-trace.log.1";
    QFile::copy(oldlog, fp);
    return;
}


void SysTrace::rotateTrace(void)
{
    QString f = QDir::tempPath() + "/rbutil-trace.log.1";
    if(QFileInfo::exists(f)) {
        QFile::remove(f);
    }
    QFile::rename(QDir::tempPath() + "/rbutil-trace.log", f);
}


void SysTrace::changeEvent(QEvent *e)
{
    if(e->type() == QEvent::LanguageChange) {
        ui.retranslateUi(this);
    } else {
        QWidget::changeEvent(e);
    }
}

