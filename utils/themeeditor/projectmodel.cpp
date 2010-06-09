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
#include "projectfiles.h"

ProjectModel::ProjectModel(QString config, QObject *parent) :
    QAbstractItemModel(parent)
{
    root = new ProjectRoot(config);
}

ProjectModel::~ProjectModel()
{
    if(root)
        delete root;
}

QModelIndex ProjectModel::index(int row, int column,
                                const QModelIndex& parent) const
{

    if(!hasIndex(row, column, parent))
        return QModelIndex();

    ProjectNode* foundParent = root;
    if(parent.isValid())
        foundParent = static_cast<ProjectNode*>(parent.internalPointer());

    if(row < foundParent->numChildren() && row >= 0)
        return createIndex(row, column, foundParent->child(row));
    else
        return QModelIndex();
}

QModelIndex ProjectModel::parent(const QModelIndex &child) const
{
    if(!child.isValid())
        return QModelIndex();

    ProjectNode* foundParent = static_cast<ProjectNode*>
                                   (child.internalPointer())->parent();

    if(foundParent == 0)
        return QModelIndex();

    return createIndex(foundParent->row(), 0, foundParent);
}

int ProjectModel::rowCount(const QModelIndex &parent) const
{
    if(!root)
        return 0;

    if(!parent.isValid())
        return root->numChildren();

    if(parent.column() != 0)
        return 0;

    return static_cast<ProjectNode*>(parent.internalPointer())->numChildren();
}

int ProjectModel::columnCount(const QModelIndex &parent) const
{
    return numColumns;
}

QVariant ProjectModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    if(role != Qt::DisplayRole)
        return QVariant();

    return static_cast<ProjectNode*>
            (index.internalPointer())->data(index.column());
}

QVariant ProjectModel::headerData(int col, Qt::Orientation orientation,
                                  int role) const
{
    return QVariant();
}

Qt::ItemFlags ProjectModel::flags(const QModelIndex &index) const
{
    return static_cast<ProjectNode*>
            (index.internalPointer())->flags(index.column());
}

bool ProjectModel::setData(const QModelIndex &index, const QVariant &value,
                           int role)
{
    return true;
}

/* Constructor and destructor for the root class */
ProjectRoot::ProjectRoot(QString config)
{
    children.append(new ProjectFiles(this));
}

ProjectRoot::~ProjectRoot()
{
    for(int i = 0; i < children.count(); i++)
        delete children[i];
}
