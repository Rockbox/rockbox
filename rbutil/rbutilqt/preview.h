/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id$
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

#ifndef PREVIEW_H
#define PREVIEW_H

#include <QtGui>
#include "ui_previewfrm.h"


class PreviewDlg : public QDialog
{
    Q_OBJECT

public:
    PreviewDlg(QWidget *parent = 0);
    void setPixmap(QPixmap p);
    void setText(QString text);

private slots:
    void mouseMoveEvent(QMouseEvent * event);
    void leaveEvent(QEvent * event);

private:
    Ui::PreviewFrm ui;


};


class PreviewLabel : public QLabel
{
    Q_OBJECT

public:
    PreviewLabel(QWidget * parent = 0, Qt::WindowFlags f = 0);

    void setPixmap(QPixmap p);
    void setText(QString text);
private slots:
    void mouseMoveEvent(QMouseEvent * event);
    void enterEvent(QEvent * event);
    void leaveEvent(QEvent * event);
    void timeout();

private:
    QTimer hovertimer;
    int mousex;
    int mousey;
    PreviewDlg* preview;
};


#endif
