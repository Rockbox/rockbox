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

#include "parsetreemodel.h"
#include "skinhighlighter.h"
#include "skindocument.h"
#include "preferencesdialog.h"
#include "projectmodel.h"

namespace Ui {
    class EditorWindow;
}

class EditorWindow : public QMainWindow {
    Q_OBJECT
public:
    EditorWindow(QWidget *parent = 0);
    ~EditorWindow();

protected:
    virtual void closeEvent(QCloseEvent* event);

private slots:
    void showPanel();
    void newTab();
    void shiftTab(int index);
    bool closeTab(int index);
    void closeCurrent();
    void saveCurrent();
    void saveCurrentAs();
    void openFile();
    void openProject();
    void tabTitleChanged(QString title);
    void updateCurrent(); /* Generates code in the current tab */

private:
    /* Setup functions */
    void loadSettings();
    void saveSettings();
    void setupUI();
    void setupMenus();
    void addTab(SkinDocument* doc);

    Ui::EditorWindow *ui;
    PreferencesDialog* prefs;
    QLabel* parseStatus;
    ProjectModel* project;
};

#endif // EDITORWINDOW_H
