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


#include "projectmodel.h"
#include "editorwindow.h"

#include <QFile>
#include <QTextStream>
#include <QMap>
#include <QDir>

ProjectModel::ProjectModel(QString config, EditorWindow* mainWindow,
                           QObject *parent)
                               : QAbstractListModel(parent),
                               mainWindow(mainWindow)
{
    /* Reading the config file */
    QFile cfg(config);
    cfg.open(QFile::ReadOnly | QFile::Text);
    if(!cfg.isReadable())
        return;

    settings.insert("configfile", config);

    QTextStream fin(&cfg);

    /* Storing the base directory */
    QString confDir = config;
    confDir.chop(confDir.length() - confDir.lastIndexOf('/') - 1);
    QDir base(confDir);
    base.cdUp();
    settings.insert("themebase", base.canonicalPath());

    while(!fin.atEnd())
    {
        QString current = fin.readLine();
        QList<QString> parts = current.split(':');

        /* A valid setting has at least one : */
        if(parts.count() < 2)
            continue;

        QString setting;
        for(int i = 1; i < parts.count(); i++)
            setting.append(parts[i].trimmed());

        settings.insert(parts[0].trimmed(), setting);
    }

    cfg.close();

    /* Adding the files, starting with the .cfg */
    config.replace(base.canonicalPath() + "/", "");
    files.append(config);

    QList<QString> keys;
    keys.append("wps");
    keys.append("rwps");
    keys.append("sbs");
    keys.append("rsbs");
    keys.append("fms");
    keys.append("rfms");

    for(int i = 0; i < keys.count(); i++)
    {
        QString file = settings.value(keys[i], "");
        if(file != "" && file != "-")
        {
            file.replace("/.rockbox/", "");
            files.append(file);
        }
    }


}

ProjectModel::~ProjectModel()
{
}

int ProjectModel::rowCount(const QModelIndex& parent) const
{
    return files.count();
}

QVariant ProjectModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    if(role != Qt::DisplayRole)
        return QVariant();

    return files[index.row()];
}

void ProjectModel::activated(const QModelIndex &index)
{
    if(index.row() == 0)
    {
        ConfigDocument* doc = new ConfigDocument(settings,
                                                 settings.value("themebase",
                                                                "") + "/" +
                                                 files[index.row()]);
        QObject::connect(doc, SIGNAL(configFileChanged(QString)),
                         mainWindow, SLOT(configFileChanged(QString)));
        mainWindow->loadConfigTab(doc);
    }
    else
    {
        mainWindow->loadTabFromSkinFile(settings.value("themebase", "")
                                        + "/" + files[index.row()]);
    }
}
