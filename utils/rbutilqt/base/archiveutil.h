/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2013 Amaury Pouly
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef ARCHIVEUTIL_H
#define ARCHIVEUTIL_H

#include <QtCore>

class ArchiveUtil : public QObject
{
    Q_OBJECT

    public:
        ArchiveUtil(QObject* parent);
        ~ArchiveUtil();
        virtual bool close(void) = 0;
        virtual bool extractArchive(const QString& dest, QString file = "") = 0;
        virtual QStringList files(void) = 0;

    signals:
       void logProgress(int, int);
       void logItem(QString, int);
};
#endif

 
