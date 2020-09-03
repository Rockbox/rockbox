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
#include "projectmodel.h"
#include "ui_editorwindow.h"
#include "rbfontcache.h"
#include "rbtextcache.h"
#include "newprojectdialog.h"
#include "projectexporter.h"

#include <QDesktopWidget>
#include <QFileSystemModel>
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QGraphicsScene>
#include <QDir>
#include <QFile>

const int EditorWindow::numRecent = 5;

EditorWindow::EditorWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::EditorWindow), parseTreeSelection(0)
{
    ui->setupUi(this);
    prefs = new PreferencesDialog(this);
    project = 0;
    setupUI();
    loadSettings();
    setupMenus();
}


EditorWindow::~EditorWindow()
{
    delete ui;
    delete prefs;
    if(project)
        delete project;
    delete deviceConfig;
    delete deviceDock;
    delete timer;
    delete timerDock;

    RBFontCache::clearCache();
    RBTextCache::clearCache();
}

void EditorWindow::loadTabFromSkinFile(QString fileName)
{
    docToTop(fileName);

    /* Checking to see if the file is already open */
    for(int i = 0; i < ui->editorTabs->count(); i++)
    {
        TabContent* current = dynamic_cast<TabContent*>
                                (ui->editorTabs->widget(i));
        if(current->file() == fileName)
        {
            ui->editorTabs->setCurrentIndex(i);
            return;
        }
    }

    /* Adding a new document*/
    SkinDocument* doc = new SkinDocument(parseStatus, fileName, project,
                                         deviceConfig);
    addTab(doc);
    ui->editorTabs->setCurrentWidget(doc);

}

void EditorWindow::loadConfigTab(ConfigDocument* doc)
{
    for(int i = 0; i < ui->editorTabs->count(); i++)
    {
        TabContent* current = dynamic_cast<TabContent*>
                              (ui->editorTabs->widget(i));
        if(current->file() == doc->file())
        {
            ui->editorTabs->setCurrentIndex(i);
            doc->deleteLater();
            return;
        }
    }

    addTab(doc);
    ui->editorTabs->setCurrentWidget(doc);

    QObject::connect(doc, SIGNAL(titleChanged(QString)),
                     this, SLOT(tabTitleChanged(QString)));
}

void  EditorWindow::loadSettings()
{

    QSettings settings;

    /* Main Window location */
    settings.beginGroup("EditorWindow");
    QSize size = settings.value("size").toSize();
    QPoint pos = settings.value("position").toPoint();
    QByteArray state = settings.value("state").toByteArray();

    /* Recent docs/projects */
    recentDocs = settings.value("recentDocs", QStringList()).toStringList();
    recentProjects = settings.value("recentProjects",
                                    QStringList()).toStringList();

    settings.endGroup();

    if(!(size.isNull() || pos.isNull() || state.isNull()))
    {
        resize(size);
        move(pos);
        restoreState(state);
    }

}

void EditorWindow::saveSettings()
{

    QSettings settings;

    /* Saving window and panel positions */
    settings.beginGroup("EditorWindow");
    settings.setValue("position", pos());
    settings.setValue("size", size());
    settings.setValue("state", saveState());

    /* Saving recent docs/projects */
    settings.setValue("recentDocs", recentDocs);
    settings.setValue("recentProjects", recentProjects);

    settings.endGroup();
}

