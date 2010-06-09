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

    static QString fileFilter()
    {
        return QObject::tr("Project Files (*.cfg);;All Files (*.*)");
    }

    ProjectModel(QString config, QObject *parent = 0);
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
    virtual Qt::ItemFlags flags(int column) const = 0;
    virtual void activated() = 0;

    int indexOf(ProjectNode* child){ return children.indexOf(child); }

protected:
    QList<ProjectNode*> children;

};

/* A simple implementation of ProjectNode for the root */
class ProjectRoot : public ProjectNode
{
public:
    ProjectRoot(QString config);
    virtual ~ProjectRoot();

    virtual ProjectNode* parent() const{ return 0; }
    virtual ProjectNode* child(int row) const
    {
        if(row >= 0 && row < children.count())
            return children[row];
        else
            return 0;
    }
    virtual int numChildren() const{ return children.count(); }
    virtual int row() const{ return 0; }
    virtual QVariant data(int column) const{ return QVariant(); }
    virtual Qt::ItemFlags flags(int column) const{ return 0; }
    virtual void activated(){ }

};


#endif // PROJECTMODEL_H
