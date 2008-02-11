/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id: ttsgui.h 15212 2007-10-19 21:49:07Z domonoky $
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef TTSGUI_H
#define TTSGUI_H

#include <QtGui>

#include "ui_ttsexescfgfrm.h"
#include "ui_sapicfgfrm.h"

class RbSettings;
class TTSSapi;

class TTSSapiGui : public QDialog
{
 Q_OBJECT
public:
    TTSSapiGui(TTSSapi* sapi,QDialog* parent = NULL);
    
    void showCfg();
    void setCfg(RbSettings* sett){settings = sett;}
public slots:

    virtual void accept(void);
    virtual void reject(void);
    virtual void reset(void);
    void updateVoices(QString language);
    void useSapi4Changed(int);
private:
    Ui::SapiCfgFrm ui;
    RbSettings* settings;
    TTSSapi* m_sapi;
};

class TTSExesGui : public QDialog
{
 Q_OBJECT
public:
    TTSExesGui(QDialog* parent = NULL);
    
    void showCfg(QString m_name);
    void setCfg(RbSettings* sett){settings = sett;}

public slots:
    virtual void accept(void);
    virtual void reject(void);
    virtual void reset(void); 
    void browse(void);
private:
    Ui::TTSExesCfgFrm ui;
    RbSettings* settings;
    QString m_name;
};

#endif
