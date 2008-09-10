/* This file is part of the KDE libraries
    Copyright (C) 2005, 2006 KJSEmbed Authors
    See included AUTHORS file.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
    
    --------------------------------------------------------------------------------------
    Imported into the WPS editor and simplified by Dominik Wenger
    
*/


#ifndef NUMBERED_TEXT_VIEW_H
#define NUMBERED_TEXT_VIEW_H

#include <QFrame>
#include <QPixmap>
#include <QTextCursor>

class QTextEdit;
class QHBoxLayout;

// Shows the Line numbers
class NumberBar : public QWidget
{
    Q_OBJECT

public:
    NumberBar( QWidget *parent );
    ~NumberBar();

    void markLine( int lineno );

    void setTextEdit( QTextEdit *edit );
    void paintEvent( QPaintEvent *ev );

private:
    QTextEdit *edit;
    QPixmap markerIcon;
    int markedLine;
    QRect markedRect;
};

// Shows a QTextEdit with Line numbers
class NumberedTextView : public QFrame
{
    Q_OBJECT

public:
    NumberedTextView( QWidget *parent = 0 );
    ~NumberedTextView();

    /** Returns the QTextEdit of the main view. */
    QTextEdit *textEdit() const { return view; }

    /* marks the line with a icon */
    void markLine( int lineno );
    
    void scrolltoLine( int lineno );
    
private slots:
    void textChanged( int pos, int removed, int added );
    
private:
    QTextEdit *view;
    NumberBar *numbers;
    QHBoxLayout *box;
    QTextCursor highlight;
    int markedLine;
};


#endif // NUMBERED_TEXT_VIEW_H

