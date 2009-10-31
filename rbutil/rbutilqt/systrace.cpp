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


QString SysTrace::debugbuffer;

SysTrace::SysTrace(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    ui.textTrace->setReadOnly(true);
    ui.textTrace->setLayoutDirection(Qt::LeftToRight);
    refresh();

    connect(ui.buttonClose, SIGNAL(clicked()), this, SLOT(close()));
    connect(ui.buttonSave, SIGNAL(clicked()), this, SLOT(save()));
    connect(ui.buttonRefresh, SIGNAL(clicked()), this, SLOT(refresh()));
}

void SysTrace::refresh(void)
{
    int pos = ui.textTrace->verticalScrollBar()->value();
    ui.textTrace->setHtml("<pre>" + debugbuffer + "</pre>");
    ui.textTrace->verticalScrollBar()->setValue(pos);
}

void SysTrace::save(void)
{
    QString fp = QFileDialog::getSaveFileName(this, tr("Save system trace log"),
                        QDir::homePath(), "*.log");
    if(fp == "")
        return;
        
    QFile fh(fp);
    if(!fh.open(QIODevice::WriteOnly))
        return;
    fh.write(debugbuffer.toUtf8(), debugbuffer.size());
    fh.close();
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

