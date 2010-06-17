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

#ifndef RBSCREEN_H
#define RBSCREEN_H

#include <QGraphicsItem>

#include "projectmodel.h"

class RBScreen : public QGraphicsItem
{

public:
    RBScreen(ProjectModel* project = 0, QGraphicsItem *parent = 0);
    virtual ~RBScreen();

    QPainterPath shape() const;
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget);

    static QString safeSetting(ProjectModel* project, QString key,
                               QString fallback)
    {
        if(project)
            return project->getSetting(key, fallback);
        else
            return fallback;
    }

    static QColor stringToColor(QString str, QColor fallback);

private:
    int width;
    int height;
    QColor bgColor;
    QColor fgColor;
    QPixmap* backdrop;

    ProjectModel* project;

};

#endif // RBSCREEN_H
