/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Amaury Pouly
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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QSettings>
#include "backend.h"
#include "settings.h"
#include "utils.h"

class DocumentTabWidget;

class DocumentTab
{
public:
    DocumentTab() { m_tab = 0; }
    virtual bool Quit() = 0;
    virtual QWidget *GetWidget() = 0;
    void SetTabWidget(DocumentTabWidget *tab);

protected:
    void OnModified(bool modified);
    void SetTabName(const QString& name);
    DocumentTabWidget *m_tab;
    QString m_tabname;
};

class DocumentTabWidget : public YTabWidget
{
    Q_OBJECT
public:
    DocumentTabWidget();
    bool CloseTab(int index);
    void SetTabModified(DocumentTab *tab, bool mod);
    void SetTabName(DocumentTab *tab, const QString& name);

private slots:
    void OnCloseTab(int index);
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(Backend *backend);
    void center();
    void ReadSettings();
    void WriteSettings();

private:
    void closeEvent(QCloseEvent *event);

protected:
    void AddTab(DocumentTab *tab);
    bool Quit();

private slots:
    void OnQuit();
    void OnAbout();
    void OnAboutQt();
    void OnLoadDesc();
    void OnNewRegTab();
    void OnNewRegEdit();

private:
    DocumentTabWidget *m_tab;
    Backend *m_backend;
};

#endif 
