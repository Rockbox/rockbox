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

#include <QtGui>

#include "browseof.h"
#include "browsedirtree.h"


BrowseOF::BrowseOF(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    this->setModal(true);
    
    connect(ui.browseOFButton,SIGNAL(clicked()),this,SLOT(onBrowse()));
}

void BrowseOF::setFile(QString file)
{
    ui.OFlineEdit->setText(file);    
}

void BrowseOF::onBrowse()
{
    BrowseDirtree browser(this);
    browser.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
       
    if(QFileInfo(ui.OFlineEdit->text()).exists())
    {
        QDir d(ui.OFlineEdit->text());
        browser.setDir(d);
    }
    
    if(browser.exec() == QDialog::Accepted)
    {
        qDebug() << browser.getSelected();
        setFile(browser.getSelected());
    }
}

QString BrowseOF::getFile()
{
    return ui.OFlineEdit->text();
}

void BrowseOF::accept()
{
    this->close();
    setResult(QDialog::Accepted);
}



