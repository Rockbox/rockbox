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

ParseTreeModel::ParseTreeModel(char* document, QObject* parent):
        QAbstractItemModel(parent)
{
    this->tree = skin_parse(document);
    this->root = new ParseTreeNode(tree);
}


ParseTreeModel::~ParseTreeModel()
{
    if(root)
        delete root;
    if(tree)
        skin_free_tree(tree);
}

QString ParseTreeModel::genCode()
{
    return root->genCode();
}

QModelIndex ParseTreeModel::index(int row, int column,
                                  const QModelIndex& parent) const
{
    if(!hasIndex(row, column, parent))
        return QModelIndex();

    ParseTreeNode* foundParent;

    if(parent.isValid())
        foundParent = static_cast<ParseTreeNode*>(parent.internalPointer());
    else
        foundParent = root;

    if(row < foundParent->numChildren() && row >= 0)
        return createIndex(row, column, foundParent->child(row));
    else
        return QModelIndex();
}

QModelIndex ParseTreeModel::parent(const QModelIndex &child) const
{
    if(!child.isValid())
        return QModelIndex();

    ParseTreeNode* foundParent = static_cast<ParseTreeNode*>
                                 (child.internalPointer())->getParent();

    if(foundParent == root)
        return QModelIndex();

    return createIndex(foundParent->getRow(), 0, foundParent);
}

int ParseTreeModel::rowCount(const QModelIndex &parent) const
{
    if(!parent.isValid())
        return root->numChildren();

    if(parent.column() > 0)
        return 0;

    return static_cast<ParseTreeNode*>(parent.internalPointer())->numChildren();
}

int ParseTreeModel::columnCount(const QModelIndex &parent) const
{
    return 3;
}
QVariant ParseTreeModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    if(role != Qt::DisplayRole)
        return QVariant();

    return static_cast<ParseTreeNode*>(index.internalPointer())->
            data(index.column());
}