void EditorWindow::setupUI()
{
    /* Connecting the tab bar signals */
    QObject::connect(ui->editorTabs, SIGNAL(currentChanged(int)),
                     this, SLOT(shiftTab(int)));
    QObject::connect(ui->editorTabs, SIGNAL(tabCloseRequested(int)),
                     this, SLOT(closeTab(int)));

    /* Connecting the code gen button */
    QObject::connect(ui->fromTree, SIGNAL(pressed()),
                     this, SLOT(updateCurrent()));

    /* Connecting the preferences dialog */
    QObject::connect(ui->actionPreferences, SIGNAL(triggered()),
                     prefs, SLOT(exec()));

    /* Setting up the parse status label */
    parseStatus = new QLabel(this);
    ui->statusbar->addPermanentWidget(parseStatus);

    /* Setting the selection for parse tree highlighting initially NULL */
    parseTreeSelection = 0;

    /* Adding the skin viewer */
    viewer = new SkinViewer(this);
    ui->skinPreviewLayout->addWidget(viewer);

    /* Positioning the device settings dialog */
    deviceDock = new QDockWidget(tr("Device Configuration"), this);
    deviceConfig = new DeviceState(deviceDock);

    deviceDock->setObjectName("deviceDock");
    deviceDock->setWidget(deviceConfig);
    deviceDock->setFloating(true);
    deviceDock->move(QPoint(x() + width() / 2, y() + height() / 2));
    deviceDock->hide();

    /* Positioning the timer panel */
    timerDock = new QDockWidget(tr("Timer"), this);
    timer = new SkinTimer(deviceConfig, timerDock);

    timerDock->setObjectName("timerDock");
    timerDock->setWidget(timer);
    timerDock->setFloating(true);
    timerDock->move(QPoint(x() + width() / 2, y() + height() / 2));
    timerDock->hide();

    shiftTab(-1);
}

void EditorWindow::setupMenus()
{
    /* Adding actions to the toolbar */
    ui->toolBar->addAction(ui->actionNew_Document);
    ui->toolBar->addAction(ui->actionOpen_Document);
    ui->toolBar->addAction(ui->actionSave_Document);
    ui->toolBar->addAction(ui->actionSave_Document_As);

    ui->toolBar->addSeparator();
    ui->toolBar->addAction(ui->actionUndo);
    ui->toolBar->addAction(ui->actionRedo);

    ui->toolBar->addSeparator();
    ui->toolBar->addAction(ui->actionCut);
    ui->toolBar->addAction(ui->actionCopy);
    ui->toolBar->addAction(ui->actionPaste);

    ui->toolBar->addSeparator();
    ui->toolBar->addAction(ui->actionFind_Replace);

    /* Connecting panel show actions */
    QObject::connect(ui->actionFile_Panel, SIGNAL(triggered()),
                     this, SLOT(showPanel()));
    QObject::connect(ui->actionDisplay_Panel, SIGNAL(triggered()),
                     this, SLOT(showPanel()));
    QObject::connect(ui->actionPreview_Panel, SIGNAL(triggered()),
                     this, SLOT(showPanel()));
    QObject::connect(ui->actionDevice_Configuration, SIGNAL(triggered()),
                     deviceDock, SLOT(show()));
    QObject::connect(ui->actionTimer, SIGNAL(triggered()),
                     timerDock, SLOT(show()));

    /* Connecting the document management actions */
    QObject::connect(ui->actionNew_Document, SIGNAL(triggered()),
                     this, SLOT(newTab()));
    QObject::connect(ui->actionNew_Project, SIGNAL(triggered()),
                     this, SLOT(newProject()));

    QObject::connect(ui->actionClose_Document, SIGNAL(triggered()),
                     this, SLOT(closeCurrent()));
    QObject::connect(ui->actionClose_Project, SIGNAL(triggered()),
                     this, SLOT(closeProject()));

    QObject::connect(ui->actionSave_Document, SIGNAL(triggered()),
                     this, SLOT(saveCurrent()));
    QObject::connect(ui->actionSave_Document_As, SIGNAL(triggered()),
                     this, SLOT(saveCurrentAs()));
    QObject::connect(ui->actionExport_Project, SIGNAL(triggered()),
                     this, SLOT(exportProject()));

    QObject::connect(ui->actionOpen_Document, SIGNAL(triggered()),
                     this, SLOT(openFile()));

    QObject::connect(ui->actionOpen_Project, SIGNAL(triggered()),
                     this, SLOT(openProject()));

    /* Connecting the edit menu */
    QObject::connect(ui->actionUndo, SIGNAL(triggered()),
                     this, SLOT(undo()));
    QObject::connect(ui->actionRedo, SIGNAL(triggered()),
                     this, SLOT(redo()));
    QObject::connect(ui->actionCut, SIGNAL(triggered()),
                     this, SLOT(cut()));
    QObject::connect(ui->actionCopy, SIGNAL(triggered()),
                     this, SLOT(copy()));
    QObject::connect(ui->actionPaste, SIGNAL(triggered()),
                     this, SLOT(paste()));
    QObject::connect(ui->actionFind_Replace, SIGNAL(triggered()),
                     this, SLOT(findReplace()));

    /* Adding the recent docs/projects menus */
    for(int i = 0; i < numRecent; i++)
    {
        recentDocsMenu.append(new QAction(tr("Recent Doc"),
                                          ui->menuRecent_Files));
        recentDocsMenu.last()
                ->setShortcut(QKeySequence(tr("CTRL+")
                                           + QString::number(i + 1)));
        QObject::connect(recentDocsMenu.last(), SIGNAL(triggered()),
                         this, SLOT(openRecentFile()));
        ui->menuRecent_Files->addAction(recentDocsMenu.last());


        recentProjectsMenu.append(new QAction(tr("Recent Project"),
                                              ui->menuRecent_Projects));
        recentProjectsMenu.last()
                ->setShortcut(QKeySequence(tr("CTRL+SHIFT+") +
                                           QString::number(i + 1)));
        QObject::connect(recentProjectsMenu.last(), SIGNAL(triggered()),
                         this, SLOT(openRecentProject()));
        ui->menuRecent_Projects->addAction(recentProjectsMenu.last());
    }
    refreshRecentMenus();
}

