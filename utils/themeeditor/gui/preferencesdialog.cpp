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

#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"
#include "fontdownloader.h"
#include "targetdownloader.h"

#include <QSettings>
#include <QColorDialog>
#include <QFileDialog>

PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);
    setupUI();
    loadSettings();
}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

void PreferencesDialog::loadSettings()
{
    loadColors();
    loadFont();
    loadRender();

    QSettings settings;
    settings.beginGroup("CodeEditor");
    ui->completionBox->setChecked(settings.value("completeSyntax",
                                                 true).toBool());
    settings.endGroup();
}

void PreferencesDialog::loadColors()
{

    QSettings settings;

    /* The list of buttons from the SkinHighlighter group */

    settings.beginGroup("SkinHighlighter");

    commentColor = settings.value("commentColor",
                                  QColor(0, 180, 0)).value<QColor>();
    setButtonColor(ui->commentButton, commentColor);

    escapedColor = settings.value("escapedColor",
                                  QColor(120,120,120)).value<QColor>();
    setButtonColor(ui->escapedButton, escapedColor);

    conditionalColor = settings.value("conditionalColor",
                                      QColor(0, 0, 180)).value<QColor>();
    setButtonColor(ui->conditionalButton, conditionalColor);

    tagColor = settings.value("tagColor",
                              QColor(180, 0, 0)).value<QColor>();
    setButtonColor(ui->tagButton, tagColor);

    settings.endGroup();

    /* Buttons from the editor group */
    settings.beginGroup("SkinDocument");

    fgColor = settings.value("fgColor", QColor(Qt::black)).value<QColor>();
    setButtonColor(ui->fgButton, fgColor);

    bgColor = settings.value("bgColor", QColor(Qt::white)).value<QColor>();
    setButtonColor(ui->bgButton, bgColor);

    errorColor = settings.value("errorColor", QColor(Qt::red)).value<QColor>();
    setButtonColor(ui->errorButton, errorColor);

    settings.endGroup();
}

void PreferencesDialog::loadFont()
{
    QSettings settings;
    settings.beginGroup("SkinDocument");

    QFont def("Monospace");
    def.setStyleHint(QFont::TypeWriter);

    QVariant family = settings.value("fontFamily", def);
    int size = settings.value("fontSize", 12).toInt();

    settings.endGroup();

    ui->fontSelect->setCurrentFont(family.value<QFont>());
    ui->fontSize->setValue(size);

}

void PreferencesDialog::loadRender()
{
    QSettings settings;
    settings.beginGroup("RBFont");

    QString confDir = QDir::homePath() + "/.rbthemeeditor";

    ui->fontBox->setText(settings.value("fontDir", confDir + "/fonts/")
                         .toString());

    settings.endGroup();

    settings.beginGroup("EditorWindow");

    ui->autoExpandBox->setChecked(settings.value("autoExpandTree",
                                                 false).toBool());
    ui->autoHighlightBox->setChecked(settings.value("autoHighlightTree",
                                                    false).toBool());

    settings.endGroup();

    settings.beginGroup("TargetData");

    ui->dbBox->setText(settings.value("targetDbPath",
                                      confDir + "/targetdb")
                       .toString());

    settings.endGroup();
}

void PreferencesDialog::saveSettings()
{
    saveColors();
    saveFont();
    saveRender();

    QSettings settings;
    settings.beginGroup("CodeEditor");
    settings.setValue("completeSyntax", ui->completionBox->isChecked());
    settings.endGroup();

}

void PreferencesDialog::saveColors()
{
    QSettings settings;

    /* Saving the editor colors */
    settings.beginGroup("SkinDocument");

    settings.setValue("fgColor", fgColor);
    settings.setValue("bgColor", bgColor);
    settings.setValue("errorColor", errorColor);

    settings.endGroup();

    /* Saving the highlighting colors */
    settings.beginGroup("SkinHighlighter");

    settings.setValue("tagColor", tagColor);
    settings.setValue("commentColor", commentColor);
    settings.setValue("conditionalColor", conditionalColor);
    settings.setValue("escapedColor", escapedColor);

    settings.endGroup();
}

