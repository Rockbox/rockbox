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
    ui(new Ui::FindReplaceDialog), editor(0)
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
    QObject::connect(ui->findButton, SIGNAL(pressed()),
                     this, SLOT(find()));
    QObject::connect(ui->replaceButton, SIGNAL(pressed()),
                     this, SLOT(replace()));
    QObject::connect(ui->replaceAllButton, SIGNAL(pressed()),
                     this, SLOT(replaceAll()));
    QObject::connect(ui->closeButton, SIGNAL(pressed()),
                     this, SLOT(close()));
    QObject::connect(ui->findBox, SIGNAL(textChanged(QString)),
                     this, SLOT(textChanged()));

    textChanged();
}

void FindReplaceDialog::find()
{

    if(!editor)
        return;

    editor->setTextCursor(editor->document()->find(ui->findBox->text()));

}

void FindReplaceDialog::replace()
{

}

void FindReplaceDialog::replaceAll()
{

}

void FindReplaceDialog::textChanged()
{
    bool enabled = ui->findBox->text() != "";

    ui->findButton->setEnabled(enabled);
    ui->replaceButton->setEnabled(enabled);
    ui->replaceAllButton->setEnabled(enabled);
}