void EditorWindow::addTab(TabContent *doc)
{
    ui->editorTabs->addTab(doc, doc->title());

    /* Connecting to title change events */
    QObject::connect(doc, SIGNAL(titleChanged(QString)),
                     this, SLOT(tabTitleChanged(QString)));
    QObject::connect(doc, SIGNAL(lineChanged(int)),
                     this, SLOT(lineChanged(int)));

    /* Connecting to settings change events */
    doc->connectPrefs(prefs);
}


void EditorWindow::newTab()
{
    SkinDocument* doc = new SkinDocument(parseStatus, project, deviceConfig);
    addTab(doc);
    ui->editorTabs->setCurrentWidget(doc);
}

void EditorWindow::newProject()
{
    NewProjectDialog dialog(this);
    if(dialog.exec() == QDialog::Rejected)
        return;

    /* Assembling the new project if the dialog was accepted */
    NewProjectDialog::NewProjectInfo info = dialog.results();

    QDir path(info.path);
    if(!path.exists())
    {
        QMessageBox::warning(this, tr("Error Creating Project"), tr("Error:"
                             " Project directory does not exist"));
        return;
    }

    if(path.exists(info.name))
    {
        QMessageBox::warning(this, tr("Error Creating Project"), tr("Error:"
                             " Project directory already exists"));
        return;
    }

    if(!path.mkdir(info.name))
    {
        QMessageBox::warning(this, tr("Error Creating Project"), tr("Error:"
                             " Project directory not writeable"));
        return;
    }

    path.cd(info.name);

    /* Making standard directories */
    path.mkdir("fonts");
    path.mkdir("icons");
    path.mkdir("backdrops");

    /* Adding the desired wps documents */
    path.mkdir("wps");
    path.cd("wps");

    if(info.sbs)
        createFile(path.filePath(info.name + ".sbs"), tr("# SBS Document\n"));
    if(info.wps)
        createFile(path.filePath(info.name + ".wps"), tr("# WPS Document\n"));
    if(info.fms)
        createFile(path.filePath(info.name + ".fms"), tr("# FMS Document\n"));
    if(info.rsbs)
        createFile(path.filePath(info.name + ".rsbs"), tr("# RSBS Document\n"));
    if(info.rwps)
        createFile(path.filePath(info.name + ".rwps"), tr("# RWPS Document\n"));
    if(info.rfms)
        createFile(path.filePath(info.name + ".rfms"), tr("# RFMS Document\n"));

    path.mkdir(info.name);

    path.cdUp();

    /* Adding the config file */
    path.mkdir("themes");
    path.cd("themes");

    /* Generating the config file */
    QString config = tr("# Config file for ") + info.name + "\n";
    config.append("#target: " + info.target + "\n\n");
    QString wpsBase = "/.rockbox/wps/";
    if(info.sbs)
        config.append("sbs: " + wpsBase + info.name + ".sbs\n");
    if(info.wps)
        config.append("wps: " + wpsBase + info.name + ".wps\n");
    if(info.fms)
        config.append("fms: " + wpsBase + info.name + ".fms\n");
    if(info.rsbs)
        config.append("rsbs: " + wpsBase + info.name + ".rsbs\n");
    if(info.rwps)
        config.append("rwps: " + wpsBase + info.name + ".rwps\n");
    if(info.rfms)
        config.append("rfms: " + wpsBase + info.name + ".rfms\n");


    createFile(path.filePath(info.name + ".cfg"),
               config);

    /* Opening the new project */
    loadProjectFile(path.filePath(info.name + ".cfg"));
}

