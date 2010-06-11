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
#include <QLabel>
#include <QHBoxLayout>

#include "skinhighlighter.h"
#include "parsetreemodel.h"
#include "preferencesdialog.h"
#include "codeeditor.h"

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

    SkinDocument(QLabel* statusLabel, QWidget *parent = 0);
    SkinDocument(QLabel* statusLabel, QString file, QWidget* parent = 0);
    virtual ~SkinDocument();

    void connectPrefs(PreferencesDialog* prefs);

    ParseTreeModel* getModel(){ return model; }
    QString getFile(){ return fileName; }
    QString getTitle(){ return title; }
    QString getStatus(){ return parseStatus; }
    void genCode(){ editor->document()->setPlainText(model->genCode()); }

    void save();
    void saveAs();

    bool requestClose();

signals:
    void titleChanged(QString);

public slots:
    void settingsChanged();

private slots:
    void codeChanged();

private:
    void setupUI();

    QString title;
    QString fileName;
    QString saved;
    QString parseStatus;

    QLayout* layout;
    CodeEditor* editor;

    SkinHighlighter* highlighter;
    ParseTreeModel* model;

    QLabel* statusLabel;

    bool blockUpdate;
};

#endif // SKINDOCUMENT_H
