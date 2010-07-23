/*
 * Copyright 2010, Robert Bieber
 * Licensed under the LGPLv2.1, see the COPYING file for more information
 */

#ifndef VARIANTEDITOR_H
#define VARIANTEDITOR_H

#include <QTextDocument>

class QTextEdit;
class QPlainTextEdit;
class QPushButton;
class QTextCursor;

/**
  * A simple class that transparently wraps a pointer to either
  * a QPlainTextEdit or a QTextEdit, providing access to functions common
  * to each of them
  */

class VariantEditor
{
public:
    VariantEditor(QTextEdit* textEdit);
    VariantEditor(QPlainTextEdit* textEdit);

    void connectToSetEnabled(QPushButton* button);

    QTextDocument* document();
    void setTextCursor(const QTextCursor& cursor);
    bool find(const QString& exp, QTextDocument::FindFlags flags);
    QTextCursor textCursor() const;

private:
    QPlainTextEdit* plainTextEdit;
    QTextEdit* textEdit;

    enum{ Plain, Rich} type;

};

#endif // VARIANTEDITOR_H