void EditorWindow::shiftTab(int index)
{
    TabContent* widget = dynamic_cast<TabContent*>
                         (ui->editorTabs->currentWidget());
    if(index < 0)
    {
        ui->parseTree->setModel(0);
        ui->actionSave_Document->setEnabled(false);
        ui->actionSave_Document_As->setEnabled(false);
        ui->actionClose_Document->setEnabled(false);
        ui->fromTree->setEnabled(false);
        ui->actionUndo->setEnabled(false);
        ui->actionRedo->setEnabled(false);
        ui->actionCut->setEnabled(false);
        ui->actionCopy->setEnabled(false);
        ui->actionPaste->setEnabled(false);
        ui->actionFind_Replace->setEnabled(false);
        viewer->connectSkin(0);
    }
    else if(widget->type() == TabContent::Config)
    {
        ui->actionSave_Document->setEnabled(true);
        ui->actionSave_Document_As->setEnabled(true);
        ui->actionClose_Document->setEnabled(true);
        ui->actionUndo->setEnabled(false);
        ui->actionRedo->setEnabled(false);
        ui->actionCut->setEnabled(false);
        ui->actionCopy->setEnabled(false);
        ui->actionPaste->setEnabled(false);
        ui->actionFind_Replace->setEnabled(false);
        viewer->connectSkin(0);
    }
    else if(widget->type() == TabContent::Skin)
    {
        /* Syncing the tree view and the status bar */
        SkinDocument* doc = dynamic_cast<SkinDocument*>(widget);
        ui->parseTree->setModel(doc->getModel());
        parseStatus->setText(doc->getStatus());

        ui->actionSave_Document->setEnabled(true);
        ui->actionSave_Document_As->setEnabled(true);
        ui->actionClose_Document->setEnabled(true);
        ui->fromTree->setEnabled(true);

        ui->actionUndo->setEnabled(true);
        ui->actionRedo->setEnabled(true);
        ui->actionCut->setEnabled(true);
        ui->actionCopy->setEnabled(true);
        ui->actionPaste->setEnabled(true);
        ui->actionFind_Replace->setEnabled(true);

        sizeColumns();

        /* Syncing the preview */
        viewer->connectSkin(doc);


    }

    /* Hiding all the find/replace dialogs */
    for(int i = 0; i < ui->editorTabs->count(); i++)
        if(dynamic_cast<TabContent*>(ui->editorTabs->widget(i))->type() ==
           TabContent::Skin)
            dynamic_cast<SkinDocument*>(ui->editorTabs->widget(i))->hideFind();

}

bool EditorWindow::closeTab(int index)
{
    TabContent* widget = dynamic_cast<TabContent*>
                           (ui->editorTabs->widget(index));
    if(widget->requestClose())
    {
        ui->editorTabs->removeTab(index);
        widget->deleteLater();
        return true;
    }

    return false;
}

void EditorWindow::closeCurrent()
{
    closeTab(ui->editorTabs->currentIndex());
}

void EditorWindow::closeProject()
{
    if(project)
    {
        project->deleteLater();
        project = 0;
    }

    for(int i = 0; i < ui->editorTabs->count(); i++)
    {
        TabContent* doc = dynamic_cast<TabContent*>
                          (ui->editorTabs->widget(i));
        if(doc->type() == TabContent::Skin)
        {
            dynamic_cast<SkinDocument*>(doc)->setProject(project);
            if(i == ui->editorTabs->currentIndex())
            {
                viewer->connectSkin(dynamic_cast<SkinDocument*>(doc));
            }
        }
    }

    ui->actionClose_Project->setEnabled(false);
    ui->actionExport_Project->setEnabled(false);
}

