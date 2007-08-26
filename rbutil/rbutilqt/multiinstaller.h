/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id: multiinstaller.h 14462 2007-08-26 16:44:23Z domonoky $
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
 
#ifndef MULTIINSTALLER_H
#define MULTIINSTALLER_H

#include <QtGui>

#include "progressloggerinterface.h"


class MultiInstaller : public QObject
{
    Q_OBJECT
    
public:
    MultiInstaller(QObject* parent);

    void setUserSettings(QSettings*);
    void setDeviceSettings(QSettings*);
    void setProxy(QUrl proxy);
    void setVersionStrings(QMap<QString, QString>);
    
    void installComplete();
    void installSmall();

    
private slots:
    void installdone(bool error);
    
private:
    bool installBootloader();
    bool installRockbox();
    bool installDoom();
    bool installFonts();
    bool installThemes();
    
    ProgressloggerInterface* logger;
    QSettings *devices;
    QSettings *userSettings;
    
    QString m_mountpoint,m_plattform;
    QUrl m_proxy;
    QMap<QString, QString> version;
    
    
    volatile bool installed;
    volatile bool m_error;
};
#endif
