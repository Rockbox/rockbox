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


#include <QTextDocument>
#include <QTextBlock>
#include <QTextEdit>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QPainter>
#include <QAbstractTextDocumentLayout>
#include <QDebug>

#include "numberedtextview.h"

NumberBar::NumberBar( QWidget *parent )
    : QWidget( parent ), edit(0), markedLine(-1)
{
    // Make room for 4 digits and the breakpoint icon
    setFixedWidth( fontMetrics().width( QString("0000") + 10 + 32 ) );
    markerIcon = QPixmap( "images/marker.png" );
}

NumberBar::~NumberBar()
{
}


void NumberBar::markLine( int lineno )
{
    markedLine = lineno;
}

void NumberBar::setTextEdit( QTextEdit *edit )
{
    this->edit = edit;
    connect( edit->document()->documentLayout(), SIGNAL( update(const QRectF &) ),
	     this, SLOT( update() ) );
    connect( edit->verticalScrollBar(), SIGNAL(valueChanged(int) ),
	     this, SLOT( update() ) );
}

void NumberBar::paintEvent( QPaintEvent * )
{
    QAbstractTextDocumentLayout *layout = edit->document()->documentLayout();
    int contentsY = edit->verticalScrollBar()->value();
    qreal pageBottom = contentsY + edit->viewport()->height();
    const QFontMetrics fm = fontMetrics();
    const int ascent = fontMetrics().ascent() + 1; // height = ascent + descent + 1
    int lineCount = 1;

    QPainter p(this);

    markedRect = QRect();
   
    for ( QTextBlock block = edit->document()->begin();
	  block.isValid(); block = block.next(), ++lineCount ) 
    {

        const QRectF boundingRect = layout->blockBoundingRect( block );

        QPointF position = boundingRect.topLeft();
        if ( position.y() + boundingRect.height() < contentsY )
            continue;
        if ( position.y() > pageBottom )
            break;

        const QString txt = QString::number( lineCount );
        p.drawText( width() - fm.width(txt), qRound( position.y() ) - contentsY + ascent, txt );

        // marker
        if ( markedLine == lineCount )
        {
            p.drawPixmap( 1, qRound( position.y() ) - contentsY, markerIcon );
            markedRect = QRect( 1, qRound( position.y() ) - contentsY, markerIcon.width(), markerIcon.height() );
        }
    }
}

NumberedTextView::NumberedTextView( QWidget *parent )
    : QFrame( parent )
{
    setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
    setLineWidth( 2 );

    // Setup the main view
    view = new QTextEdit( this );
    //view->setFontFamily( "Courier" );
    view->setLineWrapMode( QTextEdit::NoWrap );
    view->setFrameStyle( QFrame::NoFrame );
   
    connect( view->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(textChanged(int,int,int)) );

    // Setup the line number pane
    numbers = new NumberBar( this );
    numbers->setTextEdit( view );

    // Test
    markLine(2);
    
    //setup layout
    box = new QHBoxLayout( this );
    box->setSpacing( 0 );
    box->setMargin( 0 );
    box->addWidget( numbers );
    box->addWidget( view );
}

NumberedTextView::~NumberedTextView()
{
}

void NumberedTextView::markLine( int lineno )
{
    markedLine = lineno;
    numbers->markLine( lineno );
    textChanged(1,1,1);
}

void NumberedTextView::scrolltoLine( int lineno )
{
    int max = view->verticalScrollBar()->maximum();
    int min = view->verticalScrollBar()->minimum();
    int lines = view->document()->blockCount();
    view->verticalScrollBar()->setValue( (max*lineno)/lines+min );
}

void NumberedTextView::textChanged( int pos, int removed, int added )
{
    Q_UNUSED( pos );

    if ( removed == 0 && added == 0 )
	return;

    QTextBlock block = highlight.block();
    QTextBlockFormat fmt = block.blockFormat();
    QColor bg = view->palette().base().color();
    fmt.setBackground( bg );
    highlight.setBlockFormat( fmt );

    int lineCount = 1;
    for ( QTextBlock block = view->document()->begin();
	  block.isValid(); block = block.next(), ++lineCount ) 
    {
        if ( lineCount == markedLine )
        {
            fmt = block.blockFormat();
            QColor bg = Qt::red;
            fmt.setBackground( bg.light(150) );

            highlight = QTextCursor( block );
            highlight.movePosition( QTextCursor::EndOfBlock, QTextCursor::KeepAnchor );
            highlight.setBlockFormat( fmt );

            break;
        }
    }
}

