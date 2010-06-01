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

    tree = 0;

    /* Connecting the buttons */
    QObject::connect(ui->code, SIGNAL(cursorPositionChanged()),
                     this, SLOT(updateTree()));
    QObject::connect(ui->fromTree, SIGNAL(pressed()),
                     this, SLOT(updateCode()));
}

void EditorWindow::updateTree()
{
    if(tree)
        delete tree;

    tree = new ParseTreeModel(ui->code->document()->toPlainText().toAscii());
    ui->parseTree->setModel(tree);
    ui->parseTree->expandAll();
}

void EditorWindow::updateCode()
{
    tree->genCode();
    ui->code->setDocument(new QTextDocument(tree->genCode()));
}

EditorWindow::~EditorWindow()
{
    delete ui;
    if(tree)
        delete tree;
}
