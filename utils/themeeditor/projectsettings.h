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

#ifndef PROJCETSETTINGS_H
#define PROJECTSETTINGS_H

#include "projectmodel.h"
#include <QHash>

class ProjectSettings : public ProjectNode
{
public:
    ProjectSettings(QHash<QString, QString>& settings, ProjectModel* model,
                 ProjectNode* parent);
    virtual ~ProjectSettings();

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
class ProjectSetting: public ProjectNode
{
public:
    ProjectSetting(QPair<QString, QString> setting, ProjectModel* model,
                   ProjectNode* parent);
    virtual ~ProjectSetting();

    virtual ProjectNode* parent() const{ return parentLink; }
    virtual ProjectNode* child(int row) const{ return 0; }
    virtual int numChildren() const{ return 0; }
    virtual int row() const{
        return parentLink->indexOf(const_cast<ProjectSetting*>(this));
    }
    virtual QVariant data(int column) const;
    virtual Qt::ItemFlags flags(int column) const;
    virtual void activated();

private:
    ProjectNode* parentLink;
    QPair<QString, QString> setting;
};

#endif // PROJECTSETTINGS_H
