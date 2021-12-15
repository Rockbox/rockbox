/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Riebeling
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

#ifndef SYSINFO_H
#define SYSINFO_H

#include <QDialog>
#include <QWidget>
#include "ui_sysinfofrm.h"

class Sysinfo : public QDialog
{
    Q_OBJECT

    public:
        enum InfoType {
            InfoHtml,
            InfoText,
        };
        Sysinfo(QWidget *parent = nullptr);

        static QString getInfo(InfoType type = InfoHtml);
    private:
        void changeEvent(QEvent *event);
        Ui::SysinfoFrm ui;

    private slots:
         void updateSysinfo(void);

};

#endif