void EditorWindow::saveCurrent()
{
    if(ui->editorTabs->currentIndex() >= 0)
        dynamic_cast<TabContent*>(ui->editorTabs->currentWidget())->save();
}

void EditorWindow::saveCurrentAs()
{
    if(ui->editorTabs->currentIndex() >= 0)
        dynamic_cast<TabContent*>(ui->editorTabs->currentWidget())->saveAs();
}

void EditorWindow::exportProject()
{
    QDir dir = project->getSetting("themebase", "");
    dir.cdUp();
    QString file = project->getSetting("configfile", "").split("/").
                   last().split(".").first() + ".zip";
    file = dir.filePath(file);

    file = QFileDialog::getSaveFileName(this, tr("Export Project"),
                                        file, "Zip Files (*.zip *.ZIP);;"
                                              "All Files (*)");

    if(file != "")
    {
        ProjectExporter* exporter = new ProjectExporter(file, project, this);
        exporter->show();
    }
}

void EditorWindow::openFile()
{
    QStringList fileNames;
    QSettings settings;

    settings.beginGroup("SkinDocument");
    QString directory = settings.value("defaultDirectory", "").toString();
    fileNames = QFileDialog::getOpenFileNames(this, tr("Open Files"), directory,
                                              SkinDocument::fileFilter());

    for(int i = 0; i < fileNames.count(); i++)
    {
        if(!QFile::exists(fileNames[i]))
            continue;

        QString current = fileNames[i];

        loadTabFromSkinFile(current);

        /* And setting the new default directory */
        current.chop(current.length() - current.lastIndexOf('/') - 1);
        settings.setValue("defaultDirectory", current);

    }

    settings.endGroup();
}

void EditorWindow::openProject()
{
    QString fileName;
    QSettings settings;

    settings.beginGroup("ProjectModel");
    QString directory = settings.value("defaultDirectory", "").toString();
    fileName = QFileDialog::getOpenFileName(this, tr("Open Project"), directory,
                                            ProjectModel::fileFilter());

    settings.endGroup();
    loadProjectFile(fileName);

}

void EditorWindow::openRecentFile()
{
    loadTabFromSkinFile(dynamic_cast<QAction*>(QObject::sender())->text());
}

void EditorWindow::openRecentProject()
{
    loadProjectFile(dynamic_cast<QAction*>(QObject::sender())->text());
}

void EditorWindow::configFileChanged(QString configFile)
{

    if(QFile::exists(configFile))
    {

        if(project)
            delete project;

        project = new ProjectModel(configFile, this);
        ui->projectTree->setModel(project);

        QObject::connect(ui->projectTree, SIGNAL(activated(QModelIndex)),
                         project, SLOT(activated(QModelIndex)));

        for(int i = 0; i < ui->editorTabs->count(); i++)
        {
            TabContent* doc = dynamic_cast<TabContent*>
                              (ui->editorTabs->widget(i));
            if(doc->type() == TabContent::Skin)
            {
                dynamic_cast<SkinDocument*>(doc)->setProject(project);
                if(i == ui->editorTabs->currentIndex())
                {
                    viewer->connectSkin(dynamic_cast<SkinDocument*>(doc));
                }
            }
        }

    }


}

void EditorWindow::tabTitleChanged(QString title)
{
    TabContent* sender = dynamic_cast<TabContent*>(QObject::sender());
    ui->editorTabs->setTabText(ui->editorTabs->indexOf(sender), title);
}

void EditorWindow::showPanel()
{
    if(sender() == ui->actionFile_Panel)
        ui->projectDock->setVisible(true);
    if(sender() == ui->actionPreview_Panel)
        ui->skinPreviewDock->setVisible(true);
    if(sender() == ui->actionDisplay_Panel)
        ui->parseTreeDock->setVisible(true);
}

void EditorWindow::closeEvent(QCloseEvent* event)
{

    saveSettings();

    /* Closing all the tabs */
    for(int i = 0; i < ui->editorTabs->count(); i++)
    {
        if(!dynamic_cast<TabContent*>
           (ui->editorTabs->widget(i))->requestClose())
        {
            event->ignore();
            return;
        }
    }

    event->accept();
}

void EditorWindow::updateCurrent()
{
    if(ui->editorTabs->currentIndex() < 0)
        return;

    dynamic_cast<SkinDocument*>
            (ui->editorTabs->currentWidget())->genCode();
}

