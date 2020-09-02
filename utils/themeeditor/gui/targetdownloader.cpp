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

#include "targetdownloader.h"
#include "ui_targetdownloader.h"

#include "quazip.h"
#include "quazipfile.h"
#include "quazipfileinfo.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QCloseEvent>

#include <QDebug>

TargetDownloader::TargetDownloader(QWidget *parent, QString path) :
    QDialog(parent),
    ui(new Ui::TargetDownloader), reply(0), cancelled(false)
{
    ui->setupUi(this);

    QObject::connect(ui->cancelButton, SIGNAL(clicked()),
                     this, SLOT(cancel()));

    manager = new QNetworkAccessManager();

    fout.setFileName(path);
    if(fout.open(QFile::WriteOnly))
    {
        ui->label->setText(tr("Downloading targetdb"));

        QNetworkRequest request;
        request.setUrl(
                QUrl("https://git.rockbox.org/cgit/rockbox.git/plain/"
                     "utils/themeeditor/resources/targetdb"));
        request.setRawHeader("User-Agent", "Rockbox Theme Editor");

        reply = manager->get(request);

        QObject::connect(reply, SIGNAL(readyRead()),
                         this, SLOT(dataReceived()));
        QObject::connect(reply, SIGNAL(finished()),
                         this, SLOT(finished()));
        QObject::connect(reply, SIGNAL(downloadProgress(qint64,qint64)),
                         this, SLOT(progress(qint64,qint64)));
    }
    else
    {
        ui->label->setText(tr("Error: Couldn't open output file"));
    }

}

TargetDownloader::~TargetDownloader()
{
    delete ui;
    fout.close();
    manager->deleteLater();

    if(reply)
    {
        reply->abort();
        reply->deleteLater();
    }
}

void TargetDownloader::cancel()
{
    cancelled = true;

    if(reply)
    {
        reply->abort();
        reply->deleteLater();
        reply = 0;
    }

    fout.close();
    fout.remove();

    close();
}

void TargetDownloader::dataReceived()
{
    fout.write(reply->readAll());
}

void TargetDownloader::progress(qint64 bytes, qint64 available)
{
    if(available > 0)
    {
        ui->progressBar->setMaximum(available);
        ui->progressBar->setValue(bytes);
    }
}

void TargetDownloader::finished()
{
    if(cancelled)
        return;

    fout.close();
    reply->deleteLater();
    reply = 0;
    ui->label->setText(tr("Download complete"));
    hide();
    this->deleteLater();
}

void TargetDownloader::netError(QNetworkReply::NetworkError code)
{
    ui->label->setText(tr("Network error: ") + reply->errorString());
}

void TargetDownloader::closeEvent(QCloseEvent *event)
{
    cancel();
    event->accept();
}
