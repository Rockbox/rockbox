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


#ifndef UNINSTALL_H
#define UNINSTALL_H



#include <QtGui>


#include "progressloggerinterface.h"


class Uninstaller : public QObject
{
    Q_OBJECT
public:
    Uninstaller(QObject* parent,QString mountpoint) ;
    ~Uninstaller(){}

    void deleteAll(ProgressloggerInterface* dp);
    void uninstall(ProgressloggerInterface* dp);

    bool uninstallPossible();

    QStringList getAllSections();

    void setSections(QStringList sections) {uninstallSections = sections;}


private slots:


private:
    QString m_mountpoint;

    QStringList uninstallSections;

    ProgressloggerInterface* m_dp;
};



#endif

