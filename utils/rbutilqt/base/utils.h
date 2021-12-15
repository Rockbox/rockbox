/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
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


#ifndef UTILS_H
#define UTILS_H

#include <QtCore/QObject>

#include <QString>
#include <QUrl>

class Utils : public QObject
{
public:
    enum Size {
        FilesystemTotal,
        FilesystemFree,
        FilesystemClusterSize,
    };
    enum MountpointsFilter {
        MountpointsAll,
        MountpointsSupported,
    };

    static bool recursiveRmdir(const QString &dirName);
    static QString resolvePathCase(QString path);
    static qulonglong filesystemFree(QString path);
    static qulonglong filesystemTotal(QString path);
    static qulonglong filesystemSize(QString path, enum Size type);
    static QString filesystemType(QString path);
    static QString findExecutable(QString name);
    static QString checkEnvironment(bool permission);
    static int compareVersionStrings(QString s1, QString s2);
    static QString trimVersionString(QString s);
    static QString filesystemName(QString path);
    static QStringList mountpoints(enum MountpointsFilter type = MountpointsAll);
    static QString resolveDevicename(QString path);
    static QString resolveMountPoint(QString device);
    static QMap<QString, QList<int> > findRunningProcess(QStringList names);
    static QList<int> suspendProcess(QList<int> pidlist, bool suspend);
    static bool ejectDevice(QString device);
    static qint64 recursiveFolderSize(QString path);
};

#endif

