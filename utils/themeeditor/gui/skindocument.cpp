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
 * This program is free software;  can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your optiyouon) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "skindocument.h"

#include <QFile>
#include <QSettings>
#include <QColor>
#include <QMessageBox>
#include <QFileDialog>

#include <iostream>

#include <QDebug>

const int SkinDocument::updateInterval = 500;

SkinDocument::SkinDocument(QLabel* statusLabel, ProjectModel* project,
                           DeviceState* device, QWidget *parent)
                               :TabContent(parent), statusLabel(statusLabel),
                               project(project), device(device),
                               treeInSync(true)
{
    setupUI();

    titleText = "Untitled";
    fileName = "";
    saved = "";
    parseStatus = tr("Empty document");
    blockUpdate = false;
    currentLine = -1;
}

SkinDocument::SkinDocument(QLabel* statusLabel, QString file,
                           ProjectModel* project, DeviceState* device,
                           QWidget *parent)
                               :TabContent(parent), fileName(file),
                               statusLabel(statusLabel), project(project),
                               device(device), treeInSync(true)
{
    setupUI();
    blockUpdate = false;

    /* Loading the file */
    if(QFile::exists(fileName))
    {
        QFile fin(fileName);
        fin.open(QFile::ReadOnly);
        editor->document()->setPlainText(QString(fin.readAll()));
        saved = editor->document()->toPlainText();
        editor->setTextCursor(QTextCursor(editor->document()->begin()));
        fin.close();
    }

    /* Setting the title */
    QStringList decomposed = fileName.split('/');
    titleText = decomposed.last();

    /* Setting the current screen device setting */
    QString extension = titleText.split(".").last().toLower().right(3);
    if(extension == "wps")
    {
        device->setData("cs", "WPS");
    }
    else if(extension == "sbs")
    {
        device->setData("cs", "Menus");
    }
    else if(extension == "fms")
    {
        device->setData("cs", "FM Radio Screen");
    }

    lastUpdate = QTime::currentTime();
}

SkinDocument::~SkinDocument()
{
    highlighter->deleteLater();
    model->deleteLater();
}

void SkinDocument::connectPrefs(PreferencesDialog* prefs)
{
    QObject::connect(prefs, SIGNAL(accepted()),
                     this, SLOT(settingsChanged()));
    QObject::connect(prefs, SIGNAL(accepted()),
                     highlighter, SLOT(loadSettings()));
}

bool SkinDocument::requestClose()
{
    /* Storing the response in blockUpdate will also block updates to the
       status bar if the tab is being closed */
    if(editor->document()->toPlainText() != saved)
    {
        /* Spawning the "Are you sure?" dialog */
        QMessageBox confirm(this);
        confirm.setWindowTitle(tr("Confirm Close"));
        confirm.setText(titleText + tr(" has been modified."));
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
                blockUpdate = false;
            else
                blockUpdate = true;
            break;

        case QMessageBox::Discard:
            blockUpdate = true;
            break;

        case QMessageBox::Cancel:
            blockUpdate = false;
            break;
        }
    }
    else
        blockUpdate = true;

    return blockUpdate;
}

void SkinDocument::setupUI()
{
    /* Setting up the text edit */
    layout = new QHBoxLayout;
    editor = new CodeEditor(this);
    editor->setLineWrapMode(QPlainTextEdit::NoWrap);
    layout->addWidget(editor);

    setLayout(layout);

    /* Attaching the syntax highlighter */
    highlighter = new SkinHighlighter(editor->document());

    /* Setting up the model */
    model = new ParseTreeModel("");

    QObject::connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                     this, SLOT(modelChanged()));

    /* Connecting the editor's signal */
    QObject::connect(editor, SIGNAL(textChanged()),
                     this, SLOT(codeChanged()));
    QObject::connect(editor, SIGNAL(cursorPositionChanged()),
                     this, SLOT(cursorChanged()));

    /* Connecting to device setting changes */
    QObject::connect(device, SIGNAL(settingsChanged()),
                     this, SLOT(deviceChanged()));

    /* Attaching the find/replace dialog */
    findReplace = new FindReplaceDialog(this);
    findReplace->setModal(false);
    findReplace->setTextEdit(editor);
    findReplace->hide();

    settingsChanged();

    /* Setting up a timer to check for updates */
    checkUpdate.setInterval(500);
    QObject::connect(&checkUpdate, SIGNAL(timeout()),
                     this, SLOT(codeChanged()));
}

