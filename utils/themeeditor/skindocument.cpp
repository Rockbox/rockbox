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

#include "skindocument.h"

#include <QFile>
#include <QTimer>
#include <QSettings>
#include <QMessageBox>
#include <QFileDialog>

SkinDocument::SkinDocument(QWidget *parent) :
    QWidget(parent)
{
    setupUI();

    title = "Untitled";
    fileName = "";
    saved = "";
}

SkinDocument::~SkinDocument()
{
    delete highlighter;
    delete model;
}

bool SkinDocument::requestClose()
{
    if(editor->document()->toPlainText() != saved)
    {
        /* Spawning the "Are you sure?" dialog */
        QMessageBox confirm(this);
        confirm.setWindowTitle(tr("Confirm Close"));
        confirm.setText(title + tr(" has been modified."));
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
            if(editor->document()->toPlainText() != saved)
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

void SkinDocument::setupUI()
{
    /* Setting up the text edit */
    layout = new QHBoxLayout;
    editor = new QPlainTextEdit(this);
    layout->addWidget(editor);

    setLayout(layout);

    /* Attaching the syntax highlighter */
    highlighter = new SkinHighlighter(QColor(0,180,0), QColor(255,0,0),
                                      QColor(0,0,255), QColor(120,120,120),
                                      editor->document());

    /* Setting up the model */
    model = new ParseTreeModel("");

    /* Connecting the editor's signal */
    QObject::connect(editor, SIGNAL(textChanged()),
                     this, SLOT(codeChanged()));
}

void SkinDocument::codeChanged()
{
    model->changeTree(editor->document()->toPlainText().toAscii());

    if(editor->document()->toPlainText() != saved)
        emit titleChanged(title + QChar('*'));
    else
        emit titleChanged(title);
}

void SkinDocument::save()
{
    QFile fout(fileName);

    if(!fout.exists())
    {
        saveAs();
        return;
    }

    fout.open(QFile::WriteOnly);
    fout.write(editor->document()->toPlainText().toAscii());
    fout.close();

    saved = editor->document()->toPlainText();
    QStringList decompose = fileName.split('/');
    title = decompose[decompose.count() - 1];
    emit titleChanged(title);

}

void SkinDocument::saveAs()
{
    /* Determining the directory to open */
    QString directory = fileName;

    QSettings settings;
    settings.beginGroup("SkinDocument");
    if(directory == "")
        directory = settings.value("defaultDirectory", "").toString();

    fileName = QFileDialog::getSaveFileName(this, tr("Save Document"),
                                            directory, fileFilter());
    directory = fileName;
    if(fileName == "")
        return;

    directory.chop(fileName.length() - fileName.lastIndexOf('/') - 1);
    settings.setValue("defaultDirectory", directory);
    settings.endGroup();

    QFile fout(fileName);
    fout.open(QFile::WriteOnly);
    fout.write(editor->document()->toPlainText().toAscii());
    fout.close();

    saved = editor->document()->toPlainText();
    QStringList decompose = fileName.split('/');
    title = decompose[decompose.count() - 1];
    emit titleChanged(title);

}
