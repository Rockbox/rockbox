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

#include "skinviewer.h"
#include "ui_skinviewer.h"

SkinViewer::SkinViewer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SkinViewer)
{
    ui->setupUi(this);

    QObject::connect(ui->zoomOutButton, SIGNAL(pressed()),
                     this, SLOT(zoomOut()));
    QObject::connect(ui->zoomInButton, SIGNAL(pressed()),
                     this, SLOT(zoomIn()));
    QObject::connect(ui->zoomEvenButton, SIGNAL(pressed()),
                     this, SLOT(zoomEven()));

}

SkinViewer::~SkinViewer()
{
    delete ui;
}

void SkinViewer::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void SkinViewer::connectSkin(SkinDocument *skin)
{
    if(skin)
    {
        ui->viewer->setScene(skin->scene());
        QObject::connect(skin, SIGNAL(antiSync(bool)),
                         ui->codeGenButton, SLOT(setEnabled(bool)));
        QObject::connect(skin, SIGNAL(antiSync(bool)),
                         ui->codeUndoButton, SLOT(setEnabled(bool)));

        QObject::connect(ui->codeGenButton, SIGNAL(pressed()),
                         skin, SLOT(genCode()));
        QObject::connect(ui->codeUndoButton, SIGNAL(pressed()),
                         skin, SLOT(parseCode()));

        QObject::connect(skin->scene(), SIGNAL(mouseMoved(QString)),
                         ui->coordinateLabel, SLOT(setText(QString)));

        doc = skin;
    }
    else
    {
        ui->viewer->setScene(0);
        ui->coordinateLabel->setText("");

        doc = 0;
    }

    bool antiSync;
    if(skin && !skin->isSynced())
        antiSync = true;
    else
        antiSync = false;

    ui->codeGenButton->setEnabled(antiSync);
    ui->codeUndoButton->setEnabled(antiSync);
}

void SkinViewer::zoomIn()
{
    ui->viewer->scale(1.2, 1.2);
}

void SkinViewer::zoomOut()
{
    ui->viewer->scale(1/1.2, 1/1.2);
}

void SkinViewer::zoomEven()
{
    ui->viewer->resetTransform();
}
