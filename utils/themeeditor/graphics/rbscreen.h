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
#include "rbrenderinfo.h"
#include "rbimage.h"

class RBViewport;

class RBScreen : public QGraphicsItem
{

public:
    RBScreen(const RBRenderInfo& info, QGraphicsItem *parent = 0);
    virtual ~RBScreen();

    QPainterPath shape() const;
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget);

    int getWidth() const{ return width; }
    int getHeight() const{ return height; }

    void loadViewport(QString name, RBViewport* view)
    {
        namedViewports.insert(name, view);
    }
    void showViewport(QString name);

    void loadImage(QString name, RBImage* image)
    {
        images.insert(name, image);
    }
    RBImage* getImage(QString name){ return images.value(name, 0); }

    void setBackdrop(QString filename);

    static QColor stringToColor(QString str, QColor fallback);


private:
    int width;
    int height;
    QColor bgColor;
    QColor fgColor;
    QPixmap* backdrop;
    QString themeBase;

    ProjectModel* project;

    QMap<QString, RBViewport*> namedViewports;
    QMap<QString, RBImage*> images;
    QMap<QString, QString>* settings;

};

#endif // RBSCREEN_H
