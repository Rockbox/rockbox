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

#ifndef BROWSEOF_H
#define BROWSEOF_H

#include <QtGui>
#include "ui_browseoffrm.h"

class BrowseOF : public QDialog
{
    Q_OBJECT

    public:
        BrowseOF(QWidget *parent = 0);
        void setFile(QString file);
        QString getFile();


    private slots:
        void onBrowse();

    private:
        Ui::BrowseOFFrm ui;

    private slots:
        void accept(void);
};

#endif
