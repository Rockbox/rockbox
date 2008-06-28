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
#include "detect.h"


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
    info += tr("<b>OS</b><br/>") + Detect::osVersionString() + "<hr/>";
    info += tr("<b>Username:</b><br/>%1<hr/>").arg(Detect::userName());
#if defined(Q_OS_WIN32)
    info += tr("<b>Permissions:</b><br/>%1<hr/>").arg(Detect::userPermissionsString());
#endif
    info += tr("<b>Attached USB devices:</b><br/>");
    QMap<uint32_t, QString> usbids = Detect::listUsbDevices();
    QList<uint32_t> usbkeys = usbids.keys();
    for(int i = 0; i < usbkeys.size(); i++)
        info += tr("VID: %1 PID: %2, %3<br/>")
            .arg((usbkeys.at(i)&0xffff0000)>>16, 4, 16, QChar('0'))
            .arg(usbkeys.at(i)&0xffff, 4, 16, QChar('0'))
	    .arg(usbids.value(usbkeys.at(i)));

    ui.textBrowser->setHtml(info);
}

