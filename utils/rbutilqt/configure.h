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

#ifndef CONFIGURE_H
#define CONFIGURE_H

#include "ui_configurefrm.h"
#include <QDialog>
#include <QWidget>
#include <QUrl>

class Config : public QDialog
{
    Q_OBJECT
    public:
        Config(QWidget *parent = nullptr,int index=0);

    signals:
        void settingsUpdated(void);

    public slots:
        void accept(void);
        void abort(void);

    private:
        void setUserSettings();
        void setDevices();

        Ui::ConfigForm ui;

        QStringList findLanguageFiles(void);
        QString languageName(const QString&);
        QMap<QString, QString> lang;
        QString language;
        QString programPath;
        QUrl proxy;
        QString mountpoint;
        void updateCacheInfo(QString);
        void changeEvent(QEvent *event);
        void selectDevice(QString device, QString mountpoint);

    private slots:
        void showProxyPassword(bool show);
        void setNoProxy(bool);
        void setSystemProxy(bool);
        void updateLanguage(void);
        void refreshMountpoint(void);
        void browseCache(void);
        void autodetect(void);
        void setMountpoint(QString);
        void updateMountpoint(QString);
        void updateMountpoint(int);
        void cacheClear(void);
        void configTts(void);
        void configEnc(void);
        void updateTtsState(int);
        void updateEncState();
        void testTts();
        void showDisabled(bool);
};

#endif
