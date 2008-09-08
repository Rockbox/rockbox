/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Rostilav Checkan
 *   $Id$
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

#ifndef QSYNTAXER_H
#define QSYNTAXER_H
//
#include <QSyntaxHighlighter>

class QTextCharFormat;

class QSyntaxer : public QSyntaxHighlighter {
    Q_OBJECT
    struct HighlightingRule {
        QRegExp pattern;
        QTextCharFormat format;
    };
    QMap<int,HighlightingRule> hrules;
public:
    QSyntaxer(QTextDocument *parent = 0);

protected:
    void highlightBlock(const QString &text);
};
#endif
