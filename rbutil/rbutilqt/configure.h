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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef CONFIGURE_H
#define CONFIGURE_H

#include "ui_configurefrm.h"
#include "browsedirtree.h"
#include <QtGui>

class Config : public QDialog
{
    Q_OBJECT
    public:
        Config(QWidget *parent = 0,int index=0);
        void setUserSettings(QSettings*);
        void setDevices(QSettings*);

    signals:
        void settingsUpdated(void);

    public slots:
        void accept(void);
        void abort(void);

    private:
        Ui::ConfigForm ui;
        QSettings *userSettings;
        QSettings *devices;
        QStringList findLanguageFiles(void);
        QString languageName(const QString&);
        QMap<QString, QString> lang;
        QString language;
        QString programPath;
        QUrl proxy;
        void updateCacheInfo(QString);

        BrowseDirtree *browser;
        BrowseDirtree *cbrowser;

    private slots:
        void setNoProxy(bool);
        void setSystemProxy(bool);
        void updateLanguage(void);
        void browseFolder(void);
        void browseCache(void);
        void autodetect(void);
        void setMountpoint(QString);
        void setCache(QString);
        void cacheClear(void);
        void browseTts(void);
        void configEnc(void);
        void updateTtsOpts(int);
        void updateEncState(int);
};

#endif
