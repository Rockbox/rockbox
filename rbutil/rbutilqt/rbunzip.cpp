/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2008 by Dominik Riebeling
 *   $Id$
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "rbunzip.h"
#include <QtCore>


UnZip::ErrorCode RbUnZip::extractArchive(const QString& dest)
{
    QStringList files = this->fileList();
    UnZip::ErrorCode error;
    m_abortunzip = false;

    int total = files.size();
    for(int i = 0; i < total; i++) {
        qDebug() << __func__ << files.at(i);
        error = this->extractFile(files.at(i), dest, UnZip::ExtractPaths);
        emit unzipProgress(i + 1, total);
        QCoreApplication::processEvents(); // update UI
        if(m_abortunzip)
            error = SkipAll;
        if(error != Ok)
            break;
    }
    return error;
}

void RbUnZip::abortUnzip(void)
{
    m_abortunzip = true;
}

