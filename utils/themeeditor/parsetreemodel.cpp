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
    return QModelIndex();
}

QModelIndex ParseTreeModel::parent(const QModelIndex &child) const
{
    return QModelIndex();
}

int ParseTreeModel::rowCount(const QModelIndex &parent) const
{
    return 0;
}

int ParseTreeModel::columnCount(const QModelIndex &parent) const
{
    return 0;
}
QVariant ParseTreeModel::data(const QModelIndex &index, int role) const
{
    return QVariant();
}
