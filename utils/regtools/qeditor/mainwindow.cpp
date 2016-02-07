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
#include <QWidget>
#include <QApplication>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QTabBar>
#include <QCloseEvent>
#include <QDebug>

#include "mainwindow.h"
#include "regtab.h"
#include "regedit.h"

/**
 * DocumentTab
 */
void DocumentTab::OnModified(bool modified)
{
    if(m_tab)
        m_tab->SetTabModified(this, modified);
}

void DocumentTab::SetTabWidget(DocumentTabWidget *tab)
{
    m_tab = tab;
    SetTabName(m_tabname);
}

void DocumentTab::SetTabName(const QString& name)
{
    m_tabname = name;
    if(m_tab)
        m_tab->SetTabName(this, name);
}

/**
 * DocumentTabWidget
 */

DocumentTabWidget::DocumentTabWidget()
{
    setMovable(true);
    setTabsClosable(true);
    connect(this, SIGNAL(tabCloseRequested(int)), this, SLOT(OnCloseTab(int)));
}

void DocumentTabWidget::SetTabModified(DocumentTab *doc, bool modified)
{
    int index = indexOf(doc->GetWidget());
    if(modified)
        setTabIcon(index, QIcon::fromTheme("document-save"));
    else
        setTabIcon(index, QIcon());
}

void DocumentTabWidget::SetTabName(DocumentTab *doc, const QString& name)
{
    setTabText(indexOf(doc->GetWidget()), name);
}

bool DocumentTabWidget::CloseTab(int index)
{
    QWidget *w = this->widget(index);
    DocumentTab *doc = dynamic_cast< DocumentTab* >(w);
    if(doc->Quit())
    {
        removeTab(index);
        delete w;
        return true;
    }
    else
        return false;
}

void DocumentTabWidget::OnCloseTab(int index)
{
    CloseTab(index);
}

/**
 * MainWindow
 */

MainWindow::MainWindow(Backend *backend)
    :m_backend(backend)
{
    QAction *new_regtab_act = new QAction(YIconManager::Get()->GetIcon(YIconManager::DocumentNew),
        tr("Register &Tab"), this);
    QAction *new_regedit_act = new QAction(YIconManager::Get()->GetIcon(YIconManager::DocumentEdit),
        tr("Register &Editor"), this);
    QAction *load_desc_act = new QAction(YIconManager::Get()->GetIcon(YIconManager::DocumentOpen),
        tr("&Soc Description"), this);
    QAction *quit_act = new QAction(YIconManager::Get()->GetIcon(YIconManager::ApplicationExit),
        tr("&Quit"), this);
    QAction *about_act = new QAction(YIconManager::Get()->GetIcon(YIconManager::HelpAbout),
        tr("&About"), this);
    QAction *about_qt_act = new QAction(YIconManager::Get()->GetIcon(YIconManager::HelpAbout),
        tr("About &Qt"), this);

    connect(new_regtab_act, SIGNAL(triggered()), this, SLOT(OnNewRegTab()));
    connect(new_regedit_act, SIGNAL(triggered()), this, SLOT(OnNewRegEdit()));
    connect(load_desc_act, SIGNAL(triggered()), this, SLOT(OnLoadDesc()));
    connect(quit_act, SIGNAL(triggered()), this, SLOT(OnQuit()));
    connect(about_act, SIGNAL(triggered()), this, SLOT(OnAbout()));
    connect(about_qt_act, SIGNAL(triggered()), this, SLOT(OnAboutQt()));

    QMenu *app_menu = new QMenu;
    QMenu *file_menu = app_menu->addMenu(tr("&File"));
    QMenu *new_submenu = file_menu->addMenu(YIconManager::Get()->GetIcon(YIconManager::DocumentNew), "&New");
    QMenu *load_submenu = file_menu->addMenu(YIconManager::Get()->GetIcon(YIconManager::DocumentOpen), "&Load");
    file_menu->addAction(quit_act);

    new_submenu->addAction(new_regtab_act);
    new_submenu->addAction(new_regedit_act);

    load_submenu->addAction(load_desc_act);

    QMenu *about_menu = app_menu->addMenu(tr("&About"));
    about_menu->addAction(about_act);
    about_menu->addAction(about_qt_act);

    m_tab = new DocumentTabWidget();
    m_tab->setTabOpenable(true);
    m_tab->setTabOpenMenu(new_submenu);
    m_tab->setOtherMenu(app_menu);

    setCentralWidget(m_tab);

    ReadSettings();
#ifdef HAVE_HWSTUB
    /* acquire hwstub manager */
    HWStubManager::Get()->setParent(this);
#endif
    /* acquire icon manager */
    YIconManager::Get()->setParent(this);

    OnNewRegTab();
}

void MainWindow::ReadSettings()
{
    restoreGeometry(Settings::Get()->value("mainwindow/geometry").toByteArray());
}

void MainWindow::WriteSettings()
{
    Settings::Get()->setValue("mainwindow/geometry", saveGeometry());
}

void MainWindow::OnQuit()
{
    close();
}

void MainWindow::OnAbout()
{
    QString soc_desc_ver = QString("%1.%2.%3").arg(soc_desc::MAJOR_VERSION)
        .arg(soc_desc::MINOR_VERSION).arg(soc_desc::REVISION_VERSION);
    QMessageBox::about(this, "About", 
        "<h1>QEditor</h1>"
        "<h2>Version " APP_VERSION "</h2>"
        "<p>Written by Amaury Pouly</p>"
        "<p>Libraries:</p>"
        "<ul><li>soc_desc: " + soc_desc_ver + "</li>"
#ifdef HAVE_HWSTUB
        "<li>hwstub: " HWSTUB_VERSION "</li>"
#else
        "<li>hwstub: not compiled in</li>"
#endif
        "</ul>");
}

void MainWindow::OnAboutQt()
{
    QMessageBox::aboutQt(this);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(!Quit())
        return event->ignore();
    WriteSettings();
    event->accept();
}

void MainWindow::OnLoadDesc()
{
    QFileDialog *fd = new QFileDialog(this);
    QStringList filters;
    filters << "XML files (*.xml)";
    filters << "All files (*)";
    fd->setNameFilters(filters);
    fd->setFileMode(QFileDialog::ExistingFiles);
    fd->setDirectory(Settings::Get()->value("loaddescdir", QDir::currentPath()).toString());
    if(fd->exec())
    {
        QStringList filenames = fd->selectedFiles();
        for(int i = 0; i < filenames.size(); i++)
            if(!m_backend->LoadSocDesc(filenames[i]))
            {
                QMessageBox msg;
                msg.setText(QString("Cannot load ") + filenames[i]);
                msg.exec();
            }
        Settings::Get()->setValue("loaddescdir", fd->directory().absolutePath());
    }
}

void MainWindow::AddTab(DocumentTab *doc)
{
    m_tab->setCurrentIndex(m_tab->addTab(doc->GetWidget(), ""));
    doc->SetTabWidget(m_tab);
}

void MainWindow::OnNewRegTab()
{
    AddTab(new RegTab(m_backend, this));
}

void MainWindow::OnNewRegEdit()
{
    AddTab(new RegEdit(m_backend, this));
}

bool MainWindow::Quit()
{
    while(m_tab->count() > 0)
    {
        if(!m_tab->CloseTab(0))
            return false;
    }
    return true;
}
