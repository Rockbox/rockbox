/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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

#ifndef RBSCENE_H
#define RBSCENE_H

#include <QGraphicsScene>
#include <QGraphicsProxyWidget>

class RBScreen;
class RBConsole;

class RBScene : public QGraphicsScene
{
    Q_OBJECT

public:
    RBScene(QObject* parent = 0);
    ~RBScene();

    void moveMouse(QString position){ emit mouseMoved(position); }

    void setScreenSize(qreal w, qreal h)
    {
        screen = QRectF(0, 0, w, h);
        if(consoleProxy)
            consoleProxy->resize(screen.width(), screen.height());
    }

    void setScreenSize(QRectF screen){
        this->screen = screen;
        if(consoleProxy)
            consoleProxy->resize(screen.width(), screen.height());
    }

    void addWarning(QString warning);

public slots:
    void clear();

signals:
    void mouseMoved(QString position);

private:
    QGraphicsProxyWidget* consoleProxy;
    RBConsole* console;

    QRectF screen;
};

#endif // RBSCENE_H
