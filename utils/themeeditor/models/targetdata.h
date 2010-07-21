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

#ifndef TARGETDATA_H
#define TARGETDATA_H

#include <QFile>
#include <QString>
#include <QHash>
#include <QRect>

class TargetData
{

public:
    enum ScreenDepth
    {
        RGB,
        Grey,
        Mono,
        None
    };

    TargetData();
    virtual ~TargetData();

    int count(){ return indices.count(); }
    int index(QString id){ return indices.value(id, -1); }

    QString id(int index){ return indices.key(index, ""); }
    QString name(int index){ return entries[index].name; }
    QRect screenSize(int index){ return entries[index].size; }
    QRect remoteSize(int index){ return entries[index].rSize; }
    ScreenDepth screenDepth(int index){ return entries[index].depth; }
    ScreenDepth remoteDepth(int index){ return entries[index].rDepth; }
    bool fm(int index){ return entries[index].fm; }
    bool canRecord(int index){ return entries[index].record; }

private:
    struct Entry
    {
        Entry(QString id, QString name, QRect size, ScreenDepth depth,
              QRect rSize, ScreenDepth rDepth, bool fm, bool record)
                  : id(id), name(name), size(size), depth(depth), rSize(rSize),
                  rDepth(rDepth), fm(fm), record(record){ }

        QString id;
        QString name;
        QRect size;
        ScreenDepth depth;
        QRect rSize;
        ScreenDepth rDepth;
        bool fm;
        bool record;
    };

    static const QString reserved;

    QString scanString(QString data, int& index);
    int scanInt(QString data, int& index);
    ScreenDepth scanDepth(QString data, int& index);
    void skipComment(QString data, int& index);
    bool require(QChar required, QString data, int& index);

    QHash<QString, int> indices;
    QList<Entry> entries;

};

#endif // TARGETDATA_H
