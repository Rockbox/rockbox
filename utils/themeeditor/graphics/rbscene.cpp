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

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsItem>
#include <QGraphicsProxyWidget>

#include "rbscene.h"
#include "rbconsole.h"

RBScene::RBScene(QObject* parent)
    : QGraphicsScene(parent), consoleProxy(0), console(0)
{
}

RBScene::~RBScene()
{
    if(console)
        console->deleteLater();

    if(consoleProxy)
        consoleProxy->deleteLater();
}

void RBScene::clear()
{
    QGraphicsScene::clear();

    console = new RBConsole();
    consoleProxy = addWidget(console);
    consoleProxy->setZValue(1000);
    consoleProxy->resize(screen.width(), screen.height());
    consoleProxy->hide();
}

void RBScene::addWarning(QString warning)
{
    console->addWarning(warning);
    console->show();
}
