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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
