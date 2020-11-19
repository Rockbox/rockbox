/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
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

#ifndef INSTALLTALKWINDOW_H
#define INSTALLTALKWINDOW_H

#include <QDialog>
#include <QFileSystemModel>

#include "ui_installtalkfrm.h"
#include "progressloggergui.h"
#include "talkfile.h"

class InstallTalkWindow : public QDialog
{
    Q_OBJECT
    public:
        InstallTalkWindow(QWidget *parent = nullptr);

    public slots:
        void accept(void);
        void change(void);

    private slots:
        void updateSettings(void);
        void saveSettings(void);

    signals:
        void settingsUpdated(void);

    private:
        void changeEvent(QEvent *event);
        TalkFileCreator* talkcreator;
        Ui::InstallTalkFrm ui;
        ProgressLoggerGui* logger;
        QFileSystemModel *fsm;
};

#endif
