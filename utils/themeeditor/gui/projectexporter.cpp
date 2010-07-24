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

#include "projectexporter.h"
#include "ui_projectexporter.h"

#include "quazipfile.h"

#include <QTextStream>
#include <QDir>

ProjectExporter::ProjectExporter(QString path, ProjectModel* project,
                                 QWidget *parent)
                                     :QDialog(parent),
                                     ui(new Ui::ProjectExporter), zipFile(path)
{
    ui->setupUi(this);

    QObject::connect(ui->closeButton, SIGNAL(clicked()),
                     this, SLOT(close()));

    if(zipFile.open(QuaZip::mdCreate))
    {
        html += tr("<span style=\"color:orange\">Resource Check: "
                   "Not implemented yet</span><br>");
        ui->statusBox->document()->setHtml(html);
        writeZip(project->getSetting("themebase", ""));
        zipFile.close();

        html += tr("<span style=\"color:green\">Project exported "
                   "successfully</span><br>");
        ui->statusBox->document()->setHtml(html);
    }
    else
    {
        html += tr("<span style = \"color:red\">"
                   "Error opening zip file</span><br>");
        ui->statusBox->document()->setHtml(html);
    }
}

ProjectExporter::~ProjectExporter()
{
    delete ui;
}

void ProjectExporter::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void ProjectExporter::closeEvent(QCloseEvent *event)
{
    close();
    event->accept();
}

void ProjectExporter::close()
{
    deleteLater();
    hide();
}

void ProjectExporter::writeZip(QString path, QString base)
{
    if(base == "")
        base = path;
    if(path == "")
    {
        html += tr("<span style = \"color:red\">"
                   "Error: Couldn't locate project directory</span><br>");
        ui->statusBox->document()->setHtml(html);
        return;
    }

    QDir dir(path);

    /* First adding any files in the directory */
    QFileInfoList files = dir.entryInfoList(QDir::Files);
    for(int i = 0; i < files.count(); i++)
    {
        QFileInfo current = files[i];

        QString newPath = current.absoluteFilePath().replace(base, "/.rockbox");

        QuaZipFile fout(&zipFile);
        QFile fin(current.absoluteFilePath());

        fin.open(QFile::ReadOnly | QFile::Text);
        fout.open(QIODevice::WriteOnly,
                  QuaZipNewInfo(newPath, current.absoluteFilePath()));

        fout.write(fin.readAll());

        fin.close();
        fout.close();
    }

    /* Then recursively adding any directories */
    QFileInfoList dirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for(int i = 0; i < dirs.count(); i++)
    {
        QFileInfo current = dirs[i];
        writeZip(current.absoluteFilePath(), base);
    }
}
