/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2011 Dominik Riebeling
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef ZIPUTIL_H
#define ZIPUTIL_H

#include <QtCore>
#include "quazip/quazip.h"
#include "quazip/quazipfile.h"
#include "quazip/quazipfileinfo.h"

class ZipUtil : public QObject
{
    Q_OBJECT

    public:
        ZipUtil(QObject* parent);
        ~ZipUtil();
        bool open(QString& zipfile, QuaZip::Mode mode = QuaZip::mdUnzip);
        bool close(void);
        bool extractArchive(QString& dest, QString file = "");
        bool appendDirToArchive(QString& source, QString& basedir);
        bool appendFileToArchive(QString& file, QString& basedir);
        qint64 totalUncompressedSize(unsigned int clustersize = 0);
        QStringList files(void);

    signals:
       void logProgress(int, int);
       void logItem(QString, int);

    private:
        QList<QuaZipFileInfo> contentProperties();
        QuaZip* m_zip;
        QuaZipFile* m_file;

};
#endif

