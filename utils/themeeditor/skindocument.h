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

#ifndef SKINDOCUMENT_H
#define SKINDOCUMENT_H

#include <QWidget>
#include <QHBoxLayout>
#include <QPlainTextEdit>

#include "skinhighlighter.h"
#include "parsetreemodel.h"

class SkinDocument : public QWidget
{
Q_OBJECT
public:
    static QString fileFilter()
    {
        return tr("WPS Files (*.wps *.rwps);;"
                  "SBS Files (*.sbs *.rsbs);;"
                  "FMS Files (*.fms *.rfms);;"
                  "All Skin Files (*.wps *.rwps *.sbs "
                  "*.rsbs *.fms *.rfms);;"
                  "All Files (*.*)");
    }

    SkinDocument(QWidget *parent = 0);
    virtual ~SkinDocument();

    ParseTreeModel* getModel(){ return model; }
    QString getTitle(){ return title; }
    void genCode(){ editor->document()->setPlainText(model->genCode()); }

    void save();
    void saveAs();

    bool requestClose();

signals:
    void titleChanged(QString);

private slots:
    void codeChanged();

private:
    void setupUI();

    QString title;
    QString fileName;
    QString saved;

    QLayout* layout;
    QPlainTextEdit* editor;

    SkinHighlighter* highlighter;
    ParseTreeModel* model;
};

#endif // SKINDOCUMENT_H
