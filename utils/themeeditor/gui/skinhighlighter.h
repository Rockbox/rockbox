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

#ifndef SKINHIGHLIGHTER_H
#define SKINHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QPlainTextEdit>

#include "tag_table.h"
#include "symbols.h"

class SkinHighlighter : public QSyntaxHighlighter
{
Q_OBJECT
public:
    /*
     * font - The font used for all text
     * normal - The normal text color
     * escaped - The color for escaped characters
     * tag - The color for tags and their delimiters
     * conditional - The color for conditionals and their delimiters
     *
     */
    SkinHighlighter(QTextDocument* doc);
    virtual ~SkinHighlighter();

    void highlightBlock(const QString& text);

public slots:
    void loadSettings();

private:
    QColor escaped;
    QColor tag;
    QColor conditional;
    QColor comment;

};

#endif // SKINHIGHLIGHTER_H
