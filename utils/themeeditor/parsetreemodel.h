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

extern "C"
{
#include "skin_parser.h"
#include "skin_debug.h"
}

#ifndef PARSETREEMODEL_H
#define PARSETREEMODEL_H

#include <QAbstractItemModel>
#include <QList>

#include "parsetreenode.h"

class ParseTreeModel : public QAbstractItemModel
{

    Q_OBJECT

public:
    /* Initializes a tree with a skin document in a string */
    ParseTreeModel(char* document, QObject* parent = 0);
    virtual ~ParseTreeModel();

    QString genCode();

    QModelIndex index(int row, int column, const QModelIndex& parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;

private:
    ParseTreeNode* root;
    struct skin_element* tree;
};



#endif // PARSETREEMODEL_H
