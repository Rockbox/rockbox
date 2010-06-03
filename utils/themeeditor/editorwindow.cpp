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

#include <iostream>

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
}

void EditorWindow::setupUI()
{
    /* Establishing the parse tree */
    tree = new ParseTreeModel(ui->codeEdit->document()->toPlainText().
                              toAscii());
    ui->parseTree->setModel(tree);

    /* Setting up the syntax highlighter */
    highlighter = new SkinHighlighter(QColor(0,255,0), QColor(255,0,0),
                                      QColor(0,0,255), QColor(150,150,150),
                                      ui->codeEdit->document());

    /* Connecting the buttons */
    QObject::connect(ui->codeEdit, SIGNAL(cursorPositionChanged()),
                     this, SLOT(codeChanged()));
    QObject::connect(ui->fromTree, SIGNAL(pressed()),
                     this, SLOT(updateCode()));

}

void EditorWindow::setupMenus()
{
    /* When there are menus to setup, they'll be set up here */
}

void EditorWindow::codeChanged()
{
    tree->changeTree(ui->codeEdit->document()->toPlainText().toAscii());
    ui->parseTree->expandAll();
}

void EditorWindow::updateCode()
{
    tree->genCode();
    ui->codeEdit->document()->setPlainText(tree->genCode());
}

EditorWindow::~EditorWindow()
{
    delete ui;
    if(tree)
        delete tree;
}
