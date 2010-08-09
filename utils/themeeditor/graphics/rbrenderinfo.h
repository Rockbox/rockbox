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

#ifndef RBRENDERINFO_H
#define RBRENDERINFO_H

#include <QMap>

class RBScreen;
class ProjectModel;
class ParseTreeModel;
class DeviceState;
class SkinDocument;

class RBRenderInfo
{
public:
    RBRenderInfo(ParseTreeModel* model,  ProjectModel* project,
                 SkinDocument* doc, QMap<QString, QString>* settings,
                 DeviceState* device, RBScreen* screen);
    RBRenderInfo();
    RBRenderInfo(const RBRenderInfo& other);
    virtual ~RBRenderInfo();

    const RBRenderInfo& operator=(const RBRenderInfo& other);

    ProjectModel* project() const{ return mProject; }
    DeviceState* device() const{ return mDevice; }
    SkinDocument* document() const{ return mDoc; }
    QMap<QString, QString>* settings() const{ return mSettings; }
    RBScreen* screen() const{ return mScreen; }
    ParseTreeModel* model() const{ return mModel; }

private:
    ProjectModel* mProject;
    SkinDocument* mDoc;
    QMap<QString, QString>* mSettings;
    DeviceState* mDevice;
    RBScreen* mScreen;
    ParseTreeModel* mModel;
};

#endif // RBRENDERINFO_H
