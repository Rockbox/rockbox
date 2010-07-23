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
        writeZip(project);
    }
    else
    {
        html += tr("<span style = \"color:red\">Error opening zip file</span><br>");
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

void ProjectExporter::writeZip(ProjectModel *project)
{
    (void)project;
    zipFile.close();
}
