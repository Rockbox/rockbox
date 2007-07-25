/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Riebeling
 *   $Id:$
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

    QTranslator translator;
//    translator.load("rbutil_de.qm");
    app.installTranslator(&translator);

    RbUtilQt window(0);
    window.show();

//    app.connect( &app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()) );
    return app.exec();

}
