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

#include "skindocument.h"

SkinDocument::SkinDocument(QWidget *parent) :
    QWidget(parent)
{
    setupUI();

    title = "Untitled";
}

SkinDocument::~SkinDocument()
{
    delete highlighter;
    delete model;
}

void SkinDocument::setupUI()
{
    /* Setting up the text edit */
    layout = new QHBoxLayout;
    editor = new QPlainTextEdit(this);
    layout->addWidget(editor);

    setLayout(layout);

    /* Attaching the syntax highlighter */
    highlighter = new SkinHighlighter(QColor(0,180,0), QColor(255,0,0),
                                      QColor(0,0,255), QColor(120,120,120),
                                      editor->document());

    /* Setting up the model */
    model = new ParseTreeModel("");

    /* Connecting the editor's signal */
    QObject::connect(editor, SIGNAL(textChanged()),
                     this, SLOT(codeChanged()));
}

void SkinDocument::codeChanged()
{
    model->changeTree(editor->document()->toPlainText().toAscii());
}
