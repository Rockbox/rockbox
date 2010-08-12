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
#include "rbfont.h"
#include "rbalbumart.h"
#include "rbviewport.h"

class RBScreen : public QGraphicsItem
{

public:
    RBScreen(const RBRenderInfo& info, bool remote = false,
             QGraphicsItem *parent = 0);
    virtual ~RBScreen();

    QPainterPath shape() const;
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget);

    int getWidth() const{ return width; }
    int getHeight() const{ return height; }

    void loadViewport(QString name, RBViewport* view);
    void showViewport(QString name);
    bool viewPortDisplayed(QString name)
    {
        return displayedViewports.contains(name);
    }

    void loadImage(QString name, RBImage* image)
    {
        images.insert(name, image);
        image->hide();
    }
    RBImage* getImage(QString name){ return images.value(name, 0); }

    void loadFont(int id, RBFont* font);
    RBFont* getFont(int id);

    void setBackdrop(QString filename);
    bool hasBackdrop(){ return backdrop != 0; }
    void makeCustomUI(QString id);
    void setCustomUI(RBViewport* viewport){ customUI = viewport; }
    RBViewport* getCustomUI(){ return customUI; }

    static QColor stringToColor(QString str, QColor fallback);

    QColor foreground(){ return fgColor; }
    QColor background(){ return bgColor; }

    void setAlbumArt(RBAlbumArt* art){ albumArt = art; }
    void showAlbumArt(RBViewport* view)
    {
        if(albumArt)
        {
            albumArt->setParentItem(view);
            albumArt->show();
            albumArt->enableMove();
        }
    }

    void setDefault(RBViewport* view){ defaultView = view; }
    void endSbsRender();
    void breakSBS();

    void RtlMirror(){ ax = true; }
    bool isRtlMirrored(){ bool ret = ax; ax = false; return ret; }

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event);

private:
    int width;
    int height;
    int fullWidth;
    int fullHeight;
    QColor bgColor;
    QColor fgColor;
    QPixmap* backdrop;
    QString themeBase;

    ProjectModel* project;

    QMap<QString, QList<RBViewport*>*> namedViewports;
    QMap<QString, RBImage*> images;
    QMap<QString, QString>* settings;
    QMap<int, RBFont*> fonts;
    QList<QString> displayedViewports;

    RBAlbumArt* albumArt;
    RBViewport* customUI;
    RBViewport* defaultView;

    QList<QGraphicsItem*> sbsChildren;

    bool ax;
};

#endif // RBSCREEN_H
