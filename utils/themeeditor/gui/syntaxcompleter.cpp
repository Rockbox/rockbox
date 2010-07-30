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

#include <QFile>
#include <QTreeWidgetItem>

#include "syntaxcompleter.h"
#include "codeeditor.h"

SyntaxCompleter::SyntaxCompleter(CodeEditor *parent) :
    QTreeWidget(parent)
{
    setHeaderHidden(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    setWordWrap(true);
    setColumnCount(2);

    QObject::connect(this, SIGNAL(activated(QModelIndex)),
                     parent, SLOT(insertTag()));

    QFile fin(":/resources/tagdb");
    fin.open(QFile::ReadOnly | QFile::Text);

    while(!fin.atEnd())
    {
        QString line(fin.readLine());
        if(line.trimmed().length() == 0 || line.trimmed()[0] == '#')
            continue;

        QStringList split = line.split(":");
        QStringList tag;
        tag.append(split[0].trimmed());
        tag.append(split[1].trimmed());
        tags.insertMulti(split[0].trimmed().toLower(), tag);
    }

    filter("");

}

void SyntaxCompleter::filter(QString text)
{
    clear();

    for(QMap<QString, QStringList>::iterator i = tags.begin()
        ; i != tags.end(); i++)
    {
        if(text.length() == 1)
        {
            if(text[0].toLower() != i.key()[0].toLower())
                continue;
        }
        else if(text.length() == 2)
        {
            if(text[0].toLower() != i.key()[0].toLower() || i.key().length() < 2
               || text[1].toLower() != i.key()[1].toLower())
                continue;
        }
        else if(text.length() > 2)
        {
            hide();
        }

        addTopLevelItem(new QTreeWidgetItem(i.value()));
    }

    if(topLevelItemCount() > 0)
        setCurrentIndex(indexFromItem(topLevelItem(0)));

    resizeColumnToContents(0);
    setColumnWidth(0, columnWidth(0) + 10); // Auto-resize is too small
    resizeColumnToContents(1);
}