void PreferencesDialog::saveFont()
{
    QSettings settings;
    settings.beginGroup("SkinDocument");

    settings.setValue("fontFamily", ui->fontSelect->currentFont());
    settings.setValue("fontSize", ui->fontSize->value());

    settings.endGroup();
}

void PreferencesDialog::saveRender()
{
    QSettings settings;
    settings.beginGroup("RBFont");

    settings.setValue("fontDir", ui->fontBox->text());

    settings.endGroup();

    settings.beginGroup("EditorWindow");

    settings.setValue("autoExpandTree", ui->autoExpandBox->isChecked());
    settings.setValue("autoHighlightTree", ui->autoHighlightBox->isChecked());

    settings.endGroup();

    settings.beginGroup("TargetData");
    settings.setValue("targetDbPath", ui->dbBox->text());
    settings.endGroup();
}

void PreferencesDialog::setupUI()
{
    /* Connecting color buttons */
    QList<QPushButton*> buttons;
    buttons.append(ui->bgButton);
    buttons.append(ui->fgButton);
    buttons.append(ui->commentButton);
    buttons.append(ui->tagButton);
    buttons.append(ui->conditionalButton);
    buttons.append(ui->escapedButton);
    buttons.append(ui->errorButton);

    for(int i = 0; i < buttons.count(); i++)
        QObject::connect(buttons[i], SIGNAL(pressed()),
                         this, SLOT(colorClicked()));

    QObject::connect(ui->fontBrowseButton, SIGNAL(clicked()),
                     this, SLOT(browseFont()));
    QObject::connect(ui->browseDB, SIGNAL(clicked()),
                     this, SLOT(browseDB()));
    QObject::connect(ui->dlFontsButton, SIGNAL(clicked()),
                     this, SLOT(dlFonts()));
    QObject::connect(ui->dlTargetButton, SIGNAL(clicked()),
                     this, SLOT(dlTargetDB()));
}

void PreferencesDialog::colorClicked()
{
    QColor* toEdit = 0, newColor;

    if(QObject::sender() == ui->bgButton)
        toEdit = &bgColor;
    else if(QObject::sender() == ui->fgButton)
        toEdit = &fgColor;
    else if(QObject::sender() == ui->commentButton)
        toEdit = &commentColor;
    else if(QObject::sender() == ui->tagButton)
        toEdit = &tagColor;
    else if(QObject::sender() == ui->conditionalButton)
        toEdit = &conditionalColor;
    else if(QObject::sender() == ui->escapedButton)
        toEdit = &escapedColor;
    else if(QObject::sender() == ui->errorButton)
        toEdit = &errorColor;

    if(!toEdit)
        return;

    newColor = QColorDialog::getColor(*toEdit, this);
    if (newColor.isValid())
    {
        *toEdit = newColor;
        setButtonColor(dynamic_cast<QPushButton*>(QObject::sender()), *toEdit);
    }
}

void PreferencesDialog::browseFont()
{
    QString path = QFileDialog::
                   getExistingDirectory(this, "Font Directory",
                                        ui->fontBox->text());
    ui->fontBox->setText(path);
}

void PreferencesDialog::browseDB()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Target DB"),
                                                QDir(ui->dbBox->text()).
                                                absolutePath(),
                                                "All Files (*)");
    ui->dbBox->setText(path);
}

void PreferencesDialog::dlFonts()
{
    FontDownloader* dl = new FontDownloader(this, ui->fontBox->text());
    dl->show();
}

void PreferencesDialog::dlTargetDB()
{
    TargetDownloader* dl = new TargetDownloader(this, ui->dbBox->text());
    dl->show();
}

void PreferencesDialog::accept()
{
    saveSettings();
    QDialog::accept();
}

void PreferencesDialog::reject()
{
    loadSettings();
    QDialog::reject();
}
