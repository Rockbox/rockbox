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


#include "parsetreemodel.h"
#include <QObject>

ParseTreeModel::ParseTreeModel(char* wps, QObject* parent):
        QAbstractItemModel(parent)
{
    this->wps = skin_parse(wps);
    skin_debug_tree(this->wps);
    this->root = new ParseTreeNode(this->wps, 0, true);
}


ParseTreeModel::~ParseTreeModel()
{
    delete root;
}

QModelIndex ParseTreeModel::index(int row, int column,
                                  const QModelIndex& parent) const
{
    if(!hasIndex(row, column, parent))
        return QModelIndex();

    ParseTreeNode* parentLookup;

    if(!parent.isValid())
        parentLookup = root;
    else
        parentLookup = static_cast<ParseTreeNode*>(parent.internalPointer());

    ParseTreeNode* childLookup = parentLookup->child(row);
    if(childLookup)
        return createIndex(row, column, childLookup);
    else
        return QModelIndex();
}

QModelIndex ParseTreeModel::parent(const QModelIndex &child) const
{
    if(!child.isValid())
        return QModelIndex();

    ParseTreeNode* childLookup = static_cast<ParseTreeNode*>
                                 (child.internalPointer());
    ParseTreeNode* parentLookup = childLookup->parent();

    if(parentLookup == root)
        return QModelIndex();

    return createIndex(parentLookup->row(), 0, parentLookup);
}

int ParseTreeModel::rowCount(const QModelIndex &parent) const
{
    ParseTreeNode* parentLookup;
    if(parent.column() > 0)
        return 0;

    if(!parent.isValid())
        parentLookup = root;
    else
        parentLookup = static_cast<ParseTreeNode*>(parent.internalPointer());

    return parentLookup->childCount();
}

int ParseTreeModel::columnCount(const QModelIndex &parent) const
{
    return 2;
}

QVariant ParseTreeModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    ParseTreeNode* item = static_cast<ParseTreeNode*>(index.internalPointer());
    return item->data(index.column());
}
