/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Robert Bieber
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "editorwindow.h"
#include "ui_editorwindow.h"

#include <QDesktopWidget>
#include <QFileSystemModel>
#include <QSettings>

EditorWindow::EditorWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::EditorWindow)
{
    ui->setupUi(this);
    loadSettings();
    setupUI();
    setupMenus();
}

void EditorWindow::loadSettings()
{

    QSettings settings;

    /* Main Window location */
    settings.beginGroup("MainWindow");
    QSize size = settings.value("size").toSize();
    QPoint pos = settings.value("position").toPoint();
    QByteArray state = settings.value("state").toByteArray();
    settings.endGroup();

    if(!(size.isNull() || pos.isNull() || state.isNull()))
    {
        resize(size);
        move(pos);
        restoreState(state);
    }


}

void EditorWindow::saveSettings()
{

    QSettings settings;

    /* Saving window and panel positions */
    settings.beginGroup("MainWindow");
    settings.setValue("position", pos());
    settings.setValue("size", size());
    settings.setValue("state", saveState());
    settings.endGroup();
}

void EditorWindow::setupUI()
{
    /* Displaying some files to test the file tree view */
    QFileSystemModel* model = new QFileSystemModel;
    model->setRootPath(QDir::currentPath());
    ui->fileTree->setModel(model);

    /* Connecting the tab bar signals */
    QObject::connect(ui->editorTabs, SIGNAL(currentChanged(int)),
                     this, SLOT(shiftTab(int)));
    QObject::connect(ui->editorTabs, SIGNAL(tabCloseRequested(int)),
                     this, SLOT(closeTab(int)));

}

void EditorWindow::setupMenus()
{
    /* Connecting panel show actions */
    QObject::connect(ui->actionFile_Panel, SIGNAL(triggered()),
                     this, SLOT(showPanel()));
    QObject::connect(ui->actionDisplay_Panel, SIGNAL(triggered()),
                     this, SLOT(showPanel()));
    QObject::connect(ui->actionPreview_Panel, SIGNAL(triggered()),
                     this, SLOT(showPanel()));

    /* Connecting the document management actions */
    QObject::connect(ui->actionNew_Document, SIGNAL(triggered()),
                     this, SLOT(newTab()));
    QObject::connect(ui->actionToolbarNew, SIGNAL(triggered()),
                     this, SLOT(newTab()));

    QObject::connect(ui->actionClose_Document, SIGNAL(triggered()),
                     this, SLOT(closeCurrent()));

    QObject::connect(ui->actionSave_Document, SIGNAL(triggered()),
                     this, SLOT(saveCurrent()));
    QObject::connect(ui->actionSave_Document_As, SIGNAL(triggered()),
                     this, SLOT(saveCurrentAs()));
    QObject::connect(ui->actionToolbarSave, SIGNAL(triggered()),
                     this, SLOT(saveCurrent()));

}


void EditorWindow::newTab()
{
    SkinDocument* doc = new SkinDocument;
    ui->editorTabs->addTab(doc, doc->getTitle());

    /* Connecting to title change events */
    QObject::connect(doc, SIGNAL(titleChanged(QString)),
                     this, SLOT(tabTitleChanged(QString)));
}

void EditorWindow::shiftTab(int index)
{
    if(index < 0)
    {
        ui->parseTree->setModel(0);
        ui->actionSave_Document->setEnabled(false);
        ui->actionSave_Document_As->setEnabled(false);
        ui->actionClose_Document->setEnabled(false);
    }
    else
    {
        ui->parseTree->setModel(dynamic_cast<SkinDocument*>
                                (ui->editorTabs->currentWidget())->getModel());
        ui->actionSave_Document->setEnabled(true);
        ui->actionSave_Document_As->setEnabled(true);
        ui->actionClose_Document->setEnabled(true);
    }
}

bool EditorWindow::closeTab(int index)
{
    SkinDocument* widget = dynamic_cast<SkinDocument*>
                           (ui->editorTabs->widget(index));
    if(widget->requestClose())
    {
        ui->editorTabs->removeTab(index);
        widget->deleteLater();
        return true;
    }

    return false;
}

void EditorWindow::closeCurrent()
{
    closeTab(ui->editorTabs->currentIndex());
}

void EditorWindow::saveCurrent()
{
    if(ui->editorTabs->currentIndex() >= 0)
        dynamic_cast<SkinDocument*>(ui->editorTabs->currentWidget())->save();
}

void EditorWindow::saveCurrentAs()
{
    if(ui->editorTabs->currentIndex() >= 0)
        dynamic_cast<SkinDocument*>(ui->editorTabs->currentWidget())->saveAs();
}


void EditorWindow::tabTitleChanged(QString title)
{
    SkinDocument* sender = dynamic_cast<SkinDocument*>(QObject::sender());
    ui->editorTabs->setTabText(ui->editorTabs->indexOf(sender), title);
}

void EditorWindow::showPanel()
{
    if(sender() == ui->actionFile_Panel)
        ui->fileDock->setVisible(true);
    if(sender() == ui->actionPreview_Panel)
        ui->skinPreviewDock->setVisible(true);
    if(sender() == ui->actionDisplay_Panel)
        ui->parseTreeDock->setVisible(true);
}

void EditorWindow::closeEvent(QCloseEvent* event)
{

    saveSettings();

    /* Closing all the tabs */
    for(int i = 0; i < ui->editorTabs->count(); i++)
    {
        if(!dynamic_cast<SkinDocument*>
           (ui->editorTabs->widget(i))->requestClose())
        {
            event->ignore();
            return;
        }
    }

    event->accept();
}

EditorWindow::~EditorWindow()
{
    delete ui;
}
