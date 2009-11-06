/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
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

#ifndef ENCTTSSETTINGS_H
#define ENCTTSSETTINGS_H
 
#include <QtCore>

//! \brief This class stores everything needed to display a Setting.
//!
class EncTtsSetting : public QObject
{
    Q_OBJECT
public:
    enum ESettingType
    {
        eBASE,
        eBOOL,
        eDOUBLE,
        eINT,
        eSTRING,
        eREADONLYSTRING,
        eSTRINGLIST,
    };
    enum EButton
    {
        eNOBTN,
        eBROWSEBTN,
        eREFRESHBTN
    };
    
    //! constructor for a String or Bool setting
    EncTtsSetting(QObject* parent,ESettingType type,QString name,QVariant current,EButton btn = eNOBTN);
    //! contructor for a  Stringlist setting, ie a enumeration   
    EncTtsSetting(QObject* parent,ESettingType type,QString name,QVariant current,QStringList list,EButton btn = eNOBTN);
    //! constructor for a setting with a min-max range
    EncTtsSetting(QObject* parent,ESettingType type,QString name,QVariant current,QVariant min,QVariant max,EButton = eNOBTN);

    //! get currentValue
    QVariant current() {return m_currentValue;}
    //! set currentValue
    void setCurrent(QVariant current,bool noticeGui=true);
      
   //! get name of the Setting
    QString name() {return m_name;}
    //! get the type of the setting
    ESettingType type() {return m_type;}
    //! get what type of button this setting needs
    EButton button() {return m_btn;}
    //! get the minValue (only valid for a range setting, ie eDOUBLE or eINT)
    QVariant min() {return m_minValue; }
    //! get the maxValue (only valid for a range setting, ie eDOUBLE or eINT)
    QVariant max() {return m_maxValue; }
    //! get the enumerationlist (only valid for eSTRINGLIST settings)
    QStringList list() {return m_list;}
    //! set the enumeration list
    void setList(QStringList list){m_list = list;}
    
signals:
    //! connect to this signal if you want to get noticed when the data changes
    void dataChanged();
    //! connect to this if you want to react on refresh button
    void refresh();
    //! will be emited when the gui should update this setting
    void updateGui();
    
private:
    ESettingType m_type;  
    EButton m_btn;
    QString m_name;
    QVariant m_currentValue;
    QVariant m_minValue;
    QVariant m_maxValue;
    QStringList m_list;
};


//! \brief this class is the Interface for Encoder and TTS engines, to display settings
//! It wraps nearly everything needed, only updateModel() and commitModel() needs to be reimplemented
//!
class EncTtsSettingInterface : public QObject
{
    Q_OBJECT
public:
    EncTtsSettingInterface(QObject* parent) : QObject(parent) {}

    //! get the Settings list
    QList<EncTtsSetting*> getSettings() {generateSettings(); return settingsList;}

    //! Chlid class should commit the from SettingsList to permanent storage
    virtual void saveSettings() = 0;
    
signals:
    void busy();  // emit this if a operation takes time
    void busyEnd(); // emit this at the end of a busy section

protected:
    //! Child class should fill in the setttingsList
    virtual void generateSettings() = 0;

    //! insert a setting
    void insertSetting(int id,EncTtsSetting* setting);
    //! retrieve a specific setting
    EncTtsSetting* getSetting(int id);
    
private:
    //! The setting storage.
    QList<EncTtsSetting*> settingsList;

};
#endif