void SkinDocument::settingsChanged()
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

void SkinDocument::cursorChanged()
{
    if(editor->isError(editor->textCursor().blockNumber() + 1))
    {
        QTextCursor line = editor->textCursor();
        line.movePosition(QTextCursor::StartOfLine);
        line.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        skin_parse(line.selectedText().toLatin1());
        if(skin_error_line() > 0)
            parseStatus = tr("Error on line ") +
                          QString::number(line.blockNumber() + 1)
                          + tr(", column ") + QString::number(skin_error_col())
                          + tr(": ") +
                          skin_error_message();
        statusLabel->setText(parseStatus);
    }
    else if(editor->hasErrors())
    {
        parseStatus = tr("Errors in document");
        statusLabel->setText(parseStatus);
    }
    else if(editor->textCursor().blockNumber() != currentLine)
    {
        currentLine = editor->textCursor().blockNumber();
        emit lineChanged(editor->textCursor().blockNumber() + 1);
    }

}

void SkinDocument::codeChanged()
{
    if(blockUpdate)
        return;

    if(editor->document()->isEmpty())
    {
        parseStatus = tr("Empty document");
        statusLabel->setText(parseStatus);
        return;
    }

    editor->clearErrors();
    parseStatus = model->changeTree(editor->document()->
                                    toPlainText().toLatin1());

    treeInSync = true;
    emit antiSync(false);

    if(skin_error_line() > 0)
        parseStatus = tr("Errors in document");
    statusLabel->setText(parseStatus);

    /* Highlighting if an error was found */
    if(skin_error_line() > 0)
    {
        editor->addError(skin_error_line());

        /* Now we're going to attempt parsing again at each line, until we find
           one that won't error out*/
        QTextDocument doc(editor->document()->toPlainText());
        int base = 0;
        while(skin_error_line() > 0 && !doc.isEmpty())
        {
            QTextCursor rest(&doc);

            for(int i = 0; i < skin_error_line(); i++)
                rest.movePosition(QTextCursor::NextBlock,
                                  QTextCursor::KeepAnchor);
            if(skin_error_line() == doc.blockCount())
                rest.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);

            rest.removeSelectedText();
            base += skin_error_line();

            skin_parse(doc.toPlainText().toLatin1());

            if(skin_error_line() > 0)
                editor->addError(base + skin_error_line());

        }
    }

    if(editor->document()->toPlainText() != saved)
        emit titleChanged(titleText + QChar('*'));
    else
        emit titleChanged(titleText);

    if(lastUpdate.msecsTo(QTime::currentTime()) >= updateInterval)
    {
        model->render(project, device, this, &fileName);
        checkUpdate.stop();
        lastUpdate = QTime::currentTime();
    }
    else
    {
        checkUpdate.start();
    }
    cursorChanged();

}

void SkinDocument::modelChanged()
{
    treeInSync = false;
    emit antiSync(true);
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
    fout.write(editor->document()->toPlainText().toLatin1());
    fout.close();

    saved = editor->document()->toPlainText();
    QStringList decompose = fileName.split('/');
    titleText = decompose.last();
    emit titleChanged(titleText);

    scene();

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
    fout.write(editor->document()->toPlainText().toLatin1());
    fout.close();

    saved = editor->document()->toPlainText();
    QStringList decompose = fileName.split('/');
    titleText = decompose[decompose.count() - 1];
    emit titleChanged(titleText);

    scene();

}

QString SkinDocument::findSetting(QString key, QString fallback)
{
    if(!project)
        return fallback;
    else
        return project->getSetting(key, fallback);
}
