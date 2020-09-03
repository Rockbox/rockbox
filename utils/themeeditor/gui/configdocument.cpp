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
#include "preferencesdialog.h"

#include <QMessageBox>
#include <QFile>
#include <QSettings>
#include <QFileDialog>
#include <QPair>

ConfigDocument::ConfigDocument(QMap<QString, QString>& settings, QString file,
                               QWidget *parent)
                                   : TabContent(parent),
                                   ui(new Ui::ConfigDocument),
                                   filePath(file), block(false)
{
    ui->setupUi(this);
    editor = new CodeEditor(this);
    editor->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    editor->setLineWrapMode(CodeEditor::NoWrap);
    ui->splitter->addWidget(editor);

    QObject::connect(editor, SIGNAL(textChanged()),
                     this, SLOT(textChanged()));

    /* Loading the splitter status */
    QSettings appSettings;
    appSettings.beginGroup("ConfigDocument");

    if(!appSettings.value("textVisible", true).toBool())
    {
        editor->setVisible(false);
        ui->textButton->setChecked(false);
    }
    else
    {
        ui->textButton->setChecked(true);
    }
    if(!appSettings.value("linesVisible", false).toBool())
    {
        ui->scrollArea->setVisible(false);
        ui->linesButton->setChecked(false);
    }
    else
    {
        ui->linesButton->setChecked(true);
    }

    if(!appSettings.value("splitter", QByteArray()).toByteArray().isNull())
        ui->splitter->restoreState(appSettings.value("splitter").toByteArray());

    appSettings.endGroup();

    /* Populating the known keys list */
    QFile fin(":/resources/configkeys");
    fin.open(QFile::ReadOnly);

    QStringList* container = &primaryKeys;
    while(!fin.atEnd())
    {
        QString current = QString(fin.readLine());
        if(current == "-\n")
            container = &secondaryKeys;
        else if(current != "\n")
            container->append(current.trimmed());
    }

    /* Loading the text file */
    QFile finT(settings.value("configfile", ""));
    if(finT.open(QFile::Text | QFile::ReadOnly))
    {
        editor->setPlainText(QString(finT.readAll()));
        finT.close();
    }

    saved = toPlainText();

    QObject::connect(ui->addKeyButton, SIGNAL(pressed()),
                     this, SLOT(addClicked()));

    QObject::connect(ui->linesButton, SIGNAL(toggled(bool)),
                     this, SLOT(buttonChecked()));
    QObject::connect(ui->textButton, SIGNAL(toggled(bool)),
                     this, SLOT(buttonChecked()));

    settingsChanged();
}

ConfigDocument::~ConfigDocument()
{
    delete ui;
    editor->deleteLater();
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
    fout.write(toPlainText().toLatin1());
    fout.close();

    saved = toPlainText();
    emit titleChanged(title());
    emit configFileChanged(file());
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
    fout.write(toPlainText().toLatin1());
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
    return editor->toPlainText();
}

void ConfigDocument::syncFromBoxes()
{
    if(block)
        return;
    blockUpdates();

    QString buffer = "";

    for(int i = 0; i < keys.count(); i++)
    {
        buffer += keys[i]->currentText();
        buffer += ":";
        buffer += values[i]->text();
        buffer += "\n";
    }

    editor->setPlainText(buffer);
}

void ConfigDocument::syncFromText()
{
    if(block)
        return;

    blockUpdates();

    QStringList lines = editor->toPlainText().split("\n");
    QList<QPair<QString, QString> > splits;
    for(int i = 0; i < lines.count(); i++)
    {
        QString line = lines[i];
        QStringList split = line.split(":");
        if(split.count() != 2)
            continue;

        splits.append(QPair<QString, QString>(split[0].trimmed(),
                                              split[1].trimmed()));
    }

    while(deleteButtons.count() > splits.count())
    {
        deleteButtons[0]->deleteLater();
        keys[0]->deleteLater();
        values[0]->deleteLater();
        containers[0]->deleteLater();
        labels[0]->deleteLater();

        deleteButtons.removeAt(0);
        keys.removeAt(0);
        values.removeAt(0);
        containers.removeAt(0);
        labels.removeAt(0);
    }

    int initialCount = deleteButtons.count();
    for(int i = 0; i < splits.count(); i++)
    {
        if(i >= initialCount)
        {
            addRow(splits[i].first, splits[i].second);
        }
        else
        {
            int index = keys[i]->findText(splits[i].first);
            if(index != -1)
                keys[i]->setCurrentIndex(index);
            else
                keys[i]->setEditText(splits[i].first);
            values[i]->setText(splits[i].second);
        }
    }
}

