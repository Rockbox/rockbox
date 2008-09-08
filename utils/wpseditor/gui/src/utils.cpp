/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Rostilav Checkan
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

#include "utils.h"
#include <QPointer>
#include <QtGlobal>
#include "qwpseditorwindow.h"

extern QPointer<QWpsEditorWindow> win;

int qlogger(const char* fmt,...) {
    va_list ap;
    va_start(ap, fmt);
    QString s;
    s.vsprintf(fmt,ap);
    va_end(ap);
    s.replace("\n","");
    //qDebug()<<s;
    if (win==0)
        qDebug()<<s;
    if (s.indexOf("ERR")>=0)
        s = "<font color=red>"+s+"</font>";
    if (win!=0)
        win->logMsg(s);
    return s.length();
}

int qlogger(const QString& s) {
    return qlogger(s.toAscii().data());
}
