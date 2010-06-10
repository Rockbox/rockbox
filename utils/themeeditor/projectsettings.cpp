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

#include "projectsettings.h"

ProjectSettings::ProjectSettings(QHash<QString, QString>& settings,
                           ProjectModel* model, ProjectNode* parent)
                               : parentLink(parent)
{
    QHash<QString, QString>::iterator i;
    for(i = settings.begin(); i != settings.end(); i++)
    {
        QPair<QString, QString> value(i.key(), i.value());
        children.append(new ProjectSetting(value, model, this));
    }
}

ProjectSettings::~ProjectSettings()
{
    for(int i = 0; i < children.count(); i++)
        delete children[i];
}

ProjectNode* ProjectSettings::parent() const
{
    return parentLink;
}

ProjectNode* ProjectSettings::child(int row) const
{
    if(row >= 0 && row < children.count())
        return children[row];

    return 0;
}

int ProjectSettings::numChildren() const
{
    return children.count();
}

int ProjectSettings::row() const
{
    return parentLink->indexOf(const_cast<ProjectSettings*>(this));
}

QVariant ProjectSettings::data(int column) const
{
    if(column == 0)
        return QObject::tr("Project Settings");
    else
        return QVariant();
}

Qt::ItemFlags ProjectSettings::flags(int column) const
{
    if(column == 0)
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    else
        return 0;
}

void ProjectSettings::activated()
{

}

/* Project File functions */
ProjectSetting::ProjectSetting(QPair<QString, QString> setting,
                               ProjectModel* model, ProjectNode* parent)
                                   :parentLink(parent), setting(setting)
{
    this->model = model;
}

ProjectSetting::~ProjectSetting()
{

}

QVariant ProjectSetting::data(int column) const
{
    if(column == 0)
        return setting.first;
    else if(column == 1)
        return setting.second;
    else
        return QVariant();
}

Qt::ItemFlags ProjectSetting::flags(int column) const
{
    if(column == 0 || column == 1)
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    else
        return 0;
}

void ProjectSetting::activated()
{
}

