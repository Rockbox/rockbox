/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2012 Dominik Riebeling
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * This file is a modified version of the AMS installer by Dominik Wenger
 *
 ****************************************************************************/

#ifndef BOOTLOADERINSTALLHELPER_H
#define BOOTLOADERINSTALLHELPER_H

#include <QtCore>
#include "bootloaderinstallbase.h"

class BootloaderInstallHelper : public QObject
{
    Q_OBJECT
    public:
        static BootloaderInstallBase* createBootloaderInstaller(QObject* parent, QString type);
        static BootloaderInstallBase::Capabilities bootloaderInstallerCapabilities(QObject *parent, QString type);
        static QString preinstallHints(QString model);
        static QString postinstallHints(QString model);
};

#endif

