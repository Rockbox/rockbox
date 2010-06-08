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

#include <QAbstractItemModel>

class ProjectNode;

class ProjectModel : public QAbstractItemModel
{
Q_OBJECT
public:
    static const int numColumns = 1;

    ProjectModel(QObject *parent = 0);
    virtual ~ProjectModel();

    QModelIndex index(int row, int column, const QModelIndex& parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int col, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);


signals:

public slots:

private:
    ProjectNode* root;

};

/* A simple abstract class required for categories */
class ProjectNode
{
public:
    virtual ProjectNode* parent() const = 0;
    virtual ProjectNode* child(int row) const = 0;
    virtual int numChildren() const = 0;
    virtual int row() const = 0;
    virtual QVariant data(int column) const = 0;
    virtual QString title() const = 0;
    virtual Qt::ItemFlags flags(const QModelIndex& index) const = 0;
    virtual void activated(const QModelIndex& index) = 0;

};

#endif // PROJECTMODEL_H
