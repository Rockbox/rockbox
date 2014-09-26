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
#include <QCoreApplication>
#include <QDebug>
#include "settings.h"

Settings::Settings()
{
}

Settings::~Settings()
{
    if(m_settings)
        delete m_settings;
}

QSettings *Settings::GetSettings()
{
    if(!m_settings)
    {
        QDir dir(QCoreApplication::applicationDirPath());
        QString filename = dir.filePath(QCoreApplication::organizationDomain() + ".ini");
        m_settings = new QSettings(filename, QSettings::IniFormat);
    }
    return m_settings;
}

QSettings *Settings::Get()
{
    return g_settings.GetSettings();
}

Settings Settings::g_settings;