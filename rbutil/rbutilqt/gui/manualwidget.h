/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2012 by Dominik Riebeling
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef MANUALWIDGET_H
#define MANUALWIDGET_H

#include <QtGui>
#include "ui_manualwidgetfrm.h"

class ManualWidget : public QWidget
{
    Q_OBJECT
    public:
        ManualWidget(QWidget *parent = 0);

    public slots:
        void downloadManual(void);
        void updateManual();

    private:
        Ui::ManualWidgetFrm ui;
        QString platform;
};

#endif

