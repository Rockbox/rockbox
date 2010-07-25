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

#ifndef FONTDOWNLOADER_H
#define FONTDOWNLOADER_H

#include <QDialog>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Ui {
    class FontDownloader;
}

class FontDownloader : public QDialog {
    Q_OBJECT
public:    
    FontDownloader(QWidget *parent, QString dir);
    virtual ~FontDownloader();

private slots:
    void cancel();

    void dataReceived();
    void progress(qint64 bytes, qint64 available);
    void finished();
    void netError(QNetworkReply::NetworkError code);

private:
    void closeEvent(QCloseEvent *event);

    Ui::FontDownloader *ui;

    QNetworkAccessManager* manager;
    QDir dir;
    QFile fout;
    QNetworkReply* reply;

    bool cancelled;
};

#endif // FONTDOWNLOADER_H
