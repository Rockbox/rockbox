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

#include "newprojectdialog.h"
#include "ui_newprojectdialog.h"

#include <QSettings>
#include <QFileDialog>
#include <QDir>

NewProjectDialog::NewProjectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewProjectDialog)
{
    ui->setupUi(this);

    /* Getting the default directory from the application settings */
    QSettings settings;
    settings.beginGroup("NewProjectDialog");

    ui->locationBox->setText(settings.value("defaultDir",
                                            QDir::home().absolutePath())
                             .toString());

    settings.endGroup();

    /* Connecting the browse button */
    QObject::connect(ui->browseButton, SIGNAL(clicked()),
                     this, SLOT(browse()));
}

NewProjectDialog::~NewProjectDialog()
{
    delete ui;
}

void NewProjectDialog::accept()
{
    status.name = ui->nameBox->text();
    status.path = ui->locationBox->text();
    status.sbs = ui->sbsBox->isChecked();
    status.wps = ui->wpsBox->isChecked();
    status.fms = ui->fmsBox->isChecked();
    status.rsbs = ui->rsbsBox->isChecked();
    status.rwps = ui->rwpsBox->isChecked();
    status.rfms = ui->rfmsBox->isChecked();

    QSettings settings;
    settings.beginGroup("NewProjectDialog");

    settings.setValue("defaultDir", ui->locationBox->text());

    settings.endGroup();

    QDialog::accept();
}

void NewProjectDialog::reject()
{
    ui->nameBox->setText(status.name);
    ui->locationBox->setText(status.path);
    ui->sbsBox->setChecked(status.sbs);
    ui->wpsBox->setChecked(status.wps);
    ui->fmsBox->setChecked(status.fms);
    ui->rsbsBox->setChecked(status.rsbs);
    ui->rwpsBox->setChecked(status.rwps);
    ui->rfmsBox->setChecked(status.rfms);

    QSettings settings;
    settings.beginGroup("NewProjectDialog");

    ui->locationBox->setText(settings.value("defaultDir",
                                            QDir::home().absolutePath())
                             .toString());

    settings.endGroup();

    QDialog::reject();
}

void NewProjectDialog::browse()
{
    QString path;
    path = QFileDialog::getExistingDirectory(this, "New Project Location",
                                             ui->locationBox->text());
    ui->locationBox->setText(path);
}
