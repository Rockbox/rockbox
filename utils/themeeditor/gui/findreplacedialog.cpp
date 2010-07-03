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

#include "findreplacedialog.h"
#include "ui_findreplacedialog.h"

#include <QTextBlock>

FindReplaceDialog::FindReplaceDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FindReplaceDialog), editor(0), textFound()
{
    ui->setupUi(this);
    setupUI();
}

FindReplaceDialog::~FindReplaceDialog()
{
    delete ui;
}

void FindReplaceDialog::changeEvent(QEvent *e)
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

void FindReplaceDialog::closeEvent(QCloseEvent* event)
{
    ui->statusLabel->setText("");
    event->accept();
}

void FindReplaceDialog::setupUI()
{
    QObject::connect(ui->findButton, SIGNAL(clicked()),
                     this, SLOT(find()));
    QObject::connect(ui->replaceButton, SIGNAL(clicked()),
                     this, SLOT(replace()));
    QObject::connect(ui->replaceAllButton, SIGNAL(clicked()),
                     this, SLOT(replaceAll()));
    QObject::connect(ui->closeButton, SIGNAL(clicked()),
                     this, SLOT(close()));
    QObject::connect(ui->findBox, SIGNAL(textChanged(QString)),
                     this, SLOT(textChanged()));

    textChanged();
}

void FindReplaceDialog::find()
{

    if(!editor)
        return;

    QTextDocument::FindFlags flags = 0;
    if(ui->caseBox->isChecked())
        flags |= QTextDocument::FindCaseSensitively;
    if(ui->backwardsBox->isChecked())
        flags |= QTextDocument::FindBackward;

    QTextCursor start = textFound.isNull() ? editor->textCursor() : textFound;

    textFound = editor->document()->find(ui->findBox->text(), start, flags);

    if(textFound.isNull() && ui->wrapBox->isChecked())
    {
        if(ui->backwardsBox->isChecked())
        {
            textFound = editor->document()
                        ->find(ui->findBox->text(),
                               editor->document()->toPlainText().length(),
                               flags);
        }
        else
        {
            textFound = editor->document()->find(ui->findBox->text(), 0, flags);
        }
    }

    QPalette newPal;
    if(!textFound.isNull())
    {
        newPal.setColor(QPalette::Foreground, QColor(0, 150, 0));
        ui->statusLabel->setPalette(newPal);
        ui->statusLabel->setText(tr("Match Found"));
        editor->setTextCursor(textFound);
    }
    else
    {
        newPal.setColor(QPalette::Foreground, Qt::red);
        ui->statusLabel->setPalette(newPal);
        ui->statusLabel->setText(tr("Match Not Found"));
        editor->setTextCursor(start);
    }

}

void FindReplaceDialog::replace()
{
    if(textFound.isNull())
        find();

    if(textFound.isNull())
        return;

    editor->setTextCursor(textFound);
    editor->insertPlainText(ui->replaceBox->text());
    textFound = QTextCursor();
}

void FindReplaceDialog::replaceAll()
{

    do
    {
        if(!textFound.isNull())
        {
            editor->setTextCursor(textFound);
            editor->insertPlainText(ui->replaceBox->text());
        }

        find();
    }while(!textFound.isNull());

}

void FindReplaceDialog::textChanged()
{
    bool enabled = ui->findBox->text() != "";

    ui->findButton->setEnabled(enabled);
    ui->replaceButton->setEnabled(enabled);
    ui->replaceAllButton->setEnabled(enabled);
}
