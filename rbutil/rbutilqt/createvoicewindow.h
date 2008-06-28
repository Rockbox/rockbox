/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id: installtalkwindow.h 15926 2007-12-14 19:49:11Z domonoky $
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

#ifndef CREATEVOICEWINDOW_H
#define CREATEVOICEWINDOW_H

#include <QtGui>

#include "ui_createvoicefrm.h"
#include "progressloggergui.h"
#include "voicefile.h"
#include "rbsettings.h"

class CreateVoiceWindow : public QDialog
{
    Q_OBJECT
    public:
        CreateVoiceWindow(QWidget *parent = 0);
        void setSettings(RbSettings* sett);
        void setProxy(QUrl proxy){m_proxy = proxy;}

    public slots:
        void accept(void);
        void change(void);
        void updateSettings(void);

    signals:
        void settingsUpdated(void);

    private:
        VoiceFileCreator* voicecreator;
        Ui::CreateVoiceFrm ui;
        ProgressLoggerGui* logger;
        RbSettings* settings;
        QUrl m_proxy;
};

#endif
