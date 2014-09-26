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
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationVersion(APP_VERSION);

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
