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
 
#ifndef ENCTTSCFGGUI_H
#define ENCTTSCFGGUI_H

#include <QtGui>
#include "encttssettings.h"

//! \brief Shows and manages a configuration gui for encoders and tts enignes
//! 
class EncTtsCfgGui: public QDialog
{
    Q_OBJECT
public:
    //! Creates the UI. give it a endoer or tts engine with already set config. uses show() or exec() to show it.
    EncTtsCfgGui(QDialog* parent, EncTtsSettingInterface* interface,QString name);
      
private slots:
    //! accept current configuration values and close window
    void accept(void);   
    //! close window and dont save configuration
    void reject(void);
    //! updates the corresponding setting from the sending Widget
    void updateSetting();
    //! updates corresponding Widget from the sending Setting. 
    void updateWidget();
    //! shows a busy dialog. counts calls.
    void showBusy();
    //! hides the busy dialog, counts calls
    void hideBusy();
    //! used via the SignalMapper for all Browse buttons
    void browse(QObject*);
    
private:
    //! creates all dynamic window content
    void setUpWindow();
    //! creates the Widgets needed for one setting. returns a Layout with the widgets
    QWidget* createWidgets(EncTtsSetting* setting);
    //! creates a button when needed by the setting.
    QWidget* createButton(EncTtsSetting* setting);
    //! name of the Encoder or TTS for which this UI is
    QString m_name;
    //! the interface pointer to the TTS or encoder
    EncTtsSettingInterface* m_settingInterface;
    //! Dialog, shown when enc or tts is busy
    QProgressDialog*  m_busyDlg;
    //! List of settings from the TTS or Encoder
    QList<EncTtsSetting*> m_settingsList;
    //! Maps settings and the correspondig Widget
    QMap<EncTtsSetting*,QWidget*> m_settingsWidgetsMap;
    //! Maps all browse buttons to the corresponding Setting
    QSignalMapper m_browseBtnMap;
    //! counter how often busyShow() is called, 
    int m_busyCnt;
};


#endif

