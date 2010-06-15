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

#include "projectmodel.h"
#include "configdocument.h"
#include "ui_configdocument.h"

#include <QMessageBox>
#include <QFile>
#include <QSettings>
#include <QFileDialog>

ConfigDocument::ConfigDocument(QMap<QString, QString>& settings, QString file,
                               QWidget *parent)
                                   : TabContent(parent),
                                   ui(new Ui::ConfigDocument),
                                   filePath(file)
{
    ui->setupUi(this);

    QMap<QString, QString>::iterator i;
    for(i = settings.begin(); i != settings.end(); i++)
        addRow(i.key(), i.value());

    saved = toPlainText();

    QObject::connect(ui->addKeyButton, SIGNAL(pressed()),
                     this, SLOT(addClicked()));
}

ConfigDocument::~ConfigDocument()
{
    delete ui;
}

void ConfigDocument::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

QString ConfigDocument::title() const
{
    QStringList decompose = filePath.split("/");
    return decompose.last();
}

void ConfigDocument::save()
{
    QFile fout(filePath);

    if(!fout.exists())
    {
        saveAs();
        return;
    }

    fout.open(QFile::WriteOnly);
    fout.write(toPlainText().toAscii());
    fout.close();

    saved = toPlainText();
    emit titleChanged(title());

}

void ConfigDocument::saveAs()
{
    /* Determining the directory to open */
    QString directory = filePath;

    QSettings settings;
    settings.beginGroup("ProjectModel");
    if(directory == "")
        directory = settings.value("defaultDirectory", "").toString();

    filePath = QFileDialog::getSaveFileName(this, tr("Save Document"),
                                            directory,
                                            ProjectModel::fileFilter());
    directory = filePath;
    if(filePath == "")
        return;

    directory.chop(filePath.length() - filePath.lastIndexOf('/') - 1);
    settings.setValue("defaultDirectory", directory);
    settings.endGroup();

    QFile fout(filePath);
    fout.open(QFile::WriteOnly);
    fout.write(toPlainText().toAscii());
    fout.close();

    saved = toPlainText();
    emit titleChanged(title());
    emit configFileChanged(file());

}

bool ConfigDocument::requestClose()
{
    if(toPlainText() != saved)
    {
        /* Spawning the "Are you sure?" dialog */
        QMessageBox confirm(this);
        confirm.setWindowTitle(tr("Confirm Close"));
        confirm.setText(title() + tr(" has been modified."));
        confirm.setInformativeText(tr("Do you want to save your changes?"));
        confirm.setStandardButtons(QMessageBox::Save | QMessageBox::Discard
                                   | QMessageBox::Cancel);
        confirm.setDefaultButton(QMessageBox::Save);
        int confirmation = confirm.exec();

        switch(confirmation)
        {
        case QMessageBox::Save:
            save();
            /* After calling save, make sure the user actually went through */
            if(toPlainText() != saved)
                return false;
            else
                return true;

        case QMessageBox::Discard:
            return true;

        case QMessageBox::Cancel:
            return false;
        }
    }
    return true;
}

QString ConfigDocument::toPlainText() const
{
    QString buffer = "";

    for(int i = 0; i < keys.count(); i++)
    {
        buffer += keys[i]->text();
        buffer += ":";
        buffer += values[i]->text();
        buffer += "\n";
    }

    return buffer;
}

void ConfigDocument::addRow(QString key, QString value)
{
    QHBoxLayout* layout = new QHBoxLayout();
    QLineEdit* keyEdit = new QLineEdit(key, this);
    QLineEdit* valueEdit = new QLineEdit(value, this);
    QPushButton* delButton = new QPushButton(tr("-"), this);

    layout->addWidget(keyEdit);
    layout->addWidget(valueEdit);
    layout->addWidget(delButton);

    delButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    delButton->setMaximumWidth(35);

    QObject::connect(delButton, SIGNAL(clicked()),
                     this, SLOT(deleteClicked()));

    QObject::connect(keyEdit, SIGNAL(textChanged(QString)),
                     this, SLOT(textChanged()));
    QObject::connect(valueEdit, SIGNAL(textChanged(QString)),
                     this, SLOT(textChanged()));

    ui->configBoxes->addLayout(layout);

    containers.append(layout);
    keys.append(keyEdit);
    values.append(valueEdit);
    deleteButtons.append(delButton);

}

void ConfigDocument::deleteClicked()
{
    QPushButton* button = dynamic_cast<QPushButton*>(sender());
    int row = deleteButtons.indexOf(button);

    deleteButtons[row]->deleteLater();
    keys[row]->deleteLater();
    values[row]->deleteLater();
    containers[row]->deleteLater();

    deleteButtons.removeAt(row);
    keys.removeAt(row);
    values.removeAt(row);
    containers.removeAt(row);

    if(saved != toPlainText())
        emit titleChanged(title() + "*");
    else
        emit titleChanged(title());
}

void ConfigDocument::addClicked()
{
    addRow(tr("Key"), tr("Value"));
}

void ConfigDocument::textChanged()
{
    if(toPlainText() != saved)
        emit titleChanged(title() + "*");
    else
        emit titleChanged(title());
}
