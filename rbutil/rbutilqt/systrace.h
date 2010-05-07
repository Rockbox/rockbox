/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Riebeling
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

#ifndef SYSTRACE_H
#define SYSTRACE_H

#include <QtGui>
#include "ui_systracefrm.h"

class SysTrace : public QDialog
{
    Q_OBJECT
    public:
        SysTrace(QWidget *parent);
        static void debug(QtMsgType type, const char* msg);
        static QString getTrace() {return debugbuffer;}
        static void save(QString filename = "");
    private:
        static void flush(void);
        Ui::SysTraceFrm ui;
        static QString debugbuffer;
        static QString lastmessage;
        static unsigned int repeat;

    private slots:
        void saveCurrentTrace(void);
        void savePreviousTrace(void);
        void refresh(void);
        
};

#endif

