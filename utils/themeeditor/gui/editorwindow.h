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

#ifndef EDITORWINDOW_H
#define EDITORWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QItemSelectionModel>
#include <QDockWidget>

#include "parsetreemodel.h"
#include "skinhighlighter.h"
#include "skindocument.h"
#include "configdocument.h"
#include "preferencesdialog.h"
#include "skinviewer.h"
#include "devicestate.h"
#include "skintimer.h"
#include "targetdata.h"

class ProjectModel;
class TabContent;

namespace Ui
{
    class EditorWindow;
}

class EditorWindow : public QMainWindow
{
    Q_OBJECT
public:
    static const int numRecent;

    EditorWindow(QWidget *parent = 0);
    virtual ~EditorWindow();

    /* A public function so external widgets can load files */
    void loadTabFromSkinFile(QString fileName);
    void loadConfigTab(ConfigDocument* doc);

protected:
    virtual void closeEvent(QCloseEvent* event);

public slots:
    void configFileChanged(QString configFile);

private slots:
    void showPanel();
    void newTab();
    void newProject();
    void shiftTab(int index);
    bool closeTab(int index);
    void closeCurrent();
    void closeProject();
    void saveCurrent();
    void saveCurrentAs();
    void exportProject();
    void openFile();
    void openProject();
    void openRecentFile();
    void openRecentProject();
    void tabTitleChanged(QString title);
    void updateCurrent(); /* Generates code in the current tab */
    void lineChanged(int line); /* Used for auto-expand */
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void findReplace();

private:
    /* Setup functions */
    void loadSettings();
    void saveSettings();
    void setupUI();
    void setupMenus();

    void addTab(TabContent* doc);
    void expandLine(ParseTreeModel* model, QModelIndex parent, int line,
                    bool highlight);
    void sizeColumns();

    void loadProjectFile(QString fileName);
    static void createFile(QString filename, QString contents);

    /* Functions to manipulate the recent projects/documents menus */
    void docToTop(QString file);
    void projectToTop(QString file);
    void refreshRecentMenus();

    Ui::EditorWindow *ui;
    PreferencesDialog* prefs;
    QLabel* parseStatus;
    ProjectModel* project;
    QItemSelectionModel* parseTreeSelection;
    SkinViewer* viewer;
    DeviceState* deviceConfig;
    QDockWidget* deviceDock;
    SkinTimer* timer;
    QDockWidget* timerDock;

    QStringList recentDocs;
    QStringList recentProjects;
    QList<QAction*> recentDocsMenu;
    QList<QAction*> recentProjectsMenu;
};

#endif // EDITORWINDOW_H