void EditorWindow::lineChanged(int line)
{
    QSettings settings;
    settings.beginGroup("EditorWindow");

    if(settings.value("autoExpandTree", false).toBool())
    {
        ui->parseTree->collapseAll();
        ParseTreeModel* model = dynamic_cast<ParseTreeModel*>
                                (ui->parseTree->model());
        parseTreeSelection = new QItemSelectionModel(model);
        expandLine(model, QModelIndex(), line,
                   settings.value("autoHighlightTree", false).toBool());
        sizeColumns();
        ui->parseTree->setSelectionModel(parseTreeSelection);
    }

    settings.endGroup();
}

void EditorWindow::undo()
{
    TabContent* doc = dynamic_cast<TabContent*>
                      (ui->editorTabs->currentWidget());
    if(doc->type() == TabContent::Skin)
        dynamic_cast<SkinDocument*>(doc)->getEditor()->undo();
}

void EditorWindow::redo()
{
    TabContent* doc = dynamic_cast<TabContent*>
                      (ui->editorTabs->currentWidget());
    if(doc->type() == TabContent::Skin)
        dynamic_cast<SkinDocument*>(doc)->getEditor()->redo();

}

void EditorWindow::cut()
{
    TabContent* doc = dynamic_cast<TabContent*>
                      (ui->editorTabs->currentWidget());
    if(doc->type() == TabContent::Skin)
        dynamic_cast<SkinDocument*>(doc)->getEditor()->cut();
}

void EditorWindow::copy()
{
    TabContent* doc = dynamic_cast<TabContent*>
                      (ui->editorTabs->currentWidget());
    if(doc->type() == TabContent::Skin)
        dynamic_cast<SkinDocument*>(doc)->getEditor()->copy();
}

void EditorWindow::paste()
{
    TabContent* doc = dynamic_cast<TabContent*>
                      (ui->editorTabs->currentWidget());
    if(doc->type() == TabContent::Skin)
        dynamic_cast<SkinDocument*>(doc)->getEditor()->paste();
}

void EditorWindow::findReplace()
{
    TabContent* doc = dynamic_cast<TabContent*>
                      (ui->editorTabs->currentWidget());
    if(doc->type() == TabContent::Skin)
        dynamic_cast<SkinDocument*>(doc)->showFind();
}


void EditorWindow::expandLine(ParseTreeModel* model, QModelIndex parent,
                              int line, bool highlight)
{
    for(int i = 0; i < model->rowCount(parent); i++)
    {
        QModelIndex dataType = model->index(i, ParseTreeModel::typeColumn,
                                            parent);
        QModelIndex dataVal = model->index(i, ParseTreeModel::valueColumn,
                                           parent);
        QModelIndex data = model->index(i, ParseTreeModel::lineColumn, parent);
        QModelIndex recurse = model->index(i, 0, parent);

        expandLine(model, recurse, line, highlight);

        if(model->data(data, Qt::DisplayRole) == line)
        {
            ui->parseTree->expand(parent);
            ui->parseTree->expand(data);
            ui->parseTree->scrollTo(parent, QAbstractItemView::PositionAtTop);

            if(highlight)
            {
                parseTreeSelection->select(data,
                                           QItemSelectionModel::Select);
                parseTreeSelection->select(dataType,
                                           QItemSelectionModel::Select);
                parseTreeSelection->select(dataVal,
                                           QItemSelectionModel::Select);
            }
        }
    }

}

void EditorWindow::sizeColumns()
{
    /* Setting the column widths */
    ui->parseTree->resizeColumnToContents(ParseTreeModel::lineColumn);
    ui->parseTree->resizeColumnToContents(ParseTreeModel::typeColumn);
    ui->parseTree->resizeColumnToContents(ParseTreeModel::valueColumn);
}

