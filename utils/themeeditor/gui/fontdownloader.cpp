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

#include "fontdownloader.h"
#include "ui_fontdownloader.h"

#include "quazip.h"
#include "quazipfile.h"
#include "quazipfileinfo.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QCloseEvent>

#include <QDebug>

FontDownloader::FontDownloader(QWidget *parent, QString path) :
    QDialog(parent),
    ui(new Ui::FontDownloader), dir(path), reply(0), cancelled(false)
{
    ui->setupUi(this);

    QObject::connect(ui->cancelButton, SIGNAL(clicked()),
                     this, SLOT(cancel()));

    manager = new QNetworkAccessManager();

    if(!dir.exists())
        dir.mkpath(path);

    if(dir.isReadable())
    {
        fout.setFileName(dir.absolutePath() + "/fonts.zip");
        if(fout.open(QFile::WriteOnly))
        {
            ui->label->setText(tr("Downloading font pack"));

            QNetworkRequest request;
            request.setUrl(QUrl("http://download.rockbox.org"
                                "/daily/fonts/rockbox-fonts.zip"));
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
            ui->label->setText(tr("Error: Couldn't open archive file"));
        }
    }
    else
    {
        ui->label->setText(tr("Error: Fonts directory not readable"));
    }

}

FontDownloader::~FontDownloader()
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

void FontDownloader::cancel()
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

void FontDownloader::dataReceived()
{
    fout.write(reply->readAll());
}

void FontDownloader::progress(qint64 bytes, qint64 available)
{
    if(available > 0)
    {
        ui->progressBar->setMaximum(available);
        ui->progressBar->setValue(bytes);
    }
}

void FontDownloader::finished()
{
    if(cancelled)
        return;

    fout.close();

    reply->deleteLater();
    reply = 0;
    ui->label->setText(tr("Download complete"));

    /* Extracting the ZIP archive */
    QuaZip archive(fout.fileName());
    QuaZipFile zipFile(&archive);
    archive.open(QuaZip::mdUnzip);

    bool more;
    for(more = archive.goToFirstFile(); more; more = archive.goToNextFile())
    {
        if(archive.getCurrentFileName().split("/").last() == "")
            continue;

        QFile fontFile(dir.absolutePath() + "/" +
                       archive.getCurrentFileName().split("/").last());
        fontFile.open(QFile::WriteOnly);

        zipFile.open(QIODevice::ReadOnly);
        fontFile.write(zipFile.readAll());
        zipFile.close();

        fontFile.close();
    }

    archive.close();
    QFile::remove(dir.absolutePath() + "/fonts.zip");

    hide();
    this->deleteLater();
}

void FontDownloader::netError(QNetworkReply::NetworkError code)
{
    ui->label->setText(tr("Network error: ") + reply->errorString());
}

void FontDownloader::closeEvent(QCloseEvent *event)
{
    cancel();
    event->accept();
}
