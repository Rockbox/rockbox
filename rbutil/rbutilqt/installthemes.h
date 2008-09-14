/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Riebeling
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

#ifndef INSTALLTHEMES_H
#define INSTALLTHEMES_H

#include <QtGui>
#include <QTemporaryFile>

#include "ui_installthemesfrm.h"
#include "httpget.h"
#include "installzip.h"
#include "progressloggergui.h"
#include "rbsettings.h"

class ThemesInstallWindow : public QDialog
{
    Q_OBJECT

    public:
        ThemesInstallWindow(QWidget* parent = 0);
        ~ThemesInstallWindow();
        void setSettings(RbSettings* sett){settings=sett;}
        void downloadInfo(void);
        void show(void);

    public slots:
        void accept(void);

    private:
        Ui::ThemeInstallFrm ui;
        RbSettings* settings;
        HttpGet *getter;
        HttpGet igetter;
        QTemporaryFile themesInfo;
        QString resolution(void);
        int currentItem;
        void resizeEvent(QResizeEvent*);
        QByteArray imgData;
        ProgressLoggerGui *logger;
        ZipInstaller *installer;
        QString file;
        QString fileName;

        QString infocachedir;

    private slots:
        void downloadDone(bool);
        void downloadDone(int, bool);
        void updateDetails(int);
        void updateImage(bool);
        void abort(void);
};


#endif
