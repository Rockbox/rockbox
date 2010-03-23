/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Riebeling
 *   $Id$
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <QtGui>
#include "systrace.h"
#include "ui_systracefrm.h"

#include "rbsettings.h"

QString SysTrace::debugbuffer;

SysTrace::SysTrace(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    ui.textTrace->setReadOnly(true);
    ui.textTrace->setLayoutDirection(Qt::LeftToRight);
    refresh();

    connect(ui.buttonClose, SIGNAL(clicked()), this, SLOT(close()));
    connect(ui.buttonSave, SIGNAL(clicked()), this, SLOT(saveCurrentTrace()));
    connect(ui.buttonSavePrevious, SIGNAL(clicked()), this, SLOT(savePreviousTrace()));
    connect(ui.buttonRefresh, SIGNAL(clicked()), this, SLOT(refresh()));
}

void SysTrace::refresh(void)
{
    int pos = ui.textTrace->verticalScrollBar()->value();
    ui.textTrace->setHtml("<pre>" + debugbuffer + "</pre>");
    ui.textTrace->verticalScrollBar()->setValue(pos);
    QString oldlog = RbSettings::value(RbSettings::CachePath).toString()
                     + "/rbutil-trace.log";
    ui.buttonSavePrevious->setEnabled(QFileInfo(oldlog).isFile());
}


void SysTrace::save(QString filename)
{
    if(filename.isEmpty())
        filename = RbSettings::value(RbSettings::CachePath).toString()
                    + "/rbutil-trace.log";
    // append save date to the trace. Append it directly instead of using
    // qDebug() as the handler might have been unregistered.
    debugbuffer.append("[SysTrace] saving trace at ");
    debugbuffer.append(QDateTime::currentDateTime().toString(Qt::ISODate));
    debugbuffer.append("\n");
    QFile fh(filename);
    if(!fh.open(QIODevice::WriteOnly))
        return;
    fh.write(debugbuffer.toUtf8(), debugbuffer.size());
    fh.close();
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

    QString oldlog = RbSettings::value(RbSettings::CachePath).toString()
                     + "/rbutil-trace.log";
    QFile::copy(oldlog, fp);
    return;
}

void SysTrace::debug(QtMsgType type, const char* msg)
{
    (void)type;
    debugbuffer.append(msg);
    debugbuffer.append("\n");
#if !defined(NODEBUG)
    fprintf(stderr, "%s\n", msg);
#endif

}

