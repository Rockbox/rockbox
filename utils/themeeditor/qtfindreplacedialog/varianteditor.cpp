/*
 * Copyright 2010, Robert Bieber
 * Licensed under the LGPLv2.1, see the COPYING file for more information
 */

#include <QPushButton>
#include <QTextEdit>
#include <QPlainTextEdit>

#include "varianteditor.h"

VariantEditor::VariantEditor(QPlainTextEdit *plainTextEdit)
    : plainTextEdit(plainTextEdit), textEdit(0), type(Plain)
{
}

VariantEditor::VariantEditor(QTextEdit *textEdit)
    : plainTextEdit(0), textEdit(textEdit), type(Rich)
{
}

void VariantEditor::connectToSetEnabled(QPushButton *button)
{
    if(type == Rich)
        QObject::connect(textEdit, SIGNAL(copyAvailable(bool)),
                         button, SLOT(setEnabled(bool)));
    else
        QObject::connect(plainTextEdit, SIGNAL(copyAvailable(bool)),
                         button, SLOT(setEnabled(bool)));
}

QTextDocument* VariantEditor::document()
{
    return type == Rich ? textEdit->document() : plainTextEdit->document();
}

void VariantEditor::setTextCursor(const QTextCursor& cursor)
{
    if(type == Rich)
        textEdit->setTextCursor(cursor);
    else
        plainTextEdit->setTextCursor(cursor);
}

bool VariantEditor::find(const QString& exp, QTextDocument::FindFlags flags)
{
    return type == Rich ? textEdit->find(exp, flags) : plainTextEdit->find(exp, flags);
}

QTextCursor VariantEditor::textCursor() const
{
    return type == Rich ? textEdit->textCursor() : plainTextEdit->textCursor();
}
