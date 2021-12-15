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

#include <QDialog>
#include <QDir>
#include "sysinfo.h"
#include "ui_sysinfofrm.h"
#include "system.h"
#include "utils.h"
#include "autodetection.h"


Sysinfo::Sysinfo(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);

    updateSysinfo();
    connect(ui.buttonOk, &QAbstractButton::clicked, this, &Sysinfo::close);
    connect(ui.buttonRefresh, &QAbstractButton::clicked, this, &Sysinfo::updateSysinfo);
}

void Sysinfo::updateSysinfo(void)
{
    ui.textBrowser->setHtml(getInfo());
}

QString Sysinfo::getInfo(Sysinfo::InfoType type)
{
    QString info;
    info += tr("<b>OS</b><br/>") + System::osVersionString() + "<hr/>";
    info += tr("<b>Username</b><br/>%1<hr/>").arg(System::userName());
#if defined(Q_OS_WIN32)
    info += tr("<b>Permissions</b><br/>%1<hr/>").arg(System::userPermissionsString());
#endif
    info += tr("<b>Attached USB devices</b><br/>");
    QMultiMap<uint32_t, QString> usbids = System::listUsbDevices();
    QList<uint32_t> usbkeys = usbids.keys();
    for(int i = 0; i < usbkeys.size(); i++) {
        info += tr("VID: %1 PID: %2, %3")
            .arg((usbkeys.at(i)&0xffff0000)>>16, 4, 16, QChar('0'))
            .arg(usbkeys.at(i)&0xffff, 4, 16, QChar('0'))
            .arg(usbids.value(usbkeys.at(i)));
            if(i + 1 < usbkeys.size())
                info += "<br/>";
    }
    info += "<hr/>";

    info += "<b>" + tr("Filesystem") + "</b>";
    QStringList drives = Utils::mountpoints();
    info += "<table>";
    info += "<tr><td>" + tr("Mountpoint") + "</td><td>" + tr("Label")
            + "</td><td>" + tr("Free") + "</td><td>" + tr("Total") + "</td><td>"
            + tr("Type") + "</td><td></tr>";
    for(int i = 0; i < drives.size(); i++) {
        info += tr("<tr><td>%1</td><td>%4</td><td>%2 GiB</td><td>%3 GiB</td><td>%5</td></tr>")
            .arg(QDir::toNativeSeparators(drives.at(i)))
            .arg((double)Utils::filesystemFree(drives.at(i)) / (1<<30), 0, 'f', 2)
            .arg((double)Utils::filesystemTotal(drives.at(i)) / (1<<30), 0, 'f', 2)
            .arg(Utils::filesystemName(drives.at(i)))
            .arg(Utils::filesystemType(drives.at(i)));
    }
    info += "</table>";
    info += "<hr/>";
    if(type == InfoText) {
        info.replace(QRegExp("(<[^>]+>)+"),"\n");
    }

    return info;
}


void Sysinfo::changeEvent(QEvent *e)
{
    if(e->type() == QEvent::LanguageChange) {
        ui.retranslateUi(this);
    } else {
        QWidget::changeEvent(e);
    }
}

