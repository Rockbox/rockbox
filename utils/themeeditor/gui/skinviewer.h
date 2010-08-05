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

#ifndef SKINVIEWER_H
#define SKINVIEWER_H

#include <QWidget>
#include <QGraphicsScene>

#include "skindocument.h"

namespace Ui {
    class SkinViewer;
}

class SkinViewer : public QWidget {
    Q_OBJECT
public:
    SkinViewer(QWidget *parent = 0);
    ~SkinViewer();

    void connectSkin(SkinDocument* skin);

public slots:
    void zoomIn();
    void zoomOut();
    void zoomEven();

private slots:

protected:
    void changeEvent(QEvent *e);

private:
    Ui::SkinViewer *ui;

    SkinDocument* doc;
};

#endif // SKINVIEWER_H
