/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Amaury Pouly
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
#include <QApplication>
#include <QDir>
#include <QTextCodec>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationVersion(APP_VERSION);
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    /** NOTE: Qt4 only
     * use the locale codec as the C-string codec, otherwise QString::toStdString()
     * performs as toLatin1() which breaks conversion on virtually all systems.
     * FIXME The documentation mentions that the C-string codec should produce ASCII
     * compatible (ie 7-bit) encodings but nowadays most system are using UTF-8
     * so I don't see why this is a problem */
    QTextCodec::setCodecForCStrings(QTextCodec::codecForLocale());
#endif

    Backend backend;;
    QDir dir(QCoreApplication::applicationDirPath());
    dir.cdUp();
    dir.cd("desc");
    dir.setFilter(QDir::Files);
    QFileInfoList list = dir.entryInfoList();
    for(int i = 0; i < list.size(); i++)
    {
        QFileInfo fileInfo = list.at(i);
        if(fileInfo.fileName().right(4) != ".xml" || fileInfo.fileName().left(5) != "regs-")
            continue;
        backend.LoadSocDesc(fileInfo.absoluteFilePath());
    }

    QCoreApplication::setOrganizationName("Rockbox");
    QCoreApplication::setApplicationName("Register Editor");
    QCoreApplication::setOrganizationDomain("rockbox.org");
    MainWindow win(&backend);
    win.show();
    return app.exec();
}
