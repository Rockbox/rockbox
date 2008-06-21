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
#include "sysinfo.h"
#include "ui_sysinfofrm.h"
#include "utils.h"


Sysinfo::Sysinfo(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    this->setModal(true);

    updateSysinfo();
    connect(ui.buttonOk, SIGNAL(clicked()), this, SLOT(close()));
    connect(ui.buttonRefresh, SIGNAL(clicked()), this, SLOT(updateSysinfo()));
}


void Sysinfo::updateSysinfo(void)
{
    QString info;
    info += tr("<b>OS</b><br/>") + getOsVersionString() + "<hr/>";
    info += tr("<b>Username:</b><br/>%1<hr/>").arg(getUserName());
#if defined(Q_OS_WIN32)
    info += tr("<b>Permissions:</b><br/>%1<hr/>").arg(getUserPermissionsString());
#endif
    info += tr("<b>Attached USB devices:</b><br/>");
    QList<uint32_t> usbids = listUsbIds();
    for(int i = 0; i < usbids.size(); i++)
        info += tr("VID: %1 PID: %2<br/>")
            .arg((usbids.at(i)&0xffff0000)>>16, 4, 16, QChar('0'))
            .arg(usbids.at(i)&0xffff, 4, 16, QChar('0'));

    ui.textBrowser->setHtml(info);
}

