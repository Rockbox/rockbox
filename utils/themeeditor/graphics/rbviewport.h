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

#ifndef RBVIEWPORT_H
#define RBVIEWPORT_H

#include "skin_parser.h"
#include "rbfont.h"
#include "rbmovable.h"

class RBScreen;
class RBRenderInfo;
class ParseTreeNode;

#include <QGraphicsItem>

class SkinDocument;

class RBViewport : public RBMovable
{
public:
    enum Alignment
    {
        Left,
        Center,
        Right
    };

    static const double scrollRate;

    RBViewport(skin_element* node, const RBRenderInfo& info,
               ParseTreeNode* pNode);
    virtual ~RBViewport();

    QPainterPath shape() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget);

    void setBGColor(QColor color){ background = color; }
    QColor getBGColor(){ return background; }
    void setFGColor(QColor color){ foreground = color; }
    QColor getFGColor(){ return foreground; }
    void makeCustomUI(){ customUI = true; }
    void clearCustomUI(){ customUI = false; }

    void newLine();
    void write(QString text);
    void alignText(Alignment align){ textAlign = align; }
    int getTextOffset(){ return textOffset.y(); }
    void addTextOffset(int height){ textOffset.setY(textOffset.y() + height); }
    void flushText()
    {
        if(textOffset.x() < 0)
            return;
        alignLeft();
        alignRight();
        alignCenter();
    }
    void scrollText(double time){ scrollTime = time; }

    void enableStatusBar(){ showStatusBar = true; }

    void showPlaylist(const RBRenderInfo& info, int start,
                      ParseTreeNode* lines);

    void makeFullScreen();

protected:
    void saveGeometry();

private:

    void alignLeft();
    void alignCenter();
    void alignRight();

    RBFont* font;
    QColor foreground;
    QColor background;

    bool debug;
    bool customUI;
    QPoint textOffset;
    int lineHeight;

    RBScreen* screen;

    QString leftText;
    QString centerText;
    QString rightText;
    Alignment textAlign;

    bool showStatusBar;
    QPixmap statusBarTexture;

    RBText* leftGraphic;
    RBText* centerGraphic;
    RBText* rightGraphic;

    double scrollTime;

    int baseParam;
    ParseTreeNode* node;
    SkinDocument* doc;

    bool mirrored;
};

#endif // RBVIEWPORT_H
