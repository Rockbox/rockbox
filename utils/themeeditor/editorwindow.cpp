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
    /* When there are settings to load, they'll be loaded here */
    /* For now, we'll just set the window to take up most of the screen */
    QDesktopWidget* desktop = QApplication::desktop();

    QRect availableSpace = desktop->availableGeometry(desktop->primaryScreen());
    QRect buffer(availableSpace.left() + availableSpace.width() / 10,
                 availableSpace.top() + availableSpace.height() / 10,
                 availableSpace.width() * 8 / 10,
                 availableSpace.height() * 8 / 10);
    this->setGeometry(buffer);

}

void EditorWindow::setupUI()
{
    /* Displaying some files to test the file tree view */
    QFileSystemModel* model = new QFileSystemModel;
    model->setRootPath(QDir::currentPath());
    ui->fileTree->setModel(model);

    /* Connecting the buttons */
    QObject::connect(ui->fromTree, SIGNAL(pressed()),
                     this, SLOT(updateCode()));

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
    event->accept();
}

EditorWindow::~EditorWindow()
{
    delete ui;
}
