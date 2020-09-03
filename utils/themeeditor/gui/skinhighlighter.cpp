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

#include "skinhighlighter.h"

#include <QSettings>

SkinHighlighter::SkinHighlighter(QTextDocument* doc)
    :QSyntaxHighlighter(doc)
{
    loadSettings();
}

SkinHighlighter::~SkinHighlighter()
{

}

void SkinHighlighter::highlightBlock(const QString& text)
{
    for(int i = 0; i < text.length(); i++)
    {
        QChar c = text[i];

        /* Checking for delimiters */
        if(c == ARGLISTOPENSYM
           || c == ARGLISTCLOSESYM
           || c == ARGLISTSEPARATESYM)
            setFormat(i, 1, tag);

        if(c == ENUMLISTOPENSYM
           || c == ENUMLISTCLOSESYM
           || c == ENUMLISTSEPARATESYM)
            setFormat(i, 1, conditional);

        /* Checking for comments */
        if(c == COMMENTSYM)
        {
            setFormat(i, text.length() - i, comment);
            return;
        }

        if(c == TAGSYM)
        {
            if(text.length() - i < 2)
                return;

            if(find_escape_character(text[i + 1].toLatin1()))
            {
                /* Checking for escaped characters */

                setFormat(i, 2, escaped);
                i++;
            }
            else if(text[i + 1] != CONDITIONSYM)
            {
                /* Checking for normal tags */

                char lookup[3];
                const struct tag_info* found = 0;

                /* First checking for a two-character tag name */
                lookup[2] = '\0';

                if(text.length() - i >= 3)
                {
                    lookup[0] = text[i + 1].toLatin1();
                    lookup[1] = text[i + 2].toLatin1();

                    found = find_tag(lookup);
                }

                if(found)
                {
                    setFormat(i, 3, tag);
                    i += 2;
                }
                else
                {
                    lookup[1] = '\0';
                    lookup[0] = text[i + 1].toLatin1();
                    found = find_tag(lookup);

                    if(found)
                    {
                        setFormat(i, 2, tag);
                        i++;
                    }
                }

            }
            else if(text[i + 1] == CONDITIONSYM)
            {
                /* Checking for conditional tags */

                if(text.length() - i < 3)
                    return;

                char lookup[3];
                const struct tag_info* found = 0;

                lookup[2] = '\0';

                if(text.length() - i >= 4)
                {
                    lookup[0] = text[i + 2].toLatin1();
                    lookup[1] = text[i + 3].toLatin1();

                    found = find_tag(lookup);
                }

                if(found)
                {
                    setFormat(i, 4, conditional);
                    i += 3;
                }
                else
                {
                    lookup[1] = '\0';
                    lookup[0] = text[i + 2].toLatin1();

                    found = find_tag(lookup);

                    if(found)
                    {
                        setFormat(i, 3, conditional);
                        i += 2;
                    }
                }

            }
        }
    }
}

void SkinHighlighter::loadSettings()
{
    QSettings settings;

    settings.beginGroup("SkinHighlighter");

    /* Loading the highlighting colors */
    tag = settings.value("tagColor", QColor(180,0,0)).value<QColor>();
    conditional = settings.value("conditionalColor",
                                 QColor(0, 0, 180)).value<QColor>();
    escaped = settings.value("escapedColor",
                             QColor(120,120,120)).value<QColor>();
    comment = settings.value("commentColor",
                             QColor(0, 180, 0)).value<QColor>();

    settings.endGroup();

    rehighlight();
}
