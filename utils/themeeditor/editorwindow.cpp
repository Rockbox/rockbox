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

    /* Connecting the document opening/closing actions */
    QObject::connect(ui->actionNew_Document, SIGNAL(triggered()),
                     this, SLOT(newTab()));
}


void EditorWindow::newTab()
{
    SkinDocument* doc = new SkinDocument;
    ui->editorTabs->addTab(doc, doc->getTitle());
}

void EditorWindow::shiftTab(int index)
{
    if(index < 0)
        ui->parseTree->setModel(0);
    else
        ui->parseTree->setModel(dynamic_cast<SkinDocument*>
                                (ui->editorTabs->currentWidget())->getModel());
}

void EditorWindow::closeTab(int index)
{
    SkinDocument* widget = dynamic_cast<SkinDocument*>
                           (ui->editorTabs->widget(index));
    if(widget->requestClose())
    {
        ui->editorTabs->removeTab(index);
        widget->deleteLater();
    }
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
    event->accept();
}

EditorWindow::~EditorWindow()
{
    delete ui;
}
