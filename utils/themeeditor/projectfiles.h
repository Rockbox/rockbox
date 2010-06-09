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

#ifndef PROJECTFILES_H
#define PROJECTFILES_H

#include "projectmodel.h"
#include <QHash>

class ProjectFiles : public ProjectNode
{
public:
    ProjectFiles(QHash<QString, QString>& settings, ProjectNode* parent);
    virtual ~ProjectFiles();

    virtual ProjectNode* parent() const;
    virtual ProjectNode* child(int row) const;
    virtual int numChildren() const;
    virtual int row() const;
    virtual QVariant data(int column) const;
    virtual Qt::ItemFlags flags(int column) const;
    virtual void activated();

private:
    ProjectNode* parentLink;

};

/* A class to enumerate a single file */
class ProjectFile: public ProjectNode
{
public:
    ProjectFile(QString file, ProjectNode* parent);
    virtual ~ProjectFile();

    virtual ProjectNode* parent() const{ return parentLink; }
    virtual ProjectNode* child(int row) const{ return 0; }
    virtual int numChildren() const{ return 0; }
    virtual int row() const{
        return parentLink->indexOf(const_cast<ProjectFile*>(this));
    }
    virtual QVariant data(int column) const;
    virtual Qt::ItemFlags flags(int column) const;
    virtual void activated();

private:
    ProjectNode* parentLink;
    QString file;
};

#endif // PROJECTFILES_H
