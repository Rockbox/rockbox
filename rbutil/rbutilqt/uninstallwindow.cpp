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


UninstallWindow::UninstallWindow(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    ui.UninstalllistWidget->setAlternatingRowColors(true);
    connect(ui.UninstalllistWidget,SIGNAL(itemSelectionChanged()),this,SLOT(selectionChanged()));
    connect(ui.CompleteRadioBtn,SIGNAL(toggled(bool)),this,SLOT(UninstallMethodChanged(bool)));
}


void UninstallWindow::accept()
{
    logger = new ProgressLoggerGui(this);
    logger->show();

    if(ui.CompleteRadioBtn->isChecked())
    {
        uninstaller->deleteAll(logger);
    }
    else
    {
        uninstaller->uninstall(logger);
    }
    connect(logger,SIGNAL(closed()),this,SLOT(close()));
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


void UninstallWindow::setSettings(RbSettings *sett)
{
    settings = sett;

    QString mountpoint =settings->mountpoint();
    uninstaller = new Uninstaller(this,mountpoint);

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
