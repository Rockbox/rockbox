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


#include <QtGui>
#include "rbutilqt.h"


int main( int argc, char ** argv ) {
    QApplication app( argc, argv );

    QString absolutePath = qApp->applicationDirPath();
    // portable installation:
    // check for a configuration file in the program folder.
    QSettings *user;
    if(QFileInfo(absolutePath + "/RockboxUtility.ini").isFile())
        user = new QSettings(absolutePath + "/RockboxUtility.ini", QSettings::IniFormat, 0);
    else user = new QSettings(QSettings::IniFormat, QSettings::UserScope, "rockbox.org", "RockboxUtility");

    QTranslator translator;
    if(user->value("defaults/lang").toString() != "")
    // install translator
    if(user->value("defaults/lang", "").toString() != "") {
        if(!translator.load(user->value("defaults/lang").toString(), absolutePath))
            translator.load(user->value("defaults/lang").toString(), ":/lang");
    }
    delete user;
    app.installTranslator(&translator);

    RbUtilQt window(0);
    window.show();

//    app.connect( &app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()) );
    return app.exec();

}
