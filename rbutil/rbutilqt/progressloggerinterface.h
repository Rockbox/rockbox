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

#ifndef PROGRESSLOGGERINTERFACE_H
#define PROGRESSLOGGERINTERFACE_H

#include <QtCore>

enum {
    LOGNOICON, LOGOK, LOGINFO, LOGWARNING, LOGERROR
};



class ProgressloggerInterface : public QObject
{
   Q_OBJECT

public:
    ProgressloggerInterface(QObject* parent) : QObject(parent) {}

    virtual void setProgressValue(int value)=0;
    virtual void setProgressMax(int max)=0;
    virtual int getProgressMax()=0;

signals:
    virtual void aborted()=0;


public slots:
    virtual void addItem(const QString &text)=0; //! add a string to the progress
    virtual void addItem(const QString &text, int flag)=0; //! add a string to the list, with icon

    virtual void abort()=0;
    virtual void undoAbort()=0;
    virtual void close()=0;
    virtual void show()=0;

private:

};

#endif