void ConfigDocument::addRow(QString key, QString value)
{
    QHBoxLayout* layout = new QHBoxLayout();
    NoScrollCombo* keyEdit = new NoScrollCombo(this);
    QLineEdit* valueEdit = new QLineEdit(value, this);
    QPushButton* delButton = new QPushButton(tr("-"), this);
    QLabel* label = new QLabel(":");

    /* Loading the combo box options */
    keyEdit->setInsertPolicy(QComboBox::InsertAlphabetically);
    keyEdit->setEditable(true);
    keyEdit->addItems(primaryKeys);
    keyEdit->insertSeparator(keyEdit->count());
    keyEdit->addItems(secondaryKeys);
    if(keyEdit->findText(key) != -1)
        keyEdit->setCurrentIndex(keyEdit->findText(key));
    else
        keyEdit->setEditText(key);


    layout->addWidget(keyEdit);
    layout->addWidget(label);
    layout->addWidget(valueEdit);
    layout->addWidget(delButton);

    delButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    delButton->setMaximumWidth(35);

    QObject::connect(delButton, SIGNAL(clicked()),
                     this, SLOT(deleteClicked()));
    QObject::connect(keyEdit, SIGNAL(currentIndexChanged(QString)),
                     this, SLOT(boxesChanged()));
    QObject::connect(keyEdit, SIGNAL(textChanged(QString)),
                     this, SLOT(boxesChanged()));
    QObject::connect(valueEdit, SIGNAL(textChanged(QString)),
                     this, SLOT(boxesChanged()));

    ui->configBoxes->addLayout(layout);

    containers.append(layout);
    keys.append(keyEdit);
    values.append(valueEdit);
    deleteButtons.append(delButton);
    labels.append(label);

}

void ConfigDocument::deleteClicked()
{
    QPushButton* button = dynamic_cast<QPushButton*>(sender());
    int row = deleteButtons.indexOf(button);

    deleteButtons[row]->deleteLater();
    keys[row]->deleteLater();
    values[row]->deleteLater();
    containers[row]->deleteLater();
    labels[row]->deleteLater();

    deleteButtons.removeAt(row);
    keys.removeAt(row);
    values.removeAt(row);
    containers.removeAt(row);
    labels.removeAt(row);

    if(saved != toPlainText())
        emit titleChanged(title() + "*");
    else
        emit titleChanged(title());
}

void ConfigDocument::addClicked()
{
    addRow(tr("Key"), tr("Value"));
}

void ConfigDocument::boxesChanged()
{
    syncFromBoxes();
    contentsChanged();
}

void ConfigDocument::textChanged()
{
    syncFromText();
    contentsChanged();
}

void ConfigDocument::contentsChanged()
{
    if(toPlainText() != saved)
        emit titleChanged(title() + "*");
    else
        emit titleChanged(title());
}

void ConfigDocument::buttonChecked()
{
    editor->setVisible(ui->textButton->isChecked());
    ui->scrollArea->setVisible(ui->linesButton->isChecked());

    QSettings settings;
    settings.beginGroup("ConfigDocument");
    settings.setValue("textVisible", ui->textButton->isChecked());
    settings.setValue("linesVisible", ui->linesButton->isChecked());
    settings.endGroup();
}

void ConfigDocument::connectPrefs(PreferencesDialog *prefs)
{
    QObject::connect(prefs, SIGNAL(accepted()),
                     this, SLOT(settingsChanged()));
}


void ConfigDocument::settingsChanged()
{
    /* Setting the editor colors */
    QSettings settings;
    settings.beginGroup("SkinDocument");

    QColor fg = settings.value("fgColor", QColor(Qt::black)).value<QColor>();
    QColor bg = settings.value("bgColor", QColor(Qt::white)).value<QColor>();
    QPalette palette;
    palette.setColor(QPalette::All, QPalette::Base, bg);
    palette.setColor(QPalette::All, QPalette::Text, fg);
    editor->setPalette(palette);

    QColor highlight = settings.value("errorColor", QColor(Qt::red)).value<QColor>();
    editor->setErrorColor(highlight);

    /* Setting the font */
    QFont def("Monospace");
    def.setStyleHint(QFont::TypeWriter);
    QFont family = settings.value("fontFamily", def).value<QFont>();
    family.setPointSize(settings.value("fontSize", 12).toInt());
    editor->setFont(family);

    editor->repaint();

    settings.endGroup();

}
