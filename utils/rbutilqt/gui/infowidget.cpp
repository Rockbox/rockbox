/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2012 by Dominik Riebeling
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <QWidget>
#include <QDebug>
#include "infowidget.h"
#include "rbsettings.h"
#include "Logger.h"

InfoWidget::InfoWidget(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);

    ui.treeInfo->setAlternatingRowColors(true);
    ui.treeInfo->setHeaderLabels(QStringList() << tr("File") << tr("Version"));
    ui.treeInfo->expandAll();
    ui.treeInfo->setColumnCount(2);
    ui.treeInfo->setLayoutDirection(Qt::LeftToRight);
}


void InfoWidget::updateInfo(void)
{
    LOG_INFO() << "updating install info";

    QString mp = RbSettings::value(RbSettings::Mountpoint).toString();
    QSettings log(mp + "/.rockbox/rbutil.log", QSettings::IniFormat, this);
    QStringList groups = log.childGroups();
    QTreeWidgetItem *w, *w2;
    QString min, max;
    QTreeWidgetItem *loading = new QTreeWidgetItem;
    loading->setText(0, tr("Loading, please wait ..."));
    ui.treeInfo->clear();
    ui.treeInfo->addTopLevelItem(loading);
    ui.treeInfo->resizeColumnToContents(0);
    QCoreApplication::processEvents();

    // get and populate new items
    for(int a = 0; a < groups.size(); a++) {
        log.beginGroup(groups.at(a));
        QStringList keys = log.allKeys();
        w = new QTreeWidgetItem;
        w->setFlags(Qt::ItemIsEnabled);
        w->setText(0, groups.at(a));
        ui.treeInfo->addTopLevelItem(w);
        // get minimum and maximum version information so we can hilight old files
        min = max = log.value(keys.at(0)).toString();
        for(int b = 0; b < keys.size(); b++) {
            QString v = log.value(keys.at(b)).toString();
            if(v > max)
                max = v;
            if(v < min)
                min = v;

            QString file = mp + "/" + keys.at(b);
            if(QFileInfo(file).isDir()) {
                // ignore folders
                continue;
            }
            w2 = new QTreeWidgetItem(w, QStringList() << "/"
                    + keys.at(b) << v);
            if(v != max) {
                w2->setForeground(0, QBrush(QColor(255, 0, 0)));
                w2->setForeground(1, QBrush(QColor(255, 0, 0)));
            }
            w->addChild(w2);
        }
        log.endGroup();
        if(min != max)
            w->setData(1, Qt::DisplayRole, QString("%1 / %2").arg(min, max));
        else
            w->setData(1, Qt::DisplayRole, max);
        QCoreApplication::processEvents();
    }
    ui.treeInfo->takeTopLevelItem(0);
    ui.treeInfo->resizeColumnToContents(0);
}


void InfoWidget::changeEvent(QEvent *e)
{
    if(e->type() == QEvent::LanguageChange) {
        ui.retranslateUi(this);
        ui.treeInfo->setHeaderLabels(QStringList() << tr("File") << tr("Version"));
    } else {
        QWidget::changeEvent(e);
    }
}