void EditorWindow::loadProjectFile(QString fileName)
{
    QSettings settings;
    settings.beginGroup("ProjectModel");

    if(QFile::exists(fileName))
    {
        projectToTop(fileName);

        if(project)
            project->deleteLater();

        ui->actionClose_Project->setEnabled(true);
        ui->actionExport_Project->setEnabled(true);

        project = new ProjectModel(fileName, this);
        ui->projectTree->setModel(project);

        /* Setting target info if necessary */
        TargetData targets;
        QString target = project->getSetting("#target", "");
        if(target != "" && targets.index(target) >= 0)
        {
            int index = targets.index(target);

            QRect screen = targets.screenSize(index);
            deviceConfig->setData("screenwidth", screen.width());
            deviceConfig->setData("screenheight", screen.height());

            if(targets.remoteDepth(index) != TargetData::None)
            {
                QRect remote = targets.remoteSize(index);
                deviceConfig->setData("remotewidth", remote.width());
                deviceConfig->setData("remoteheight", remote.height());
            }

            deviceConfig->setData("tp", targets.fm(index));
            deviceConfig->setData("Rp", targets.canRecord(index));
        }

        if(project->getSetting("#screenwidth") != "")
            deviceConfig->setData("screenwidth",
                                  project->getSetting("#screenwidth"));
        if(project->getSetting("#screenheight") != "")
            deviceConfig->setData("screenheight",
                                  project->getSetting("#screenheight"));

        QObject::connect(ui->projectTree, SIGNAL(activated(QModelIndex)),
                         project, SLOT(activated(QModelIndex)));

        fileName.chop(fileName.length() - fileName.lastIndexOf('/') - 1);
        settings.setValue("defaultDirectory", fileName);

        for(int i = 0; i < ui->editorTabs->count(); i++)
        {
            TabContent* doc = dynamic_cast<TabContent*>
                              (ui->editorTabs->widget(i));
            if(doc->type() == TabContent::Skin)
            {
                dynamic_cast<SkinDocument*>(doc)->setProject(project);
                if(i == ui->editorTabs->currentIndex())
                {
                    viewer->connectSkin(dynamic_cast<SkinDocument*>(doc));
                }
            }
        }

    }

    settings.endGroup();

}

void EditorWindow::createFile(QString filename, QString contents)
{
    QFile fout(filename);
    fout.open(QFile::WriteOnly);

    fout.write(contents.toLatin1());

    fout.close();
}

void EditorWindow::docToTop(QString file)
{
    if(!QFile::exists(file))
        return;

    int index = recentDocs.indexOf(file);
    if(index == -1)
    {
        /* Bumping off the last file */
        if(recentDocs.count() >= numRecent)
            recentDocs.removeLast();
        recentDocs.prepend(file);
    }
    else
    {
        /* Shuffling file to the top of the list */
        recentDocs.removeAt(index);
        recentDocs.prepend(file);
    }

    refreshRecentMenus();
}

void EditorWindow::projectToTop(QString file)
{
    if(!QFile::exists(file))
        return;

    int index = recentProjects.indexOf(file);
    if(index == -1)
    {
        /* Bumping off the last project */
        if(recentProjects.count() >= numRecent)
            recentProjects.removeLast();
        recentProjects.prepend(file);
    }
    else
    {
        /* Shuffling file to the top of the list */
        recentProjects.removeAt(index);
        recentProjects.prepend(file);
    }

    refreshRecentMenus();
}

void EditorWindow::refreshRecentMenus()
{
    /* Clearing any deleted documents */
    for(int i = 0; i < recentDocs.count(); i++)
        if(!QFile::exists(recentDocs[i]))
            recentDocs.removeAt(i--);

    /* Clearing any deleted projects */
    for(int i = 0; i < recentProjects.count(); i++)
        if(!QFile::exists(recentProjects[i]))
            recentProjects.removeAt(i--);

    /* First hiding all the menu items */
    for(int i = 0; i < recentDocsMenu.count(); i++)
        recentDocsMenu[i]->setVisible(false);
    for(int i = 0; i < recentProjectsMenu.count(); i++)
        recentProjectsMenu[i]->setVisible(false);

    /* Then setting the text of and showing any available */
    for(int i = 0; i < recentDocs.count(); i++)
    {
        recentDocsMenu[i]->setText(recentDocs[i]);
        recentDocsMenu[i]->setVisible(true);
    }

    for(int i = 0; i < recentProjects.count(); i++)
    {
        recentProjectsMenu[i]->setText(recentProjects[i]);
        recentProjectsMenu[i]->setVisible(true);
    }
}
