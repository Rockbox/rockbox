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

#ifndef PROJECTMODEL_H
#define PROJECTMODEL_H

#include <QAbstractListModel>
#include <QMap>

class EditorWindow;

class ProjectModel : public QAbstractListModel
{
Q_OBJECT
public:
    static const int numColumns = 2;

    static QString fileFilter()
    {
        return QObject::tr("Project Files (*.cfg);;All Files (*)");
    }

    ProjectModel(QString config, EditorWindow* mainWindow, QObject *parent = 0);
    virtual ~ProjectModel();

    int rowCount(const QModelIndex& parent) const;
    QVariant data(const QModelIndex &index, int role) const;

    QString getSetting(QString key, QString fallback = "")
    {
        return settings.value(key, fallback);
    }

    const QMap<QString, QString>& getSettings() const{ return settings; }

signals:

public slots:
    void activated(const QModelIndex& index);

private:
    EditorWindow* mainWindow;
    QMap<QString, QString> settings;
    QList<QString> files;
};

#endif // PROJECTMODEL_H
