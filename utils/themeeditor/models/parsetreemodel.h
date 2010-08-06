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

#include "skin_parser.h"
#include "skin_debug.h"
#include "projectmodel.h"
#include "devicestate.h"

#ifndef PARSETREEMODEL_H
#define PARSETREEMODEL_H

#include <QAbstractItemModel>
#include <QList>

#include "parsetreenode.h"
#include "devicestate.h"
#include "rbscene.h"

class ParseTreeModel : public QAbstractItemModel
{

    Q_OBJECT

public:
    /* Constants */
    static const int numColumns = 3;
    static const int typeColumn = 0;
    static const int lineColumn = 1;
    static const int valueColumn = 2;

    /* Initializes a tree with a skin document in a string */
    ParseTreeModel(const char* document, QObject* parent = 0);
    virtual ~ParseTreeModel();

    QString genCode();
    /* Changes the parse tree to a new document */
    QString changeTree(const char* document);

    /* Model implementation stuff */
    QModelIndex index(int row, int column, const QModelIndex& parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int col, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);

    RBScene* render(ProjectModel* project, DeviceState* device,
                    SkinDocument* doc, const QString* file = 0);

    static QString safeSetting(ProjectModel* project, QString key,
                               QString fallback)
    {
        if(project)
            return project->getSetting(key, fallback);
        else
            return fallback;
    }

    void paramChanged(ParseTreeNode* param);
    QModelIndex indexFromPointer(ParseTreeNode* p);

private:
    void setChildrenUnselectable(QGraphicsItem* root);

    ParseTreeNode* root;
    ParseTreeModel* sbsModel;
    struct skin_element* tree;
    RBScene* scene;
};



#endif // PARSETREEMODEL_H
