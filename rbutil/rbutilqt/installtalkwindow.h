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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef INSTALLTALKWINDOW_H
#define INSTALLTALKWINDOW_H

#include <QtGui>

#include "ui_installtalkfrm.h"
#include "progressloggergui.h"
#include "talkfile.h"
#include "rbsettings.h"

class InstallTalkWindow : public QDialog
{
    Q_OBJECT
    public:
        InstallTalkWindow(QWidget *parent = 0);
        void setSettings(RbSettings* sett);

    public slots:
        void accept(void);
        void change(void);

    private slots:
        void browseFolder(void);
        void setTalkFolder(QString folder);
        void updateSettings(void);

    signals:
        void settingsUpdated(void);

    private:
        TalkFileCreator* talkcreator;
        Ui::InstallTalkFrm ui;
        ProgressLoggerGui* logger;
        RbSettings* settings;

};

#endif
