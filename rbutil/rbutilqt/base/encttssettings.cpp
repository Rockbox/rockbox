/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id: encoders.h 17902 2008-06-30 22:09:45Z bluebrother $
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
 
#include "encttssettings.h"

    
EncTtsSetting::EncTtsSetting(QObject* parent,ESettingType type,QString name,QVariant current, EButton btn) : QObject(parent)
{
    m_btn = btn;
    m_name =name;
    m_type =type;
    m_currentValue = current;
}
 
EncTtsSetting::EncTtsSetting(QObject* parent,ESettingType type,QString name,QVariant current,QStringList list,EButton btn) : QObject(parent)
{
    m_btn = btn;
    m_name =name;
    m_type =type;
    m_currentValue = current;
    m_list = list;
}

EncTtsSetting::EncTtsSetting(QObject* parent,ESettingType type,QString name,QVariant current,QVariant min,QVariant max, EButton btn) : QObject(parent)
{
    m_btn = btn;
    m_name =name;
    m_type =type;
    m_currentValue = current;
    m_minValue = min;
    m_maxValue = max;
}

void EncTtsSetting::setCurrent(QVariant current,bool noticeGui)
{
    m_currentValue = current; 
    emit dataChanged(); 
    
    if(noticeGui) emit updateGui();
}

//! insert a setting
void EncTtsSettingInterface::insertSetting(int id,EncTtsSetting* setting)
{
    settingsList.insert(id,setting);
}

//! retrieve a specific setting
EncTtsSetting* EncTtsSettingInterface::getSetting(int id)
{
    return settingsList.at(id);
}
