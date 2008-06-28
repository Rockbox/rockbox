/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id: encodersgui.h 15212 2007-10-19 21:49:07Z domonoky $
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

#ifndef ENCODERSGUI_H
#define ENCODERSGUI_H

#include <QtGui>

class RbSettings;

#include "ui_rbspeexcfgfrm.h"
#include "ui_encexescfgfrm.h"


class EncExesGui : public QDialog
{
 Q_OBJECT
public:
    EncExesGui(QDialog* parent = NULL);
    
    void showCfg(QString m_name);
    void setCfg(RbSettings* sett){settings = sett;}

public slots:
    virtual void accept(void);
    virtual void reject(void);
    virtual void reset(void);
    void browse(void);

private:
    Ui::EncExesCfgFrm ui;
    RbSettings* settings;
    QString m_name;
};

class EncRbSpeexGui : public QDialog
{
 Q_OBJECT
public:
    EncRbSpeexGui(QDialog* parent = NULL);
    
    void showCfg(float defQ,float defV,int defC, bool defB);
    void setCfg(RbSettings* sett){settings = sett;}

public slots:
    virtual void accept(void);
    virtual void reject(void);
    virtual void reset(void);

private:
    Ui::RbSpeexCfgFrm ui;
    RbSettings* settings;
    float defaultQuality;
    float defaultVolume;
    int defaultComplexity;
    bool defaultBand;
};



#endif
