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

#ifndef INSTALLTHEMES_H
#define INSTALLTHEMES_H

#include <QtGui>
#include <QTemporaryFile>

#include "ui_installthemesfrm.h"
#include "httpget.h"
#include "installzip.h"
#include "progressloggergui.h"
#include "rbsettings.h"

class ThemesInstallWindow : public QDialog
{
    Q_OBJECT

    public:
        ThemesInstallWindow(QWidget* parent = 0);
        ~ThemesInstallWindow();
        void setSettings(RbSettings* sett){settings=sett;}
        void setProxy(QUrl);
        void downloadInfo(void);
        void show(void);

    public slots:
        void accept(void);
        void acceptAll(void);

    private:
        Ui::ThemeInstallFrm ui;
        RbSettings* settings;
        HttpGet *getter;
        HttpGet igetter;
        QTemporaryFile themesInfo;
        QString resolution(void);
        QUrl proxy;
        int currentItem;
        void resizeEvent(QResizeEvent*);
        QByteArray imgData;
        ProgressLoggerGui *logger;
        ZipInstaller *installer;
        QString file;
        QString fileName;

        QString infocachedir;

    private slots:
        void downloadDone(bool);
        void downloadDone(int, bool);
        void updateDetails(int);
        void updateImage(bool);
        void abort(void);
};


#endif
