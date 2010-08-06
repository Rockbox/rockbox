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
#include <QGraphicsScene>
#include <QTime>
#include <QTimer>

#include "findreplacedialog.h"

#include "skinhighlighter.h"
#include "parsetreemodel.h"
#include "preferencesdialog.h"
#include "codeeditor.h"
#include "tabcontent.h"
#include "projectmodel.h"
#include "devicestate.h"

class SkinDocument : public TabContent
{
Q_OBJECT
public:
    static QString fileFilter()
    {
        return tr("WPS Files (*.wps *.WPS *.rwps *.RWPS);;"
                  "SBS Files (*.sbs *.SBS *.rsbs *.RSBS);;"
                  "FMS Files (*.fms *.FMS *.rfms *.RFMS);;"
                  "All Skin Files (*.wps *.WPS *.rwps *.RWPS *.sbs *.SBS "
                  "*.rsbs *.RSBS *.fms *.FMS *.rfms *.RFMS);;"
                  "All Files (*)");
    }

    SkinDocument(QLabel* statusLabel, ProjectModel* project = 0,
                 DeviceState* device = 0, QWidget *parent = 0);
    SkinDocument(QLabel* statusLabel, QString file, ProjectModel* project = 0,
                 DeviceState* device = 0, QWidget* parent = 0);
    virtual ~SkinDocument();

    void connectPrefs(PreferencesDialog* prefs);

    ParseTreeModel* getModel(){ return model; }
    QString file() const{ return fileName; }
    QString title() const{ return titleText; }
    QString getStatus(){ return parseStatus; }
    CodeEditor* getEditor(){ return editor; }
    void setProject(ProjectModel* project){ this->project = project; }

    void save();
    void saveAs();

    bool requestClose();

    TabType type() const{ return Skin; }

    RBScene* scene()
    {
        return model->render(project, device, this, &fileName);
    }

    void showFind(){ findReplace->show(); }
    void hideFind(){ findReplace->hide(); }

    bool isSynced(){ return treeInSync; }


signals:
    void antiSync(bool outOfSync);

public slots:
    void settingsChanged();
    void cursorChanged();
    void parseCode(){ codeChanged(); }
    void genCode(){ editor->document()->setPlainText(model->genCode()); }

private slots:
    void codeChanged();
    void modelChanged();
    void deviceChanged(){ scene(); }

private:
    void setupUI();
    QString findSetting(QString key, QString fallback);

    QString titleText;
    QString fileName;
    QString saved;
    QString parseStatus;
    int currentLine;

    QLayout* layout;
    CodeEditor* editor;

    SkinHighlighter* highlighter;
    ParseTreeModel* model;

    QLabel* statusLabel;

    bool blockUpdate;

    ProjectModel* project;
    DeviceState* device;

    FindReplaceDialog* findReplace;

    QTime lastUpdate;
    static const int updateInterval;
    QTimer checkUpdate;

    bool treeInSync;
};

#endif // SKINDOCUMENT_H
