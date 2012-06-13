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

#include "installtalkwindow.h"
#include "ui_installtalkfrm.h"

#include "configure.h"
#include "rbsettings.h"
#include "systeminfo.h"

InstallTalkWindow::InstallTalkWindow(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    talkcreator = new TalkFileCreator(this);

    connect(ui.change,SIGNAL(clicked()),this,SLOT(change()));

    ui.recursive->setChecked(true);
    ui.GenerateOnlyNew->setChecked(true);
    ui.StripExtensions->setChecked(true);

    fsm = new QFileSystemModel(this);
    QString mp = RbSettings::value(RbSettings::Mountpoint).toString();
    fsm->setRootPath(mp);
    ui.treeView->setModel(fsm);
    ui.treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui.treeView->setRootIndex(fsm->index(mp));
    qDebug() << fsm->columnCount();
    fsm->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
    for(int i = 1; i < fsm->columnCount(); i++)
        ui.treeView->setColumnHidden(i, true);
    ui.treeView->setHeaderHidden(true);

    updateSettings();
}


void InstallTalkWindow::change()
{
    Config *cw = new Config(this, 4);

    // make sure the current selected folder doesn't get lost on settings
    // changes.
    QModelIndexList si = ui.treeView->selectionModel()->selectedIndexes();
    QStringList foldersToTalk;
    for(int i = 0; i < si.size(); i++) {
        if(si.at(i).column() == 0) {
            foldersToTalk.append(fsm->filePath(si.at(i)));
        }
    }
    RbSettings::setValue(RbSettings::LastTalkedFolder, foldersToTalk);
    connect(cw, SIGNAL(settingsUpdated()), this, SLOT(updateSettings()));

    cw->show();
}

void InstallTalkWindow::accept()
{
    logger = new ProgressLoggerGui(this);

    QModelIndexList si = ui.treeView->selectionModel()->selectedIndexes();
    QStringList foldersToTalk;
    for(int i = 0; i < si.size(); i++) {
        if(si.at(i).column() == 0) {
            foldersToTalk.append(fsm->filePath(si.at(i)));
        }
    }
    connect(logger,SIGNAL(closed()),this,SLOT(close()));
    logger->show();

    RbSettings::setValue(RbSettings::LastTalkedFolder, foldersToTalk);

    RbSettings::sync();

    talkcreator->setMountPoint(RbSettings::value(RbSettings::Mountpoint).toString());

    talkcreator->setGenerateOnlyNew(ui.GenerateOnlyNew->isChecked());
    talkcreator->setRecursive(ui.recursive->isChecked());
    talkcreator->setStripExtensions(ui.StripExtensions->isChecked());
    talkcreator->setTalkFolders(ui.talkFolders->isChecked());
    talkcreator->setTalkFiles(ui.talkFiles->isChecked());
    talkcreator->setIgnoreFiles(ui.ignoreFiles->text().split(",",QString::SkipEmptyParts));

    connect(talkcreator, SIGNAL(done(bool)), logger, SLOT(setFinished()));
    connect(talkcreator, SIGNAL(logItem(QString, int)), logger, SLOT(addItem(QString, int)));
    connect(talkcreator, SIGNAL(logProgress(int, int)), logger, SLOT(setProgress(int, int)));
    connect(logger,SIGNAL(aborted()),talkcreator,SLOT(abort()));

    for(int i = 0; i < foldersToTalk.size(); i++) {
        talkcreator->setDir(QDir(foldersToTalk.at(i)));
        talkcreator->createTalkFiles();
    }
}


void InstallTalkWindow::updateSettings(void)
{
    QString ttsName = RbSettings::value(RbSettings::Tts).toString();
    TTSBase* tts = TTSBase::getTTS(this,ttsName);
    if(tts->configOk())
        ui.labelTtsProfile->setText(tr("<b>%1</b>")
            .arg(TTSBase::getTTSName(ttsName)));
    else
        ui.labelTtsProfile->setText(tr("<b>%1</b>")
            .arg("Invalid TTS configuration!"));

    QStringList folders = RbSettings::value(RbSettings::LastTalkedFolder).toStringList();
    for(int i = 0; i < folders.size(); ++i) {
        QModelIndex mi = fsm->index(folders.at(i));
        ui.treeView->selectionModel()->select(mi, QItemSelectionModel::Select);
        // make sure all parent items are expanded.
        while((mi = mi.parent()) != QModelIndex()) {
            ui.treeView->setExpanded(mi, true);
        }
    }

    emit settingsUpdated();
}


void InstallTalkWindow::changeEvent(QEvent *e)
{
    if(e->type() == QEvent::LanguageChange) {
        ui.retranslateUi(this);
        updateSettings();
    } else {
        QWidget::changeEvent(e);
    }
}

