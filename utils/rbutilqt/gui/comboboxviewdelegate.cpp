/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2011 by Dominik Riebeling
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <QStyledItemDelegate>
#include <QPainter>
#include <QApplication>
#include <qdebug.h>
#include "comboboxviewdelegate.h"

void ComboBoxViewDelegate::paint(QPainter *painter,
        const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QPen pen;
    QFont font;
    pen = painter->pen();
    font = painter->font();

    painter->save();
    // paint selection
    if(option.state & QStyle::State_Selected) {
        painter->setPen(QPen(Qt::NoPen));
        painter->setBrush(QApplication::palette().highlight());
        painter->drawRect(option.rect);
        painter->restore();
        painter->save();
        pen.setColor(QApplication::palette().color(QPalette::HighlightedText));
    }
    else {
        pen.setColor(QApplication::palette().color(QPalette::Text));
    }
    // draw data (text)
    painter->setPen(pen);
    painter->drawText(option.rect, Qt::AlignLeft, index.data().toString());

    // draw user data right aligned, italic
    font.setItalic(true);
    painter->setFont(font);
    painter->drawText(option.rect, Qt::AlignRight, index.data(Qt::UserRole).toString());
    painter->restore();
}

