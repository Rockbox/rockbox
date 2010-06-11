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

#include "uninstallwindow.h"
#include "ui_uninstallfrm.h"
#include "rbsettings.h"

UninstallWindow::UninstallWindow(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    ui.UninstalllistWidget->setAlternatingRowColors(true);
    connect(ui.UninstalllistWidget,SIGNAL(itemSelectionChanged()),this,SLOT(selectionChanged()));
    connect(ui.CompleteRadioBtn,SIGNAL(toggled(bool)),this,SLOT(UninstallMethodChanged(bool)));
    
    QString mountpoint = RbSettings::value(RbSettings::Mountpoint).toString();

    uninstaller = new Uninstaller(this,mountpoint);
    logger = new ProgressLoggerGui(this);
    connect(uninstaller, SIGNAL(logItem(QString, int)), logger, SLOT(addItem(QString, int)));
    connect(uninstaller, SIGNAL(logProgress(int, int)), logger, SLOT(setProgress(int, int)));
    connect(uninstaller, SIGNAL(logFinished(void)), logger, SLOT(setFinished(void)));
    connect(logger, SIGNAL(closed()), this, SLOT(close()));

    // disable smart uninstall, if not possible
    if(!uninstaller->uninstallPossible())
    {
        ui.smartRadioButton->setEnabled(false);
        ui.smartGroupBox->setEnabled(false);
        ui.CompleteRadioBtn->setChecked(true);
    }
    else // fill in installed parts
    {
       ui.smartRadioButton->setChecked(true);
       ui.UninstalllistWidget->addItems(uninstaller->getAllSections());
    }
    
}


void UninstallWindow::accept()
{
    logger->show();
    
    if(ui.CompleteRadioBtn->isChecked())
    {
        uninstaller->deleteAll();
    }
    else
    {
        uninstaller->uninstall();
    }
    
}


void UninstallWindow::selectionChanged()
{
    QList<QListWidgetItem *> itemlist = ui.UninstalllistWidget->selectedItems();
    QStringList seletedStrings;
    for(int i=0;i < itemlist.size(); i++ )
    {
        seletedStrings << itemlist.at(i)->text();
    }

    uninstaller->setSections(seletedStrings);
}

void UninstallWindow::UninstallMethodChanged(bool complete)
{
    if(complete)
       ui.smartGroupBox->setEnabled(false);
    else
       ui.smartGroupBox->setEnabled(true);
}

