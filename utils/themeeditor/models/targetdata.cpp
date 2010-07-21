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

#include "targetdata.h"

#include <QStringList>
#include <QSettings>

const QString TargetData::reserved = "{}:#\n";

TargetData::TargetData()
{
    QSettings settings;
    settings.beginGroup("TargetData");
    QString file = settings.value("targetDbPath", "").toString();
    settings.endGroup();

    if(!QFile::exists(file))
        file = ":/targets/targetdb";

    QFile fin(file);
    fin.open(QFile::ReadOnly | QFile::Text);

    /* Reading the database */
    QString data = QString(fin.readAll());

    fin.close();

    int cursor = 0;
    int index = 0;
    while(cursor < data.count())
    {
        QString id = scanString(data, cursor);
        QString name = "";
        QRect size(0, 0, 0, 0);
        ScreenDepth depth = None;
        QRect rSize(0, 0, 0, 0);
        ScreenDepth rDepth = None;
        bool fm = false;
        bool record = false;

        if(id == "")
            break;

        if(!require('{', data, cursor))
            break;

        /* Now we have to parse each of the arguments */
        while(cursor < data.count())
        {
            QString key = scanString(data, cursor);
            if(key == "")
            {
                break;
            }
            else
            {
                if(!require(':', data, cursor))
                    break;

                if(key.toLower() == "name")
                {
                    name = scanString(data, cursor);
                }
                else if(key.toLower() == "screen")
                {
                    QString s = scanString(data, cursor);
                    if(s[0].toLower() != 'n')
                    {
                        int subCursor = 0;
                        int width = scanInt(s, subCursor);

                        if(!require('x', s, subCursor))
                            break;

                        int height = scanInt(s, subCursor);

                        if(!require('@', s, subCursor))
                            break;

                        size = QRect(0, 0, width, height);
                        depth = scanDepth(s, subCursor);
                    }
                }
                else if(key.toLower() == "remote")
                {
                    QString s = scanString(data, cursor);
                    if(s[0].toLower() != 'n')
                    {
                        int subCursor = 0;
                        int width = scanInt(s, subCursor);

                        if(!require('x', s, subCursor))
                            break;

                        int height = scanInt(s, subCursor);

                        if(!require('@', s, subCursor))
                            break;

                        rSize = QRect(0, 0, width, height);
                        rDepth = scanDepth(s, subCursor);
                    }
                }
                else if(key.toLower() == "fm")
                {
                    QString s = scanString(data, cursor);
                    if(s.toLower() == "yes")
                        fm = true;
                }
                else if(key.toLower() == "record")
                {
                    QString s  = scanString(data, cursor);
                    if(s.toLower() == "yes")
                        record = true;
                }
            }
        }

        /* Checking for the closing '}' and adding the entry */
        if(require('}', data, cursor))
        {
            entries.append(Entry(id, name, size, depth, rSize, rDepth,
                                 fm, record));
            indices.insert(id, index);
            index++;
        }
        else
        {
            break;
        }
    }
}

TargetData::~TargetData()
{
}

QString TargetData::scanString(QString data, int &index)
{
    QString retval;

    /* Skipping whitespace and comments */
    while(index < data.count() && (data[index].isSpace() || data[index] == '#'))
    {
        if(data[index] == '#')
            skipComment(data, index);
        else
            index++;
    }

    while(index < data.count() && !reserved.contains(data[index]))
    {
        if(data[index] == '%')
        {
            retval.append(data[index + 1]);
            index += 2;
        }
        else
        {
            retval.append(data[index]);
            index++;
        }
    }

    return retval.trimmed();
}

int TargetData::scanInt(QString data, int &index)
{
    /* Skipping whitespace and comments */
    while(index < data.count() && (data[index].isSpace() || data[index] == '#'))
    {
        if(data[index] == '#')
            skipComment(data, index);
        else
            index++;
    }

    QString number;
    while(index < data.count() && data[index].isDigit())
        number.append(data[index++]);

    return number.toInt();
}

TargetData::ScreenDepth TargetData::scanDepth(QString data, int &index)
{
    QString depth = scanString(data, index);

    if(depth.toLower() == "grey")
        return Grey;
    else if(depth.toLower() == "rgb")
        return RGB;
    else if(depth.toLower() == "mono")
        return Mono;
    else
        return None;
}

void TargetData::skipComment(QString data, int &index)
{
    if(data[index] != '#')
        return;

    while(index < data.count() && data[index] != '\n')
        index++;

    if(index < data.count())
        index++;
}

bool TargetData::require(QChar required, QString data, int &index)
{
    /* Skipping whitespace and comments */
    while(index < data.count() && (data[index].isSpace() || data[index] == '#'))
    {
        if(data[index] == '#')
            skipComment(data, index);
        else
            index++;
    }

    if(index == data.count())
    {
        return false;
    }
    else
    {
        if(data[index] == required)
        {
            index++;
            return true;
        }
        else
        {
            return false;
        }
    }

}
