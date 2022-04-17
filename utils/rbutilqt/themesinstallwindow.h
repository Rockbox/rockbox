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

#ifndef INSTALLTHEMES_H
#define INSTALLTHEMES_H

#include <QtGui>
#include <QTemporaryFile>

#include "ui_themesinstallfrm.h"
#include "httpget.h"
#include "zipinstaller.h"
#include "progressloggergui.h"

class ThemesInstallWindow : public QDialog
{
    Q_OBJECT

    public:
        ThemesInstallWindow(QWidget* parent = 0);
        ~ThemesInstallWindow();
        void show(void);
        void setLogger(ProgressLoggerGui* l) { logger = l; }
        void install(void);

    public slots:
        void buttonOk(void);

    signals:
        void selected(int);
        void finished(bool);

    private:
        Ui::ThemeInstallFrm ui;
        HttpGet *getter;
        HttpGet igetter;
        QTemporaryFile themesInfo;
        void resizeEvent(QResizeEvent*);
        void changeEvent(QEvent *event);
        QByteArray imgData;
        ProgressLoggerGui *logger;
        ZipInstaller *installer;
        QString file;
        QString fileName;

        QString infocachedir;

    private slots:
        void downloadInfo(void);
        void downloadDone(QNetworkReply::NetworkError error);
        void updateImage(QNetworkReply::NetworkError error);
        void abort(void);
        void updateDetails(QListWidgetItem* cur, QListWidgetItem* prev);
        void updateSize(void);
};


#endif
