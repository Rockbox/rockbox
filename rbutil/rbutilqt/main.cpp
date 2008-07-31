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

#ifdef STATIC
#include <QtPlugin>
Q_IMPORT_PLUGIN(qtaccessiblewidgets)
#endif

int main( int argc, char ** argv ) {
    QApplication app( argc, argv );

    QString absolutePath = QCoreApplication::instance()->applicationDirPath();
    // portable installation:
    // check for a configuration file in the program folder.
    QSettings *user;
    if(QFileInfo(absolutePath + "/RockboxUtility.ini").isFile())
        user = new QSettings(absolutePath + "/RockboxUtility.ini", QSettings::IniFormat, 0);
    else user = new QSettings(QSettings::IniFormat, QSettings::UserScope, "rockbox.org", "RockboxUtility");

    QString applang = QLocale::system().name();
    QTranslator translator;
    QTranslator qttrans;
    // install translator
    if(!user->value("lang", "").toString().isEmpty()) {
        applang = user->value("lang", "").toString();
    }
    if(!applang.isEmpty()) {
        if(!translator.load("rbutil_" + applang, absolutePath))
            translator.load("rbutil_" + applang, ":/lang");
        if(!qttrans.load("qt_" + applang,
            QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
            translator.load("qt_" + applang, ":/lang");

        QLocale::setDefault(applang);
    }
    delete user;
    app.installTranslator(&translator);
    app.installTranslator(&qttrans);

    RbUtilQt window(0);
    window.show();

//    app.connect( &app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()) );
    return app.exec();

}
