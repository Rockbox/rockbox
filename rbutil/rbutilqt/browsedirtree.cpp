/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Riebeling
 *   $Id: installrb.cpp 13990 2007-07-25 22:26:10Z Dominik Wenger $
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <QtGui>

#include "browsedirtree.h"
#include "ui_browsedirtreefrm.h"


BrowseDirtree::BrowseDirtree(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    this->setModal(true);
    ui.tree->setModel(&model);
    model.setReadOnly(true);
    // disable size / date / type columns
    ui.tree->setColumnHidden(1, true);
    ui.tree->setColumnHidden(2, true);
    ui.tree->setColumnHidden(3, true);
    ui.tree->setAlternatingRowColors(true);
}


void BrowseDirtree::setDir(QDir &dir)
{
    qDebug() << "BrowseDirtree::setDir()" << model.index(dir.absolutePath());

    // do not try to hilight directory if it's not valid.
    if(!dir.exists()) return;
    // hilight the set directory if it's valid
    if(model.index(dir.absolutePath()).isValid()) {
        QModelIndex p = model.index(dir.absolutePath());
        ui.tree->setCurrentIndex(p);
        ui.tree->scrollTo(p);
        ui.tree->resizeColumnToContents(0);
    }
}


void BrowseDirtree::setFilter(QDir::Filters filters)
{
    model.setFilter(filters);
}


void BrowseDirtree::accept()
{
    QString path;
    path = model.filePath(ui.tree->currentIndex());

    this->close();
    emit itemChanged(QDir::toNativeSeparators(path));
    setResult(QDialog::Accepted); 
}

QString BrowseDirtree::getSelected()
{
    QString path;
    path = model.filePath(ui.tree->currentIndex());
    return QDir::toNativeSeparators(path);
}


